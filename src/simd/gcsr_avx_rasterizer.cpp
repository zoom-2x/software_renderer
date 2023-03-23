// ----------------------------------------------------------------------------------
// -- File: gcsr_rasterizer_avx.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-08-15 16:02:19
// -- Modified: 2021-08-15 16:02:21
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

// ----------------------------------------------------------------------------------
// -- sRGB to [0,1] conversion (SIMD).
// ----------------------------------------------------------------------------------

__INLINE__ void gl_sRGBtoLinear1_8x(u32 frag_count, gl_fragment_pixel_t *pixels, u32 *packed_color)
{
    DEBUG_RM_Start(__DebugApi__, 0);

    __m128i r_mask_4x = _mm_setr_epi32(0x80808003, 0x80808007, 0x8080800B, 0x8080800F);
    __m128i g_mask_4x = _mm_setr_epi32(0x80808002, 0x80808006, 0x8080800A, 0x8080800E);
    __m128i b_mask_4x = _mm_setr_epi32(0x80808001, 0x80808005, 0x80808009, 0x8080800D);
    __m128i a_mask_4x = _mm_setr_epi32(0x80808000, 0x80808004, 0x80808008, 0x8080800C);

    __m256 oneOver255_8x = _mm256_set1_ps(ONE_OVER_255);

    __ALIGN__ u32 tmp_r[8];
    __ALIGN__ u32 tmp_g[8];
    __ALIGN__ u32 tmp_b[8];
    __ALIGN__ u32 tmp_a[8];

    u32 offset = 0;

    for (u32 i = 0; i < frag_count; ++i)
    {
#if 1
        u32 *packed_addr = packed_color + offset;
        __m128i packedColorA_4x = _mm_load_si128((__m128i *) packed_addr);
        __m128i packedColorB_4x = _mm_load_si128((__m128i *) (packed_addr + 4));
#else
        __m256i packedColor_8x = _mm256_load_si256((__m256i *) (packed_color + offset));
        __m128i packedColorA_4x = _mm256_extractf128_si256(packedColor_8x, 0);
        __m128i packedColorB_4x = _mm256_extractf128_si256(packedColor_8x, 1);
#endif

        __m128i rA_4x = _mm_shuffle_epi8(packedColorA_4x, r_mask_4x);
        __m128i gA_4x = _mm_shuffle_epi8(packedColorA_4x, g_mask_4x);
        __m128i bA_4x = _mm_shuffle_epi8(packedColorA_4x, b_mask_4x);
        __m128i aA_4x = _mm_shuffle_epi8(packedColorA_4x, a_mask_4x);

        __m128i rB_4x = _mm_shuffle_epi8(packedColorB_4x, r_mask_4x);
        __m128i gB_4x = _mm_shuffle_epi8(packedColorB_4x, g_mask_4x);
        __m128i bB_4x = _mm_shuffle_epi8(packedColorB_4x, b_mask_4x);
        __m128i aB_4x = _mm_shuffle_epi8(packedColorB_4x, a_mask_4x);

#if 0
        _mm_store_si128((__m128i *) tmp_r, rA_4x);
        _mm_store_si128((__m128i *) tmp_g, gA_4x);
        _mm_store_si128((__m128i *) tmp_b, bA_4x);
        _mm_store_si128((__m128i *) tmp_a, aA_4x);

        _mm_store_si128((__m128i *) (tmp_r + 4), rB_4x);
        _mm_store_si128((__m128i *) (tmp_g + 4), gB_4x);
        _mm_store_si128((__m128i *) (tmp_b + 4), bB_4x);
        _mm_store_si128((__m128i *) (tmp_a + 4), aB_4x);

        __m256i ri_8x = _mm256_load_si256((__m256i *) tmp_r);
        __m256i gi_8x = _mm256_load_si256((__m256i *) tmp_g);
        __m256i bi_8x = _mm256_load_si256((__m256i *) tmp_b);
        __m256i ai_8x = _mm256_load_si256((__m256i *) tmp_a);
#else
        __m256i ri_8x = _mm256_setr_m128i(rA_4x, rB_4x);
        __m256i gi_8x = _mm256_setr_m128i(gA_4x, gB_4x);
        __m256i bi_8x = _mm256_setr_m128i(bA_4x, bB_4x);
        __m256i ai_8x = _mm256_setr_m128i(aA_4x, aB_4x);
#endif

        __m256 r_8x = _mm256_cvtepi32_ps(ri_8x);
        __m256 g_8x = _mm256_cvtepi32_ps(gi_8x);
        __m256 b_8x = _mm256_cvtepi32_ps(bi_8x);
        __m256 a_8x = _mm256_cvtepi32_ps(ai_8x);

        r_8x = _mm256_mul_ps(r_8x, oneOver255_8x);
        g_8x = _mm256_mul_ps(g_8x, oneOver255_8x);
        b_8x = _mm256_mul_ps(b_8x, oneOver255_8x);
        a_8x = _mm256_mul_ps(a_8x, oneOver255_8x);

        gl_fragment_pixel_t *Current = pixels + i;

        _mm256_store_ps(Current->colorr, r_8x);
        _mm256_store_ps(Current->colorg, g_8x);
        _mm256_store_ps(Current->colorb, b_8x);
        _mm256_store_ps(Current->colora, a_8x);

        offset += GC_FRAG_SIZE;
    }

    DEBUG_RM_End(__DebugApi__, 0);
}

// ----------------------------------------------------------------------------------
// -- [0,1] to sRGB conversion.
// ----------------------------------------------------------------------------------

__INLINE__ void gl_linear1tosRGB_8x(u32 frag_count, gl_fragment_pixel_t *pixels, u32 *output)
{
    DEBUG_RM_Start(__DebugApi__, 1);

    __m128i r_mask_4x = _mm_setr_epi32(0x00808080, 0x04808080, 0x08808080, 0x0C808080);
    __m128i g_mask_4x = _mm_setr_epi32(0x80008080, 0x80048080, 0x80088080, 0x800C8080);
    __m128i b_mask_4x = _mm_setr_epi32(0x80800080, 0x80800480, 0x80800880, 0x80800C80);
    __m128i a_mask_4x = _mm_setr_epi32(0x80808000, 0x80808004, 0x80808008, 0x8080800C);

    __m128 ff_4x = _mm_set1_ps(255.0f);
    __m256 ff_8x = _mm256_set1_ps(255.0f);
    u32 offset = 0;

    for (u32 i = 0; i < frag_count; ++i)
    {
        gl_fragment_pixel_t *CurrentPixelFragments = pixels + i;
#if 0
        __m256 r_8x = _mm256_load_ps(channelR + offset);
        __m256 g_8x = _mm256_load_ps(channelG + offset);
        __m256 b_8x = _mm256_load_ps(channelB + offset);
        __m256 a_8x = _mm256_load_ps(channelA + offset);

        __m256i ri_8x = _mm256_cvtps_epi32(_mm256_mul_ps(r_8x, ff_8x));
        __m256i gi_8x = _mm256_cvtps_epi32(_mm256_mul_ps(g_8x, ff_8x));
        __m256i bi_8x = _mm256_cvtps_epi32(_mm256_mul_ps(b_8x, ff_8x));
        __m256i ai_8x = _mm256_cvtps_epi32(_mm256_mul_ps(a_8x, ff_8x));

        __m128i rA_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(ri_8x, 0), r_mask_4x);
        __m128i rB_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(ri_8x, 1), r_mask_4x);
        __m128i gA_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(gi_8x, 0), g_mask_4x);
        __m128i gB_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(gi_8x, 1), g_mask_4x);
        __m128i bA_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(bi_8x, 0), b_mask_4x);
        __m128i bB_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(bi_8x, 1), b_mask_4x);
        __m128i aA_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(ai_8x, 0), a_mask_4x);
        __m128i aB_4x = _mm_shuffle_epi8(_mm256_extractf128_si256(ai_8x, 1), a_mask_4x);

        // __m128i rA_4x = _mm_set1_epi32(255);
        // __m128i rB_4x = _mm_set1_epi32(255);
        // __m128i gA_4x = _mm_set1_epi32(255);
        // __m128i gB_4x = _mm_set1_epi32(255);
        // __m128i bA_4x = _mm_set1_epi32(255);
        // __m128i bB_4x = _mm_set1_epi32(255);
        // __m128i aA_4x = _mm_set1_epi32(255);
        // __m128i aB_4x = _mm_set1_epi32(255);

        ri_8x = _mm256_setr_m128i(rA_4x, rB_4x);
        gi_8x = _mm256_setr_m128i(gA_4x, gB_4x);
        bi_8x = _mm256_setr_m128i(bA_4x, bB_4x);
        ai_8x = _mm256_setr_m128i(aA_4x, aB_4x);

        __m256i packed_8x = _mm256_castps_si256(_mm256_or_ps(
                                    _mm256_or_ps(_mm256_castsi256_ps(ri_8x), _mm256_castsi256_ps(gi_8x)),
                                    _mm256_or_ps(_mm256_castsi256_ps(bi_8x), _mm256_castsi256_ps(ai_8x))));

        _mm256_store_si256((__m256i *) (output + offset), packed_8x);
#else
        // -- First 4 pixels.

        __m128 r_4x = _mm_load_ps(CurrentPixelFragments->colorr);
        __m128 g_4x = _mm_load_ps(CurrentPixelFragments->colorg);
        __m128 b_4x = _mm_load_ps(CurrentPixelFragments->colorb);
        __m128 a_4x = _mm_load_ps(CurrentPixelFragments->colora);

        __m128i r255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(r_4x, ff_4x)), r_mask_4x);
        __m128i g255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(g_4x, ff_4x)), g_mask_4x);
        __m128i b255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(b_4x, ff_4x)), b_mask_4x);
        __m128i a255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(a_4x, ff_4x)), a_mask_4x);

        __m128i packed_4x = _mm_or_si128(
                                _mm_or_si128(r255_4x, g255_4x),
                                _mm_or_si128(b255_4x, a255_4x));

        _mm_store_si128((__m128i *) (output + offset), packed_4x);

        // -- Next 4 pixels.

        r_4x = _mm_load_ps(CurrentPixelFragments->colorr + 4);
        g_4x = _mm_load_ps(CurrentPixelFragments->colorg + 4);
        b_4x = _mm_load_ps(CurrentPixelFragments->colorb + 4);
        a_4x = _mm_load_ps(CurrentPixelFragments->colora + 4);

        r255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(r_4x, ff_4x)), r_mask_4x);
        g255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(g_4x, ff_4x)), g_mask_4x);
        b255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(b_4x, ff_4x)), b_mask_4x);
        a255_4x = _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(a_4x, ff_4x)), a_mask_4x);

        packed_4x = _mm_or_si128(
                        _mm_or_si128(r255_4x, g255_4x),
                        _mm_or_si128(b255_4x, a255_4x));

        _mm_store_si128((__m128i *) (output + offset + 4), packed_4x);
#endif

        offset += GC_FRAG_SIZE;
    }

    DEBUG_RM_End(__DebugApi__, 1);
}

// ----------------------------------------------------------------------------------
// -- Texture sampling routine.
// ----------------------------------------------------------------------------------

void gl_sample_8x(u32 frag_count, gl_fragment_varyings_t *varyings, u32 *output_8x,
                  AssetBitmapMemory *Texture, gc_constant_t filtering)
{
    DEBUG_RM_Start(__DebugApi__, 2);

    __m256 zero_8x = _mm256_setzero_ps();
    __m256 one_8x = _mm256_set1_ps(1.0f);
    __m128i width_4x = _mm_set1_epi32(Texture->BitmapInfo.width);
    // __m256i width_8x = _mm256_set1_epi32(Texture->BitmapInfo.width);
    __m256 widthps_8x = _mm256_set1_ps(Texture->BitmapInfo.width);
    __m256 height_8x = _mm256_set1_ps((r32) Texture->BitmapInfo.height);
    __m256 widthMinusOne_8x = _mm256_set1_ps((r32) Texture->BitmapInfo.width - 1);
    __m256 heightMinusOne_8x = _mm256_set1_ps((r32) Texture->BitmapInfo.height - 1);
    __m256 half_8x = _mm256_set1_ps(0.5f);

    u32 *memory = (u32 *) Texture->memory;

    for (u32 i = 0; i < frag_count; ++i)
    {
        gl_fragment_varyings_t *Current = varyings + i;

        __m256 tu_8x = _mm256_load_ps(Current->uv_u);
        __m256 tv_8x = _mm256_load_ps(Current->uv_v);

#if 1
        __m256 gezeromask_8x = _mm256_cmp_ps(tu_8x, zero_8x, _CMP_GE_OQ);
        tu_8x = _mm256_blendv_ps(zero_8x, tu_8x, gezeromask_8x);
        __m256 leonemask_8x = _mm256_cmp_ps(tu_8x, one_8x, _CMP_LE_OQ);
        tu_8x = _mm256_blendv_ps(one_8x, tu_8x, leonemask_8x);

        gezeromask_8x = _mm256_cmp_ps(tv_8x, zero_8x, _CMP_GE_OQ);
        tv_8x = _mm256_blendv_ps(zero_8x, tv_8x, gezeromask_8x);
        leonemask_8x = _mm256_cmp_ps(tv_8x, one_8x, _CMP_LE_OQ);
        tv_8x = _mm256_blendv_ps(one_8x, tv_8x, leonemask_8x);
#else
        tu_8x = _mm256_and_ps(_mm256_cmp_ps(tu_8x, zero_8x, _CMP_GE_OQ), tu_8x);
        __m256 umask_8x = _mm256_cmp_ps(tu_8x, one_8x, _CMP_GT_OQ);
        __m256 uonemask_8x = _mm256_and_ps(one_8x, umask_8x);
        tu_8x = _mm256_or_ps(_mm256_andnot_ps(umask_8x, tu_8x), uonemask_8x);

        tv_8x = _mm256_andnot_ps(_mm256_cmp_ps(tv_8x, zero_8x, _CMP_LT_OQ), tv_8x);
        __m256 vmask_8x = _mm256_cmp_ps(tv_8x, one_8x, _CMP_GT_OQ);
        __m256 vonemask_8x = _mm256_and_ps(one_8x, vmask_8x);
        tv_8x = _mm256_or_ps(_mm256_andnot_ps(vmask_8x, tv_8x), vonemask_8x);
#endif

        tv_8x = _mm256_sub_ps(one_8x, tv_8x);

        __m256 sampleCol_8x = _mm256_mul_ps(tu_8x, widthMinusOne_8x);
        __m256 sampleRow_8x = _mm256_mul_ps(tv_8x, heightMinusOne_8x);

        __m256i col_8x = _mm256_cvttps_epi32(_mm256_add_ps(sampleCol_8x, half_8x));
        __m256i row_8x = _mm256_cvttps_epi32(_mm256_add_ps(sampleRow_8x, half_8x));

        // __ALIGN__ u32 row[8];
        // __ALIGN__ u32 col[8];
        __ALIGN__ u32 indices[8];

        // _mm256_store_si256((__m256i *) row, row_8x);
        // _mm256_store_si256((__m256i *) col, col_8x);

        // __m128i rowA_4x = _mm_load_si128((__m128i *) row);
        // __m128i colA_4x = _mm_load_si128((__m128i *) col);
        // __m128i rowB_4x = _mm_load_si128((__m128i *) row + 1);
        // __m128i colB_4x = _mm_load_si128((__m128i *) col + 1);

        __m128i rowA_4x = _mm256_extractf128_si256(row_8x, 0);
        __m128i rowB_4x = _mm256_extractf128_si256(row_8x, 1);

#if 1
        __m128i A_4x = _mm_mullo_epi32(rowA_4x, width_4x);
        __m128i B_4x = _mm_mullo_epi32(rowB_4x, width_4x);
#else
        __m128i A_4x = _mm_or_si128(
                            _mm_slli_epi32(_mm_mulhi_epi16(rowA_4x, width_4x), 16),
                            _mm_mullo_epi16(rowA_4x, width_4x));
        __m128i B_4x = _mm_or_si128(
                            _mm_slli_epi32(_mm_mulhi_epi16(rowB_4x, width_4x), 16),
                            _mm_mullo_epi16(rowB_4x, width_4x));
#endif
        __m128i colA_4x = _mm256_extractf128_si256(col_8x, 0);
        __m128i colB_4x = _mm256_extractf128_si256(col_8x, 1);
        __m128i sampleIndexA_4x = _mm_add_epi32(A_4x, colA_4x);
        __m128i sampleIndexB_4x = _mm_add_epi32(B_4x, colB_4x);

        __m256i sampleIndex_8x = _mm256_setr_m128i(sampleIndexA_4x, sampleIndexB_4x);
        // __m256i sampleIndex_8x = _mm256_insertf128_si256(_mm256_setzero_si256(), sampleIndexA_4x, 0);
        // sampleIndex_8x = _mm256_insertf128_si256(sampleIndex_8x, sampleIndexB_4x, 1);

        _mm256_store_si256((__m256i *) indices, sampleIndex_8x);

        // _mm_store_si128((__m128i *) indices, sampleIndexA_4x);
        // _mm_store_si128((__m128i *) indices + 1, sampleIndexB_4x);

        output_8x[0] = memory[indices[0]];
        output_8x[1] = memory[indices[1]];
        output_8x[2] = memory[indices[2]];
        output_8x[3] = memory[indices[3]];
        output_8x[4] = memory[indices[4]];
        output_8x[5] = memory[indices[5]];
        output_8x[6] = memory[indices[6]];
        output_8x[7] = memory[indices[7]];

        output_8x += GC_FRAG_SIZE;
    }

    DEBUG_RM_End(__DebugApi__, 2);
}

// ----------------------------------------------------------------------------------
// -- point rasterization routine.
// ----------------------------------------------------------------------------------

#ifndef GCSR_ROUTINE_gl_rasterizePoint
#define GCSR_ROUTINE_gl_rasterizePoint

__INLINE__ void gl_rasterize_point(gc_primitive_t *primitive, gc_screen_rect_t box,
                              gl_frag_pack_cursor_t *PackCursor, gc_framebuffer_t *framebuffer)
{
    gl_fragpack_t *current_pack = gl_frag_pack_get(PackCursor);

    b8 is_z_test = (global_gl->Config.flags & GL_ZTEST) > 0;

    u8 masks[8] = {
        0b10000000,
        0b01000000,
        0b00100000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b00000010,
        0b00000001
    };

    u32 sx = box.min.x;
    u32 sy = box.min.y;
    u32 ex = box.max.x;
    u32 ey = box.max.y;

    u32 z_start_index = sy * framebuffer->depthbuffer->pitch + sx;

    gc_screen_rect_t sprite_box = primitive->point.sprite_box;
    gc_point_sprite_t *sprite = &global_gl->Rasterization->PointSprites[global_gl->Config.pointType];

    r32 z = primitive->v1->pos.z;
    __m256 z_8x = _mm256_set1_ps(primitive->v1->pos.z);

    __m128i sb_minx_4x = _mm_set1_epi32(sprite_box.min.x);
    __m128i sb_miny_4x = _mm_set1_epi32(sprite_box.min.y);
    __m128i sb_maxx_4x = _mm_set1_epi32(sprite_box.max.x);
    __m128i sb_maxy_4x = _mm_set1_epi32(sprite_box.max.y);

    __m128i sb_minx_One_4x = _mm_set1_epi32(sprite_box.min.x - 1);
    __m128i sb_miny_One_4x = _mm_set1_epi32(sprite_box.min.y - 1);
    __m128i sb_maxx_One_4x = _mm_set1_epi32(sprite_box.max.x + 1);
    __m128i sb_maxy_One_4x = _mm_set1_epi32(sprite_box.max.y + 1);

    __m128i one_4x = _mm_set1_epi32(1);
    __m128i seven_4x = _mm_set1_epi32(7);
    __m256 true_8x = _mm256_castsi256_ps(_mm256_set1_epi32(0xffffffff));
    __m128 false_4x = _mm_setzero_ps();
    __m128 true_4x = _mm_castsi128_ps(_mm_set1_epi32(0xffffffff));

    __m128i unmask_4x = _mm_setr_epi32(8, 4, 2, 1);

    for (u32 by = sy; by <= ey; ++by)
    {
        u32 current_z_index = z_start_index;
        __m128i y_4x = _mm_set1_epi32(by);

        for (u32 bx = sx; bx <= ex; bx += GC_FRAG_SIZE)
        {
            __m128i x1_4x = _mm_setr_epi32(bx, bx + 1, bx + 2, bx + 3);
            __m128i x2_4x = _mm_setr_epi32(bx + 4, bx + 5, bx + 6, bx + 7);

            __m128i t1_4x = _mm_and_si128(
                                _mm_cmpgt_epi32(x1_4x, sb_minx_One_4x),
                                _mm_cmplt_epi32(x1_4x, sb_maxx_One_4x));

            __m128i t2_4x = _mm_and_si128(
                                _mm_cmpgt_epi32(x2_4x, sb_minx_One_4x),
                                _mm_cmplt_epi32(x2_4x, sb_maxx_One_4x));

            __m128i t3_4x = _mm_and_si128(
                                _mm_cmpgt_epi32(y_4x, sb_miny_One_4x),
                                _mm_cmplt_epi32(y_4x, sb_maxy_One_4x));

            __m256i x_mask_8x = _mm256_setr_m128i(t1_4x, t2_4x);
            __m256i y_mask_8x = _mm256_setr_m128i(t3_4x, t3_4x);
            __m256i fragment_mask_8x = _mm256_castps_si256(_mm256_and_ps(
                                            _mm256_castsi256_ps(x_mask_8x),
                                            _mm256_castsi256_ps(y_mask_8x)));

            // __m128i fragment_mask_4x = _mm_and_si128(_mm_or_si128(t1_4x, t2_4x), t3_4x);
            u32 _mask = _mm256_movemask_ps(_mm256_castsi256_ps(fragment_mask_8x));

            if (_mask)
            {
                s32 boxRelX = bx - sprite_box.min.x;
                s32 boxRelY = by - sprite_box.min.y;
                u8 smask = sprite->mask[boxRelY];
                u8 currentMask = 0xff;
                u8 mask1 = 0;
                u8 mask2 = 0;

                if (boxRelX < 0)
                {
                    u32 offset = -1 * boxRelX;
                    currentMask = ((currentMask << offset) & smask) >> offset;
                }
                else
                {
                    u32 offset = boxRelX;
                    currentMask = ((currentMask >> offset) & smask) << offset;
                }

                mask1 = (0xf0 & currentMask) >> 4;
                mask2 = 0x0f & currentMask;

                __m128i mask1_4x = _mm_set1_epi32(mask1);
                __m128i mask2_4x = _mm_set1_epi32(mask2);

                __m128i mask1_check_4x = _mm_and_si128(mask1_4x, unmask_4x);
                __m128i mask2_check_4x = _mm_and_si128(mask2_4x, unmask_4x);

                __m128i active1_4x = _mm_cmpeq_epi32(mask1_check_4x, unmask_4x);
                __m128i active2_4x = _mm_cmpeq_epi32(mask2_check_4x, unmask_4x);
                __m256i active_8x = _mm256_setr_m128i(active1_4x, active2_4x);

                r32 *ba = (r32 *) framebuffer->depthbuffer->data + current_z_index;
                __m256 p_bz_8x = _mm256_load_ps(ba);

                __m256 z_check_8x;

                if (is_z_test)
                    z_check_8x = _mm256_cmp_ps(z_8x, p_bz_8x, _CMP_LE_OQ);
                else
                    z_check_8x = true_8x;

                fragment_mask_8x = _mm256_castps_si256(
                                        _mm256_and_ps(
                                            _mm256_castsi256_ps(fragment_mask_8x),
                                            _mm256_and_ps(z_check_8x, _mm256_castsi256_ps(active_8x))));
                u32 _check = _mm256_movemask_ps(_mm256_castsi256_ps(fragment_mask_8x));

                if (_check)
                {
                    gl_fragment_pixel_t *CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                    gl_fragment_varyings_t *CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;

                    for (u32 ii = 0; ii < GC_FRAG_SIZE; ++ii)
                    {
                        u32 maskcheck = Mi(fragment_mask_8x, ii);

                        if (maskcheck)
                        {
                            // -- Write the pixel to the fragpack.

                            u32 x = bx + ii;
                            u32 y = by;
                            u32 pixelIndex = current_pack->currentCount;

                            CurrentPixelFragments->pos_x[pixelIndex] = x;
                            CurrentPixelFragments->pos_y[pixelIndex] = y;
                            CurrentPixelFragments->offset[pixelIndex] = y * framebuffer->width + x;

                            CurrentPixelFragments->z[pixelIndex] = z;

                            current_pack->pixelCount++;
                            current_pack->currentCount++;

                            SDL_assert(current_pack->pixelCount <= GL_FRAGPACK_PIXELS);
                            SDL_assert(current_pack->frag_count <= GL_FRAGPACK_FRAGMENTS);

                            // -- The current fragment is full.

                            if (current_pack->currentCount == GC_FRAG_SIZE)
                            {
                                current_pack->frag_count++;
                                CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                                CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;

                                // Reset the count for the next fragment.
                                current_pack->currentCount = 0;
                            }

                            // -- Check if the end of the fragpack was reached.

                            if (current_pack->frag_count == GL_FRAGPACK_FRAGMENTS)
                            {
                                gl_frag_pack_close(PackCursor);
                                SDL_assert(current_pack->frag_count <= GL_FRAGPACK_FRAGMENTS);

                                current_pack = gl_frag_pack_get(PackCursor);
                                CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                                CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;
                            }
                        }
                    }
                }
            }

            current_z_index += GC_FRAG_SIZE;
        }

        z_start_index += framebuffer->depthbuffer->pitch;
    }
}
#endif

// ----------------------------------------------------------------------------------
// -- Line block rasterization routine.
// ----------------------------------------------------------------------------------

#ifndef GCSR_ROUTINE_gl_rasterize_line
#define GCSR_ROUTINE_gl_rasterize_line

__INLINE__ void gl_rasterize_line(gc_primitive_t *primitive, gc_screen_rect_t box,
                             gl_frag_pack_cursor_t *PackCursor, gc_framebuffer_t *framebuffer)
{
    gl_fragpack_t *current_pack = gl_frag_pack_get(PackCursor);

    u8 masks[8] = {
        0b10000000,
        0b01000000,
        0b00100000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b00000010,
        0b00000001
    };

    b8 is_z_test = (global_gl->Config.flags & GL_ZTEST) > 0;

    u32 sx = box.min.x;
    u32 sy = box.min.y;
    u32 ex = box.max.x;
    u32 ey = box.max.y;

    if (ey >= framebuffer->height)
        ey = framebuffer->height;

    u32 lineMinX = primitive->Line.box.min.x < 0 ? 0 : primitive->Line.box.min.x;
    u32 lineMinY = primitive->Line.box.min.y < 0 ? 0 : primitive->Line.box.min.y;
    u32 lineMaxX = primitive->Line.box.max.x < 0 ? 0 : primitive->Line.box.max.x;
    u32 lineMaxY = primitive->Line.box.max.y < 0 ? 0 : primitive->Line.box.max.y;

    gc_vertex_t *v1 = primitive->v1;
    gc_vertex_t *v2 = primitive->v2;

    vec4 v1Pos = v1->pos;
    vec4 v2Pos = v2->pos;

    u32 z_start_index = sy * framebuffer->depthbuffer->pitch + sx;

    __m256 true_8x = _mm256_castsi256_ps(_mm256_set1_epi32(0xffffffff));

    __m256 zero_8x = _mm256_setzero_ps();
    __m256 one_8x = _mm256_set1_ps(1.0f);
    __m256 half_8x = _mm256_set1_ps(0.5f);
    __m256i four_8x = _mm256_set1_epi32(4);

    __m128i width_4x = _mm_set1_epi32(framebuffer->width);

    __m128i lineMinX_4x = _mm_set1_epi32(lineMinX);
    __m128i lineMinY_4x = _mm_set1_epi32(lineMinY);
    __m128i lineMaxX_4x = _mm_set1_epi32(lineMaxX);
    __m128i lineMaxY_4x = _mm_set1_epi32(lineMaxY);

    __m128i lineMinX_m1_4x = _mm_set1_epi32(lineMinX - 1);
    __m128i lineMinY_m1_4x = _mm_set1_epi32(lineMinY - 1);
    __m128i lineMaxX_p1_4x = _mm_set1_epi32(lineMaxX + 1);
    __m128i lineMaxY_p1_4x = _mm_set1_epi32(lineMaxY + 1);

    __m256 v1_posz_8x = _mm256_set1_ps(v1Pos.z);
    __m256 v2_posz_8x = _mm256_set1_ps(v2Pos.z);

    __m256 fy_a_8x = _mm256_set1_ps(primitive->Line.fy_a);
    __m256 fy_b_8x = _mm256_set1_ps(primitive->Line.fy_b);
    __m256 fx_a_8x = _mm256_set1_ps(primitive->Line.fx_a);
    __m256 fx_b_8x = _mm256_set1_ps(primitive->Line.fx_b);

    __m256 line_ta_8x = _mm256_set1_ps(primitive->Line.t_a);
    __m256 line_tb_8x = _mm256_set1_ps(primitive->Line.t_b);

    for (u32 by = sy; by <= ey; ++by)
    {
        u32 current_z_index = z_start_index;
        __m128i y_4x = _mm_set1_epi32(by);
        __m256 y_8x = _mm256_set1_ps(by);

        for (u32 bx = sx; bx <= ex; bx += GC_FRAG_SIZE)
        {
            __m128i x1_4x = _mm_setr_epi32(bx, bx + 1, bx + 2, bx + 3);
            __m128i x2_4x = _mm_setr_epi32(bx + 4, bx + 5, bx + 6, bx + 7);

            __m256 x_8x = _mm256_setr_ps(bx, bx + 1, bx + 2, bx + 3, bx + 4, bx + 5, bx + 6, bx + 7);

            __m256i width_mask_8x = _mm256_setr_m128i(
                                        _mm_cmplt_epi32(x1_4x, width_4x),
                                        _mm_cmplt_epi32(x2_4x, width_4x));

            __m256i fragment_mask_8x = _mm256_setzero_si256();

            if (primitive->Line.isVertical)
            {
                fragment_mask_8x = _mm256_setr_m128i(
                                        _mm_cmpeq_epi32(x1_4x, lineMinX_4x),
                                        _mm_cmpeq_epi32(x2_4x, lineMinX_4x));
            }
            else if (primitive->Line.isHorizontal)
            {
                __m128i ycheck_4x = _mm_cmpeq_epi32(y_4x, lineMinY_4x);
                fragment_mask_8x = _mm256_setr_m128i(ycheck_4x, ycheck_4x);
            }
            else if (primitive->Line.isFy)
            {
                __m256 res_8x = _mm256_add_ps(_mm256_mul_ps(fy_a_8x, x_8x), fy_b_8x);
                __m256i ty_8x = _mm256_cvttps_epi32(_mm256_add_ps(res_8x, half_8x));

                __m128i ty1_4x = _mm256_extractf128_si256(ty_8x, 0);
                __m128i ty2_4x = _mm256_extractf128_si256(ty_8x, 1);

                __m256i ty_check_8x = _mm256_setr_m128i(
                                            _mm_cmpeq_epi32(ty1_4x, y_4x),
                                            _mm_cmpeq_epi32(ty2_4x, y_4x));

                fragment_mask_8x = _mm256_castps_si256(
                                        _mm256_and_ps(
                                            _mm256_cmp_ps(res_8x, zero_8x, _CMP_GT_OQ),
                                            _mm256_castsi256_ps(ty_check_8x)));
            }
            else
            {
                __m256 res_8x = _mm256_add_ps(_mm256_mul_ps(fx_a_8x, y_8x), fx_b_8x);
                __m256i tx_8x = _mm256_cvttps_epi32(_mm256_add_ps(res_8x, half_8x));

                __m128i tx1_4x = _mm256_extractf128_si256(tx_8x, 0);
                __m128i tx2_4x = _mm256_extractf128_si256(tx_8x, 1);

                __m256i tx_check_8x = _mm256_setr_m128i(
                                            _mm_cmpeq_epi32(tx1_4x, x1_4x),
                                            _mm_cmpeq_epi32(tx2_4x, x2_4x));

                fragment_mask_8x = _mm256_castps_si256(
                                        _mm256_and_ps(
                                            _mm256_cmp_ps(res_8x, zero_8x, _CMP_GT_OQ),
                                            _mm256_castsi256_ps(tx_check_8x)));
            }

            __m256 pt_8x;
            __m256 z_check_8x;

            if (primitive->Line.isTy)
                pt_8x = _mm256_add_ps(_mm256_mul_ps(line_ta_8x, y_8x), line_tb_8x);
            else
                pt_8x = _mm256_add_ps(_mm256_mul_ps(line_ta_8x, x_8x), line_tb_8x);

            r32 *ba = (r32 *) framebuffer->depthbuffer->data + current_z_index;
            __m256 depth_buffer_val_8x = _mm256_load_ps(ba);
            __m256 pz_8x = _mm256_add_ps(
                                _mm256_mul_ps(v1_posz_8x, _mm256_sub_ps(one_8x, pt_8x)),
                                _mm256_mul_ps(v2_posz_8x, pt_8x));

            if (is_z_test)
                z_check_8x = _mm256_cmp_ps(pz_8x, depth_buffer_val_8x, _CMP_LE_OQ);
            else
                z_check_8x = true_8x;

            u32 _zcheck = _mm256_movemask_ps(z_check_8x);

            if (_zcheck)
            {
                __m256i xcheck1_8x = _mm256_setr_m128i(
                                        _mm_cmpgt_epi32(x1_4x, lineMinX_m1_4x),
                                        _mm_cmpgt_epi32(x2_4x, lineMinX_m1_4x));

                __m256i xcheck2_8x = _mm256_setr_m128i(
                                        _mm_cmplt_epi32(x1_4x, lineMaxX_p1_4x),
                                        _mm_cmplt_epi32(x2_4x, lineMaxX_p1_4x));

                __m256i ycheck1_8x = _mm256_setr_m128i(
                                        _mm_cmpgt_epi32(y_4x, lineMinY_m1_4x),
                                        _mm_cmpgt_epi32(y_4x, lineMinY_m1_4x));

                __m256i ycheck2_8x = _mm256_setr_m128i(
                                        _mm_cmplt_epi32(y_4x, lineMaxY_p1_4x),
                                        _mm_cmplt_epi32(y_4x, lineMaxY_p1_4x));

                __m256 ct1_8x = _mm256_and_ps(
                                    _mm256_castsi256_ps(xcheck1_8x),
                                    _mm256_castsi256_ps(xcheck2_8x));

                __m256 ct2_8x = _mm256_and_ps(
                                    _mm256_castsi256_ps(ycheck1_8x),
                                    _mm256_castsi256_ps(ycheck2_8x));

                fragment_mask_8x = _mm256_castps_si256(_mm256_and_ps(
                                        _mm256_castsi256_ps(fragment_mask_8x),
                                        _mm256_and_ps(
                                            _mm256_and_ps(_mm256_and_ps(ct1_8x, ct2_8x), z_check_8x),
                                            _mm256_castsi256_ps(width_mask_8x))));

                // fragment_mask_8x = _mm256_shuffle_ps(fragment_mask_8x, 0b00011011);
                // fragment_mask_8x = _mm256_permute2f128_ps(fragment_mask_8x, fragment_mask_8x, 0b00000001);
                u32 _mask = _mm256_movemask_ps(_mm256_castsi256_ps(fragment_mask_8x));

                if (_mask)
                {
                    // NOTE(gabic): Attribute interpolation ?

                    gl_fragment_pixel_t *CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                    gl_fragment_varyings_t *CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;

                    for (u32 ii = 0; ii < GC_FRAG_SIZE; ++ii)
                    {
                        // u32 maskcheck = masks[ii] & Mi(fragment_mask_8x, ii);
                        u32 maskcheck = Mi(fragment_mask_8x, ii);

                        if (maskcheck)
                        {
                            // -- Write the pixel to the fragpack.

                            u32 x = bx + ii;
                            u32 y = by;
                            u32 pixelIndex = current_pack->currentCount;

                            CurrentPixelFragments->pos_x[pixelIndex] = x;
                            CurrentPixelFragments->pos_y[pixelIndex] = y;
                            CurrentPixelFragments->offset[pixelIndex] = y * framebuffer->width + x;

                            CurrentPixelFragments->z[pixelIndex] = M(pz_8x, ii);

                            current_pack->pixelCount++;
                            current_pack->currentCount++;

                            SDL_assert(current_pack->pixelCount <= GL_FRAGPACK_PIXELS);
                            SDL_assert(current_pack->frag_count <= GL_FRAGPACK_FRAGMENTS);

                            // -- The current fragment is full.

                            if (current_pack->currentCount == GC_FRAG_SIZE)
                            {
                                current_pack->frag_count++;
                                CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                                CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;

                                // Reset the count for the next fragment.
                                current_pack->currentCount = 0;
                            }

                            // -- Check if the end of the fragpack was reached.

                            if (current_pack->frag_count == GL_FRAGPACK_FRAGMENTS)
                            {
                                gl_frag_pack_close(PackCursor);
                                SDL_assert(current_pack->frag_count <= GL_FRAGPACK_FRAGMENTS);

                                current_pack = gl_frag_pack_get(PackCursor);
                                CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                                CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;
                            }
                        }
                    }
                }
            }

            current_z_index += GC_FRAG_SIZE;
        }

        z_start_index += framebuffer->depthbuffer->pitch;
    }
}

#endif

// ----------------------------------------------------------------------------------
// -- triangle block rasterization routine (AVX).
// ----------------------------------------------------------------------------------

#ifndef GCSR_ROUTINE_gl_rasterize_triangle
#define GCSR_ROUTINE_gl_rasterize_triangle

__INLINE__ void gl_rasterize_triangle(gc_primitive_t *primitive, u32 sx, u32 sy, u32 ex, u32 ey,
                                 gl_frag_pack_cursor_t *PackCursor, gc_framebuffer_t *framebuffer)
{
    DEBUG_RM_Start(__DebugApi__, 3);

    gl_fragpack_t *current_pack = gl_frag_pack_get(PackCursor);

    u8 masks[8] = {
        0b10000000,
        0b01000000,
        0b00100000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b00000010,
        0b00000001
    };

    b8 is_z_test = (global_gl->Config.flags & GL_ZTEST) > 0;
    u32 z_start_index = sy * framebuffer->depthbuffer->pitch + sx;

    if (ey >= framebuffer->height)
        ey = framebuffer->height;

    __m256 zero_8x = _mm256_setzero_ps();
    __m256 one_8x = _mm256_set1_ps(1.0f);
    __m256i four_8x = _mm256_set1_epi32(4);
    __m256 width_8x = _mm256_set1_ps(framebuffer->width);
    __m256 one_over_area_8x = _mm256_set1_ps(primitive->triangle.one_over_area);

    // ----------------------------------------------------------------------------------
    // -- primitive data.
    // ----------------------------------------------------------------------------------

    gc_vertex_t *v1 = primitive->v1;
    gc_vertex_t *v2 = primitive->v2;
    gc_vertex_t *v3 = primitive->v3;

    vec4 v1Pos = v1->pos;
    vec4 v2Pos = v2->pos;
    vec4 v3Pos = v3->pos;

    __m256 v1_z_8x = _mm256_set1_ps(v1Pos.z);
    __m256 v2_z_8x = _mm256_set1_ps(v2Pos.z);
    __m256 v3_z_8x = _mm256_set1_ps(v3Pos.z);

    __m256 v1w_8x = _mm256_set1_ps(v1Pos.w);
    __m256 v2w_8x = _mm256_set1_ps(v2Pos.w);
    __m256 v3w_8x = _mm256_set1_ps(v3Pos.w);

    __m256 v1_uv_u_8x = _mm256_set1_ps(v1->uv.u);
    __m256 v1_uv_v_8x = _mm256_set1_ps(v1->uv.v);

    __m256 v2_uv_u_8x = _mm256_set1_ps(v2->uv.u);
    __m256 v2_uv_v_8x = _mm256_set1_ps(v2->uv.v);

    __m256 v3_uv_u_8x = _mm256_set1_ps(v3->uv.u);
    __m256 v3_uv_v_8x = _mm256_set1_ps(v3->uv.v);

    __m256 tlc_x_8x = _mm256_cmp_ps(_mm256_set1_ps(primitive->triangle.tlc_x), zero_8x, _CMP_GT_OQ);
    __m256 tlc_y_8x = _mm256_cmp_ps(_mm256_set1_ps(primitive->triangle.tlc_y), zero_8x, _CMP_GT_OQ);
    __m256 tlc_z_8x = _mm256_cmp_ps(_mm256_set1_ps(primitive->triangle.tlc_z), zero_8x, _CMP_GT_OQ);

    __m256 mzero_epsilon_8x = _mm256_set1_ps(-GL_EPSILON_ZERO);
    __m256 zero_epsilon_8x = _mm256_set1_ps(GL_EPSILON_ZERO);

    __m256 l1a_8x = _mm256_set1_ps(v3Pos.x - v2Pos.x);
    __m256 l1b_8x = _mm256_set1_ps(v3Pos.y - v2Pos.y);

    __m256 l2a_8x = _mm256_set1_ps(v1Pos.x - v3Pos.x);
    __m256 l2b_8x = _mm256_set1_ps(v1Pos.y - v3Pos.y);

    __m256 v2y_8x = _mm256_set1_ps(v2Pos.y);
    __m256 v2x_8x = _mm256_set1_ps(v2Pos.x);

    __m256 v3y_8x = _mm256_set1_ps(v3Pos.y);
    __m256 v3x_8x = _mm256_set1_ps(v3Pos.x);

    __m256 triangle_one_over_area_8x = _mm256_set1_ps(primitive->triangle.one_over_area);

    for (u32 by = sy; by <= ey; ++by)
    {
        __m256 y_8x = _mm256_set1_ps(by);
        u32 current_z_index = z_start_index;

        for (u32 bx = sx; bx <= ex; bx += GC_FRAG_SIZE)
        {
            __m256 x_8x = _mm256_setr_ps(bx, bx + 1, bx + 2, bx + 3, bx + 4, bx + 5, bx + 6, bx + 7);
            __m256 pz_8x = one_8x;

            __m256 width_mask_8x = _mm256_cmp_ps(x_8x, width_8x, _CMP_LT_OQ);

            // -- Compute the barycentric coordinates.

            __m256 l1_8x = _mm256_mul_ps(
                                _mm256_sub_ps(
                                    _mm256_mul_ps(l1a_8x, _mm256_sub_ps(y_8x, v2y_8x)),
                                    _mm256_mul_ps(l1b_8x, _mm256_sub_ps(x_8x, v2x_8x))),
                                triangle_one_over_area_8x);

            __m256 l2_8x = _mm256_mul_ps(
                                _mm256_sub_ps(
                                    _mm256_mul_ps(l2a_8x, _mm256_sub_ps(y_8x, v3y_8x)),
                                    _mm256_mul_ps(l2b_8x, _mm256_sub_ps(x_8x, v3x_8x))),
                                triangle_one_over_area_8x);

            __m256 l3_8x = _mm256_sub_ps(one_8x, _mm256_add_ps(l1_8x, l2_8x));

            __m256 t1_8x = _mm256_mul_ps(l1_8x, v1_z_8x);
            __m256 t2_8x = _mm256_mul_ps(l2_8x, v2_z_8x);
            __m256 t3_8x = _mm256_mul_ps(l3_8x, v3_z_8x);

            __m256 z_8x = _mm256_add_ps(_mm256_add_ps(t1_8x, t2_8x), t3_8x);

            r32 *ba = (r32 *) framebuffer->depthbuffer->data + current_z_index;
            __m256 depth_buffer_val_8x = _mm256_load_ps(ba);
            __m256 z_check_8x;

            if (is_z_test)
                z_check_8x = _mm256_cmp_ps(z_8x, depth_buffer_val_8x, _CMP_LE_OQ);
            else
                z_check_8x = _mm256_cmp_ps(one_8x, one_8x, _CMP_EQ_OQ);
                // z_check_8x = _mm256_set1_ps(0xffffffff);

            __m256 p_check_x_8x = _mm256_and_ps(
                                    _mm256_cmp_ps(l1_8x, mzero_epsilon_8x, _CMP_GE_OQ),
                                    _mm256_or_ps(
                                        // _mm256_cmp_ps(l1_8x, mzero_epsilon_8x, _CMP_GT_OQ),
                                        _mm256_cmp_ps(l1_8x, zero_8x, _CMP_GT_OQ),
                                        tlc_x_8x));

            __m256 p_check_y_8x = _mm256_and_ps(
                                    _mm256_cmp_ps(l2_8x, mzero_epsilon_8x, _CMP_GE_OQ),
                                    _mm256_or_ps(
                                        // _mm256_cmp_ps(l2_8x, mzero_epsilon_8x, _CMP_GT_OQ),
                                        _mm256_cmp_ps(l2_8x, zero_8x, _CMP_GT_OQ),
                                        tlc_y_8x));

            __m256 p_check_z_8x = _mm256_and_ps(
                                    _mm256_cmp_ps(l3_8x, mzero_epsilon_8x, _CMP_GE_OQ),
                                    _mm256_or_ps(
                                        // _mm256_cmp_ps(l3_8x, mzero_epsilon_8x, _CMP_GT_OQ),
                                        _mm256_cmp_ps(l3_8x, zero_8x, _CMP_GT_OQ),
                                        tlc_z_8x));

            __m256 final_check_8x = _mm256_and_ps(
                                        _mm256_and_ps(
                                            _mm256_and_ps(p_check_x_8x, p_check_y_8x),
                                            p_check_z_8x),
                                        z_check_8x);

            __m256 fragment_mask_8x = _mm256_and_ps(final_check_8x, width_mask_8x);
            u32 _mask = _mm256_movemask_ps(fragment_mask_8x);

            if (_mask)
            {
                // ----------------------------------------------------------------------------------
                // -- Attribute interpolation.
                // ----------------------------------------------------------------------------------

                __m256 one_over_w_8x = _mm256_div_ps(
                                        one_8x,
                                        _mm256_add_ps(
                                            _mm256_add_ps(
                                                _mm256_mul_ps(l1_8x, v1w_8x),
                                                _mm256_mul_ps(l2_8x, v2w_8x)),
                                            _mm256_mul_ps(l3_8x, v3w_8x)));

                __m256 uv_u_8x = _mm256_mul_ps(
                                            one_over_w_8x,
                                            _mm256_add_ps(
                                                _mm256_add_ps(
                                                    _mm256_mul_ps(l1_8x, v1_uv_u_8x),
                                                    _mm256_mul_ps(l2_8x, v2_uv_u_8x)),
                                                _mm256_mul_ps(l3_8x, v3_uv_u_8x)));

                __m256 uv_v_8x = _mm256_mul_ps(
                                            one_over_w_8x,
                                            _mm256_add_ps(
                                                _mm256_add_ps(
                                                    _mm256_mul_ps(l1_8x, v1_uv_v_8x),
                                                    _mm256_mul_ps(l2_8x, v2_uv_v_8x)),
                                                _mm256_mul_ps(l3_8x, v3_uv_v_8x)));

                // ----------------------------------------------------------------------------------
                // -- Write to fragpack.
                // ----------------------------------------------------------------------------------

                gl_fragment_pixel_t *CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                gl_fragment_varyings_t *CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;

                for (u32 ii = 0; ii < GC_FRAG_SIZE; ++ii)
                {
                    u32 maskcheck = masks[ii] & Mi(fragment_mask_8x, ii);

                    if (maskcheck)
                    {
                        // -- Write the pixel to the fragpack.

                        u32 x = bx + ii;
                        u32 y = by;
                        u32 pixelIndex = current_pack->currentCount;

                        CurrentPixelFragments->pos_x[pixelIndex] = x;
                        CurrentPixelFragments->pos_y[pixelIndex] = y;
                        CurrentPixelFragments->offset[pixelIndex] = y * framebuffer->width + x;

                        CurrentPixelFragments->z[pixelIndex] = M(z_8x, ii);

                        CurrentAttributeFragments->l1[pixelIndex] = M(l1_8x, ii);
                        CurrentAttributeFragments->l2[pixelIndex] = M(l2_8x, ii);
                        CurrentAttributeFragments->l3[pixelIndex] = M(l3_8x, ii);

                        CurrentAttributeFragments->uv_u[pixelIndex] = M(uv_u_8x, ii);
                        CurrentAttributeFragments->uv_v[pixelIndex] = M(uv_v_8x, ii);

                        current_pack->pixelCount++;
                        current_pack->currentCount++;

                        SDL_assert(current_pack->pixelCount <= GL_FRAGPACK_PIXELS);
                        SDL_assert(current_pack->frag_count <= GL_FRAGPACK_FRAGMENTS);

                        // -- The current fragment is full.

                        if (current_pack->currentCount == GC_FRAG_SIZE)
                        {
                            current_pack->frag_count++;
                            CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                            CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;

                            // Reset the count for the next fragment.
                            current_pack->currentCount = 0;
                        }

                        // -- Check if the end of the fragpack was reached.

                        if (current_pack->frag_count == GL_FRAGPACK_FRAGMENTS)
                        {
                            gl_frag_pack_close(PackCursor);
                            SDL_assert(current_pack->frag_count <= GL_FRAGPACK_FRAGMENTS);

                            current_pack = gl_frag_pack_get(PackCursor);
                            CurrentPixelFragments = current_pack->pixels + current_pack->frag_count;
                            CurrentAttributeFragments = current_pack->varyings + current_pack->frag_count;
                        }
                    }
                }
            }

            current_z_index += GC_FRAG_SIZE;
        }

        z_start_index += framebuffer->depthbuffer->pitch;
    }

    DEBUG_RM_End(__DebugApi__, 3);
}
#endif

// ----------------------------------------------------------------------------------
// -- Computes the wireframe color for a specified single-pass-wireframe object.
// ----------------------------------------------------------------------------------

#ifndef GCSR_ROUTINE_gl_singlePassWireframeFilter
#define GCSR_ROUTINE_gl_singlePassWireframeFilter

__INLINE__ void gl_single_pass_wireframe_filter(gc_single_pass_wireframe_t *Wireframe,
                                         vec4 v1, vec4 v2, vec4 v3,
                                         r32 d1[4], r32 d2[4], r32 d3[4],
                                         u32 fragx[4], u32 fragy[4],
                                         r32 fragr[4], r32 fragg[4], r32 fragb[4], r32 fraga[4])
{
}

#endif