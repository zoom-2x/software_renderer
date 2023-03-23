// ----------------------------------------------------------------------------------
// -- File: gcsr_sse_tilebuffer.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-06 09:48:01
// -- Modified: 2022-10-06 09:48:02
// ----------------------------------------------------------------------------------

__INLINE__ void _sse_post_fragment(gc_fragment_t *fragment)
{
    b8 is_postprocessing = PIPE_FLAG(GC_POST_PROCESSING);
    pipe_param_merged_table_t *overwrites = &GCSR.gl->pipeline.params.overwrites;

    __m128 r_4x = _mm_load_ps(fragment->r);
    __m128 g_4x = _mm_load_ps(fragment->g);
    __m128 b_4x = _mm_load_ps(fragment->b);

    if (is_postprocessing)
    {
        // ----------------------------------------------------------------------------------
        // -- Color tint.
        // ----------------------------------------------------------------------------------

        r_4x = _mm_mul_ps(r_4x, _mm_set1_ps(PIPE_PARAM_VECTOR(1, tint_color, 0)));
        g_4x = _mm_mul_ps(g_4x, _mm_set1_ps(PIPE_PARAM_VECTOR(1, tint_color, 1)));
        b_4x = _mm_mul_ps(b_4x, _mm_set1_ps(PIPE_PARAM_VECTOR(1, tint_color, 2)));

        // ----------------------------------------------------------------------------------
        // -- Saturation.
        // ----------------------------------------------------------------------------------

        if (PIPE_PARAM_VALUE_FLOAT(1, saturation) != 1.0f)
        {
            __m128 saturation = _mm_set1_ps(PIPE_PARAM_VALUE_FLOAT(1, saturation));

            __m128 sy = _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(r_4x, _mm_set1_ps(0.3f)),
                                _mm_mul_ps(g_4x, _mm_set1_ps(0.59f))),
                            _mm_mul_ps(b_4x, _mm_set1_ps(0.11f)));

            r_4x = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(r_4x, sy), saturation), sy);
            g_4x = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(g_4x, sy), saturation), sy);
            b_4x = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(b_4x, sy), saturation), sy);
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Tone mapping.
    // ----------------------------------------------------------------------------------

    if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_CLAMP)
    {
        r_4x = sse_clamp(r_4x, 0, 1);
        g_4x = sse_clamp(g_4x, 0, 1);
        b_4x = sse_clamp(b_4x, 0, 1);
    }
    else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_REINHARD)
    {
        r_4x = _mm_mul_ps(r_4x, _mm_rcp_ps(_mm_add_ps(_mm_set1_ps(1.0f), r_4x)));
        g_4x = _mm_mul_ps(g_4x, _mm_rcp_ps(_mm_add_ps(_mm_set1_ps(1.0f), g_4x)));
        b_4x = _mm_mul_ps(b_4x, _mm_rcp_ps(_mm_add_ps(_mm_set1_ps(1.0f), b_4x)));
    }
    else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_FILMIC)
    {
        r_4x = FILMIC_4x(r_4x);
        g_4x = FILMIC_4x(g_4x);
        b_4x = FILMIC_4x(b_4x);
    }
    else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_ACES)
    {}
    else if (PIPE_PARAM_VALUE_INTEGER(1, tone_mapping) == TONE_MAPPING_ACES_APROX)
    {
        r_4x = ACES_APROX_4x(r_4x);
        g_4x = ACES_APROX_4x(g_4x);
        b_4x = ACES_APROX_4x(b_4x);
    }

    _mm_store_ps(fragment->r, r_4x);
    _mm_store_ps(fragment->g, g_4x);
    _mm_store_ps(fragment->b, b_4x);
}

void _post_fragment(gc_fragment_t *fragment);

void sse_save_tile_buffer_rgbau8(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{
    OPTICK_EVENT("sse_save_tile_buffer_rgbau8");

    texture2d_t *color_texture = framebuffer->color;
    gc_fragment_t *fragment = tile_buffer->fragments;

    u32 offset = tile_buffer->y * color_texture->mips->header->width + tile_buffer->x;
    u32 frag_row_pitch = GL_FRAG_HEIGHT * color_texture->mips->header->width;

    u32 *color = (u32 *) color_texture->mips->data + offset;
    // r32 *depth = framebuffer->depth + offset;

    s32 max_x = tile_buffer->x + GL_BIN_WIDTH - 1;
    s32 max_y = tile_buffer->y + GL_BIN_HEIGHT - 1;
    s32 dx = GL_FRAG_WIDTH << 1;

    SSE_PACK_CONST();

    for (s32 y = tile_buffer->y; y < max_y; y += GL_FRAG_HEIGHT)
    {
        u32 *pixel_row_a = color;
        u32 *pixel_row_b = color + color_texture->mips->header->width;

        for (s32 x = tile_buffer->x; x < max_x; x += dx)
        {
            gc_fragment_t *frag_a = fragment++;
            gc_fragment_t *frag_b = fragment++;

            // _debug_fragment_x = x;
            // _debug_fragment_y = y;

            _sse_post_fragment(frag_a);
            _sse_post_fragment(frag_b);

            // _post_fragment(frag_a);
            // _post_fragment(frag_b);

            sse_color_t sse_color_a;
            sse_color_t sse_color_b;

            sse_color_a.r = _mm_load_ps(frag_a->r);
            sse_color_a.g = _mm_load_ps(frag_a->g);
            sse_color_a.b = _mm_load_ps(frag_a->b);

            sse_color_b.r = _mm_load_ps(frag_b->r);
            sse_color_b.g = _mm_load_ps(frag_b->g);
            sse_color_b.b = _mm_load_ps(frag_b->b);

            SSE_GAMMA_LINEAR_TO_SRGB(sse_color_a);
            SSE_GAMMA_LINEAR_TO_SRGB(sse_color_b);

            __m128i pack1_4x = SSE_PACK_COLOR(sse_color_a);
            __m128i pack2_4x = SSE_PACK_COLOR(sse_color_b);

            __m128i pack1s_4x = _mm_shuffle_epi32(pack1_4x, 0b01001110);
            __m128i pack2s_4x = _mm_shuffle_epi32(pack2_4x, 0b01001110);

            pack1_4x = _mm_castps_si128(_mm_blend_ps(_mm_castsi128_ps(pack1_4x), _mm_castsi128_ps(pack2s_4x), 0b00001100));
            pack2_4x = _mm_castps_si128(_mm_blend_ps(_mm_castsi128_ps(pack1s_4x), _mm_castsi128_ps(pack2_4x), 0b00001100));

            _mm_store_si128((__m128i *) pixel_row_a, pack1_4x);
            _mm_store_si128((__m128i *) pixel_row_b, pack2_4x);

            pixel_row_a += GC_FRAG_SIZE;
            pixel_row_b += GC_FRAG_SIZE;
        }

        color += frag_row_pitch;
    }
}

void sse_save_tile_buffer_rgbaf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{}

void sse_save_tile_buffer_rgf(gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer)
{}