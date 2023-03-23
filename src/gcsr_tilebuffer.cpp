// ----------------------------------------------------------------------------------
// -- File: gcsr_tilebuffer.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C
// -- Description:
// -- Created: 2022-08-15 13:58:44
// -- Modified: 2022-08-23 11:34:03
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

static u32 _debug_fragment_x = 0;
static u32 _debug_fragment_y = 0;
static u32 _debug_primitive_id0 = 0;
static u32 _debug_primitive_id1 = 0;

#if defined(GC_PIPE_AVX)
#include "simd/gcsr_avx_tilebuffer.cpp"
#elif defined(GC_PIPE_SSE)
#include "simd/gcsr_sse_tilebuffer.cpp"
#endif

// ----------------------------------------------------------------------------------
// -- Processing order.
// ----------------------------------------------------------------------------------
// -- 1) Color tint
// -- 2) Saturate
// -- 3) Tone mapping
// -- 4) Shadows
// ----------------------------------------------------------------------------------

__INLINE__ void _post_fragment(gc_fragment_t *fragment)
{
    b8 is_postprocessing = PIPE_FLAG(GC_POST_PROCESSING);
    pipe_param_merged_table_t *overwrites = &GCSR.gl->pipeline.params.overwrites;

    r32 tmp_r[GC_FRAG_SIZE];
    r32 tmp_g[GC_FRAG_SIZE];
    r32 tmp_b[GC_FRAG_SIZE];

    r32 sy[GC_FRAG_SIZE];

    sy[0] = fragment->r[0] * 0.30f + fragment->g[0] * 0.59f + fragment->b[0] * 0.11f;
    sy[1] = fragment->r[1] * 0.30f + fragment->g[1] * 0.59f + fragment->b[1] * 0.11f;
    sy[2] = fragment->r[2] * 0.30f + fragment->g[2] * 0.59f + fragment->b[2] * 0.11f;
    sy[3] = fragment->r[3] * 0.30f + fragment->g[3] * 0.59f + fragment->b[3] * 0.11f;

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        tmp_r[i] = fragment->r[i];
        tmp_g[i] = fragment->g[i];
        tmp_b[i] = fragment->b[i];

        // ----------------------------------------------------------------------------------
        // -- Copy the result if masked.
        // ----------------------------------------------------------------------------------

        // if (fragment->mask & masks[i])
        {
            if (is_postprocessing)
            {
                // ----------------------------------------------------------------------------------
                // -- Color tint.
                // ----------------------------------------------------------------------------------

                tmp_r[i] *= PIPE_PARAM_VECTOR(1, tint_color, 0);
                tmp_g[i] *= PIPE_PARAM_VECTOR(1, tint_color, 1);
                tmp_b[i] *= PIPE_PARAM_VECTOR(1, tint_color, 2);

                // ----------------------------------------------------------------------------------
                // -- Saturation.
                // ----------------------------------------------------------------------------------

                if (PIPE_PARAM_VALUE_FLOAT(1, saturation) != 1.0f)
                {
                    tmp_r[i] = (tmp_r[i] - sy[i]) * PIPE_PARAM_VALUE_FLOAT(1, saturation) + sy[i];
                    tmp_g[i] = (tmp_g[i] - sy[i]) * PIPE_PARAM_VALUE_FLOAT(1, saturation) + sy[i];
                    tmp_b[i] = (tmp_b[i] - sy[i]) * PIPE_PARAM_VALUE_FLOAT(1, saturation) + sy[i];
                }
            }

            // ----------------------------------------------------------------------------------
            // -- Tone mapping.
            // ----------------------------------------------------------------------------------

            if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_CLAMP)
            {
                tmp_r[i] = clamp(0, 1, tmp_r[i]);
                tmp_g[i] = clamp(0, 1, tmp_g[i]);
                tmp_b[i] = clamp(0, 1, tmp_b[i]);
            }
            else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_REINHARD)
            {
                tmp_r[i] = tmp_r[i] / (1 + tmp_r[i]);
                tmp_g[i] = tmp_g[i] / (1 + tmp_g[i]);
                tmp_b[i] = tmp_b[i] / (1 + tmp_b[i]);
            }
            else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_ACES)
            {}
            else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_ACES_APROX)
            {
                tmp_r[i] = ACES_APROX(tmp_r[i]);
                tmp_g[i] = ACES_APROX(tmp_g[i]);
                tmp_b[i] = ACES_APROX(tmp_b[i]);
            }
            else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_FILMIC)
            {
                tmp_r[i] = FILMIC(tmp_r[i]);
                tmp_g[i] = FILMIC(tmp_g[i]);
                tmp_b[i] = FILMIC(tmp_b[i]);
            }

            fragment->r[i] = tmp_r[i];
            fragment->g[i] = tmp_g[i];
            fragment->b[i] = tmp_b[i];
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Format RGBAU8.
// ----------------------------------------------------------------------------------
// -- Load the tilebuffer from the framebuffer.
// ----------------------------------------------------------------------------------

void load_tile_buffer_rgbau8(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
#if 0
    OPTICK_EVENT("load_tile_buffer_rgbau8");

    texture2d_t *color_texture = framebuffer->color;
    gc_fragment_t *fragment = tile_buffer->fragments;
    u32 offset = tile_buffer->y * color_texture->mips->header->width + tile_buffer->x;
    u32 frag_row_pitch = GL_FRAG_HEIGHT * color_texture->mips->header->width;

    u32 *color = (u32 *) color_texture->mips->data + offset;
    r32 *depth = framebuffer->depth + offset;

    s32 max_x = tile_buffer->x + GL_BIN_WIDTH - 1;
    s32 max_y = tile_buffer->y + GL_BIN_HEIGHT - 1;
    s32 dx = (GL_FRAG_WIDTH << 1);

    for (s32 y = tile_buffer->y; y < max_y; y += GL_FRAG_HEIGHT)
    {
        u32 *pixel_row_a = color;
        u32 *pixel_row_b = color + color_texture->mips->header->width;

        r32 *depth_row_a = depth;
        r32 *depth_row_b = depth + color_texture->mips->header->width;

        for (s32 x = tile_buffer->x; x < max_x; x += dx)
        {
            gc_fragment_t *frag_a = fragment++;
            gc_fragment_t *frag_b = fragment++;

            u32 color_a0 = *pixel_row_a++;
            u32 color_a1 = *pixel_row_a++;
            u32 color_a2 = *pixel_row_a++;
            u32 color_a3 = *pixel_row_a++;

            u32 color_b0 = *pixel_row_b++;
            u32 color_b1 = *pixel_row_b++;
            u32 color_b2 = *pixel_row_b++;
            u32 color_b3 = *pixel_row_b++;

            gc_vec_t color_rgba_A0 = gl_unpack(color_a0);
            gc_vec_t color_rgba_A1 = gl_unpack(color_a1);
            gc_vec_t color_rgba_A2 = gl_unpack(color_a2);
            gc_vec_t color_rgba_A3 = gl_unpack(color_a3);

            gc_vec_t color_rgba_B0 = gl_unpack(color_b0);
            gc_vec_t color_rgba_B1 = gl_unpack(color_b1);
            gc_vec_t color_rgba_B2 = gl_unpack(color_b2);
            gc_vec_t color_rgba_B3 = gl_unpack(color_b3);

            gl_gamma_srgb_to_linear(&color_rgba_A0);
            gl_gamma_srgb_to_linear(&color_rgba_A1);
            gl_gamma_srgb_to_linear(&color_rgba_A2);
            gl_gamma_srgb_to_linear(&color_rgba_A3);

            gl_gamma_srgb_to_linear(&color_rgba_B0);
            gl_gamma_srgb_to_linear(&color_rgba_B1);
            gl_gamma_srgb_to_linear(&color_rgba_B2);
            gl_gamma_srgb_to_linear(&color_rgba_B3);

            frag_a->r[0] = color_rgba_A0.data[0];
            frag_a->r[1] = color_rgba_A1.data[0];
            frag_a->r[2] = color_rgba_B0.data[0];
            frag_a->r[3] = color_rgba_B1.data[0];

            frag_a->g[0] = color_rgba_A0.data[1];
            frag_a->g[1] = color_rgba_A1.data[1];
            frag_a->g[2] = color_rgba_B0.data[1];
            frag_a->g[3] = color_rgba_B1.data[1];

            frag_a->b[0] = color_rgba_A0.data[2];
            frag_a->b[1] = color_rgba_A1.data[2];
            frag_a->b[2] = color_rgba_B0.data[2];
            frag_a->b[3] = color_rgba_B1.data[2];

            frag_b->r[0] = color_rgba_A2.data[0];
            frag_b->r[1] = color_rgba_A3.data[0];
            frag_b->r[2] = color_rgba_B2.data[0];
            frag_b->r[3] = color_rgba_B3.data[0];

            frag_b->g[0] = color_rgba_A2.data[1];
            frag_b->g[1] = color_rgba_A3.data[1];
            frag_b->g[2] = color_rgba_B2.data[1];
            frag_b->g[3] = color_rgba_B3.data[1];

            frag_b->b[0] = color_rgba_A2.data[2];
            frag_b->b[1] = color_rgba_A3.data[2];
            frag_b->b[2] = color_rgba_B2.data[2];
            frag_b->b[3] = color_rgba_B3.data[2];

            frag_a->z[0] = *depth_row_a++;
            frag_a->z[1] = *depth_row_a++;
            frag_a->z[2] = *depth_row_b++;
            frag_a->z[3] = *depth_row_b++;

            frag_b->z[0] = *depth_row_a++;
            frag_b->z[1] = *depth_row_a++;
            frag_b->z[2] = *depth_row_b++;
            frag_b->z[3] = *depth_row_b++;

            frag_a->shadow[0] = 1;
            frag_a->shadow[1] = 1;
            frag_a->shadow[2] = 1;
            frag_a->shadow[3] = 1;

            frag_b->shadow[0] = 1;
            frag_b->shadow[1] = 1;
            frag_b->shadow[2] = 1;
            frag_b->shadow[3] = 1;

            frag_a->mask = 0;
            frag_b->mask = 0;
        }

        color += frag_row_pitch;
        depth += frag_row_pitch;
    }
#endif
}

// ----------------------------------------------------------------------------------
// -- Save the tilebuffer to the framebuffer.
// ----------------------------------------------------------------------------------

void save_tile_buffer_rgbau8(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
    texture2d_t *color_texture = framebuffer->color;
    gc_fragment_t *fragment = tile_buffer->fragments;

    u32 offset = tile_buffer->y * color_texture->mips->header->width + tile_buffer->x;
    u32 frag_row_pitch = GL_FRAG_HEIGHT * color_texture->mips->header->width;

    u32 *color = (u32 *) color_texture->mips->data + offset;
    // r32 *depth = framebuffer->depth + offset;

    s32 max_x = tile_buffer->x + GL_BIN_WIDTH - 1;
    s32 max_y = tile_buffer->y + GL_BIN_HEIGHT - 1;
    s32 dx = GL_FRAG_WIDTH << 1;

    for (s32 y = tile_buffer->y; y < max_y; y += GL_FRAG_HEIGHT)
    {
        u32 *pixel_row_a = color;
        u32 *pixel_row_b = color + color_texture->mips->header->width;

        // r32 *depth_row_a = depth;
        // r32 *depth_row_b = depth + color_texture->mips->header->width;

        for (s32 x = tile_buffer->x; x < max_x; x += dx)
        {
            gc_fragment_t *frag_a = fragment++;
            gc_fragment_t *frag_b = fragment++;

            _debug_fragment_x = x;
            _debug_fragment_y = y;

            if (x == 512 && y == 226)
            {
                u8 a = 0;
                a++;
            }

            _post_fragment(frag_a);
            _post_fragment(frag_b);

            VINIT4(color_A0, frag_a->r[0], frag_a->g[0], frag_a->b[0], 1);
            VINIT4(color_A1, frag_a->r[1], frag_a->g[1], frag_a->b[1], 1);
            VINIT4(color_B0, frag_a->r[2], frag_a->g[2], frag_a->b[2], 1);
            VINIT4(color_B1, frag_a->r[3], frag_a->g[3], frag_a->b[3], 1);

            VINIT4(color_A2, frag_b->r[0], frag_b->g[0], frag_b->b[0], 1);
            VINIT4(color_A3, frag_b->r[1], frag_b->g[1], frag_b->b[1], 1);
            VINIT4(color_B2, frag_b->r[2], frag_b->g[2], frag_b->b[2], 1);
            VINIT4(color_B3, frag_b->r[3], frag_b->g[3], frag_b->b[3], 1);

            gl_gamma_linear_to_srgb(&color_A0);
            gl_gamma_linear_to_srgb(&color_A1);
            gl_gamma_linear_to_srgb(&color_B0);
            gl_gamma_linear_to_srgb(&color_B1);

            gl_gamma_linear_to_srgb(&color_A2);
            gl_gamma_linear_to_srgb(&color_A3);
            gl_gamma_linear_to_srgb(&color_B2);
            gl_gamma_linear_to_srgb(&color_B3);

            u32 packed_color_A0 = gl_pack(color_A0);
            u32 packed_color_A1 = gl_pack(color_A1);
            u32 packed_color_B0 = gl_pack(color_B0);
            u32 packed_color_B1 = gl_pack(color_B1);

            u32 packed_color_A2 = gl_pack(color_A2);
            u32 packed_color_A3 = gl_pack(color_A3);
            u32 packed_color_B2 = gl_pack(color_B2);
            u32 packed_color_B3 = gl_pack(color_B3);

            *pixel_row_a++ = packed_color_A0;
            *pixel_row_a++ = packed_color_A1;
            *pixel_row_a++ = packed_color_A2;
            *pixel_row_a++ = packed_color_A3;

            *pixel_row_b++ = packed_color_B0;
            *pixel_row_b++ = packed_color_B1;
            *pixel_row_b++ = packed_color_B2;
            *pixel_row_b++ = packed_color_B3;

            // *depth_row_a++ = frag_a->z[0];
            // *depth_row_a++ = frag_a->z[1];
            // *depth_row_a++ = frag_b->z[0];
            // *depth_row_a++ = frag_b->z[1];

            // *depth_row_b++ = frag_a->z[2];
            // *depth_row_b++ = frag_a->z[3];
            // *depth_row_b++ = frag_b->z[2];
            // *depth_row_b++ = frag_b->z[3];
        }

        color += frag_row_pitch;
        // depth += frag_row_pitch;
    }
}

// ----------------------------------------------------------------------------------
// -- Format RGBAF.
// ----------------------------------------------------------------------------------

void load_tile_buffer_rgbaf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
#if 0
    OPTICK_EVENT("load_tile_buffer_rgbaf");

    texture2d_t *color_texture = framebuffer->color;
    gc_fragment_t *fragment = tile_buffer->fragments;
    u32 offset = tile_buffer->y * color_texture->mips->header->width + tile_buffer->x;
    u32 frag_row_pitch = GL_FRAG_HEIGHT * color_texture->mips->header->width;

    texpixel_rgbaf_t *color = (texpixel_rgbaf_t *) color_texture->mips->data + offset;
    r32 *depth = framebuffer->depth + offset;

    s32 max_x = tile_buffer->x + GL_BIN_WIDTH - 1;
    s32 max_y = tile_buffer->y + GL_BIN_HEIGHT - 1;
    s32 dx = (GL_FRAG_WIDTH << 1);

    for (s32 y = tile_buffer->y; y < max_y; y += GL_FRAG_HEIGHT)
    {
        texpixel_rgbaf_t *pixel_row_a = color;
        texpixel_rgbaf_t *pixel_row_b = color + color_texture->mips->header->width;

        r32 *depth_row_a = depth;
        r32 *depth_row_b = depth + color_texture->mips->header->width;

        for (s32 x = tile_buffer->x; x < max_x; x += dx)
        {
            gc_fragment_t *frag_a = fragment++;
            gc_fragment_t *frag_b = fragment++;

            texpixel_rgbaf_t *color_A0 = pixel_row_a++;
            texpixel_rgbaf_t *color_A1 = pixel_row_a++;
            texpixel_rgbaf_t *color_A2 = pixel_row_a++;
            texpixel_rgbaf_t *color_A3 = pixel_row_a++;

            texpixel_rgbaf_t *color_B0 = pixel_row_b++;
            texpixel_rgbaf_t *color_B1 = pixel_row_b++;
            texpixel_rgbaf_t *color_B2 = pixel_row_b++;
            texpixel_rgbaf_t *color_B3 = pixel_row_b++;

            gl_gamma_srgb_to_linear_texpixel(color_A0);
            gl_gamma_srgb_to_linear_texpixel(color_A1);
            gl_gamma_srgb_to_linear_texpixel(color_A2);
            gl_gamma_srgb_to_linear_texpixel(color_A3);

            gl_gamma_srgb_to_linear_texpixel(color_B0);
            gl_gamma_srgb_to_linear_texpixel(color_B1);
            gl_gamma_srgb_to_linear_texpixel(color_B2);
            gl_gamma_srgb_to_linear_texpixel(color_B3);

            frag_a->r[0] = color_A0->r;
            frag_a->r[1] = color_A1->r;
            frag_a->r[2] = color_B0->r;
            frag_a->r[3] = color_B1->r;

            frag_a->g[0] = color_A0->g;
            frag_a->g[1] = color_A1->g;
            frag_a->g[2] = color_B0->g;
            frag_a->g[3] = color_B1->g;

            frag_a->b[0] = color_A0->b;
            frag_a->b[1] = color_A1->b;
            frag_a->b[2] = color_B0->b;
            frag_a->b[3] = color_B1->b;

            frag_b->r[0] = color_A2->r;
            frag_b->r[1] = color_A3->r;
            frag_b->r[2] = color_B2->r;
            frag_b->r[3] = color_B3->r;

            frag_b->g[0] = color_A2->g;
            frag_b->g[1] = color_A3->g;
            frag_b->g[2] = color_B2->g;
            frag_b->g[3] = color_B3->g;

            frag_b->b[0] = color_A2->b;
            frag_b->b[1] = color_A3->b;
            frag_b->b[2] = color_B2->b;
            frag_b->b[3] = color_B3->b;

            frag_a->z[0] = *depth_row_a++;
            frag_a->z[1] = *depth_row_a++;
            frag_a->z[2] = *depth_row_b++;
            frag_a->z[3] = *depth_row_b++;

            frag_b->z[0] = *depth_row_a++;
            frag_b->z[1] = *depth_row_a++;
            frag_b->z[2] = *depth_row_b++;
            frag_b->z[3] = *depth_row_b++;

            frag_a->mask = 0;
            frag_b->mask = 0;
        }

        color += frag_row_pitch;
        depth += frag_row_pitch;
    }
#endif
}

void save_tile_buffer_rgbaf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
    texture2d_t *color_texture = framebuffer->color;
    gc_fragment_t *fragment = tile_buffer->fragments;

    u32 offset = tile_buffer->y * color_texture->mips->header->width + tile_buffer->x;
    u32 frag_row_pitch = GL_FRAG_HEIGHT * color_texture->mips->header->width;

    texpixel_rgbaf_t *color = (texpixel_rgbaf_t *) color_texture->mips->data + offset;
    // r32 *depth = framebuffer->depth + offset;

    s32 max_x = tile_buffer->x + GL_BIN_WIDTH - 1;
    s32 max_y = tile_buffer->y + GL_BIN_HEIGHT - 1;
    s32 dx = GL_FRAG_WIDTH << 1;

    for (s32 y = tile_buffer->y; y < max_y; y += GL_FRAG_HEIGHT)
    {
        texpixel_rgbaf_t *pixel_row_a = color;
        texpixel_rgbaf_t *pixel_row_b = color + color_texture->mips->header->width;

        // r32 *depth_row_a = depth;
        // r32 *depth_row_b = depth + color_texture->mips->header->width;

        for (s32 x = tile_buffer->x; x < max_x; x += dx)
        {
            gc_fragment_t *frag_a = fragment++;
            gc_fragment_t *frag_b = fragment++;

            _post_fragment(frag_a);
            _post_fragment(frag_b);

            texpixel_rgbaf_t *pixel_row_a0 = pixel_row_a++;
            texpixel_rgbaf_t *pixel_row_a1 = pixel_row_a++;
            texpixel_rgbaf_t *pixel_row_a2 = pixel_row_a++;
            texpixel_rgbaf_t *pixel_row_a3 = pixel_row_a++;

            texpixel_rgbaf_t *pixel_row_b0 = pixel_row_b++;
            texpixel_rgbaf_t *pixel_row_b1 = pixel_row_b++;
            texpixel_rgbaf_t *pixel_row_b2 = pixel_row_b++;
            texpixel_rgbaf_t *pixel_row_b3 = pixel_row_b++;

            // -- First row.

            pixel_row_a0->r = frag_a->r[0];
            pixel_row_a0->g = frag_a->g[0];
            pixel_row_a0->b = frag_a->b[0];
            pixel_row_a0->a = 1;

            pixel_row_a1->r = frag_a->r[1];
            pixel_row_a1->g = frag_a->g[1];
            pixel_row_a1->b = frag_a->b[1];
            pixel_row_a1->a = 1;

            pixel_row_a2->r = frag_b->r[0];
            pixel_row_a2->g = frag_b->g[0];
            pixel_row_a2->b = frag_b->b[0];
            pixel_row_a2->a = 1;

            pixel_row_a3->r = frag_b->r[1];
            pixel_row_a3->g = frag_b->g[1];
            pixel_row_a3->b = frag_b->b[1];
            pixel_row_a3->a = 1;

            // -- Second row.

            pixel_row_b0->r = frag_a->r[2];
            pixel_row_b0->g = frag_a->g[2];
            pixel_row_b0->b = frag_a->b[2];
            pixel_row_b0->a = 1;

            pixel_row_b1->r = frag_a->r[3];
            pixel_row_b1->g = frag_a->g[3];
            pixel_row_b1->b = frag_a->b[3];
            pixel_row_b1->a = 1;

            pixel_row_b2->r = frag_b->r[2];
            pixel_row_b2->g = frag_b->g[2];
            pixel_row_b2->b = frag_b->b[2];
            pixel_row_b2->a = 1;

            pixel_row_b3->r = frag_b->r[3];
            pixel_row_b3->g = frag_b->g[3];
            pixel_row_b3->b = frag_b->b[3];
            pixel_row_b3->a = 1;

            gl_gamma_linear_to_srgb_texpixel(pixel_row_a0);
            gl_gamma_linear_to_srgb_texpixel(pixel_row_a1);
            gl_gamma_linear_to_srgb_texpixel(pixel_row_a2);
            gl_gamma_linear_to_srgb_texpixel(pixel_row_a3);

            gl_gamma_linear_to_srgb_texpixel(pixel_row_b0);
            gl_gamma_linear_to_srgb_texpixel(pixel_row_b1);
            gl_gamma_linear_to_srgb_texpixel(pixel_row_b2);
            gl_gamma_linear_to_srgb_texpixel(pixel_row_b3);

            // *depth_row_a++ = frag_a->z[0];
            // *depth_row_a++ = frag_a->z[1];
            // *depth_row_a++ = frag_b->z[0];
            // *depth_row_a++ = frag_b->z[1];

            // *depth_row_b++ = frag_a->z[2];
            // *depth_row_b++ = frag_a->z[3];
            // *depth_row_b++ = frag_b->z[2];
            // *depth_row_b++ = frag_b->z[3];
        }

        color += frag_row_pitch;
        // depth += frag_row_pitch;
    }
}

// ----------------------------------------------------------------------------------
// -- Format RGBF.
// ----------------------------------------------------------------------------------

void load_tile_buffer_rgbf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
}

void save_tile_buffer_rgbf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
}

// ----------------------------------------------------------------------------------
// -- Format RGF.
// ----------------------------------------------------------------------------------

void load_tile_buffer_rgf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
#if 0
    OPTICK_EVENT("load_tile_buffer_rgf");

    texture2d_t *color_texture = framebuffer->color;
    gc_fragment_t *fragment = tile_buffer->fragments;
    u32 offset = tile_buffer->y * color_texture->mips->header->width + tile_buffer->x;
    u32 frag_row_pitch = GL_FRAG_HEIGHT * color_texture->mips->header->width;

    texpixel_rgf_t *color = (texpixel_rgf_t *) color_texture->mips->data + offset;
    r32 *depth = framebuffer->depth + offset;

    s32 max_x = tile_buffer->x + GL_BIN_WIDTH - 1;
    s32 max_y = tile_buffer->y + GL_BIN_HEIGHT - 1;
    s32 dx = (GL_FRAG_WIDTH << 1);

    for (s32 y = tile_buffer->y; y < max_y; y += GL_FRAG_HEIGHT)
    {
        texpixel_rgf_t *pixel_row_a = color;
        texpixel_rgf_t *pixel_row_b = color + color_texture->mips->header->width;

        r32 *depth_row_a = depth;
        r32 *depth_row_b = depth + color_texture->mips->header->width;

        for (s32 x = tile_buffer->x; x < max_x; x += dx)
        {
            gc_fragment_t *frag_a = fragment++;
            gc_fragment_t *frag_b = fragment++;

            texpixel_rgf_t *color_A0 = pixel_row_a++;
            texpixel_rgf_t *color_A1 = pixel_row_a++;
            texpixel_rgf_t *color_A2 = pixel_row_a++;
            texpixel_rgf_t *color_A3 = pixel_row_a++;

            texpixel_rgf_t *color_B0 = pixel_row_b++;
            texpixel_rgf_t *color_B1 = pixel_row_b++;
            texpixel_rgf_t *color_B2 = pixel_row_b++;
            texpixel_rgf_t *color_B3 = pixel_row_b++;

            frag_a->r[0] = color_A0->r;
            frag_a->r[1] = color_A1->r;
            frag_a->r[2] = color_B0->r;
            frag_a->r[3] = color_B1->r;

            frag_a->g[0] = color_A0->g;
            frag_a->g[1] = color_A1->g;
            frag_a->g[2] = color_B0->g;
            frag_a->g[3] = color_B1->g;

            frag_b->r[0] = color_A2->r;
            frag_b->r[1] = color_A3->r;
            frag_b->r[2] = color_B2->r;
            frag_b->r[3] = color_B3->r;

            frag_b->g[0] = color_A2->g;
            frag_b->g[1] = color_A3->g;
            frag_b->g[2] = color_B2->g;
            frag_b->g[3] = color_B3->g;

            frag_a->z[0] = *depth_row_a++;
            frag_a->z[1] = *depth_row_a++;
            frag_a->z[2] = *depth_row_b++;
            frag_a->z[3] = *depth_row_b++;

            frag_b->z[0] = *depth_row_a++;
            frag_b->z[1] = *depth_row_a++;
            frag_b->z[2] = *depth_row_b++;
            frag_b->z[3] = *depth_row_b++;

            frag_a->mask = 0;
            frag_b->mask = 0;
        }

        color += frag_row_pitch;
        depth += frag_row_pitch;
    }
#endif
}

void save_tile_buffer_rgf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
    texture2d_t *color_texture = framebuffer->color;
    gc_fragment_t *fragment = tile_buffer->fragments;

    u32 offset = tile_buffer->y * color_texture->mips->header->width + tile_buffer->x;
    u32 frag_row_pitch = GL_FRAG_HEIGHT * color_texture->mips->header->width;

    texpixel_rgf_t *color = (texpixel_rgf_t *) color_texture->mips->data + offset;
    // r32 *depth = framebuffer->depth + offset;

    s32 max_x = tile_buffer->x + GL_BIN_WIDTH - 1;
    s32 max_y = tile_buffer->y + GL_BIN_HEIGHT - 1;
    s32 dx = GL_FRAG_WIDTH << 1;

    for (s32 y = tile_buffer->y; y < max_y; y += GL_FRAG_HEIGHT)
    {
        texpixel_rgf_t *pixel_row_a = color;
        texpixel_rgf_t *pixel_row_b = color + color_texture->mips->header->width;

        // r32 *depth_row_a = depth;
        // r32 *depth_row_b = depth + color_texture->mips->header->width;

        for (s32 x = tile_buffer->x; x < max_x; x += dx)
        {
            gc_fragment_t *frag_a = fragment++;
            gc_fragment_t *frag_b = fragment++;

            texpixel_rgf_t *pixel_row_a0 = pixel_row_a++;
            texpixel_rgf_t *pixel_row_a1 = pixel_row_a++;
            texpixel_rgf_t *pixel_row_a2 = pixel_row_a++;
            texpixel_rgf_t *pixel_row_a3 = pixel_row_a++;

            texpixel_rgf_t *pixel_row_b0 = pixel_row_b++;
            texpixel_rgf_t *pixel_row_b1 = pixel_row_b++;
            texpixel_rgf_t *pixel_row_b2 = pixel_row_b++;
            texpixel_rgf_t *pixel_row_b3 = pixel_row_b++;

            // -- First row.

            pixel_row_a0->r = frag_a->r[0];
            pixel_row_a0->g = frag_a->g[0];

            pixel_row_a1->r = frag_a->r[1];
            pixel_row_a1->g = frag_a->g[1];

            pixel_row_a2->r = frag_b->r[0];
            pixel_row_a2->g = frag_b->g[0];

            pixel_row_a3->r = frag_b->r[1];
            pixel_row_a3->g = frag_b->g[1];

            // -- Second row.

            pixel_row_b0->r = frag_a->r[2];
            pixel_row_b0->g = frag_a->g[2];

            pixel_row_b1->r = frag_a->r[3];
            pixel_row_b1->g = frag_a->g[3];

            pixel_row_b2->r = frag_b->r[2];
            pixel_row_b2->g = frag_b->g[2];

            pixel_row_b3->r = frag_b->r[3];
            pixel_row_b3->g = frag_b->g[3];

            // *depth_row_a++ = frag_a->z[0];
            // *depth_row_a++ = frag_a->z[1];
            // *depth_row_a++ = frag_b->z[0];
            // *depth_row_a++ = frag_b->z[1];

            // *depth_row_b++ = frag_a->z[2];
            // *depth_row_b++ = frag_a->z[3];
            // *depth_row_b++ = frag_b->z[2];
            // *depth_row_b++ = frag_b->z[3];
        }

        color += frag_row_pitch;
        // depth += frag_row_pitch;
    }
}