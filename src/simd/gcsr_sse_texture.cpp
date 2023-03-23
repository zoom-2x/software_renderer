// ----------------------------------------------------------------------------------
// -- File: gcsr_sse_texture.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-06 19:37:56
// -- Modified: 2022-10-06 19:37:57
// ----------------------------------------------------------------------------------

void _sse_tex_clamp(r32 *in_u, r32 *in_v, r32 *out_u, r32 *out_v)
{
    OPTICK_EVENT("_sse_tex_clamp");

    __m128 u_4x = _mm_load_ps(in_u);
    __m128 v_4x = _mm_load_ps(in_v);

    u_4x = _mm_max_ps(_mm_min_ps(u_4x, _mm_set1_ps(1.0f)), _mm_setzero_ps());
    v_4x = _mm_max_ps(_mm_min_ps(v_4x, _mm_set1_ps(1.0f)), _mm_setzero_ps());

    _mm_store_ps(out_u, u_4x);
    _mm_store_ps(out_v, v_4x);
}

void _sse_tex_repeat(r32 *in_u, r32 *in_v, r32 *out_u, r32 *out_v)
{
    OPTICK_EVENT("_sse_tex_repeat");

    __m128 u_4x = _mm_load_ps(in_u);
    __m128 v_4x = _mm_load_ps(in_v);

    u_4x = _mm_sub_ps(u_4x, _mm_floor_ps(u_4x));
    v_4x = _mm_sub_ps(v_4x, _mm_floor_ps(v_4x));

    u_4x = _mm_max_ps(u_4x, _mm_setzero_ps());
    v_4x = _mm_max_ps(v_4x, _mm_setzero_ps());

    _mm_store_ps(out_u, u_4x);
    _mm_store_ps(out_v, v_4x);
}

void _sse_tex_mirror(r32 *in_u, r32 *in_v, r32 *out_u, r32 *out_v)
{
    OPTICK_EVENT("_sse_tex_mirror");

    __m128 u_4x = _mm_load_ps(in_u);
    __m128 v_4x = _mm_load_ps(in_v);

    __m128 sign_mask1 = _mm_cmplt_ps(u_4x, _mm_setzero_ps());
    __m128 sign_mask2 = _mm_cmplt_ps(v_4x, _mm_setzero_ps());

    u_4x = _mm_mul_ps(u_4x, _mm_or_ps(_mm_and_ps(sign_mask1, _mm_set1_ps(-1)), _mm_andnot_ps(sign_mask1, _mm_set1_ps(1))));
    v_4x = _mm_mul_ps(v_4x, _mm_or_ps(_mm_and_ps(sign_mask2, _mm_set1_ps(-1)), _mm_andnot_ps(sign_mask2, _mm_set1_ps(1))));

    __m128i min_u_4x = _mm_cvttps_epi32(u_4x);
    __m128i min_v_4x = _mm_cvttps_epi32(v_4x);
    __m128i max_u_4x = _mm_add_epi32(min_u_4x, _mm_set1_epi32(1));
    __m128i max_v_4x = _mm_add_epi32(min_v_4x, _mm_set1_epi32(1));

    __m128i tu = _mm_srli_epi32(min_u_4x, 1);
    __m128i tv = _mm_srli_epi32(min_v_4x, 1);

    tu = _mm_add_epi32(tu, tu);
    tv = _mm_add_epi32(tv, tv);

    __m128i mask1 = _mm_cmpeq_epi32(tu, min_u_4x);
    __m128i mask2 = _mm_cmpeq_epi32(tv, min_v_4x);

    u_4x = _mm_or_ps(
                _mm_andnot_ps(_mm_castsi128_ps(mask1), _mm_sub_ps(_mm_cvtepi32_ps(max_u_4x), u_4x)),
                _mm_and_ps(_mm_castsi128_ps(mask1), _mm_sub_ps(u_4x, _mm_cvtepi32_ps(min_u_4x))));

    v_4x = _mm_or_ps(
                _mm_andnot_ps(_mm_castsi128_ps(mask2), _mm_sub_ps(_mm_cvtepi32_ps(max_v_4x), v_4x)),
                _mm_and_ps(_mm_castsi128_ps(mask2), _mm_sub_ps(v_4x, _mm_cvtepi32_ps(min_v_4x))));

    u_4x = _mm_max_ps(u_4x, _mm_setzero_ps());
    v_4x = _mm_max_ps(v_4x, _mm_setzero_ps());

    u_4x = _mm_min_ps(u_4x, _mm_set1_ps(1.0f));
    v_4x = _mm_min_ps(v_4x, _mm_set1_ps(1.0f));

    _mm_store_ps(out_u, u_4x);
    _mm_store_ps(out_v, v_4x);
}

__INLINE__ void gc_sse_compute_lod(u32 width, u32 height, u32 mip_count, r32 *dudx, r32 *dudy, r32 *dvdx, r32 *dvdy, lod_t *lod)
{
    OPTICK_EVENT("gc_sse_compute_lod");

    r32 tmp[GC_FRAG_SIZE];
    r32 mip_lod[GC_FRAG_SIZE];

    __m128 width_4x = _mm_set1_ps(width);
    __m128 height_4x = _mm_set1_ps(height);

    __m128 tdudx_4x = _mm_mul_ps(_mm_load_ps(dudx), width_4x);
    __m128 tdudy_4x = _mm_mul_ps(_mm_load_ps(dudy), width_4x);
    __m128 tdvdx_4x = _mm_mul_ps(_mm_load_ps(dvdx), height_4x);
    __m128 tdvdy_4x = _mm_mul_ps(_mm_load_ps(dvdy), height_4x);

    tdudx_4x = SSE_ABS_PS(tdudx_4x);
    tdudy_4x = SSE_ABS_PS(tdudy_4x);
    tdvdx_4x = SSE_ABS_PS(tdvdx_4x);
    tdvdy_4x = SSE_ABS_PS(tdvdy_4x);

    __m128 t1 = _mm_add_ps(_mm_mul_ps(tdudx_4x, tdudx_4x), _mm_mul_ps(tdvdx_4x, tdvdx_4x));
    __m128 t2 = _mm_add_ps(_mm_mul_ps(tdudy_4x, tdudy_4x), _mm_mul_ps(tdvdy_4x, tdvdy_4x));
    __m128 mask = _mm_cmpge_ps(t1, t2);

    __m128 scale = SSE_CPYGE_PS(mask, t1, t2);
    _mm_store_ps(tmp, scale);

    mip_lod[0] = log2f(tmp[0]);
    mip_lod[1] = log2f(tmp[1]);
    mip_lod[2] = log2f(tmp[2]);
    mip_lod[3] = log2f(tmp[3]);

    __m128 mip_lod_4x = _mm_mul_ps(_mm_set1_ps(0.5f), _mm_load_ps(mip_lod));
    mip_lod_4x = sse_clamp(mip_lod_4x, 0, mip_count - 1);

    __m128 low_4x = _mm_floor_ps(mip_lod_4x);
    __m128 high_4x = _mm_ceil_ps(mip_lod_4x);

    _mm_store_si128((__m128i *) lod->low, _mm_cvttps_epi32(low_4x));
    _mm_store_si128((__m128i *) lod->high, _mm_cvttps_epi32(high_4x));
    _mm_store_ps(lod->interp, _mm_sub_ps(mip_lod_4x, low_4x));
}

#define gl_sse_texture_compute_lod(texture, fragment, lod) gc_sse_compute_lod((texture)->mips->header->width, (texture)->mips->header->height, (texture)->mip_count, fragment->dudx, fragment->dudy, fragment->dvdx, fragment->dvdy, lod)
#define gl_sse_cubemap_compute_lod(texture, fragment, lod) gc_sse_compute_lod((texture)->faces[0]->mips->header->width, (texture)->faces[0]->mips->header->height, (texture)->mip_count, fragment->dudx, fragment->dudy, fragment->dvdx, fragment->dvdy, lod)

void sse_cube_uv_from_vec(fv3_t *v, fv2_t *texcoord, u32 *face_index)
{}

void _sse_clear_rgbau8(texture2d_t *texture, gc_vec_t color)
{}

void _sse_clear_rgbaf(texture2d_t *texture, gc_vec_t color)
{}

void _sse_clear_rgbf(texture2d_t *texture, gc_vec_t color)
{}

void _sse_clear_rgf(texture2d_t *texture, gc_vec_t color)
{}

void _sse_sample_rgbau8(texture2d_t *texture, r32 *u, r32 *v, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_sse_sample_rgbau8");

    texture_mip_t *selected_texture_0 = texture->mips + lod[0];
    texture_mip_t *selected_texture_1 = texture->mips + lod[1];
    texture_mip_t *selected_texture_2 = texture->mips + lod[2];
    texture_mip_t *selected_texture_3 = texture->mips + lod[3];

    u32 *texture_pointer_0 = (u32 *) selected_texture_0->data;
    u32 *texture_pointer_1 = (u32 *) selected_texture_1->data;
    u32 *texture_pointer_2 = (u32 *) selected_texture_2->data;
    u32 *texture_pointer_3 = (u32 *) selected_texture_3->data;

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

    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];
    u32 offset[GC_FRAG_SIZE];

    texture->settings.tex_wrap(u, v, buffer0, buffer1);

    __m128 u0_4x = _mm_load_ps(buffer0);
    __m128 v0_4x = _mm_load_ps(buffer1);

    __m128 tex0_x_4x = _mm_mul_ps(u0_4x, width_4x);
    __m128 tex0_y_4x = _mm_mul_ps(v0_4x, height_4x);

    __m128i x0_4x = _mm_cvttps_epi32(tex0_x_4x);
    __m128i y0_4x = _mm_cvttps_epi32(tex0_y_4x);

    __m128i offset_4x = SSE_TEX_OFFSET(x0_4x, y0_4x, pitch_4x);
    _mm_store_si128((__m128i *) offset, offset_4x);

    __m128i samples_4x = _mm_setr_epi32(
        texture_pointer_0[offset[0]],
        texture_pointer_1[offset[1]],
        texture_pointer_2[offset[2]],
        texture_pointer_3[offset[3]]);

    sse_color_t sample0_4x;
    sse_unpack_color(samples_4x, &sample0_4x);

    if (texture->settings.flags & TEXTURE_FILTER)
    {
        sse_color_t sample1_4x;
        sse_color_t sample2_4x;
        sse_color_t sample3_4x;

        __m128 tx_4x = _mm_sub_ps(tex0_x_4x, _mm_floor_ps(tex0_x_4x));
        __m128 ty_4x = _mm_sub_ps(tex0_y_4x, _mm_floor_ps(tex0_y_4x));

        __m128 u1_4x = _mm_add_ps(u0_4x, tex_du_4x);
        __m128 v1_4x = _mm_add_ps(v0_4x, tex_dv_4x);

        _mm_store_ps(buffer0, u1_4x);
        _mm_store_ps(buffer1, v1_4x);

        texture->settings.tex_wrap(buffer0, buffer1, buffer0, buffer1);

        u1_4x = _mm_load_ps(buffer0);
        v1_4x = _mm_load_ps(buffer1);

        __m128 tex1_x_4x = _mm_mul_ps(u1_4x, width_4x);
        __m128 tex1_y_4x = _mm_mul_ps(v1_4x, height_4x);

        __m128i x1_4x = _mm_cvttps_epi32(tex1_x_4x);
        __m128i y1_4x = _mm_cvttps_epi32(tex1_y_4x);

        // -- Sample 1.

        offset_4x = SSE_TEX_OFFSET(x1_4x, y0_4x, pitch_4x);
        _mm_store_si128((__m128i *) offset, offset_4x);

        samples_4x = _mm_setr_epi32(
            texture_pointer_0[offset[0]],
            texture_pointer_1[offset[1]],
            texture_pointer_2[offset[2]],
            texture_pointer_3[offset[3]]);

        sse_unpack_color(samples_4x, &sample1_4x);

        // -- Sample 2.

        offset_4x = SSE_TEX_OFFSET(x0_4x, y1_4x, pitch_4x);
        _mm_store_si128((__m128i *) offset, offset_4x);

        samples_4x = _mm_setr_epi32(
            texture_pointer_0[offset[0]],
            texture_pointer_1[offset[1]],
            texture_pointer_2[offset[2]],
            texture_pointer_3[offset[3]]);

        sse_unpack_color(samples_4x, &sample2_4x);

        // -- Sample 3.

        offset_4x = SSE_TEX_OFFSET(x1_4x, y1_4x, pitch_4x);
        _mm_store_si128((__m128i *) offset, offset_4x);

        samples_4x = _mm_setr_epi32(
            texture_pointer_0[offset[0]],
            texture_pointer_1[offset[1]],
            texture_pointer_2[offset[2]],
            texture_pointer_3[offset[3]]);

        sse_unpack_color(samples_4x, &sample3_4x);

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
    }

    _mm_store_ps(output->r, sample0_4x.r);
    _mm_store_ps(output->g, sample0_4x.g);
    _mm_store_ps(output->b, sample0_4x.b);
    _mm_store_ps(output->a, sample0_4x.a);
}

void _sse_sample_rgbaf(texture2d_t *texture, r32 *u, r32 *v, u32 *lod, shader_color_t *output)
{}

void _sse_sample_rgbf(texture2d_t *texture, r32 *u, r32 *v, u32 *lod, shader_color_t *output)
{}

void _sse_sample_rgf(texture2d_t *texture, r32 *u, r32 *v, u32 *lod, shader_color_t *output)
{}

void _sse_cube_sample_rgbau8(cube_texture_t *cubemap, r32 *u, r32 *v, u32 *face_index, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_sse_cube_sample_rgbau8");

    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];
    u32 offset[GC_FRAG_SIZE];

    texture2d_t *face_0 = cubemap->faces[face_index[0]];
    texture2d_t *face_1 = cubemap->faces[face_index[1]];
    texture2d_t *face_2 = cubemap->faces[face_index[2]];
    texture2d_t *face_3 = cubemap->faces[face_index[3]];

    texture_mip_t *selected_texture_0 = face_0->mips + lod[0];
    texture_mip_t *selected_texture_1 = face_1->mips + lod[1];
    texture_mip_t *selected_texture_2 = face_2->mips + lod[2];
    texture_mip_t *selected_texture_3 = face_3->mips + lod[3];

    u32 *texture_pointer_0 = (u32 *) selected_texture_0->data;
    u32 *texture_pointer_1 = (u32 *) selected_texture_1->data;
    u32 *texture_pointer_2 = (u32 *) selected_texture_2->data;
    u32 *texture_pointer_3 = (u32 *) selected_texture_3->data;

    __m128 width_4x = _mm_setr_ps(
        face_0->mips->header->width - 1,
        face_1->mips->header->width - 1,
        face_2->mips->header->width - 1,
        face_3->mips->header->width - 1
    );

    __m128 height_4x = _mm_setr_ps(
        face_0->mips->header->height - 1,
        face_1->mips->header->height - 1,
        face_2->mips->header->height - 1,
        face_3->mips->header->height - 1
    );

    __m128i pitch_4x = _mm_setr_epi32(
        face_0->mips->header->width,
        face_1->mips->header->width,
        face_2->mips->header->width,
        face_3->mips->header->width
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

    face_0->settings.tex_wrap(u, v, buffer0, buffer1);

    __m128 u0_4x = _mm_load_ps(buffer0);
    __m128 v0_4x = _mm_load_ps(buffer1);

    __m128 tex0_x_4x = _mm_mul_ps(u0_4x, width_4x);
    __m128 tex0_y_4x = _mm_mul_ps(v0_4x, height_4x);

    __m128i x0_4x = _mm_cvttps_epi32(tex0_x_4x);
    __m128i y0_4x = _mm_cvttps_epi32(tex0_y_4x);

    __m128i offset_4x = SSE_TEX_OFFSET(x0_4x, y0_4x, pitch_4x);
    _mm_store_si128((__m128i *) offset, offset_4x);

    __m128i samples_4x = _mm_setr_epi32(
        texture_pointer_0[offset[0]],
        texture_pointer_1[offset[1]],
        texture_pointer_2[offset[2]],
        texture_pointer_3[offset[3]]);

    sse_color_t sample0_4x;
    sse_unpack_color(samples_4x, &sample0_4x);

    if (face_0->settings.flags & TEXTURE_FILTER)
    {
        sse_color_t sample1_4x;
        sse_color_t sample2_4x;
        sse_color_t sample3_4x;

        __m128 tx_4x = _mm_sub_ps(tex0_x_4x, _mm_floor_ps(tex0_x_4x));
        __m128 ty_4x = _mm_sub_ps(tex0_y_4x, _mm_floor_ps(tex0_y_4x));

        __m128 u1_4x = _mm_add_ps(u0_4x, tex_du_4x);
        __m128 v1_4x = _mm_add_ps(v0_4x, tex_dv_4x);

        _mm_store_ps(buffer0, u1_4x);
        _mm_store_ps(buffer1, v1_4x);

        face_0->settings.tex_wrap(buffer0, buffer1, buffer0, buffer1);

        u1_4x = _mm_load_ps(buffer0);
        v1_4x = _mm_load_ps(buffer1);

        __m128 tex1_x_4x = _mm_mul_ps(u1_4x, width_4x);
        __m128 tex1_y_4x = _mm_mul_ps(v1_4x, height_4x);

        __m128i x1_4x = _mm_cvttps_epi32(tex1_x_4x);
        __m128i y1_4x = _mm_cvttps_epi32(tex1_y_4x);

        // -- Sample 1.

        offset_4x = SSE_TEX_OFFSET(x1_4x, y0_4x, pitch_4x);
        _mm_store_si128((__m128i *) offset, offset_4x);

        samples_4x = _mm_setr_epi32(
            texture_pointer_0[offset[0]],
            texture_pointer_1[offset[1]],
            texture_pointer_2[offset[2]],
            texture_pointer_3[offset[3]]);

        sse_unpack_color(samples_4x, &sample1_4x);

        // -- Sample 2.

        offset_4x = SSE_TEX_OFFSET(x0_4x, y1_4x, pitch_4x);
        _mm_store_si128((__m128i *) offset, offset_4x);

        samples_4x = _mm_setr_epi32(
            texture_pointer_0[offset[0]],
            texture_pointer_1[offset[1]],
            texture_pointer_2[offset[2]],
            texture_pointer_3[offset[3]]);

        sse_unpack_color(samples_4x, &sample2_4x);

        // -- Sample 3.

        offset_4x = SSE_TEX_OFFSET(x1_4x, y1_4x, pitch_4x);
        _mm_store_si128((__m128i *) offset, offset_4x);

        samples_4x = _mm_setr_epi32(
            texture_pointer_0[offset[0]],
            texture_pointer_1[offset[1]],
            texture_pointer_2[offset[2]],
            texture_pointer_3[offset[3]]);

        sse_unpack_color(samples_4x, &sample3_4x);

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
    }

    _mm_store_ps(output->r, sample0_4x.r);
    _mm_store_ps(output->g, sample0_4x.g);
    _mm_store_ps(output->b, sample0_4x.b);
    _mm_store_ps(output->a, sample0_4x.a);
}

void _sse_cube_sample_rgbaf(cube_texture_t *cubemap, r32 *u, r32 *v, u32 *face_index, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_sse_cube_sample_rgbaf");

    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];
    u32 offset[GC_FRAG_SIZE];

    texture2d_t *face_0 = cubemap->faces[face_index[0]];
    texture2d_t *face_1 = cubemap->faces[face_index[1]];
    texture2d_t *face_2 = cubemap->faces[face_index[2]];
    texture2d_t *face_3 = cubemap->faces[face_index[3]];

    texture_mip_t *selected_texture_0 = face_0->mips + lod[0];
    texture_mip_t *selected_texture_1 = face_1->mips + lod[1];
    texture_mip_t *selected_texture_2 = face_2->mips + lod[2];
    texture_mip_t *selected_texture_3 = face_3->mips + lod[3];

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

    face_0->settings.tex_wrap(u, v, buffer0, buffer1);

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

    if (face_0->settings.flags & TEXTURE_FILTER)
    {
        sse_color_t sample1_4x;
        sse_color_t sample2_4x;
        sse_color_t sample3_4x;

        __m128 tx_4x = _mm_sub_ps(tex0_x_4x, _mm_floor_ps(tex0_x_4x));
        __m128 ty_4x = _mm_sub_ps(tex0_y_4x, _mm_floor_ps(tex0_y_4x));

        __m128 u1_4x = _mm_add_ps(u0_4x, tex_du_4x);
        __m128 v1_4x = _mm_add_ps(v0_4x, tex_dv_4x);

        _mm_store_ps(buffer0, u1_4x);
        _mm_store_ps(buffer1, v1_4x);

        face_0->settings.tex_wrap(buffer0, buffer1, buffer0, buffer1);

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
    }

    _mm_store_ps(output->r, sample0_4x.r);
    _mm_store_ps(output->g, sample0_4x.g);
    _mm_store_ps(output->b, sample0_4x.b);
    _mm_store_ps(output->a, sample0_4x.a);
}

void _sse_cube_sample_rgbf(cube_texture_t *cubemap, r32 *u, r32 *v, u32 *face_index, u32 *lod, shader_color_t *output)
{}

void _sse_cube_sample_rgf(cube_texture_t *cubemap, r32 *u, r32 *v, u32 *face_index, u32 *lod, shader_color_t *output)
{}

void sse_texture_sample(texture2d_t *texture, r32 *u, r32 *v, lod_t *lod, shader_color_t *output)
{
    OPTICK_EVENT("sse_texture_sample");

    if (!(texture->settings.flags & TEXTURE_MIPS))
    {
        _mm_store_si128((__m128i *) lod->low, _mm_setzero_si128());
        _mm_store_si128((__m128i *) lod->high, _mm_setzero_si128());
        _mm_store_ps(lod->interp, _mm_setzero_ps());
    }
    else
    {
        if (lod->high[0] < lod->low[0]) lod->high[0] = lod->low[0];
        if (lod->high[1] < lod->low[1]) lod->high[1] = lod->low[1];
        if (lod->high[2] < lod->low[2]) lod->high[2] = lod->low[2];
        if (lod->high[3] < lod->low[3]) lod->high[3] = lod->low[3];
    }

    texture->settings.tex_sample(texture, u, v, lod->low, output);

    // NOTE(gabic): For now all the channels from the output are computed, ignoring the
    // texture pixel format.

    if (((texture->settings.flags & (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)) == (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)))
    {
        shader_color_t output_max;
        texture->settings.tex_sample(texture, u, v, lod->high, &output_max);

        __m128 r_4x = _mm_load_ps(output->r);
        __m128 g_4x = _mm_load_ps(output->g);
        __m128 b_4x = _mm_load_ps(output->b);
        __m128 a_4x = _mm_load_ps(output->a);

        __m128 r_max_4x = _mm_load_ps(output_max.r);
        __m128 g_max_4x = _mm_load_ps(output_max.g);
        __m128 b_max_4x = _mm_load_ps(output_max.b);
        __m128 a_max_4x = _mm_load_ps(output_max.a);

        __m128 interp_4x = _mm_load_ps(lod->interp);

        r_4x = _mm_add_ps(r_4x, _mm_mul_ps(_mm_sub_ps(r_max_4x, r_4x), interp_4x));
        g_4x = _mm_add_ps(g_4x, _mm_mul_ps(_mm_sub_ps(g_max_4x, g_4x), interp_4x));
        b_4x = _mm_add_ps(b_4x, _mm_mul_ps(_mm_sub_ps(b_max_4x, b_4x), interp_4x));
        a_4x = _mm_add_ps(a_4x, _mm_mul_ps(_mm_sub_ps(a_max_4x, a_4x), interp_4x));

        _mm_store_ps(output->r, r_4x);
        _mm_store_ps(output->g, g_4x);
        _mm_store_ps(output->b, b_4x);
        _mm_store_ps(output->a, a_4x);
    }
}

void sse_brdf_lut_sample(texture2d_t *lut, r32 *cos_theta, r32 *roughness, fv2_t *output)
{
    OPTICK_EVENT("sse_brdf_lut_sample");

    brdf_lut_pixel_t *pointer = (brdf_lut_pixel_t *) lut->mips->data;

    __m128i pitch_4x = _mm_set1_epi32(lut->mips->header->width);
    __m128 width_4x = _mm_set1_ps(lut->mips->header->width - 1);
    __m128 height_4x = _mm_set1_ps(lut->mips->header->height - 1);
    u32 offset[GC_FRAG_SIZE];

    __m128 cos_theta_4x = _mm_load_ps(cos_theta);
    __m128 roughness_4x = _mm_load_ps(roughness);

    __m128i x_4x = _mm_cvttps_epi32(_mm_mul_ps(cos_theta_4x, width_4x));
    __m128i y_4x = _mm_cvttps_epi32(_mm_mul_ps(roughness_4x, height_4x));

    __m128i offset_4x = SSE_TEX_OFFSET(x_4x, y_4x, pitch_4x);
    _mm_store_si128((__m128i *) offset, offset_4x);

    brdf_lut_pixel_t *sample0 = pointer + offset[0];
    brdf_lut_pixel_t *sample1 = pointer + offset[1];
    brdf_lut_pixel_t *sample2 = pointer + offset[2];
    brdf_lut_pixel_t *sample3 = pointer + offset[3];

    __m128 a_4x = _mm_setr_ps(sample0->a, sample1->a, sample2->a, sample3->a);
    __m128 b_4x = _mm_setr_ps(sample0->b, sample1->b, sample2->b, sample3->b);

    _mm_store_ps(output->x, a_4x);
    _mm_store_ps(output->y, b_4x);
}

void sse_cube_texture_sample(cube_texture_t *cubemap, fv3_t *position, lod_t *lod, b8 warp, shader_color_t *output)
{
    OPTICK_EVENT("sse_cube_texture_sample");

    u32 face_index[GC_FRAG_SIZE];
    fv2_t texcoord;

    A4SET(face_index, 0, 0, 0, 0);

    if (!(cubemap->settings.flags & TEXTURE_MIPS))
    {
        _mm_store_si128((__m128i *) lod->low, _mm_setzero_si128());
        _mm_store_si128((__m128i *) lod->high, _mm_setzero_si128());
        _mm_store_ps(lod->interp, _mm_setzero_ps());
    }
    else
    {
        if (lod->high[0] < lod->low[0]) lod->high[0] = lod->low[0];
        if (lod->high[1] < lod->low[1]) lod->high[1] = lod->low[1];
        if (lod->high[2] < lod->low[2]) lod->high[2] = lod->low[2];
        if (lod->high[3] < lod->low[3]) lod->high[3] = lod->low[3];
    }

    cube_uv_from_vec(position, &texcoord, face_index, warp);
    cubemap->settings.tex_cube_sample(cubemap, texcoord.x, texcoord.y, face_index, lod->low, output);

    // NOTE(gabic): For now only 3 channels from the output are computed, ignoring the
    // texture pixel format.

    if (((cubemap->settings.flags & (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)) == (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)))
    {
        shader_color_t output_max;
        cubemap->settings.tex_cube_sample(cubemap, texcoord.x, texcoord.y, face_index, lod->high, &output_max);

        __m128 r_4x = _mm_load_ps(output->r);
        __m128 g_4x = _mm_load_ps(output->g);
        __m128 b_4x = _mm_load_ps(output->b);
        __m128 a_4x = _mm_load_ps(output->a);

        __m128 r_max_4x = _mm_load_ps(output_max.r);
        __m128 g_max_4x = _mm_load_ps(output_max.g);
        __m128 b_max_4x = _mm_load_ps(output_max.b);
        __m128 a_max_4x = _mm_load_ps(output_max.a);

        __m128 interp_4x = _mm_load_ps(lod->interp);

        r_4x = _mm_add_ps(r_4x, _mm_mul_ps(_mm_sub_ps(r_max_4x, r_4x), interp_4x));
        g_4x = _mm_add_ps(g_4x, _mm_mul_ps(_mm_sub_ps(g_max_4x, g_4x), interp_4x));
        b_4x = _mm_add_ps(b_4x, _mm_mul_ps(_mm_sub_ps(b_max_4x, b_4x), interp_4x));
        a_4x = _mm_add_ps(a_4x, _mm_mul_ps(_mm_sub_ps(a_max_4x, a_4x), interp_4x));

        _mm_store_ps(output->r, r_4x);
        _mm_store_ps(output->g, g_4x);
        _mm_store_ps(output->b, b_4x);
        _mm_store_ps(output->a, a_4x);
    }
}

void sse_shadow_map_sun_sample(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_sun_sample");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    u32 map_size = texture->header->width - 1;
    u32 map_pitch = texture->header->width;

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    _sse_tex_clamp(u, v, local_u, local_v);

    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(map_pitch);

    __m128 u_4x = _mm_load_ps(local_u);
    __m128 v_4x = _mm_load_ps(local_v);

    __m128 tex_x_4x = _mm_mul_ps(u_4x, map_size_4x);
    __m128 tex_y_4x = _mm_mul_ps(v_4x, map_size_4x);

    __m128i x_4x = _mm_cvttps_epi32(tex_x_4x);
    __m128i y_4x = _mm_cvttps_epi32(tex_y_4x);

    __m128i offset_4x = SSE_TEX_OFFSET(x_4x, y_4x, pitch_4x);
    u32 offset[GC_FRAG_SIZE];
    _mm_store_si128((__m128i *) offset, offset_4x);

    texpixel_rgf_t *sample0 = texture_pointer + offset[0];
    texpixel_rgf_t *sample1 = texture_pointer + offset[1];
    texpixel_rgf_t *sample2 = texture_pointer + offset[2];
    texpixel_rgf_t *sample3 = texture_pointer + offset[3];

    __m128 compare_4x = _mm_load_ps(compare);
    __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    _mm_store_ps(output, check_4x);
}

void sse_shadow_map_sun_sample_linear(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_sun_sample_linear");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    u32 map_size = texture->header->width - 1;
    u32 map_pitch = texture->header->width;

    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];

    _sse_tex_clamp(u, v, buffer0, buffer1);

    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(map_pitch);
    __m128 compare_4x = _mm_load_ps(compare);

    __m128 u0_4x = _mm_load_ps(buffer0);
    __m128 v0_4x = _mm_load_ps(buffer1);

    __m128 u1_4x = _mm_add_ps(u0_4x, _mm_set1_ps(texture->header->tex_du));
    __m128 v1_4x = _mm_add_ps(v0_4x, _mm_set1_ps(texture->header->tex_dv));

    _mm_store_ps(buffer0, u1_4x);
    _mm_store_ps(buffer1, v1_4x);

    _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1);

    u1_4x = _mm_load_ps(buffer0);
    v1_4x = _mm_load_ps(buffer1);

    __m128 tex_x0_4x = _mm_mul_ps(u0_4x, map_size_4x);
    __m128 tex_y0_4x = _mm_mul_ps(v0_4x, map_size_4x);

    __m128 tex_x1_4x = _mm_mul_ps(u1_4x, map_size_4x);
    __m128 tex_y1_4x = _mm_mul_ps(v1_4x, map_size_4x);

    __m128 s_4x = _mm_sub_ps(tex_x0_4x, _mm_floor_ps(tex_x0_4x));
    __m128 t_4x = _mm_sub_ps(tex_y0_4x, _mm_floor_ps(tex_y0_4x));

    __m128i x0_4x = _mm_cvttps_epi32(tex_x0_4x);
    __m128i y0_4x = _mm_cvttps_epi32(tex_y0_4x);

    __m128i x1_4x = _mm_cvttps_epi32(tex_x1_4x);
    __m128i y1_4x = _mm_cvttps_epi32(tex_y1_4x);

    __m128i offset0_4x = SSE_TEX_OFFSET(x0_4x, y0_4x, pitch_4x);
    __m128i offset1_4x = SSE_TEX_OFFSET(x1_4x, y0_4x, pitch_4x);
    __m128i offset2_4x = SSE_TEX_OFFSET(x0_4x, y1_4x, pitch_4x);
    __m128i offset3_4x = SSE_TEX_OFFSET(x1_4x, y1_4x, pitch_4x);

    u32 offset0[GC_FRAG_SIZE];
    u32 offset1[GC_FRAG_SIZE];
    u32 offset2[GC_FRAG_SIZE];
    u32 offset3[GC_FRAG_SIZE];

    _mm_store_si128((__m128i *) offset0, offset0_4x);
    _mm_store_si128((__m128i *) offset1, offset1_4x);
    _mm_store_si128((__m128i *) offset2, offset2_4x);
    _mm_store_si128((__m128i *) offset3, offset3_4x);

    texpixel_rgf_t *sample0 = texture_pointer + offset0[0];
    texpixel_rgf_t *sample1 = texture_pointer + offset0[1];
    texpixel_rgf_t *sample2 = texture_pointer + offset0[2];
    texpixel_rgf_t *sample3 = texture_pointer + offset0[3];

    __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check0_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    sample0 = texture_pointer + offset1[0];
    sample1 = texture_pointer + offset1[1];
    sample2 = texture_pointer + offset1[2];
    sample3 = texture_pointer + offset1[3];

    sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check1_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    sample0 = texture_pointer + offset2[0];
    sample1 = texture_pointer + offset2[1];
    sample2 = texture_pointer + offset2[2];
    sample3 = texture_pointer + offset2[3];

    sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check2_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    sample0 = texture_pointer + offset3[0];
    sample1 = texture_pointer + offset3[1];
    sample2 = texture_pointer + offset3[2];
    sample3 = texture_pointer + offset3[3];

    sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check3_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    // ----------------------------------------------------------------------------------
    // -- Final interpolated output.
    // ----------------------------------------------------------------------------------

    sample_4x = _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(check0_4x, _mm_mul_ps(_mm_sub_ps(check1_4x, check0_4x), s_4x)), _mm_sub_ps(_mm_set1_ps(1.0f), t_4x)),
                    _mm_mul_ps(_mm_add_ps(check2_4x, _mm_mul_ps(_mm_sub_ps(check3_4x, check2_4x), s_4x)), t_4x));

    _mm_store_ps(output, sample_4x);
}

void sse_shadow_map_sun_sample_pcf_3x3(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_sun_sample_pcf_3x3");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    __m128 map_size_4x = _mm_set1_ps(texture->header->width - 1);
    __m128i pitch_4x = _mm_set1_epi32(texture->header->width);

    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];
    r32 pcf_u[GC_FRAG_SIZE];
    r32 pcf_v[GC_FRAG_SIZE];
    r32 tmp[GC_FRAG_SIZE];
    r32 offset_v[GC_FRAG_SIZE] = {-1, 0, 1};

    __m128 over_nine_4x = _mm_set1_ps(1.0f / 9);
    __m128 offset_u_4x = _mm_setr_ps(-1, 0, 1, 2);
    __m128 tex_du_4x = _mm_set1_ps(texture->header->tex_du);
    __m128 tex_dv_4x = _mm_set1_ps(texture->header->tex_dv);

    _sse_tex_clamp(u, v, buffer0, buffer1);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        __m128 base_u = _mm_set1_ps(buffer0[i]);
        __m128 base_v = _mm_set1_ps(buffer1[i]);
        __m128 result_4x = _mm_setzero_ps();

        __m128 patch_u = _mm_add_ps(base_u, _mm_mul_ps(offset_u_4x, tex_du_4x));
        __m128 compare_4x = _mm_set1_ps(compare[i]);
        u32 offset[GC_FRAG_SIZE];

        for (u8 j = 0; j < 3; ++j)
        {
            __m128 patch_v = _mm_add_ps(base_v, _mm_mul_ps(_mm_set1_ps(offset_v[j]), tex_dv_4x));

            _mm_store_ps(pcf_u, patch_u);
            _mm_store_ps(pcf_v, patch_v);

            _sse_tex_clamp(pcf_u, pcf_v, pcf_u, pcf_v);

            patch_u = _mm_load_ps(pcf_u);
            patch_v = _mm_load_ps(pcf_v);

            __m128i x_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_u, map_size_4x));
            __m128i y_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_v, map_size_4x));
            __m128i offset_4x = SSE_TEX_OFFSET(x_4x, y_4x, pitch_4x);

            _mm_store_si128((__m128i *) offset, offset_4x);

            texpixel_rgf_t *sample0 = texture_pointer + offset[0];
            texpixel_rgf_t *sample1 = texture_pointer + offset[1];
            texpixel_rgf_t *sample2 = texture_pointer + offset[2];

            __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, 0.0f);
            __m128 check_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

            result_4x = _mm_add_ps(result_4x, check_4x);
        }

        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_mul_ps(result_4x, over_nine_4x);

        _mm_store_ps(tmp, result_4x);

        output[i] = tmp[0];
    }
}

// ----------------------------------------------------------------------------------

#define SSE_ST3x3_COMPUTE(index) \
    tmp_u_4x = _mm_add_ps(base_u, _mm_mul_ps(_mm_load_ps(sse_stu3x3_offset[index].data), tex_du_4x)); \
    tmp_v_4x = _mm_add_ps(base_v, _mm_mul_ps(_mm_load_ps(sse_stv3x3_offset[index].data), tex_dv_4x)); \
\
    _mm_store_ps(buffer0, tmp_u_4x); \
    _mm_store_ps(buffer1, tmp_v_4x); \
    _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1); \
    tmp_u_4x = _mm_load_ps(buffer0); \
    tmp_v_4x = _mm_load_ps(buffer1); \
\
    tmp_texel_x_4x = _mm_mul_ps(tmp_u_4x, map_size_4x); \
    tmp_texel_y_4x = _mm_mul_ps(tmp_v_4x, map_size_4x); \
    __m128 s##index##_4x = _mm_sub_ps(tmp_texel_x_4x, _mm_floor_ps(tmp_texel_x_4x)); \
    __m128 t##index##_4x = _mm_sub_ps(tmp_texel_y_4x, _mm_floor_ps(tmp_texel_y_4x));

#define SSE_ST5x5_COMPUTE(index) \
    tmp_u_4x = _mm_add_ps(base_u, _mm_mul_ps(_mm_load_ps(sse_stu5x5_offset[index].data), tex_du_4x)); \
    tmp_v_4x = _mm_add_ps(base_v, _mm_mul_ps(_mm_load_ps(sse_stv5x5_offset[index].data), tex_dv_4x)); \
\
    _mm_store_ps(buffer0, tmp_u_4x); \
    _mm_store_ps(buffer1, tmp_v_4x); \
    _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1); \
    tmp_u_4x = _mm_load_ps(buffer0); \
    tmp_v_4x = _mm_load_ps(buffer1); \
\
    tmp_texel_x_4x = _mm_mul_ps(tmp_u_4x, map_size_4x); \
    tmp_texel_y_4x = _mm_mul_ps(tmp_v_4x, map_size_4x); \
    __m128 s##index##_4x = _mm_sub_ps(tmp_texel_x_4x, _mm_floor_ps(tmp_texel_x_4x)); \
    __m128 t##index##_4x = _mm_sub_ps(tmp_texel_y_4x, _mm_floor_ps(tmp_texel_y_4x));

// ----------------------------------------------------------------------------------

void sse_shadow_map_sun_sample_pcf_3x3_linear(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_sun_sample_pcf_3x3_linear");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    r32 sample_patch[16];
    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];
    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];

    __m128 over_nine_4x = _mm_set1_ps(1.0f / 9);
    __m128 offset_u_4x = _mm_setr_ps(-1, 0, 1, 2);
    r32 offset_v[GC_FRAG_SIZE] = {-1, 0, 1, 2};

    r32 map_size = texture->header->width - 1;
    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(texture->header->width);
    __m128 tex_du_4x = _mm_set1_ps(texture->header->tex_du);
    __m128 tex_dv_4x = _mm_set1_ps(texture->header->tex_dv);

    __m128 tmp_u_4x = _mm_setzero_ps();
    __m128 tmp_v_4x = _mm_setzero_ps();

    __m128 tmp_texel_x_4x = _mm_setzero_ps();
    __m128 tmp_texel_y_4x = _mm_setzero_ps();

    _sse_tex_clamp(u, v, local_u, local_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        // r32 tex_x = local_u[i] * map_size;
        // r32 tex_y = local_v[i] * map_size;

        // r32 s = tex_x - (u32) tex_x;
        // r32 t = tex_y - (u32) tex_y;

        // __m128 s_4x = _mm_set1_ps(s);
        // __m128 t_4x = _mm_set1_ps(t);

        __m128 base_u = _mm_set1_ps(local_u[i]);
        __m128 base_v = _mm_set1_ps(local_v[i]);
        __m128 result_4x = _mm_setzero_ps();

        __m128 patch_u = _mm_add_ps(base_u, _mm_mul_ps(offset_u_4x, tex_du_4x));
        __m128 compare_4x = _mm_set1_ps(compare[i]);
        u32 offset[GC_FRAG_SIZE];

        // ----------------------------------------------------------------------------------
        // -- Collect the patch compare sample data.
        // ----------------------------------------------------------------------------------

        for (u8 j = 0; j < 4; ++j)
        {
            __m128 patch_v = _mm_add_ps(base_v, _mm_mul_ps(_mm_set1_ps(offset_v[j]), tex_dv_4x));

            _mm_store_ps(buffer0, patch_u);
            _mm_store_ps(buffer1, patch_v);

            _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1);

            patch_u = _mm_load_ps(buffer0);
            patch_v = _mm_load_ps(buffer1);

            __m128i x_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_u, map_size_4x));
            __m128i y_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_v, map_size_4x));
            __m128i offset_4x = SSE_TEX_OFFSET(x_4x, y_4x, pitch_4x);
            _mm_store_si128((__m128i *) offset, offset_4x);

            texpixel_rgf_t *sample0 = texture_pointer + offset[0];
            texpixel_rgf_t *sample1 = texture_pointer + offset[1];
            texpixel_rgf_t *sample2 = texture_pointer + offset[2];
            texpixel_rgf_t *sample3 = texture_pointer + offset[3];

            __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            __m128 check_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));
            _mm_store_ps(sample_patch + (j * 4), check_4x);
        }

        // ----------------------------------------------------------------------------------
        // -- Compute the interpolators (s,t).
        // ----------------------------------------------------------------------------------

        SSE_ST3x3_COMPUTE(0);
        SSE_ST3x3_COMPUTE(1);
        SSE_ST3x3_COMPUTE(2);

        // ----------------------------------------------------------------------------------
        // -- Filter the samples.
        // ----------------------------------------------------------------------------------

        // A, B, C, D
        __m128 reg0 = _mm_setr_ps(sample_patch[0], sample_patch[1], sample_patch[2], sample_patch[4]);
        __m128 reg1 = _mm_setr_ps(sample_patch[1], sample_patch[2], sample_patch[3], sample_patch[5]);
        __m128 reg2 = _mm_setr_ps(sample_patch[4], sample_patch[5], sample_patch[6], sample_patch[8]);
        __m128 reg3 = _mm_setr_ps(sample_patch[5], sample_patch[6], sample_patch[7], sample_patch[9]);

        __m128 interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s0_4x, t0_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // E, F, G, H
        reg0 = _mm_setr_ps(sample_patch[5], sample_patch[6], sample_patch[8], sample_patch[9]);
        reg1 = _mm_setr_ps(sample_patch[6], sample_patch[7], sample_patch[9], sample_patch[10]);
        reg2 = _mm_setr_ps(sample_patch[9], sample_patch[10], sample_patch[12], sample_patch[13]);
        reg3 = _mm_setr_ps(sample_patch[10], sample_patch[11], sample_patch[13], sample_patch[14]);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s1_4x, t1_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // I
        reg0 = _mm_setr_ps(sample_patch[10], 0, 0, 0);
        reg1 = _mm_setr_ps(sample_patch[11], 0, 0, 0);
        reg2 = _mm_setr_ps(sample_patch[14], 0, 0, 0);
        reg3 = _mm_setr_ps(sample_patch[15], 0, 0, 0);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s2_4x, t2_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_mul_ps(result_4x, over_nine_4x);

        _mm_store_ps(buffer0, result_4x);
        output[i] = buffer0[0];
    }
}

void sse_shadow_map_sun_sample_pcf_5x5(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_sun_sample_pcf_5x5");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    __m128 map_size_4x = _mm_set1_ps(texture->header->width - 1);
    __m128i pitch_4x = _mm_set1_epi32(texture->header->width);

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    r32 pcf_u[GC_FRAG_SIZE];
    r32 pcf_v[GC_FRAG_SIZE];

    r32 tmp[GC_FRAG_SIZE];

    __m128 over_25_4x = _mm_set1_ps(1.0f / 25);
    r32 offset_v[5] = {-2, -1, 0, 1, 2};
    __m128 offset_u0_4x = _mm_setr_ps(-2, -1, 0, 1);
    __m128 offset_u1_4x = _mm_setr_ps(2, 0, 0, 0);
    __m128 tex_du_4x = _mm_set1_ps(texture->header->tex_du);
    __m128 tex_dv_4x = _mm_set1_ps(texture->header->tex_dv);

    _sse_tex_clamp(u, v, local_u, local_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        __m128 base_u = _mm_set1_ps(local_u[i]);
        __m128 base_v = _mm_set1_ps(local_v[i]);
        __m128 result_4x = _mm_setzero_ps();

        __m128 patch_u0 = _mm_add_ps(base_u, _mm_mul_ps(offset_u0_4x, tex_du_4x));
        __m128 patch_u1 = _mm_add_ps(base_u, _mm_mul_ps(offset_u1_4x, tex_du_4x));
        __m128 compare_4x = _mm_set1_ps(compare[i]);

        for (u8 j = 0; j < 5; ++j)
        {
            __m128 patch_v = _mm_add_ps(base_v, _mm_mul_ps(_mm_set1_ps(offset_v[j]), tex_dv_4x));

            _mm_store_ps(pcf_u, patch_u0);
            _mm_store_ps(pcf_v, patch_v);

            _sse_tex_clamp(pcf_u, pcf_v, pcf_u, pcf_v);

            patch_u0 = _mm_load_ps(pcf_u);
            patch_v = _mm_load_ps(pcf_v);

            _mm_store_ps(pcf_u, patch_u1);
            _sse_tex_clamp(pcf_u, pcf_v, pcf_u, pcf_v);
            patch_u1 = _mm_load_ps(pcf_u);

            __m128i x0_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_u0, map_size_4x));
            __m128i x1_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_u1, map_size_4x));
            __m128i y_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_v, map_size_4x));
            __m128i offset0_4x = SSE_TEX_OFFSET(x0_4x, y_4x, pitch_4x);
            __m128i offset1_4x = SSE_TEX_OFFSET(x1_4x, y_4x, pitch_4x);

            texpixel_rgf_t *sample0 = texture_pointer + _mm_extract_epi32(offset0_4x, 0);
            texpixel_rgf_t *sample1 = texture_pointer + _mm_extract_epi32(offset0_4x, 1);
            texpixel_rgf_t *sample2 = texture_pointer + _mm_extract_epi32(offset0_4x, 2);
            texpixel_rgf_t *sample3 = texture_pointer + _mm_extract_epi32(offset0_4x, 3);
            texpixel_rgf_t *sample4 = texture_pointer + _mm_extract_epi32(offset1_4x, 0);

            __m128 sample0_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            __m128 sample1_4x = _mm_setr_ps(sample4->r, 0, 0, 0);

            __m128 check0_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample0_4x), _mm_set1_ps(1.0f));
            __m128 check1_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample1_4x), _mm_set1_ps(1.0f));

            result_4x = _mm_add_ps(result_4x, check0_4x);
            result_4x = _mm_add_ps(result_4x, check1_4x);
        }

        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_mul_ps(result_4x, over_25_4x);

        _mm_store_ps(tmp, result_4x);

        output[i] = tmp[0];
    }
}

void sse_shadow_map_sun_sample_pcf_5x5_linear(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_sun_sample_pcf_5x5_linear");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    r32 map_size = texture->header->width - 1;
    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(texture->header->width);

    r32 sample_patch[36];
    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];

    __m128 over_25_4x = _mm_set1_ps(1.0f / 25);
    r32 offset_v[6] = {-2, -1, 0, 1, 2, 3};
    __m128 offset_u0_4x = _mm_setr_ps(-2, -1, 0, 1);
    __m128 offset_u1_4x = _mm_setr_ps(2, 3, 0, 0);
    __m128 tex_du_4x = _mm_set1_ps(texture->header->tex_du);
    __m128 tex_dv_4x = _mm_set1_ps(texture->header->tex_dv);

    __m128 tmp_u_4x = _mm_setzero_ps();
    __m128 tmp_v_4x = _mm_setzero_ps();

    __m128 tmp_texel_x_4x = _mm_setzero_ps();
    __m128 tmp_texel_y_4x = _mm_setzero_ps();

    _sse_tex_clamp(u, v, local_u, local_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        // r32 tex_x = local_u[i] * map_size;
        // r32 tex_y = local_v[i] * map_size;

        // r32 s = tex_x - (u32) tex_x;
        // r32 t = tex_y - (u32) tex_y;

        // __m128 s_4x = _mm_set1_ps(s);
        // __m128 t_4x = _mm_set1_ps(t);

        __m128 base_u = _mm_set1_ps(local_u[i]);
        __m128 base_v = _mm_set1_ps(local_v[i]);
        __m128 result_4x = _mm_setzero_ps();

        __m128 patch_u0 = _mm_add_ps(base_u, _mm_mul_ps(offset_u0_4x, tex_du_4x));
        __m128 patch_u1 = _mm_add_ps(base_u, _mm_mul_ps(offset_u1_4x, tex_du_4x));
        __m128 compare_4x = _mm_set1_ps(compare[i]);

        // ----------------------------------------------------------------------------------
        // -- Collect the patch compare sample data.
        // ----------------------------------------------------------------------------------

        for (u8 j = 0; j < 6; ++j)
        {
            __m128 patch_v = _mm_add_ps(base_v, _mm_mul_ps(_mm_set1_ps(offset_v[j]), tex_dv_4x));

            _mm_store_ps(buffer0, patch_u0);
            _mm_store_ps(buffer1, patch_v);

            _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1);

            patch_u0 = _mm_load_ps(buffer0);
            patch_v = _mm_load_ps(buffer1);

            _mm_store_ps(buffer0, patch_u1);
            _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1);
            patch_u1 = _mm_load_ps(buffer0);

            __m128i x0_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_u0, map_size_4x));
            __m128i x1_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_u1, map_size_4x));
            __m128i y_4x = _mm_cvttps_epi32(_mm_mul_ps(patch_v, map_size_4x));
            __m128i offset0_4x = SSE_TEX_OFFSET(x0_4x, y_4x, pitch_4x);
            __m128i offset1_4x = SSE_TEX_OFFSET(x1_4x, y_4x, pitch_4x);

            texpixel_rgf_t *sample0 = texture_pointer + _mm_extract_epi32(offset0_4x, 0);
            texpixel_rgf_t *sample1 = texture_pointer + _mm_extract_epi32(offset0_4x, 1);
            texpixel_rgf_t *sample2 = texture_pointer + _mm_extract_epi32(offset0_4x, 2);
            texpixel_rgf_t *sample3 = texture_pointer + _mm_extract_epi32(offset0_4x, 3);

            texpixel_rgf_t *sample4 = texture_pointer + _mm_extract_epi32(offset1_4x, 0);
            texpixel_rgf_t *sample5 = texture_pointer + _mm_extract_epi32(offset1_4x, 1);

            __m128 sample0_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            __m128 sample1_4x = _mm_setr_ps(sample4->r, sample5->r, 0, 0);

            __m128 check0_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample0_4x), _mm_set1_ps(1.0f));
            __m128 check1_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample1_4x), _mm_set1_ps(1.0f));

            u32 index = j * 6;

            _mm_store_ps(sample_patch + index, check0_4x);
            _mm_store_ps(buffer0, check1_4x);

            sample_patch[index + 4] = buffer0[0];
            sample_patch[index + 5] = buffer0[1];
        }

        // ----------------------------------------------------------------------------------
        // -- Compute the interpolators (s,t).
        // ----------------------------------------------------------------------------------

        SSE_ST5x5_COMPUTE(0);
        SSE_ST5x5_COMPUTE(1);
        SSE_ST5x5_COMPUTE(2);
        SSE_ST5x5_COMPUTE(3);
        SSE_ST5x5_COMPUTE(4);
        SSE_ST5x5_COMPUTE(5);
        SSE_ST5x5_COMPUTE(6);

        // ----------------------------------------------------------------------------------
        // -- Filter the samples.
        // ----------------------------------------------------------------------------------

        // A, B, C, D
        __m128 reg0 = _mm_setr_ps(sample_patch[0], sample_patch[1], sample_patch[2], sample_patch[3]);
        __m128 reg1 = _mm_setr_ps(sample_patch[1], sample_patch[2], sample_patch[3], sample_patch[4]);
        __m128 reg2 = _mm_setr_ps(sample_patch[6], sample_patch[7], sample_patch[8], sample_patch[9]);
        __m128 reg3 = _mm_setr_ps(sample_patch[7], sample_patch[8], sample_patch[9], sample_patch[10]);

        __m128 interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s0_4x, t0_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // E, F, G, H
        reg0 = _mm_setr_ps(sample_patch[4], sample_patch[6], sample_patch[7], sample_patch[8]);
        reg1 = _mm_setr_ps(sample_patch[5], sample_patch[7], sample_patch[8], sample_patch[9]);
        reg2 = _mm_setr_ps(sample_patch[10], sample_patch[12], sample_patch[13], sample_patch[14]);
        reg3 = _mm_setr_ps(sample_patch[11], sample_patch[13], sample_patch[14], sample_patch[15]);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s1_4x, t1_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // I, J, K, L
        reg0 = _mm_setr_ps(sample_patch[9], sample_patch[10], sample_patch[12], sample_patch[13]);
        reg1 = _mm_setr_ps(sample_patch[10], sample_patch[11], sample_patch[13], sample_patch[14]);
        reg2 = _mm_setr_ps(sample_patch[15], sample_patch[16], sample_patch[18], sample_patch[19]);
        reg3 = _mm_setr_ps(sample_patch[16], sample_patch[17], sample_patch[19], sample_patch[20]);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s2_4x, t2_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // M, N, O, P
        reg0 = _mm_setr_ps(sample_patch[14], sample_patch[15], sample_patch[16], sample_patch[18]);
        reg1 = _mm_setr_ps(sample_patch[15], sample_patch[16], sample_patch[17], sample_patch[19]);
        reg2 = _mm_setr_ps(sample_patch[20], sample_patch[21], sample_patch[22], sample_patch[24]);
        reg3 = _mm_setr_ps(sample_patch[21], sample_patch[22], sample_patch[23], sample_patch[25]);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s3_4x, t3_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // Q, R, S, T
        reg0 = _mm_setr_ps(sample_patch[19], sample_patch[20], sample_patch[21], sample_patch[22]);
        reg1 = _mm_setr_ps(sample_patch[20], sample_patch[21], sample_patch[22], sample_patch[23]);
        reg2 = _mm_setr_ps(sample_patch[25], sample_patch[26], sample_patch[27], sample_patch[28]);
        reg3 = _mm_setr_ps(sample_patch[26], sample_patch[27], sample_patch[28], sample_patch[29]);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s4_4x, t4_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // U, V, W, X
        reg0 = _mm_setr_ps(sample_patch[24], sample_patch[25], sample_patch[26], sample_patch[27]);
        reg1 = _mm_setr_ps(sample_patch[25], sample_patch[26], sample_patch[27], sample_patch[28]);
        reg2 = _mm_setr_ps(sample_patch[30], sample_patch[31], sample_patch[32], sample_patch[33]);
        reg3 = _mm_setr_ps(sample_patch[31], sample_patch[32], sample_patch[33], sample_patch[34]);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s5_4x, t5_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        // Y
        reg0 = _mm_setr_ps(sample_patch[28], 0, 0, 0);
        reg1 = _mm_setr_ps(sample_patch[29], 0, 0, 0);
        reg2 = _mm_setr_ps(sample_patch[34], 0, 0, 0);
        reg3 = _mm_setr_ps(sample_patch[35], 0, 0, 0);

        interp_4x = SSE_LINEAR_INTERP(reg0, reg1, reg2, reg3, s6_4x, t6_4x);
        result_4x = _mm_add_ps(result_4x, interp_4x);

        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_mul_ps(result_4x, over_25_4x);

        _mm_store_ps(buffer0, result_4x);
        output[i] = buffer0[0];
    }
}

void sse_shadow_map_point_sample(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_point_sample");

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    fv2_t texcoord;
    u32 face_index[GC_FRAG_SIZE];
    A4SET(face_index, 0, 0, 0, 0);

    cube_uv_from_vec(direction, &texcoord, face_index, false);
    _sse_tex_clamp(texcoord.x, texcoord.y, local_u, local_v);

    texture2d_t *face0 = shadow_map->faces[face_index[0]];
    texture2d_t *face1 = shadow_map->faces[face_index[1]];
    texture2d_t *face2 = shadow_map->faces[face_index[2]];
    texture2d_t *face3 = shadow_map->faces[face_index[3]];

    texpixel_rgf_t *texture_pointer0 = (texpixel_rgf_t *) face0->mips->data;
    texpixel_rgf_t *texture_pointer1 = (texpixel_rgf_t *) face1->mips->data;
    texpixel_rgf_t *texture_pointer2 = (texpixel_rgf_t *) face2->mips->data;
    texpixel_rgf_t *texture_pointer3 = (texpixel_rgf_t *) face3->mips->data;

    u32 map_size = face0->mips->header->width - 1;
    u32 map_pitch = face0->mips->header->width;

    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(map_pitch);

    __m128 u_4x = _mm_load_ps(local_u);
    __m128 v_4x = _mm_load_ps(local_v);

    __m128 tex_x_4x = _mm_mul_ps(u_4x, map_size_4x);
    __m128 tex_y_4x = _mm_mul_ps(v_4x, map_size_4x);

    __m128i x_4x = _mm_cvttps_epi32(tex_x_4x);
    __m128i y_4x = _mm_cvttps_epi32(tex_y_4x);

    __m128i offset_4x = SSE_TEX_OFFSET(x_4x, y_4x, pitch_4x);

    texpixel_rgf_t *sample0 = texture_pointer0 + _mm_extract_epi32(offset_4x, 0);
    texpixel_rgf_t *sample1 = texture_pointer1 + _mm_extract_epi32(offset_4x, 1);
    texpixel_rgf_t *sample2 = texture_pointer2 + _mm_extract_epi32(offset_4x, 2);
    texpixel_rgf_t *sample3 = texture_pointer3 + _mm_extract_epi32(offset_4x, 3);

    __m128 compare_4x = _mm_load_ps(compare);
    __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    _mm_store_ps(output, check_4x);
}

void sse_shadow_map_point_sample_linear(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_point_sample_linear");

    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];

    u32 map_pitch = shadow_map->faces[0]->mips->header->width;
    u32 map_size = map_pitch - 1;

    fv2_t texcoord;
    u32 face_index[GC_FRAG_SIZE];
    A4SET(face_index, 0, 0, 0, 0);

    cube_uv_from_vec(direction, &texcoord, face_index, false);
    _sse_tex_clamp(texcoord.x, texcoord.y, buffer0, buffer1);

    texture2d_t *face0 = shadow_map->faces[face_index[0]];
    texture2d_t *face1 = shadow_map->faces[face_index[1]];
    texture2d_t *face2 = shadow_map->faces[face_index[2]];
    texture2d_t *face3 = shadow_map->faces[face_index[3]];

    texpixel_rgf_t *texture_pointer0 = (texpixel_rgf_t *) face0->mips->data;
    texpixel_rgf_t *texture_pointer1 = (texpixel_rgf_t *) face1->mips->data;
    texpixel_rgf_t *texture_pointer2 = (texpixel_rgf_t *) face2->mips->data;
    texpixel_rgf_t *texture_pointer3 = (texpixel_rgf_t *) face3->mips->data;

    __m128 u0_4x = _mm_load_ps(buffer0);
    __m128 v0_4x = _mm_load_ps(buffer1);

    __m128 u1_4x = _mm_add_ps(u0_4x, _mm_set1_ps(face0->mips->header->tex_du));
    __m128 v1_4x = _mm_add_ps(v0_4x, _mm_set1_ps(face0->mips->header->tex_dv));

    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(map_pitch);
    __m128 compare_4x = _mm_load_ps(compare);

    _mm_store_ps(buffer0, u1_4x);
    _mm_store_ps(buffer1, v1_4x);

    _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1);

    u1_4x = _mm_load_ps(buffer0);
    v1_4x = _mm_load_ps(buffer1);

    // ----------------------------------------------------------------------------------
    // -- Linear interpolation.
    // ----------------------------------------------------------------------------------

    __m128 tex_x0_4x = _mm_mul_ps(u0_4x, map_size_4x);
    __m128 tex_y0_4x = _mm_mul_ps(v0_4x, map_size_4x);

    __m128 tex_x1_4x = _mm_mul_ps(u1_4x, map_size_4x);
    __m128 tex_y1_4x = _mm_mul_ps(v1_4x, map_size_4x);

    __m128 s_4x = _mm_sub_ps(tex_x0_4x, _mm_floor_ps(tex_x0_4x));
    __m128 t_4x = _mm_sub_ps(tex_y0_4x, _mm_floor_ps(tex_y0_4x));

    __m128i x0_4x = _mm_cvttps_epi32(tex_x0_4x);
    __m128i y0_4x = _mm_cvttps_epi32(tex_y0_4x);

    __m128i x1_4x = _mm_cvttps_epi32(tex_x1_4x);
    __m128i y1_4x = _mm_cvttps_epi32(tex_y1_4x);

    __m128i offset0_4x = SSE_TEX_OFFSET(x0_4x, y0_4x, pitch_4x);
    __m128i offset1_4x = SSE_TEX_OFFSET(x1_4x, y0_4x, pitch_4x);
    __m128i offset2_4x = SSE_TEX_OFFSET(x0_4x, y1_4x, pitch_4x);
    __m128i offset3_4x = SSE_TEX_OFFSET(x1_4x, y1_4x, pitch_4x);

    u32 offset0[GC_FRAG_SIZE];
    u32 offset1[GC_FRAG_SIZE];
    u32 offset2[GC_FRAG_SIZE];
    u32 offset3[GC_FRAG_SIZE];

    _mm_store_si128((__m128i *) offset0, offset0_4x);
    _mm_store_si128((__m128i *) offset1, offset1_4x);
    _mm_store_si128((__m128i *) offset2, offset2_4x);
    _mm_store_si128((__m128i *) offset3, offset3_4x);

    texpixel_rgf_t *sample0 = texture_pointer0 + offset0[0];
    texpixel_rgf_t *sample1 = texture_pointer1 + offset0[1];
    texpixel_rgf_t *sample2 = texture_pointer2 + offset0[2];
    texpixel_rgf_t *sample3 = texture_pointer3 + offset0[3];

    __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check0_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    sample0 = texture_pointer0 + offset1[0];
    sample1 = texture_pointer1 + offset1[1];
    sample2 = texture_pointer2 + offset1[2];
    sample3 = texture_pointer3 + offset1[3];

    sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check1_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    sample0 = texture_pointer0 + offset2[0];
    sample1 = texture_pointer1 + offset2[1];
    sample2 = texture_pointer2 + offset2[2];
    sample3 = texture_pointer3 + offset2[3];

    sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check2_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    sample0 = texture_pointer0 + offset3[0];
    sample1 = texture_pointer1 + offset3[1];
    sample2 = texture_pointer2 + offset3[2];
    sample3 = texture_pointer3 + offset3[3];

    sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
    __m128 check3_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

    // ----------------------------------------------------------------------------------
    // -- Final interpolated output.
    // ----------------------------------------------------------------------------------

    sample_4x = _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(check0_4x, _mm_mul_ps(_mm_sub_ps(check1_4x, check0_4x), s_4x)), _mm_sub_ps(_mm_set1_ps(1.0f), t_4x)),
                    _mm_mul_ps(_mm_add_ps(check2_4x, _mm_mul_ps(_mm_sub_ps(check3_4x, check2_4x), s_4x)), t_4x));

    _mm_store_ps(output, sample_4x);
}

void sse_shadow_map_point_sample_pcf(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_point_sample_pcf");

    u32 map_pitch = shadow_map->faces[0]->mips->header->width;
    u32 map_size = map_pitch - 1;

    fv3_t current_direction;
    fv2_t texcoord;

    u32 face_index[GC_FRAG_SIZE];
    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];
    A4SET(face_index, 0, 0, 0, 0);

    u32 resolution = 5;

    __m128 inv_4x = _mm_set1_ps(1.0f / (resolution * 4));
    __m128 radius_4x = _mm_set1_ps(radius);
    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(map_pitch);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        output[i] = 0;

        __m128 base_dir_x = _mm_set1_ps(direction->x[i]);
        __m128 base_dir_y = _mm_set1_ps(direction->y[i]);
        __m128 base_dir_z = _mm_set1_ps(direction->z[i]);

        __m128 result_4x = _mm_setzero_ps();
        __m128 compare_4x = _mm_set1_ps(compare[i]);

        for (u8 s = 0; s < resolution; ++s)
        {
            __m128 current_direction_x = _mm_add_ps(base_dir_x, _mm_mul_ps(_mm_load_ps(sse_cube_direction_offset_x[s].data), radius_4x));
            __m128 current_direction_y = _mm_add_ps(base_dir_y, _mm_mul_ps(_mm_load_ps(sse_cube_direction_offset_y[s].data), radius_4x));
            __m128 current_direction_z = _mm_add_ps(base_dir_z, _mm_mul_ps(_mm_load_ps(sse_cube_direction_offset_z[s].data), radius_4x));

            _mm_store_ps(current_direction.x, current_direction_x);
            _mm_store_ps(current_direction.y, current_direction_y);
            _mm_store_ps(current_direction.z, current_direction_z);

            cube_uv_from_vec(&current_direction, &texcoord, face_index, false);
            _sse_tex_clamp(texcoord.x, texcoord.y, buffer0, buffer1);

            texture2d_t *face0 = shadow_map->faces[face_index[0]];
            texture2d_t *face1 = shadow_map->faces[face_index[1]];
            texture2d_t *face2 = shadow_map->faces[face_index[2]];
            texture2d_t *face3 = shadow_map->faces[face_index[3]];

            texpixel_rgf_t *texture_pointer0 = (texpixel_rgf_t *) face0->mips->data;
            texpixel_rgf_t *texture_pointer1 = (texpixel_rgf_t *) face1->mips->data;
            texpixel_rgf_t *texture_pointer2 = (texpixel_rgf_t *) face2->mips->data;
            texpixel_rgf_t *texture_pointer3 = (texpixel_rgf_t *) face3->mips->data;

            __m128 u_4x = _mm_load_ps(buffer0);
            __m128 v_4x = _mm_load_ps(buffer1);

            __m128 tex_x_4x = _mm_mul_ps(u_4x, map_size_4x);
            __m128 tex_y_4x = _mm_mul_ps(v_4x, map_size_4x);

            __m128i x_4x = _mm_cvttps_epi32(tex_x_4x);
            __m128i y_4x = _mm_cvttps_epi32(tex_y_4x);

            __m128i offset_4x = SSE_TEX_OFFSET(x_4x, y_4x, pitch_4x);

            texpixel_rgf_t *sample0 = texture_pointer0 + _mm_extract_epi32(offset_4x, 0);
            texpixel_rgf_t *sample1 = texture_pointer1 + _mm_extract_epi32(offset_4x, 1);
            texpixel_rgf_t *sample2 = texture_pointer2 + _mm_extract_epi32(offset_4x, 2);
            texpixel_rgf_t *sample3 = texture_pointer3 + _mm_extract_epi32(offset_4x, 3);

            __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            result_4x = _mm_add_ps(result_4x, _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f)));
        }

        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_mul_ps(result_4x, inv_4x);

        _mm_store_ps(buffer0, result_4x);
        output[i] = buffer0[0];
    }
}

void sse_shadow_map_point_sample_pcf_linear(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("sse_shadow_map_point_sample_pcf_linear");

    u32 map_pitch = shadow_map->faces[0]->mips->header->width;
    u32 map_size = map_pitch - 1;

    fv3_t current_direction;
    fv2_t texcoord;

    u32 face_index[GC_FRAG_SIZE];
    r32 buffer0[GC_FRAG_SIZE];
    r32 buffer1[GC_FRAG_SIZE];
    A4SET(face_index, 0, 0, 0, 0);

    u32 resolution = 5;

    __m128 inv_4x = _mm_set1_ps(1.0f / (resolution * 4));
    __m128 radius_4x = _mm_set1_ps(radius);
    __m128 map_size_4x = _mm_set1_ps(map_size);
    __m128i pitch_4x = _mm_set1_epi32(map_pitch);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        output[i] = 0;

        __m128 base_dir_x = _mm_set1_ps(direction->x[i]);
        __m128 base_dir_y = _mm_set1_ps(direction->y[i]);
        __m128 base_dir_z = _mm_set1_ps(direction->z[i]);

        __m128 result_4x = _mm_setzero_ps();
        __m128 compare_4x = _mm_set1_ps(compare[i]);

        for (u8 s = 0; s < resolution; ++s)
        {
            __m128 current_direction_x = _mm_add_ps(base_dir_x, _mm_mul_ps(_mm_load_ps(sse_cube_direction_offset_x[s].data), radius_4x));
            __m128 current_direction_y = _mm_add_ps(base_dir_y, _mm_mul_ps(_mm_load_ps(sse_cube_direction_offset_y[s].data), radius_4x));
            __m128 current_direction_z = _mm_add_ps(base_dir_z, _mm_mul_ps(_mm_load_ps(sse_cube_direction_offset_z[s].data), radius_4x));

            _mm_store_ps(current_direction.x, current_direction_x);
            _mm_store_ps(current_direction.y, current_direction_y);
            _mm_store_ps(current_direction.z, current_direction_z);

            cube_uv_from_vec(&current_direction, &texcoord, face_index, false);
            _sse_tex_clamp(texcoord.x, texcoord.y, buffer0, buffer1);

            texture2d_t *face0 = shadow_map->faces[face_index[0]];
            texture2d_t *face1 = shadow_map->faces[face_index[1]];
            texture2d_t *face2 = shadow_map->faces[face_index[2]];
            texture2d_t *face3 = shadow_map->faces[face_index[3]];

            texpixel_rgf_t *texture_pointer0 = (texpixel_rgf_t *) face0->mips->data;
            texpixel_rgf_t *texture_pointer1 = (texpixel_rgf_t *) face1->mips->data;
            texpixel_rgf_t *texture_pointer2 = (texpixel_rgf_t *) face2->mips->data;
            texpixel_rgf_t *texture_pointer3 = (texpixel_rgf_t *) face3->mips->data;

            __m128 u0_4x = _mm_load_ps(buffer0);
            __m128 v0_4x = _mm_load_ps(buffer1);

            __m128 u1_4x = _mm_add_ps(u0_4x, _mm_set1_ps(face0->mips->header->tex_du));
            __m128 v1_4x = _mm_add_ps(v0_4x, _mm_set1_ps(face0->mips->header->tex_dv));

            _mm_store_ps(buffer0, u1_4x);
            _mm_store_ps(buffer1, v1_4x);

            _sse_tex_clamp(buffer0, buffer1, buffer0, buffer1);

            u1_4x = _mm_load_ps(buffer0);
            v1_4x = _mm_load_ps(buffer1);

            // ----------------------------------------------------------------------------------
            // -- Linear interpolation.
            // ----------------------------------------------------------------------------------

            __m128 tex_x0_4x = _mm_mul_ps(u0_4x, map_size_4x);
            __m128 tex_y0_4x = _mm_mul_ps(v0_4x, map_size_4x);

            __m128 tex_x1_4x = _mm_mul_ps(u1_4x, map_size_4x);
            __m128 tex_y1_4x = _mm_mul_ps(v1_4x, map_size_4x);

            __m128 s_4x = _mm_sub_ps(tex_x0_4x, _mm_floor_ps(tex_x0_4x));
            __m128 t_4x = _mm_sub_ps(tex_y0_4x, _mm_floor_ps(tex_y0_4x));

            __m128i x0_4x = _mm_cvttps_epi32(tex_x0_4x);
            __m128i y0_4x = _mm_cvttps_epi32(tex_y0_4x);

            __m128i x1_4x = _mm_cvttps_epi32(tex_x1_4x);
            __m128i y1_4x = _mm_cvttps_epi32(tex_y1_4x);

            __m128i offset0_4x = SSE_TEX_OFFSET(x0_4x, y0_4x, pitch_4x);
            __m128i offset1_4x = SSE_TEX_OFFSET(x1_4x, y0_4x, pitch_4x);
            __m128i offset2_4x = SSE_TEX_OFFSET(x0_4x, y1_4x, pitch_4x);
            __m128i offset3_4x = SSE_TEX_OFFSET(x1_4x, y1_4x, pitch_4x);

            u32 offset0[GC_FRAG_SIZE];
            u32 offset1[GC_FRAG_SIZE];
            u32 offset2[GC_FRAG_SIZE];
            u32 offset3[GC_FRAG_SIZE];

            _mm_store_si128((__m128i *) offset0, offset0_4x);
            _mm_store_si128((__m128i *) offset1, offset1_4x);
            _mm_store_si128((__m128i *) offset2, offset2_4x);
            _mm_store_si128((__m128i *) offset3, offset3_4x);

            texpixel_rgf_t *sample0 = texture_pointer0 + offset0[0];
            texpixel_rgf_t *sample1 = texture_pointer1 + offset0[1];
            texpixel_rgf_t *sample2 = texture_pointer2 + offset0[2];
            texpixel_rgf_t *sample3 = texture_pointer3 + offset0[3];

            __m128 sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            __m128 check0_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

            sample0 = texture_pointer0 + offset1[0];
            sample1 = texture_pointer1 + offset1[1];
            sample2 = texture_pointer2 + offset1[2];
            sample3 = texture_pointer3 + offset1[3];

            sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            __m128 check1_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

            sample0 = texture_pointer0 + offset2[0];
            sample1 = texture_pointer1 + offset2[1];
            sample2 = texture_pointer2 + offset2[2];
            sample3 = texture_pointer3 + offset2[3];

            sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            __m128 check2_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

            sample0 = texture_pointer0 + offset3[0];
            sample1 = texture_pointer1 + offset3[1];
            sample2 = texture_pointer2 + offset3[2];
            sample3 = texture_pointer3 + offset3[3];

            sample_4x = _mm_setr_ps(sample0->r, sample1->r, sample2->r, sample3->r);
            __m128 check3_4x = _mm_and_ps(_mm_cmple_ps(compare_4x, sample_4x), _mm_set1_ps(1.0f));

            // ----------------------------------------------------------------------------------
            // -- Final interpolated output.
            // ----------------------------------------------------------------------------------

            sample_4x = _mm_add_ps(
                            _mm_mul_ps(_mm_add_ps(check0_4x, _mm_mul_ps(_mm_sub_ps(check1_4x, check0_4x), s_4x)), _mm_sub_ps(_mm_set1_ps(1.0f), t_4x)),
                            _mm_mul_ps(_mm_add_ps(check2_4x, _mm_mul_ps(_mm_sub_ps(check3_4x, check2_4x), s_4x)), t_4x));

            result_4x = _mm_add_ps(result_4x, sample_4x);
        }

        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_hadd_ps(result_4x, result_4x);
        result_4x = _mm_mul_ps(result_4x, inv_4x);

        _mm_store_ps(buffer0, result_4x);
        output[i] = buffer0[0];
    }
}