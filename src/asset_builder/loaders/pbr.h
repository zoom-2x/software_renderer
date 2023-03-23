// ----------------------------------------------------------------------------------
// -- File: pbr.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-07-09 11:44:05
// -- Modified: 2022-07-09 11:44:06
// ----------------------------------------------------------------------------------

#define PI 3.14159265358979323846f
#define PI_OVER_ONE80 0.01745329251994329577
#define ONE80_OVER_PI 57.2957795130823208768
#define RAD2DEG(angle) ((r32) (angle * ONE80_OVER_PI))
#define DEG2RAD(angle) ((r32) (angle * PI_OVER_ONE80))

void samplef(u8 *texture, u32 width, u32 height, u32 components, r32 u, r32 v, gc_vec_t *output, u32 wrap, b8 filter);
void *combine_cubemap_facesf(cube_texture_t *texture, u32 *width, u32 *height, u32 *bytes, u8 mip_level);
void cube_seamless_filter(cube_texture_t *texture);

__INLINE__ void pixel3f_lerp(texpixel_rgbf_t *p1, texpixel_rgbf_t *p2, r32 t, texpixel_rgbf_t *out)
{
    out->r = p1->r + (p2->r - p1->r) * t;
    out->g = p1->g + (p2->g - p1->g) * t;
    out->b = p1->b + (p2->b - p1->b) * t;
}

void builder_cube_uv_from_vec(gc_vec_t *v, gc_vec_t *texcoord, u32 *face_index)
{
    // -- Generate the uv coordinates from the input fragment vectors.

    r32 absx = v->v3.x < 0 ? -v->v3.x : v->v3.x;
    r32 absy = v->v3.y < 0 ? -v->v3.y : v->v3.y;
    r32 absz = v->v3.z < 0 ? -v->v3.z : v->v3.z;

    b8 positive_x = v->v3.x > 0 ? true : false;
    b8 positive_y = v->v3.y > 0 ? true : false;
    b8 positive_z = v->v3.z > 0 ? true : false;

    r32 max_axis = 0;
    r32 tu = 0;
    r32 tv = 0;

    // Left.
    if (!positive_x && absx >= absy && absx >= absz)
    {
        max_axis = absx;
        tu = v->v3.y;
        tv = -v->v3.z;
        *face_index = CUBE_LEFT;
    }
    // Right.
    else if (positive_x && absx >= absy && absx >= absz)
    {
        max_axis = absx;
        tu = -v->v3.y;
        tv = -v->v3.z;
        *face_index = CUBE_RIGHT;
    }
    // Front.
    else if (positive_y && absy >= absx && absy >= absz)
    {
        max_axis = absy;
        tu = v->v3.x;
        tv = -v->v3.z;
        *face_index = CUBE_FRONT;
    }
    // Back.
    else if (!positive_y && absy >= absx && absy >= absz)
    {
        max_axis = absy;
        tu = -v->v3.x;
        tv = -v->v3.z;
        *face_index = CUBE_BACK;
    }
    // Top.
    else if (positive_z && absz >= absx && absz >= absy)
    {
        max_axis = absz;
        tu = v->v3.x;
        tv = v->v3.y;
        *face_index = CUBE_TOP;
    }
    // Bottom.
    else if (!positive_z && absz >= absx && absz >= absy)
    {
        max_axis = absz;
        tu = v->v3.x;
        tv = -v->v3.y;
        *face_index = CUBE_BOTTOM;
    }

    texcoord->v2.x = 0.5f * (tu / max_axis + 1.0f);
    texcoord->v2.y = 0.5f * (tv / max_axis + 1.0f);
}

void sse_cube_uv_from_vec(fv3_t *v, fv2_t *texcoord, u32 *face_index)
{
    // -- Generate the uv coordinates from the input fragment vectors.

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 absx = v->x[i] < 0 ? -v->x[i] : v->x[i];
        r32 absy = v->y[i] < 0 ? -v->y[i] : v->y[i];
        r32 absz = v->z[i] < 0 ? -v->z[i] : v->z[i];

        b8 positive_x = v->x[i] > 0 ? true : false;
        b8 positive_y = v->y[i] > 0 ? true : false;
        b8 positive_z = v->z[i] > 0 ? true : false;

        r32 max_axis = 0;
        r32 tu = 0;
        r32 tv = 0;

        // Left.
        if (!positive_x && absx >= absy && absx >= absz)
        {
            max_axis = absx;
            tu = v->y[i];
            tv = -v->z[i];
            face_index[i] = CUBE_LEFT;
        }
        // Right.
        else if (positive_x && absx >= absy && absx >= absz)
        {
            max_axis = absx;
            tu = -v->y[i];
            tv = -v->z[i];
            face_index[i] = CUBE_RIGHT;
        }
        // Front.
        else if (positive_y && absy >= absx && absy >= absz)
        {
            max_axis = absy;
            tu = v->x[i];
            tv = -v->z[i];
            face_index[i] = CUBE_FRONT;
        }
        // Back.
        else if (!positive_y && absy >= absx && absy >= absz)
        {
            max_axis = absy;
            tu = -v->x[i];
            tv = -v->z[i];
            face_index[i] = CUBE_BACK;
        }
        // Top.
        else if (positive_z && absz >= absx && absz >= absy)
        {
            max_axis = absz;
            tu = v->x[i];
            tv = v->y[i];
            face_index[i] = CUBE_TOP;
        }
        // Bottom.
        else if (!positive_z && absz >= absx && absz >= absy)
        {
            max_axis = absz;
            tu = v->x[i];
            tv = -v->y[i];
            face_index[i] = CUBE_BOTTOM;
        }

        texcoord->x[i] = 0.5f * (tu / max_axis + 1.0f);
        texcoord->y[i] = 0.5f * (tv / max_axis + 1.0f);
    }
}

__INLINE__ r32 tex_wrap(r32 v, u32 wrap)
{
    if (wrap == TEXTURE_WRAP_CLAMP)
        v = _tex_clamp_s(v);
    else if (wrap == TEXTURE_WRAP_REPEAT)
        v = _tex_repeat_s(v);
    else if (wrap == TEXTURE_WRAP_MIRROR)
        v = _tex_mirror_s(v);

    return v;
}

void _sse_tex_clamp(r32 *in_u, r32 *in_v, r32 *out_u, r32 *out_v)
{
    __m128 u_4x = _mm_load_ps(in_u);
    __m128 v_4x = _mm_load_ps(in_v);

    u_4x = _mm_max_ps(_mm_min_ps(u_4x, _mm_set1_ps(1.0f)), _mm_setzero_ps());
    v_4x = _mm_max_ps(_mm_min_ps(v_4x, _mm_set1_ps(1.0f)), _mm_setzero_ps());

    _mm_store_ps(out_u, u_4x);
    _mm_store_ps(out_v, v_4x);
}

b8 debug_sample_vectors = false;
#define SSE_TEX_OFFSET(x, y, pitch) _mm_add_epi32(_mm_mullo_epi32(y, pitch), x)

__INLINE__ void sse_extract_rgbaf(r32 *s0, r32 *s1, r32 *s2, r32 *s3, sse_color_t *output)
{
    __m128 s0_4x = _mm_load_ps(s0);
    __m128 s1_4x = _mm_load_ps(s1);
    __m128 s2_4x = _mm_load_ps(s2);
    __m128 s3_4x = _mm_load_ps(s3);

    __m128 t1 = _mm_shuffle_ps(s0_4x, s1_4x, 0b01000100); // r0g0r1g1
    __m128 t2 = _mm_shuffle_ps(s0_4x, s1_4x, 0b11101110); // b0a0b1a1
    __m128 t3 = _mm_shuffle_ps(s2_4x, s3_4x, 0b01000100); // r2g2r3g3
    __m128 t4 = _mm_shuffle_ps(s2_4x, s3_4x, 0b11101110); // b2a2b3a3

    output->r = _mm_shuffle_ps(t1, t3, 0b10001000);
    output->g = _mm_shuffle_ps(t1, t3, 0b11011101);
    output->b = _mm_shuffle_ps(t2, t4, 0b10001000);
    output->a = _mm_shuffle_ps(t2, t4, 0b11011101);
}

void sse_cube_samplef(cube_texture_t *cubemap, u32 *face_index, r32 *u, r32 *v, shader_color_t *output)
{
    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];
    u32 offset[GC_FRAG_SIZE];

    texture2d_t *face_0 = cubemap->faces[face_index[0]];
    texture2d_t *face_1 = cubemap->faces[face_index[1]];
    texture2d_t *face_2 = cubemap->faces[face_index[2]];
    texture2d_t *face_3 = cubemap->faces[face_index[3]];

    texture_mip_t *selected_texture_0 = face_0->mips;
    texture_mip_t *selected_texture_1 = face_1->mips;
    texture_mip_t *selected_texture_2 = face_2->mips;
    texture_mip_t *selected_texture_3 = face_3->mips;

    texpixel_rgbaf_t *texture_pointer_0 = (texpixel_rgbaf_t *) selected_texture_0->data;
    texpixel_rgbaf_t *texture_pointer_1 = (texpixel_rgbaf_t *) selected_texture_1->data;
    texpixel_rgbaf_t *texture_pointer_2 = (texpixel_rgbaf_t *) selected_texture_2->data;
    texpixel_rgbaf_t *texture_pointer_3 = (texpixel_rgbaf_t *) selected_texture_3->data;

    __m128 width_4x = _mm_setr_ps(
        selected_texture_0->header->width - 1,
        selected_texture_1->header->width - 1,
        selected_texture_2->header->width - 1,
        selected_texture_3->header->width - 1
    );

    __m128 height_4x = _mm_setr_ps(
        selected_texture_0->header->height - 1,
        selected_texture_1->header->height - 1,
        selected_texture_2->header->height - 1,
        selected_texture_3->header->height - 1
    );

    __m128i pitch_4x = _mm_setr_epi32(
        selected_texture_0->header->width,
        selected_texture_1->header->width,
        selected_texture_2->header->width,
        selected_texture_3->header->width
    );

    __m128 tex_du_4x = _mm_setr_ps(
        selected_texture_0->header->tex_du,
        selected_texture_1->header->tex_du,
        selected_texture_2->header->tex_du,
        selected_texture_3->header->tex_du
    );

    __m128 tex_dv_4x = _mm_setr_ps(
        selected_texture_0->header->tex_dv,
        selected_texture_1->header->tex_dv,
        selected_texture_2->header->tex_dv,
        selected_texture_3->header->tex_dv
    );

    _sse_tex_clamp(u, v, buffer0, buffer1);

    __m128 u0_4x = _mm_load_ps(buffer0);
    __m128 v0_4x = _mm_load_ps(buffer1);

    __m128 tex0_x_4x = _mm_mul_ps(u0_4x, width_4x);
    __m128 tex0_y_4x = _mm_mul_ps(v0_4x, height_4x);

    __m128i x0_4x = _mm_cvttps_epi32(tex0_x_4x);
    __m128i y0_4x = _mm_cvttps_epi32(tex0_y_4x);

    __m128i offset_4x = SSE_TEX_OFFSET(x0_4x, y0_4x, pitch_4x);
    _mm_store_si128((__m128i *) offset, offset_4x);

    sse_color_t sample0_4x;

    texpixel_rgbaf_t *sample0 = texture_pointer_0 + offset[0];
    texpixel_rgbaf_t *sample1 = texture_pointer_1 + offset[1];
    texpixel_rgbaf_t *sample2 = texture_pointer_2 + offset[2];
    texpixel_rgbaf_t *sample3 = texture_pointer_3 + offset[3];

    sse_extract_rgbaf((r32 *) sample0, (r32 *) sample1, (r32 *) sample2, (r32 *) sample3, &sample0_4x);

    sse_color_t sample1_4x;
    sse_color_t sample2_4x;
    sse_color_t sample3_4x;

    __m128 tx_4x = _mm_sub_ps(tex0_x_4x, _mm_floor_ps(tex0_x_4x));
    __m128 ty_4x = _mm_sub_ps(tex0_y_4x, _mm_floor_ps(tex0_y_4x));

    __m128 u1_4x = _mm_add_ps(u0_4x, tex_du_4x);
    __m128 v1_4x = _mm_add_ps(v0_4x, tex_dv_4x);

    _mm_store_ps(buffer0, u1_4x);
    _mm_store_ps(buffer1, v1_4x);

    _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1);

    u1_4x = _mm_load_ps(buffer0);
    v1_4x = _mm_load_ps(buffer1);

    __m128 tex1_x_4x = _mm_mul_ps(u1_4x, width_4x);
    __m128 tex1_y_4x = _mm_mul_ps(v1_4x, height_4x);

    __m128i x1_4x = _mm_cvttps_epi32(tex1_x_4x);
    __m128i y1_4x = _mm_cvttps_epi32(tex1_y_4x);

    // -- Sample 1.

    offset_4x = SSE_TEX_OFFSET(x1_4x, y0_4x, pitch_4x);
    _mm_store_si128((__m128i *) offset, offset_4x);

    sample0 = texture_pointer_0 + offset[0];
    sample1 = texture_pointer_1 + offset[1];
    sample2 = texture_pointer_2 + offset[2];
    sample3 = texture_pointer_3 + offset[3];

    sse_extract_rgbaf((r32 *) sample0, (r32 *) sample1, (r32 *) sample2, (r32 *) sample3, &sample1_4x);

    // -- Sample 2.

    offset_4x = SSE_TEX_OFFSET(x0_4x, y1_4x, pitch_4x);
    _mm_store_si128((__m128i *) offset, offset_4x);

    sample0 = texture_pointer_0 + offset[0];
    sample1 = texture_pointer_1 + offset[1];
    sample2 = texture_pointer_2 + offset[2];
    sample3 = texture_pointer_3 + offset[3];

    sse_extract_rgbaf((r32 *) sample0, (r32 *) sample1, (r32 *) sample2, (r32 *) sample3, &sample2_4x);

    // -- Sample 3.

    offset_4x = SSE_TEX_OFFSET(x1_4x, y1_4x, pitch_4x);
    _mm_store_si128((__m128i *) offset, offset_4x);

    sample0 = texture_pointer_0 + offset[0];
    sample1 = texture_pointer_1 + offset[1];
    sample2 = texture_pointer_2 + offset[2];
    sample3 = texture_pointer_3 + offset[3];

    sse_extract_rgbaf((r32 *) sample0, (r32 *) sample1, (r32 *) sample2, (r32 *) sample3, &sample3_4x);

    // -- Blending.

    sample0_4x.r = _mm_add_ps(sample0_4x.r, _mm_mul_ps(_mm_sub_ps(sample1_4x.r, sample0_4x.r), tx_4x));
    sample0_4x.g = _mm_add_ps(sample0_4x.g, _mm_mul_ps(_mm_sub_ps(sample1_4x.g, sample0_4x.g), tx_4x));
    sample0_4x.b = _mm_add_ps(sample0_4x.b, _mm_mul_ps(_mm_sub_ps(sample1_4x.b, sample0_4x.b), tx_4x));
    sample0_4x.a = _mm_add_ps(sample0_4x.a, _mm_mul_ps(_mm_sub_ps(sample1_4x.a, sample0_4x.a), tx_4x));

    sample1_4x.r = _mm_add_ps(sample2_4x.r, _mm_mul_ps(_mm_sub_ps(sample3_4x.r, sample2_4x.r), tx_4x));
    sample1_4x.g = _mm_add_ps(sample2_4x.g, _mm_mul_ps(_mm_sub_ps(sample3_4x.g, sample2_4x.g), tx_4x));
    sample1_4x.b = _mm_add_ps(sample2_4x.b, _mm_mul_ps(_mm_sub_ps(sample3_4x.b, sample2_4x.b), tx_4x));
    sample1_4x.a = _mm_add_ps(sample2_4x.a, _mm_mul_ps(_mm_sub_ps(sample3_4x.a, sample2_4x.a), tx_4x));

    sample0_4x.r = _mm_add_ps(sample0_4x.r, _mm_mul_ps(_mm_sub_ps(sample1_4x.r, sample0_4x.r), ty_4x));
    sample0_4x.g = _mm_add_ps(sample0_4x.g, _mm_mul_ps(_mm_sub_ps(sample1_4x.g, sample0_4x.g), ty_4x));
    sample0_4x.b = _mm_add_ps(sample0_4x.b, _mm_mul_ps(_mm_sub_ps(sample1_4x.b, sample0_4x.b), ty_4x));
    sample0_4x.a = _mm_add_ps(sample0_4x.a, _mm_mul_ps(_mm_sub_ps(sample1_4x.a, sample0_4x.a), ty_4x));

    _mm_store_ps(output->r, sample0_4x.r);
    _mm_store_ps(output->g, sample0_4x.g);
    _mm_store_ps(output->b, sample0_4x.b);
    _mm_store_ps(output->a, sample0_4x.a);
}

void samplef(u8 *texture, u32 width, u32 height, u32 components, r32 u, r32 v, gc_vec_t *output, u32 wrap, b8 filter)
{
    u32 row_pitch = width * components * sizeof(r32);
    u32 col_pitch = components * sizeof(r32);

    r32 tex_du = 1.0f / width;
    r32 tex_dv = 1.0f / height;

    u32 tex_width = width - 1;
    u32 tex_height = height - 1;

    r32 u0 = tex_wrap(u, wrap);
    r32 v0 = tex_wrap(v, wrap);

    r32 tex_x = u0 * tex_width;
    r32 tex_y = v0 * tex_height;

    u32 texel_x0 = (u32) tex_x;
    u32 texel_y0 = (u32) tex_y;

    // if (tex_x - texel_x0 > 0.9)
    //     texel_x0++;

    // if (tex_y - texel_y0 > 0.9)
    //     texel_y0++;

#if defined(DEBUG_PREFILTERED)
    if (debug_sample_vectors)
    {
        printf("texel x: %f\n", tex_x);
        printf("texel y: %f\n", tex_y);
    }
#endif

    u32 offset_0 = texel_y0 * row_pitch + texel_x0 * col_pitch;
    gc_vec_t *sample_0 = (gc_vec_t *) (texture + offset_0);

    if (filter)
    {
        r32 tx = tex_x - (u32) tex_x;
        r32 ty = tex_y - (u32) tex_y;

        // r32 threshold = 0.8f;

        // if (tx >= threshold)
        //     tx = 0.9f;
        // else
        //     tx = 0;

        // if (ty >= threshold)
        //     ty = 0.9f;
        // else
        //     ty = 0;

        r32 u1 = tex_wrap(u + tex_du, wrap);
        r32 v1 = v0;

        r32 u2 = u0;
        r32 v2 = tex_wrap(v + tex_dv, wrap);

        r32 u3 = u1;
        r32 v3 = v2;

        u32 texel_x1 = (u32) (u1 * tex_width);
        u32 texel_y1 = (u32) (v1 * tex_height);

        u32 texel_x2 = (u32) (u2 * tex_width);
        u32 texel_y2 = (u32) (v2 * tex_height);

        u32 texel_x3 = (u32) (u3 * tex_width);
        u32 texel_y3 = (u32) (v3 * tex_height);

        u32 offset_1 = texel_y1 * row_pitch + texel_x1 * col_pitch;
        u32 offset_2 = texel_y2 * row_pitch + texel_x2 * col_pitch;
        u32 offset_3 = texel_y3 * row_pitch + texel_x3 * col_pitch;

        gc_vec_t *sample_1 = (gc_vec_t *) (texture + offset_1);
        gc_vec_t *sample_2 = (gc_vec_t *) (texture + offset_2);
        gc_vec_t *sample_3 = (gc_vec_t *) (texture + offset_3);

        VINIT3(t1,
               sample_0->c.r + (sample_1->c.r - sample_0->c.r) * tx,
               sample_0->c.g + (sample_1->c.g - sample_0->c.g) * tx,
               sample_0->c.b + (sample_1->c.b - sample_0->c.b) * tx);

        VINIT3(t2,
               sample_2->c.r + (sample_3->c.r - sample_2->c.r) * tx,
               sample_2->c.g + (sample_3->c.g - sample_2->c.g) * tx,
               sample_2->c.b + (sample_3->c.b - sample_2->c.b) * tx);

        output->c.r = t1.c.r + (t2.c.r - t1.c.r) * ty;
        output->c.g = t1.c.g + (t2.c.g - t1.c.g) * ty;
        output->c.b = t1.c.b + (t2.c.b - t1.c.b) * ty;

        if (components == 4)
        {
            t1.c.a = sample_0->c.a + (sample_1->c.a - sample_0->c.a) * tx;
            t2.c.a = sample_2->c.a + (sample_3->c.a - sample_2->c.a) * tx;

            output->c.a = t1.c.a + (t2.c.a - t1.c.a) * ty;
        }
        else
            output->c.a = 1.0f;
    }
    else
    {
        output->c.r = sample_0->c.r;
        output->c.g = sample_0->c.g;
        output->c.b = sample_0->c.b;

        if (components == 4)
            output->c.a = sample_0->c.a;
        else
            output->c.a = 1.0f;
    }
}

void cube_samplef(cube_texture_t *cubemap, gc_vec_t *vec, u32 lod, gc_vec_t *out)
{
    u32 face_index = 0;
    gc_vec_t texcoord;

    lod = lod >= 0 ? lod : 0;
    lod = lod < cubemap->mip_count ? lod : cubemap->mip_count - 1;

    builder_cube_uv_from_vec(vec, &texcoord, &face_index);
    texture2d_t *face = cubemap->faces[face_index];
    texture_mip_t *base_mip_level = face->mips + lod;

    if (debug_sample_vectors)
    {
        printf("lod: %u\n", lod);
    }

    #ifdef PBR_CUBEMAP_FILTERING
    samplef((u8 *) base_mip_level->data,
            base_mip_level->header->width,
            base_mip_level->header->height,
            4,
            texcoord.v2.x, texcoord.v2.y,
            out,
            TEXTURE_WRAP_CLAMP,
            true);
    #else
    samplef((u8 *) base_mip_level->data,
            base_mip_level->header->width,
            base_mip_level->header->height,
            4,
            texcoord.v2.x, texcoord.v2.y,
            out,
            TEXTURE_WRAP_CLAMP,
            false);
    #endif
}

static char names[6][20] = { "left", "right", "front", "back", "top", "bottom" };
static char string_buffer[255];

void output_cubemap(cube_texture_t *cubemap, char *alias, const char *type, b8 mipmaps, u8 combined_mip_level)
{
    gc_file_t output_bmp;

    u32 width = 0;
    u32 height = 0;
    u32 bytes = 0;

    for (u8 i = 0; i < 6; ++i)
    {
        texture2d_t *face = cubemap->faces[i];
        texture_mip_t *mips = face->mips;

        sprintf(string_buffer, "%s%s.%s.%s.%s", DEBUG_FOLDER, alias, type, names[i], "bmp");
        open_file(&output_bmp, string_buffer, GC_FILE_WRITE);
        export_bitmapf(&output_bmp, (r32 *) mips->data, mips->header->width, mips->header->height, 4);

        if (mipmaps && cubemap->mip_count > 1)
        {
            for (u8 j = 1; j < cubemap->mip_count; ++j)
            {
                texture_mip_t *mip = mips + j;

                sprintf(string_buffer, "%s%s.mip.%s.%s.%u.%s", DEBUG_FOLDER, alias, type, names[i], j, "bmp");
                open_file(&output_bmp, string_buffer, GC_FILE_WRITE);
                export_bitmapf(&output_bmp, (r32 *) mip->data, mip->header->width, mip->header->height, 4);
            }
        }

        close_file(&output_bmp);
    }

    void *combined_cubemap = combine_cubemap_facesf(cubemap, &width, &height, &bytes, combined_mip_level);

    if (combined_cubemap)
    {
        sprintf(string_buffer, "%s%s.%s.combined.%s", DEBUG_FOLDER, alias, type, "bmp");
        open_file(&output_bmp, string_buffer, GC_FILE_WRITE);
        export_bitmapf(&output_bmp, (r32 *) combined_cubemap, width, height, 4);
        close_file(&output_bmp);
        free(combined_cubemap);
    }
}

void output_brdf_lut(texture2d_t *brdf_lut, char *alias)
{
    gc_file_t output_bmp;

    sprintf(string_buffer, "%s%s.brdf_lut.%s", DEBUG_FOLDER, alias, "bmp");
    open_file(&output_bmp, string_buffer, GC_FILE_WRITE);

    texpixel_rgf_t *src = (texpixel_rgf_t *) brdf_lut->mips->data;
    bitmap_header_t header;

    size_t data_bytes = brdf_lut->mips->header->width * brdf_lut->mips->header->height * sizeof(u32);
    header.file_type = 0x4D42;
    header.file_size = (u32) (sizeof(bitmap_header_t) + data_bytes);
    header.bitmap_offset = sizeof(bitmap_header_t);
    header.size = 108; // v4 bitmap header !
    header.width = brdf_lut->mips->header->width;
    header.height = - (s32) brdf_lut->mips->header->height;
    header.bits_per_pixel = 32;
    header.size_of_bitmap = (u32) data_bytes;

    // Internal fixed bitmap pixel format is used.
    header.red_mask = 0x00ff0000;
    header.green_mask = 0x0000ff00;
    header.blue_mask = 0x000000ff;
    header.alpha_mask = 0xff000000;

    header.cstype = 0x73524742;
    header.planes = 1;
    header.compression = 3;

    write_file(&output_bmp, sizeof(bitmap_header_t), &header);

    for (u32 y = 0; y < brdf_lut->mips->header->height; ++y)
    {
        for (u32 x = 0; x < brdf_lut->mips->header->width; ++x)
        {
            r32 r = src->r;
            r32 g = src->g;
            r32 b = 0;
            r32 a = 1.0f;

            r = r / (r + 1);
            g = g / (g + 1);
            b = b / (b + 1);

            u8 r8 = (u8) (r * 255);
            u8 g8 = (u8) (g * 255);
            u8 b8 = (u8) (b * 255);
            u8 a8 = (u8) (a * 255);

            u32 pixel_color = a8 << 24 | r8 << 16 | g8 << 8 | b8;
            write_file(&output_bmp, sizeof(u32), &pixel_color);

            src++;
        }
    }

    close_file(&output_bmp);
}

r32 distribution_ggx(gc_vec_t *n, gc_vec_t *h, r32 roughness)
{
    r32 a = roughness * roughness;
    r32 a2 = a * a;
    r32 nh_dot = v3_dot(n, h);

    if (nh_dot < 0)
        nh_dot = 0;

    r32 nh_dot2 = nh_dot * nh_dot;
    r32 nom = a2;
    r32 denom = (nh_dot2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

void cube_seamless_filter(cube_texture_t *cubemap)
{
    r32 t = 0.5f;

    for (u8 mip_level = 0; mip_level < cubemap->mip_count; ++mip_level)
    {
        texture_mip_t *front = cubemap->faces[CUBE_FRONT]->mips + mip_level;
        texture_mip_t *back = cubemap->faces[CUBE_BACK]->mips + mip_level;
        texture_mip_t *top = cubemap->faces[CUBE_TOP]->mips + mip_level;
        texture_mip_t *bottom = cubemap->faces[CUBE_BOTTOM]->mips + mip_level;
        texture_mip_t *left = cubemap->faces[CUBE_LEFT]->mips + mip_level;
        texture_mip_t *right = cubemap->faces[CUBE_RIGHT]->mips + mip_level;

        // ----------------------------------------------------------------------------------
        // -- Front face.
        // ----------------------------------------------------------------------------------

        // -- Top to front seam.

        texpixel_rgbaf_t *src = (texpixel_rgbaf_t *) top->data + (top->header->height - 1) * top->header->width;
        texpixel_rgbaf_t *dest = (texpixel_rgbaf_t *) front->data;

        for (u32 x = 0; x < front->header->width; ++x)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src++;
            dest++;
        }

        // -- Left to front seam.

        src = (texpixel_rgbaf_t *) left->data + left->header->width - 1;
        dest = (texpixel_rgbaf_t *) front->data;

        for (u32 y = 0; y < front->header->height; ++y)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src += left->header->width;
            dest += front->header->width;
        }

        // ----------------------------------------------------------------------------------
        // -- Right face.
        // ----------------------------------------------------------------------------------

        // -- Top to right seam.

        src = (texpixel_rgbaf_t *) top->data + (top->header->height - 1) * top->header->width + top->header->width - 1;
        dest = (texpixel_rgbaf_t *) right->data;

        for (u32 x = 0; x < right->header->width; ++x)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src -= top->header->width;
            dest++;
        }

        // -- Front to right seam.

        src = (texpixel_rgbaf_t *) front->data + front->header->width - 1;
        dest = (texpixel_rgbaf_t *) right->data;

        for (u32 y = 0; y < right->header->height; ++y)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src += front->header->width;
            dest += right->header->width;
        }

        // ----------------------------------------------------------------------------------
        // -- Left face.
        // ----------------------------------------------------------------------------------

        // -- Top to left seam.

        src = (texpixel_rgbaf_t *) top->data;
        dest = (texpixel_rgbaf_t *) left->data;

        for (u32 x = 0; x < left->header->width; ++x)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src += top->header->width;
            dest++;
        }

        // -- Back to left seam.

        src = (texpixel_rgbaf_t *) back->data + back->header->width - 1;
        dest = (texpixel_rgbaf_t *) left->data;

        for (u32 y = 0; y < left->header->height; ++y)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src += back->header->width;
            dest += left->header->width;
        }

        // ----------------------------------------------------------------------------------
        // -- Bottom face.
        // ----------------------------------------------------------------------------------

        // -- Front to bottom seam.

        src = (texpixel_rgbaf_t *) front->data + (front->header->height - 1) * front->header->width;
        dest = (texpixel_rgbaf_t *) bottom->data;

        for (u32 x = 0; x < bottom->header->width; ++x)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src++;
            dest++;
        }

        // -- Left to bottom seam.

        src = (texpixel_rgbaf_t *) left->data + (left->header->width - 1) + (left->header->height - 1) * left->header->width;
        dest = (texpixel_rgbaf_t *) bottom->data;

        for (u32 y = 0; y < bottom->header->height; ++y)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src--;
            dest += bottom->header->width;
        }

        // -- Back to bottom seam.

        src = (texpixel_rgbaf_t *) back->data + (back->header->width - 1) + (back->header->height - 1) * back->header->width;
        dest = (texpixel_rgbaf_t *) bottom->data + bottom->header->width * (bottom->header->height - 1);

        for (u32 x = 0; x < bottom->header->width; ++x)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src--;
            dest++;
        }

        // -- Right to bottom seam.

        src = (texpixel_rgbaf_t *) right->data + (right->header->height - 1) * right->header->width;
        dest = (texpixel_rgbaf_t *) bottom->data + (bottom->header->width - 1);

        for (u32 y = 0; y < bottom->header->height; ++y)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src++;
            dest += bottom->header->width;
        }

        // ----------------------------------------------------------------------------------
        // -- Back face.
        // ----------------------------------------------------------------------------------

        // -- Top to back seam.

        src = (texpixel_rgbaf_t *) top->data + top->header->width - 1;
        dest = (texpixel_rgbaf_t *) back->data;

        for (u32 x = 0; x < back->header->width; ++x)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src--;
            dest++;
        }

        // -- Right to back seam.

        src = (texpixel_rgbaf_t *) right->data + right->header->width - 1;
        dest = (texpixel_rgbaf_t *) back->data;

        for (u32 y = 0; y < back->header->height; ++y)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src += right->header->width;
            dest += back->header->width;
        }

        // ----------------------------------------------------------------------------------
        // -- Top face.
        // ----------------------------------------------------------------------------------

        // -- Back to top seam.

        src = (texpixel_rgbaf_t *) back->data + back->header->width - 1;
        dest = (texpixel_rgbaf_t *) top->data;

        for (u32 x = 0; x < top->header->width; ++x)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src--;
            dest++;
        }

        // -- Left to top seam.

        src = (texpixel_rgbaf_t *) left->data;
        dest = (texpixel_rgbaf_t *) top->data;

        for (u32 y = 0; y < top->header->height; ++y)
        {
            pixel3f_lerp((texpixel_rgbf_t *) src, (texpixel_rgbf_t *) dest, t, (texpixel_rgbf_t *) dest);
            src++;
            dest += top->header->width;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- IBL generation.
// ----------------------------------------------------------------------------------

r32 radical_inverse_vdc(u32 bits)
{
    r32 res = 0;

    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
    bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
    bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
    res = bits * 2.3283064365386963e-10;

    return res;
}

__INLINE__ void hammersley(u32 i, u32 n, gc_vec_t *out)
{
    out->v2.x = (r32) i / n;
    out->v2.y = radical_inverse_vdc(i);
}

void importance_sample_ggx(gc_vec_t *xi, gc_vec_t *normal, r32 roughness, gc_vec_t *out)
{
    r32 a = roughness * roughness;

    r32 phi = 2.0f * PI * xi->v2.x;
    r32 cos_theta = sqrt((1.0f - xi->v2.y) / (1.0f + (a * a - 1.0f) * xi->v2.y));
    r32 sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    // Tangent-space half vector.
    VINIT3(h, sin_theta * cosf(phi), sinf(phi) * sin_theta, cos_theta);

    // Tangent-space to world-space.
    gc_vec_t up = {0.0f, 0.0f, 1.0f};
    gc_vec_t tangent;
    gc_vec_t btangent;

    if (normal->v3.z > 0.99f || normal->v3.z < -0.99f)
    {
        up.v3.x = 1.0f;
        up.v3.y = 0.0f;
        up.v3.z = 0.0f;
    }

    v3_cross(&up, normal, &tangent);
    v3_cross(normal, &tangent, &btangent);

    v3_normalize(&tangent);
    v3_normalize(&btangent);

    out->v3.x = tangent.v3.x * h.v3.x + btangent.v3.x * h.v3.y + normal->v3.x * h.v3.z;
    out->v3.y = tangent.v3.y * h.v3.x + btangent.v3.y * h.v3.y + normal->v3.y * h.v3.z;
    out->v3.z = tangent.v3.z * h.v3.x + btangent.v3.z * h.v3.y + normal->v3.z * h.v3.z;

    v3_normalize(out);
}

// ----------------------------------------------------------------------------------

void face_position_vector(u32 x, u32 y, u32 face_size, gl_cube_faces_t face, gc_vec_t *vec)
{
    r32 xf = x + 0.5f;
    r32 yf = y + 0.5f;

    if (face == CUBE_FRONT)
    {
        vec->v3.x = ((xf / face_size) * 2 - 1.0f);
        vec->v3.y = 1;
        vec->v3.z = -((yf / face_size) * 2 - 1.0f);
    }
    else if (face == CUBE_BACK)
    {
        vec->v3.x = -((xf / face_size) * 2 - 1.0f);
        vec->v3.y = -1;
        vec->v3.z = -((yf / face_size) * 2 - 1.0f);
    }
    else if (face == CUBE_LEFT)
    {
        vec->v3.x = -1;
        vec->v3.y = (xf / face_size) * 2 - 1.0f;
        vec->v3.z = -((yf / face_size) * 2 - 1.0f);
    }
    else if (face == CUBE_RIGHT)
    {
        vec->v3.x = 1;
        vec->v3.y = -((xf / face_size) * 2 - 1.0f);
        vec->v3.z = -((yf / face_size) * 2 - 1.0f);
    }
    else if (face == CUBE_TOP)
    {
        vec->v3.x = (xf / face_size) * 2 - 1.0f;
        vec->v3.y = (yf / face_size) * 2 - 1.0f;
        vec->v3.z = 1;
    }
    else if (face == CUBE_BOTTOM)
    {
        vec->v3.x = (xf / face_size) * 2 - 1.0f;
        vec->v3.y = -((yf / face_size) * 2 - 1.0f);
        vec->v3.z = -1;
    }

    v3_normalize(vec);
}

void _environment_pixel_sample(void *source, u32 hdr_width, u32 hdr_height, gc_vec_t *v, texpixel_rgbaf_t *pixel)
{
    r32 len = sqrt(v->v3.x * v->v3.x + v->v3.y * v->v3.y + v->v3.z * v->v3.z);

    r32 phi = asin(v->v3.z / len);
    r32 theta = atan2(v->v3.x / len, v->v3.y / len);

    r32 tu = theta / (2 * PI) + 0.5f;
    r32 tv = 0.5f - phi / PI;

    gc_vec_t sampled_color;

    #ifdef PBR_CUBEMAP_FILTERING
    samplef((u8 *) source, hdr_width, hdr_height, 3, tu, tv, &sampled_color, TEXTURE_WRAP_CLAMP, true);
    #else
    samplef((u8 *) source, hdr_width, hdr_height, 3, tu, tv, &sampled_color, TEXTURE_WRAP_CLAMP, false);
    #endif

    pixel->r = sampled_color.c.r;
    pixel->g = sampled_color.c.g;
    pixel->b = sampled_color.c.b;
    pixel->a = 1.0f;
}

void generate_environment_face(cube_texture_t *environment, gl_cube_faces_t face, texpixel_rgbf_t *source, hdr_data_t *hdr_data)
{
    if (face == 0)
        PROGRESS("\r > generating environment: %u%%", 0, CUBE_TOTAL);

    texture_mip_t *base = environment->faces[face]->mips;
    texpixel_rgbaf_t *pixel = (texpixel_rgbaf_t *) base->data;
    gc_vec_t vec;

    // ----------------------------------------------------------------------------------
    // -- Copy the base texture from the hdr image (mip 0).
    // ----------------------------------------------------------------------------------

    for (u32 y = 0; y < base->header->height; ++y)
    {
        for (u32 x = 0; x < base->header->width; ++x)
        {
            face_position_vector(x, y, base->header->width, face, &vec);
            _environment_pixel_sample(source, hdr_data->width, hdr_data->height, &vec, pixel);

            pixel->r *= PBR_EXPOSURE;
            pixel->g *= PBR_EXPOSURE;
            pixel->b *= PBR_EXPOSURE;

            pixel++;
        }
    }

    gc_cube_texture_generate_mipmaps(environment);
    PROGRESS("\r > generating environment: %u%%", face + 1, CUBE_TOTAL);

    if (face == CUBE_BOTTOM)
        printf("\n");
}

void generate_irradiance_face(cube_texture_t *irradiance, cube_texture_t *environment, gl_cube_faces_t face)
{
    texture_mip_t *irradiance_face = irradiance->faces[face]->mips;
    texpixel_rgbaf_t *pixel = (texpixel_rgbaf_t *) irradiance_face->data;
    gc_vec_t normal;

    for (u32 y = 0; y < PBR_IRRADIANCE_SIZE; ++y)
    {
        for (u32 x = 0; x < PBR_IRRADIANCE_SIZE; ++x)
        {
            gc_vec_t irr;

            // The position vector is the normal of the hemisphere.
            face_position_vector(x, y, PBR_IRRADIANCE_SIZE, face, &normal);

            // Tangent-space base vectors represented in world-space coordinates.
            gc_vec_t up = {0.0f, 0.0f, 1.0f};
            gc_vec_t right;

            if (normal.v3.z > 0.99f || normal.v3.z < -0.99f)
            {
                up.v3.x = 0.0f;
                up.v3.y = 1.0f;
                up.v3.z = 0.0f;
            }

            v3_cross(&up, &normal, &right);
            v3_cross(&normal, &right, &up);

            v3_normalize(&right);
            v3_normalize(&up);

            // ----------------------------------------------------------------------------------
            // -- Integrate over the hemisphere.
            // ----------------------------------------------------------------------------------

            r32 phi_samples = PBR_IRRADIANCE_PHI_SAMPLES;
            // r32 theta_samples = PBR_IRRADIANCE_THETA_SAMPLES;

            r32 phi_step = 2.0f * PI / phi_samples;
            // r32 theta_step = 0.5f * PI / theta_samples;
            r32 theta_step = phi_step;

            r32 samples = 0;

            for (r32 phi = 0; phi < 2.0f * PI; phi += phi_step)
            {
                for (r32 theta = 0; theta < 0.5f * PI; theta += theta_step)
                {
                    // Tangent space.
                    VINIT3(t, sinf(theta) * cosf(phi), sinf(theta) * sinf(phi), cosf(theta));

                    // World space.
                    VINIT3(sample_vec,
                           right.v3.x * t.v3.x + up.v3.x * t.v3.y + normal.v3.x * t.v3.z,
                           right.v3.y * t.v3.x + up.v3.y * t.v3.y + normal.v3.y * t.v3.z,
                           right.v3.z * t.v3.x + up.v3.z * t.v3.y + normal.v3.z * t.v3.z);

                    gc_vec_t texcoord;
                    gc_vec_t sampled_color;
                    u32 face_index = 0;

                    builder_cube_uv_from_vec(&sample_vec, &texcoord, &face_index);
                    texture_mip_t *environment_texture = environment->faces[face_index]->mips;

                    samplef((u8 *) environment_texture->data,
                            environment_texture->header->width,
                            environment_texture->header->height,
                            4, texcoord.v2.x, texcoord.v2.y,
                            &sampled_color, TEXTURE_WRAP_CLAMP, true);

                    r32 tcs = cosf(theta) * sinf(theta);
                    // r32 tcs = cosf(theta);

                    irr.c.r += sampled_color.c.r * tcs;
                    irr.c.g += sampled_color.c.g * tcs;
                    irr.c.b += sampled_color.c.b * tcs;

                    samples++;
                }
            }

            pixel->r = PI * irr.c.r / samples;
            pixel->g = PI * irr.c.g / samples;
            pixel->b = PI * irr.c.b / samples;
            pixel->a = 1.0f;

            pixel++;
        }
    }
}

void _sse_generate_irradiance(u32 thread_index)
{
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);

    cube_texture_t *irradiance = threads.work.irradiance.irradiance;
    cube_texture_t *environment = threads.work.irradiance.environment;
    gl_cube_faces_t face = threads.work.irradiance.face;

    texture_mip_t *irradiance_face = irradiance->faces[face]->mips;
    gc_vec_t normal;

    r32 phi_samples = MULTIPLE_OF(PBR_IRRADIANCE_PHI_SAMPLES, 4);
    // r32 theta_samples = MULTIPLE_OF(PBR_IRRADIANCE_THETA_SAMPLES, 4);

    r32 phi_step = 2.0f * PI / phi_samples;
    // r32 theta_step = 0.5f * PI / theta_samples;
    r32 theta_step = phi_step;
    r32 theta_incr = 4 * theta_step;

    r32 tmp_val_0[4];
    // r32 tmp_val_1[4];
    // r32 tmp_val_2[4];

    u32 face_index[4];
    fv3_t worldv;
    fv2_t texcoord;
    shader_color_t output;

    while (true)
    {
        u32 tile_index = SDL_AtomicAdd(&threads.work.index, 1);

        if (tile_index >= threads.work.tile_count)
            break;

        thread_tile_t *current_tile = threads.work.tiles + tile_index;
        texpixel_rgbaf_t *base_tile_pixel = (texpixel_rgbaf_t *) irradiance_face->data +
                                            current_tile->sy * irradiance_face->header->width + current_tile->sx;

        for (u32 y = current_tile->sy; y <= current_tile->ey; ++y)
        {
            texpixel_rgbaf_t *pixel = base_tile_pixel;

            for (u32 x = current_tile->sx; x <= current_tile->ex; ++x)
            {
                // gc_vec_t irr;
                __m128 irr_r = _mm_setzero_ps();
                __m128 irr_g = _mm_setzero_ps();
                __m128 irr_b = _mm_setzero_ps();

                // The position vector is the normal of the hemisphere.
                face_position_vector(x, y, PBR_IRRADIANCE_SIZE, face, &normal);

                // Tangent-space base vectors represented in world-space coordinates.
                gc_vec_t up = {0.0f, 0.0f, 1.0f};
                gc_vec_t right;

                if (normal.v3.z > 0.99f || normal.v3.z < -0.99f)
                {
                    up.v3.x = 0.0f;
                    up.v3.y = 1.0f;
                    up.v3.z = 0.0f;
                }

                v3_cross(&up, &normal, &right);
                v3_cross(&normal, &right, &up);

                v3_normalize(&right);
                v3_normalize(&up);

                // ----------------------------------------------------------------------------------
                // -- Integrate over the hemisphere.
                // ----------------------------------------------------------------------------------

                r32 samples = 0;
                __m128 theta_incr_4x = _mm_set1_ps(theta_incr);

                for (r32 phi = 0; phi < 2.0f * PI; phi += phi_step)
                {
                    r32 cos_phi = cosf(phi);
                    r32 sin_phi = sinf(phi);

                    __m128 theta_4x = _mm_setr_ps(0, theta_step, 2 * theta_step, 3 * theta_step);

                    for (r32 theta = 0; theta < 0.5f * PI; theta += theta_incr)
                    {
                        _mm_store_ps(tmp_val_0, theta_4x);

                        // sin theta
                        // tmp_val_1[0] = sinf(tmp_val_0[0]);
                        // tmp_val_1[1] = sinf(tmp_val_0[1]);
                        // tmp_val_1[2] = sinf(tmp_val_0[2]);
                        // tmp_val_1[3] = sinf(tmp_val_0[3]);

                        // cos theta
                        // tmp_val_2[0] = cosf(tmp_val_0[0]);
                        // tmp_val_2[1] = cosf(tmp_val_0[1]);
                        // tmp_val_2[2] = cosf(tmp_val_0[2]);
                        // tmp_val_2[3] = cosf(tmp_val_0[3]);

                        // __m128 sin_theta_4x = _mm_load_ps(tmp_val_1);
                        // __m128 cos_theta_4x = _mm_load_ps(tmp_val_2);

                        __m128 sin_theta_4x = sin_ps(theta_4x);
                        __m128 cos_theta_4x = cos_ps(theta_4x);

                        __m128 mul_sin_cos_theta_4x = _mm_mul_ps(sin_theta_4x, cos_theta_4x);

                        // Tangent space vector.
                        __m128 tanv_x = _mm_mul_ps(sin_theta_4x, _mm_set1_ps(cos_phi));
                        __m128 tanv_y = _mm_mul_ps(sin_theta_4x, _mm_set1_ps(sin_phi));
                        __m128 tanv_z = cos_theta_4x;

                        // World space.
                        __m128 world_x = _mm_add_ps(
                                            _mm_add_ps(
                                                _mm_mul_ps(tanv_x, _mm_set1_ps(right.v3.x)),
                                                _mm_mul_ps(tanv_y, _mm_set1_ps(up.v3.x))),
                                            _mm_mul_ps(tanv_z, _mm_set1_ps(normal.v3.x)));

                        __m128 world_y = _mm_add_ps(
                                            _mm_add_ps(
                                                _mm_mul_ps(tanv_x, _mm_set1_ps(right.v3.y)),
                                                _mm_mul_ps(tanv_y, _mm_set1_ps(up.v3.y))),
                                            _mm_mul_ps(tanv_z, _mm_set1_ps(normal.v3.y)));

                        __m128 world_z = _mm_add_ps(
                                            _mm_add_ps(
                                                _mm_mul_ps(tanv_x, _mm_set1_ps(right.v3.z)),
                                                _mm_mul_ps(tanv_y, _mm_set1_ps(up.v3.z))),
                                            _mm_mul_ps(tanv_z, _mm_set1_ps(normal.v3.z)));

                        _mm_store_ps(worldv.x, world_x);
                        _mm_store_ps(worldv.y, world_y);
                        _mm_store_ps(worldv.z, world_z);

                        sse_cube_uv_from_vec(&worldv, &texcoord, face_index);
                        sse_cube_samplef(environment, face_index, texcoord.x, texcoord.y, &output);

                        __m128 out_r = _mm_load_ps(output.r);
                        __m128 out_g = _mm_load_ps(output.g);
                        __m128 out_b = _mm_load_ps(output.b);

                        irr_r = _mm_add_ps(irr_r, _mm_mul_ps(out_r, mul_sin_cos_theta_4x));
                        irr_g = _mm_add_ps(irr_g, _mm_mul_ps(out_g, mul_sin_cos_theta_4x));
                        irr_b = _mm_add_ps(irr_b, _mm_mul_ps(out_b, mul_sin_cos_theta_4x));

                        theta_4x = _mm_add_ps(theta_4x, theta_incr_4x);
                        samples += 4;
                    }
                }

                r32 tmp[4];
                __m128 rcp_samples = _mm_rcp_ps(_mm_set1_ps(samples));

                irr_r = _mm_hadd_ps(irr_r, irr_r);
                irr_r = _mm_hadd_ps(irr_r, irr_r);
                irr_r = _mm_mul_ps(_mm_set1_ps(PI), _mm_mul_ps(irr_r, rcp_samples));

                irr_g = _mm_hadd_ps(irr_g, irr_g);
                irr_g = _mm_hadd_ps(irr_g, irr_g);
                irr_g = _mm_mul_ps(_mm_set1_ps(PI), _mm_mul_ps(irr_g, rcp_samples));

                irr_b = _mm_hadd_ps(irr_b, irr_b);
                irr_b = _mm_hadd_ps(irr_b, irr_b);
                irr_b = _mm_mul_ps(_mm_set1_ps(PI), _mm_mul_ps(irr_b, rcp_samples));

                _mm_store_ps(tmp, irr_r);
                pixel->r = tmp[0];
                _mm_store_ps(tmp, irr_g);
                pixel->g = tmp[0];
                _mm_store_ps(tmp, irr_b);
                pixel->b = tmp[0];
                pixel->a = 1.0f;

                pixel++;
            }

            base_tile_pixel += irradiance_face->header->width;
        }
    }
}

void sse_generate_irradiance_face(cube_texture_t *irradiance, cube_texture_t *environment, gl_cube_faces_t face)
{
    if (face == 0)
        PROGRESS("\r > generating irradiance: %u%%", 0, CUBE_TOTAL);

    generate_thread_work(PBR_IRRADIANCE_SIZE);

    threads.work.irradiance.irradiance = irradiance;
    threads.work.irradiance.environment = environment;
    threads.work.irradiance.face = face;

    BUILDER_THREAD_START(PBR_IRRADIANCE_STATE);
    PROGRESS("\r > generating irradiance: %u%%", face + 1, CUBE_TOTAL);

    if (face == CUBE_BOTTOM)
        printf("\n");
}

#define USE_ARTIFACT_FIX 1
// #define DEBUG_PREFILTERED 1

void generate_prefiltered_face(cube_texture_t *prefiltered, cube_texture_t *environment, gl_cube_faces_t face)
{
    if (face == 0)
        PROGRESS("\r > generating prefiltered: %u%%", 0, CUBE_TOTAL);

    gc_vec_t normal;
    gc_vec_t hm;
    gc_vec_t h;
    gc_vec_t l;

    for (u8 mip_level = 0; mip_level < prefiltered->mip_count; ++mip_level)
    {
        texture_mip_t *current_mip = prefiltered->faces[face]->mips + mip_level;
        // texture_mip_t *env_mip = environment->faces[face]->mips + mip_level;
        texpixel_rgbaf_t *pixel = (texpixel_rgbaf_t *) current_mip->data;

        // r32 resolution = env_mip->header->width;
        r32 resolution = environment->faces[face]->mips->header->width;
        r32 roughness = (r32) mip_level / prefiltered->mip_count;

        for (u32 y = 0; y < current_mip->header->height; ++y)
        {
            for (u32 x = 0; x < current_mip->header->width; ++x)
            {
                face_position_vector(x, y, current_mip->header->width, face, &normal);
                v3_normalize(&normal);

                r32 total_weight = 0;
                gc_vec_t prefiltered_color;

                prefiltered_color.c.r = 0;
                prefiltered_color.c.g = 0;
                prefiltered_color.c.b = 0;

                for (u32 i = 0; i < PBR_PREFILTERED_SAMPLE_COUNT; ++i)
                {
                    hammersley(i, PBR_PREFILTERED_SAMPLE_COUNT, &hm);
                    importance_sample_ggx(&hm, &normal, roughness, &h);

                    r32 nh_dot = v3_dot(&normal, &h);

                    l.v3.x = 2.0f * nh_dot * h.v3.x - normal.v3.x;
                    l.v3.y = 2.0f * nh_dot * h.v3.y - normal.v3.y;
                    l.v3.z = 2.0f * nh_dot * h.v3.z - normal.v3.z;

                    v3_normalize(&l);

                    r32 nl_dot = v3_dot(&normal, &l);
                    // nl_dot = nl_dot < 0 ? 0 : nl_dot;

                    // ----------------------------------------------------------------------------------
                    // -- Artifact fix.
                    // ----------------------------------------------------------------------------------

                    r32 d = distribution_ggx(&normal, &h, roughness);
                    // r32 pdf = (d * nh_dot / (4.0 * nh_dot + 0.0001f));
                    r32 pdf = (d * nh_dot / (4.0 * nh_dot + 0.0001f));
                    r32 sa_texel = 4.0 * PI / (6.0 * resolution * resolution);
                    r32 sa_sample = 1.0 / (PBR_PREFILTERED_SAMPLE_COUNT * pdf);
                    r32 env_mip_level = roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);
                    env_mip_level = env_mip_level >= 0 ? env_mip_level : 0;

                    // ----------------------------------------------------------------------------------

                    if (nl_dot > 0)
                    {
                        gc_vec_t sample;
                        gc_vec_t sample_a;
                        gc_vec_t sample_b;

                        v3_normalize(&l);

                        u32 low = (u32) floorf(env_mip_level);
                        u32 high = (u32) ceilf(env_mip_level);
                        r32 interp = env_mip_level - low;

                        cube_samplef(environment, &l, low, &sample_a);
                        cube_samplef(environment, &l, high, &sample_b);

                        sample.c.r = sample_a.c.r + (sample_b.c.r - sample_a.c.r) * interp;
                        sample.c.g = sample_a.c.g + (sample_b.c.g - sample_a.c.g) * interp;
                        sample.c.b = sample_a.c.b + (sample_b.c.b - sample_a.c.b) * interp;

                        prefiltered_color.c.r += sample.c.r * nl_dot;
                        prefiltered_color.c.g += sample.c.g * nl_dot;
                        prefiltered_color.c.b += sample.c.b * nl_dot;

                        total_weight += nl_dot;

                    }
                }

                prefiltered_color.c.r /= total_weight;
                prefiltered_color.c.g /= total_weight;
                prefiltered_color.c.b /= total_weight;

                pixel->r = prefiltered_color.c.r;
                pixel->g = prefiltered_color.c.g;
                pixel->b = prefiltered_color.c.b;
                pixel->a = 1.0f;

                pixel++;
            }
        }
    }

    PROGRESS("\r > generating prefiltered: %u%%", face + 1, CUBE_TOTAL);

    if (face == CUBE_BOTTOM)
        printf("\n");
}

r32 geometry_schlick_ggx(r32 nv_dot, r32 roughness)
{
    r32 a = roughness;
    r32 k = (a * a) / 2.0f;
    r32 nom = nv_dot;
    r32 denom = nv_dot * (1.0f - k) + k;

    return nom / denom;
}

r32 geometry_smith(gc_vec_t *n, gc_vec_t *v, gc_vec_t *l, r32 roughness)
{
    r32 nv_dot = v3_dot(n, v);
    nv_dot = nv_dot >= 0 ? nv_dot : 0;

    r32 nl_dot = v3_dot(n, l);
    nl_dot = nl_dot >= 0 ? nl_dot : 0;

    r32 ggx2 = geometry_schlick_ggx(nv_dot, roughness);
    r32 ggx1 = geometry_schlick_ggx(nl_dot, roughness);

    return ggx1 * ggx2;
}

void integrate_brdf(r32 nv_dot, r32 roughness, gc_vec_t *out)
{
    gc_vec_t v;

    v.v3.x = sqrt(1.0f - nv_dot);
    v.v3.y = 0;
    v.v3.z = nv_dot;

    out->v2.x = 0;
    out->v2.y = 0;

    gc_vec_t n = {0.0f, 0.0f, 1.0f};
    gc_vec_t hm;
    gc_vec_t h;
    gc_vec_t l;

    for (u32 i = 0; i < PBR_BRDF_LUT_SAMPLE_COUNT; ++i)
    {
        hammersley(i, PBR_BRDF_LUT_SAMPLE_COUNT, &hm);
        importance_sample_ggx(&hm, &n, roughness, &h);

        r32 vh_dot = v3_dot(&v, &h);
        vh_dot = vh_dot >= 0 ? vh_dot : 0;

        l.v3.x = 2.0f * vh_dot * h.v3.x - v.v3.x;
        l.v3.y = 2.0f * vh_dot * h.v3.y - v.v3.y;
        l.v3.z = 2.0f * vh_dot * h.v3.z - v.v3.z;

        v3_normalize(&l);

        r32 nl_dot = l.v3.z >= 0 ? l.v3.z : 0;
        r32 nh_dot = h.v3.z >= 0 ? h.v3.z : 0;

        vh_dot = vh_dot >= 0 ? vh_dot : 0;

        if (nl_dot > 0)
        {
            r32 g = geometry_smith(&n, &v, &l, roughness);
            r32 g_visibility = (g * vh_dot) / (nh_dot * nv_dot);
            r32 fc = pow(1.0 - vh_dot, 5.0);

            out->v2.x += (1.0f - fc) * g_visibility;
            out->v2.y += fc * g_visibility;
        }
    }

    out->v2.x /= (r32) PBR_BRDF_LUT_SAMPLE_COUNT;
    out->v2.y /= (r32) PBR_BRDF_LUT_SAMPLE_COUNT;
}

void generate_brdf_lut(texture2d_t *brdf_lut)
{
    texpixel_rgf_t *pixel = (texpixel_rgf_t *) brdf_lut->mips->data;
    gc_vec_t terms;

    for (u32 y = 0; y < brdf_lut->mips->header->height; ++y)
    {
        r32 roughness = (y + 0.5f) / brdf_lut->mips->header->height;

        for (u32 x = 0; x < brdf_lut->mips->header->width; ++x)
        {
            r32 nv_dot = (x + 0.5f) / brdf_lut->mips->header->width;
            integrate_brdf(nv_dot, roughness, &terms);

            pixel->r = terms.v2.x;
            pixel->g = terms.v2.y;

            pixel++;
        }
    }
}

#if 0
void cube_link(cube_texture_t *cubemap, u32 face_size, u32 mip_levels, void *header, void *data, size_t *header_offset, size_t *data_offset)
{
    texture_mip_t *cubemap_header = (texture_mip_t *) ADDR_OFFSET(header, *header_offset);
    void *cubemap_data = ADDR_OFFSET(data, *data_offset);

    for (u8 i = 0; i < 6; ++i)
    {
        u32 mip_base_index = (i * mip_levels);
        u32 current_size = face_size;

        cubemap->faces[i] = (texture_mip_t *) ADDR_OFFSET(header, *header_offset);

        for (u8 j = 0; j < mip_levels; ++j)
        {
            texture_mip_t *current = cubemap->faces[i] + j;

            current->header = (texture_data_header_t *) ADDR_OFFSET(data, *data_offset);
            current->data = current->header + 1;

            current->header->bytes = TEX_DATA_BYTES_RGBAF(current_size, current_size);
            current->header->width = current_size;
            current->header->height = current_size;
            current->header->tex_du = 1.0f / current_size;
            current->header->tex_dv = 1.0f / current_size;

            *header_offset += sizeof(texture_mip_t);
            *data_offset += current->header->bytes;
            current_size = current_size >> 1;
        }
    }
}
#endif

#if 0
b8 load_hdri_map(char *filepath, loaded_texture_t *texture)
{
    b8 result = true;

    if (filepath)
    {
        s32 width = 0;
        s32 height = 0;
        s32 components = 0;
        u32 mult = 1;

        void *loaded_data = stbi_loadf(filepath, &width, &height, &components, 0);
        // void *scaled_loaded_data = scale_bitmap_float3_m(loaded_data, width, height, mult);
        // void *image_data = scaled_loaded_data;
        void *image_data = loaded_data;

        width *= mult;
        height *= mult;

        hdr_data_t hdr_data;

        hdr_data.width = width;
        hdr_data.height = height;
        hdr_data.du = 1.0f / width;
        hdr_data.dv = 1.0f / height;
        hdr_data.row_pitch = components * sizeof(r32) * width;
        hdr_data.col_pitch = components * sizeof(r32);

#ifdef PBR_DEBUG_BASE_HDR
        gc_file_t output_bmp;
        char hdr_filename[255];
        char *filename = extract_file_name(filepath, 0);
        sprintf(hdr_filename, "%s%s.hdr.%s", DEBUG_FOLDER, filename, "bmp");
        open_file(&output_bmp, hdr_filename, "wb");
        export_bitmapf(&output_bmp, (r32 *) image_data, width, height, 3);
        close_file(&output_bmp);
#endif

        // ----------------------------------------------------------------------------------
        // -- Memory allocation and setup.
        // ----------------------------------------------------------------------------------

        u32 env_mip_count = TEX_MIP_COUNT(PBR_ENVIRONMENT_SIZE);
        size_t environment_header_bytes = TEX_HEADER_BYTES(6 * env_mip_count);
        size_t environment_data_bytes = 6 * compute_mip_bytes_float(env_mip_count, PBR_ENVIRONMENT_SIZE);

        u32 irr_mip_count = 1;
        size_t irradiance_header_bytes = TEX_HEADER_BYTES(6 * irr_mip_count);
        size_t irradiance_data_bytes = 6 * compute_mip_bytes_float(irr_mip_count, PBR_IRRADIANCE_SIZE);

        u32 pref_mip_count = PBR_PREFILTERED_MIP_LEVELS;
        size_t prefiltered_header_bytes = TEX_HEADER_BYTES(6 * pref_mip_count);
        size_t prefiltered_data_bytes = 6 * compute_mip_bytes_float(pref_mip_count, PBR_PREFILTERED_SIZE);

        size_t lut_header_bytes = sizeof(texture_mip_t);
        size_t lut_data_bytes = sizeof(texture_data_header_t) + PBR_BRDF_LUT_SIZE * PBR_BRDF_LUT_SIZE * sizeof(brdf_lut_pixel_t);

        size_t total_bytes = environment_header_bytes + environment_data_bytes +
                             irradiance_header_bytes +
                             irradiance_data_bytes +
                             prefiltered_header_bytes +
                             prefiltered_data_bytes +
                             lut_header_bytes +
                             lut_data_bytes;

        texture->meta.type = TEXTURE_PBR_AMBIENT;
        texture->meta.data_bytes = environment_data_bytes + irradiance_data_bytes + prefiltered_data_bytes + lut_data_bytes;

        texture->meta.count[0] = 6;
        texture->meta.count[1] = 6;
        texture->meta.count[2] = 6;
        texture->meta.count[3] = 1;

        texture->meta.mips[0] = env_mip_count;
        texture->meta.mips[1] = irr_mip_count;
        texture->meta.mips[2] = pref_mip_count;
        texture->meta.mips[3] = 1;

        void *memory = malloc(total_bytes);
        memset(memory, 0, total_bytes);

        texture_mip_t *header_pointer = (texture_mip_t *) memory;
        void *data_pointer = ADDR_OFFSET(memory, environment_header_bytes + irradiance_header_bytes + prefiltered_header_bytes + lut_header_bytes);
        pbr_ambient_texture_t *pbr_map = &texture->pbr_ambient;

        texture->data = data_pointer;
        texture->memory = memory;

        size_t header_offset = 0;
        size_t data_offset = 0;

        cube_texture_t *environment = &pbr_map->environment;
        cube_texture_t *irradiance = &pbr_map->irradiance;
        cube_texture_t *prefiltered = &pbr_map->prefiltered;
        texture_mip_t *brdf_lut = pbr_map->brdf_lut;

        // ----------------------------------------------------------------------------------
        // -- Environment.
        // ----------------------------------------------------------------------------------

        cube_link(environment,
                  PBR_ENVIRONMENT_SIZE,
                  env_mip_count,
                  header_pointer,
                  data_pointer,
                  &header_offset,
                  &data_offset);

        environment->mip_count = env_mip_count;

        generate_environment_face(environment, CUBE_LEFT, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_RIGHT, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_FRONT, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_BACK, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_TOP, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_BOTTOM, (texpixel_rgbf_t *) image_data, &hdr_data);

        // cube_seamless_filter(environment);

        // ----------------------------------------------------------------------------------
        // -- Irradiance.
        // ----------------------------------------------------------------------------------

        cube_link(irradiance,
                  PBR_IRRADIANCE_SIZE,
                  irr_mip_count,
                  header_pointer,
                  data_pointer,
                  &header_offset,
                  &data_offset);

        irradiance->mip_count = irr_mip_count;

        generate_irradiance_face(irradiance, environment, CUBE_LEFT);
        generate_irradiance_face(irradiance, environment, CUBE_RIGHT);
        generate_irradiance_face(irradiance, environment, CUBE_FRONT);
        generate_irradiance_face(irradiance, environment, CUBE_BACK);
        generate_irradiance_face(irradiance, environment, CUBE_TOP);
        generate_irradiance_face(irradiance, environment, CUBE_BOTTOM);

        // ----------------------------------------------------------------------------------
        // -- Prefiltered.
        // ----------------------------------------------------------------------------------

        cube_link(prefiltered,
                  PBR_PREFILTERED_SIZE,
                  pref_mip_count,
                  header_pointer,
                  data_pointer,
                  &header_offset,
                  &data_offset);

        prefiltered->mip_count = pref_mip_count;

        generate_prefiltered_face(prefiltered, environment, CUBE_LEFT);
        generate_prefiltered_face(prefiltered, environment, CUBE_RIGHT);
        generate_prefiltered_face(prefiltered, environment, CUBE_FRONT);
        generate_prefiltered_face(prefiltered, environment, CUBE_BACK);
        generate_prefiltered_face(prefiltered, environment, CUBE_TOP);
        generate_prefiltered_face(prefiltered, environment, CUBE_BOTTOM);

        cube_seamless_filter(prefiltered);

        // ----------------------------------------------------------------------------------
        // -- Lut.
        // ----------------------------------------------------------------------------------

        pbr_map->brdf_lut = (texture_mip_t *) ADDR_OFFSET(header_pointer, header_offset);
        pbr_map->brdf_lut->header = (texture_data_header_t *) ADDR_OFFSET(data_pointer, data_offset);
        pbr_map->brdf_lut->data = pbr_map->brdf_lut->header + 1;

        pbr_map->brdf_lut->header->bytes = lut_data_bytes;
        pbr_map->brdf_lut->header->width = PBR_BRDF_LUT_SIZE;
        pbr_map->brdf_lut->header->height = PBR_BRDF_LUT_SIZE;
        pbr_map->brdf_lut->header->tex_du = 1.0f / PBR_BRDF_LUT_SIZE;
        pbr_map->brdf_lut->header->tex_dv = 1.0f / PBR_BRDF_LUT_SIZE;

        generate_brdf_lut(pbr_map->brdf_lut);

        stbi_image_free(loaded_data);
        // free(scaled_loaded_data);
    }
    else
    {
        result = false;
        printf(B_ERROR(MISSING_FILE));
    }

    return result;
}
#endif

void load_hdri_map(char *filepath, loaded_texture_t *texture)
{
    if (filepath)
    {
        s32 width = 0;
        s32 height = 0;
        s32 components = 0;

        void *loaded_data = stbi_loadf(filepath, &width, &height, &components, 0);
        void *image_data = loaded_data;

        if (!loaded_data) {
            printf(B_ERROR(MISSING_FILE), filepath);
            return;
        }

        hdr_data_t hdr_data;

        hdr_data.width = width;
        hdr_data.height = height;
        hdr_data.du = 1.0f / width;
        hdr_data.dv = 1.0f / height;
        hdr_data.row_pitch = components * sizeof(r32) * width;
        hdr_data.col_pitch = components * sizeof(r32);

#ifdef PBR_DEBUG_BASE_HDR
        gc_file_t output_bmp;
        char hdr_filename[255];
        char *filename = extract_file_name(filepath, 0);

        sprintf(hdr_filename, "%s%s.hdr.%s", DEBUG_FOLDER, filename, "bmp");
        open_file(&output_bmp, hdr_filename, GC_FILE_WRITE);
        export_bitmapf(&output_bmp, (r32 *) image_data, width, height, 3);
        close_file(&output_bmp);
#endif

        pbr_ambient_texture_t *pbr_map = gc_create_pbr_ambient_texture(0);

        texture->meta.type = ASSET_TEXTURE_PBR_AMBIENT;
        texture->meta.data_bytes = pbr_map->environment->data_bytes +
                                   pbr_map->irradiance->data_bytes +
                                   pbr_map->prefiltered->data_bytes +
                                   pbr_map->brdf_lut->data_bytes;

        texture->data = pbr_map;

        cube_texture_t *environment = pbr_map->environment;
        cube_texture_t *irradiance = pbr_map->irradiance;
        cube_texture_t *prefiltered = pbr_map->prefiltered;

        // ----------------------------------------------------------------------------------
        // -- Environment.
        // ----------------------------------------------------------------------------------

        generate_environment_face(environment, CUBE_LEFT, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_RIGHT, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_FRONT, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_BACK, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_TOP, (texpixel_rgbf_t *) image_data, &hdr_data);
        generate_environment_face(environment, CUBE_BOTTOM, (texpixel_rgbf_t *) image_data, &hdr_data);

        // cube_seamless_filter(environment);

        // ----------------------------------------------------------------------------------
        // -- Irradiance.
        // ----------------------------------------------------------------------------------

        sse_generate_irradiance_face(irradiance, environment, CUBE_LEFT);
        sse_generate_irradiance_face(irradiance, environment, CUBE_RIGHT);
        sse_generate_irradiance_face(irradiance, environment, CUBE_FRONT);
        sse_generate_irradiance_face(irradiance, environment, CUBE_BACK);
        sse_generate_irradiance_face(irradiance, environment, CUBE_TOP);
        sse_generate_irradiance_face(irradiance, environment, CUBE_BOTTOM);

        // cube_seamless_filter(irradiance);

        // ----------------------------------------------------------------------------------
        // -- Prefiltered.
        // ----------------------------------------------------------------------------------

        generate_prefiltered_face(prefiltered, environment, CUBE_LEFT);
        generate_prefiltered_face(prefiltered, environment, CUBE_RIGHT);
        generate_prefiltered_face(prefiltered, environment, CUBE_FRONT);
        generate_prefiltered_face(prefiltered, environment, CUBE_BACK);
        generate_prefiltered_face(prefiltered, environment, CUBE_TOP);
        generate_prefiltered_face(prefiltered, environment, CUBE_BOTTOM);

        cube_seamless_filter(prefiltered);

        // ----------------------------------------------------------------------------------
        // -- Lut.
        // ----------------------------------------------------------------------------------

        printf(" > generating brdf lut ...\n");

        generate_brdf_lut(pbr_map->brdf_lut);

        stbi_image_free(loaded_data);
    }
    else {
        printf(B_ERROR(MISSING_FILE));
    }
}

void _debug_pbr_ambient_texture(char *alias, loaded_texture_t *texture)
{
    pbr_ambient_texture_t *pbr_ambient = (pbr_ambient_texture_t *) texture->data;

    if (pbr_ambient->environment->mip_count)
        output_cubemap(pbr_ambient->environment, alias, "environment", false, DEBUG_CUBEMAP_ENVIRONMENT_MIP_LEVEL);

    if (pbr_ambient->irradiance->mip_count)
        output_cubemap(pbr_ambient->irradiance, alias, "irradiance", false, 0);

    if (pbr_ambient->prefiltered->mip_count)
        output_cubemap(pbr_ambient->prefiltered, alias, "prefiltered", true, DEBUG_CUBEMAP_PREFILTERED_MIP_LEVEL);

    if (pbr_ambient->brdf_lut)
        output_brdf_lut(pbr_ambient->brdf_lut, alias);
}

__INLINE__ void copy_imagef(r32 *dest, r32 *src, u32 width, u32 height, u32 pitch)
{
    u8 *base_dest = (u8 *) dest;

    for (u32 row = 0; row < height; ++row)
    {
        r32 *pixel = (r32 *) base_dest;

        for (u32 col = 0; col < width; ++col)
        {
            *pixel++ = *src++;
            *pixel++ = *src++;
            *pixel++ = *src++;
            *pixel++ = *src++;
        }

        base_dest += pitch;
    }
}

void *combine_cubemap_facesf(cube_texture_t *cubemap, u32 *width, u32 *height, u32 *bytes, u8 mip_level)
{
    void *memory = 0;

    if (mip_level >= cubemap->mip_count)
        mip_level = 0;

    texture_mip_t *first_mip_level = cubemap->faces[0]->mips + mip_level;

    if (first_mip_level->header->width && first_mip_level->header->height)
    {
        u32 cube_width = first_mip_level->header->width;
        u32 cube_height = first_mip_level->header->height;

        *width = cube_width * 4;
        *height = cube_height * 3;
        *bytes = (*height) * (*width) * 4 * sizeof(r32);

        u32 face_width_pitch = cube_width * 4 * sizeof(r32);
        u32 combined_pitch = *width * 4 * sizeof(r32);

        memory = malloc(*bytes);
        memset(memory, 0, *bytes);

        r32 *src = 0;
        r32 *dest = 0;

        // ----------------------------------------------------------------------------------
        // -- Top.
        // ----------------------------------------------------------------------------------

        texture_mip_t *top = cubemap->faces[CUBE_TOP]->mips + mip_level;

        if (top->header->width && top->header->height)
        {
            src = (r32 *) top->data;
            dest = (r32 *) ADDR_OFFSET(memory, face_width_pitch);

            copy_imagef(dest, src, cube_width, cube_height, combined_pitch);
        }

        // ----------------------------------------------------------------------------------
        // -- Left.
        // ----------------------------------------------------------------------------------

        texture_mip_t *left = cubemap->faces[CUBE_LEFT]->mips + mip_level;

        if (left->header->width && left->header->height)
        {
            src = (r32 *) left->data;
            dest = (r32 *) ADDR_OFFSET(memory, combined_pitch * cube_height);

            copy_imagef(dest, src, cube_width, cube_height, combined_pitch);
        }

        // ----------------------------------------------------------------------------------
        // -- Front.
        // ----------------------------------------------------------------------------------

        texture_mip_t *front = cubemap->faces[CUBE_FRONT]->mips + mip_level;

        if (front->header->width && front->header->height)
        {
            src = (r32 *) front->data;
            dest = (r32 *) ADDR_OFFSET(memory, combined_pitch * cube_height + face_width_pitch);

            copy_imagef(dest, src, cube_width, cube_height, combined_pitch);
        }

        // ----------------------------------------------------------------------------------
        // -- Right.
        // ----------------------------------------------------------------------------------

        texture_mip_t *right = cubemap->faces[CUBE_RIGHT]->mips + mip_level;

        if (right->header->width && right->header->height)
        {
            src = (r32 *) right->data;
            dest = (r32 *) ADDR_OFFSET(memory, combined_pitch * cube_height + 2 * face_width_pitch);

            copy_imagef(dest, src, cube_width, cube_height, combined_pitch);
        }

        // ----------------------------------------------------------------------------------
        // -- Back.
        // ----------------------------------------------------------------------------------

        texture_mip_t *back = cubemap->faces[CUBE_BACK]->mips + mip_level;

        if (back->header->width && back->header->height)
        {
            src = (r32 *) back->data;
            dest = (r32 *) ADDR_OFFSET(memory, combined_pitch * cube_height + 3 * face_width_pitch);

            copy_imagef(dest, src, cube_width, cube_height, combined_pitch);
        }

        // ----------------------------------------------------------------------------------
        // -- Bottom.
        // ----------------------------------------------------------------------------------

        texture_mip_t *bottom = cubemap->faces[CUBE_BOTTOM]->mips + mip_level;

        if (bottom->header->width && bottom->header->height)
        {
            src = (r32 *) bottom->data;
            dest = (r32 *) ADDR_OFFSET(memory, 2 * combined_pitch * cube_height + face_width_pitch);

            copy_imagef(dest, src, cube_width, cube_height, combined_pitch);
        }
    }

    return memory;
}
