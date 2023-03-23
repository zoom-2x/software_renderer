// ----------------------------------------------------------------------------------
// -- File: gcsr_color_processing.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-05-07 13:50:08
// -- Modified: 2022-05-07 13:50:09
// ----------------------------------------------------------------------------------

#ifndef GCSR_COLOR_PROCESSING_H
#define GCSR_COLOR_PROCESSING_H

// global_variable __m128 one_4x = _mm_set1_ps(1.0f);
// global_variable __m128 minus_one_4x = _mm_set1_ps(-1.0f);
// global_variable __m128 two_4x = _mm_set1_ps(2.0f);
// global_variable __m128 zero_4x = _mm_setzero_ps();
// global_variable __m128 f255_4x = _mm_set1_ps(255.0f);
// global_variable __m128i zeroi_4x = _mm_setzero_si128();
// global_variable __m128i onei_4x = _mm_set1_epi32(1);
// global_variable __m128 half_4x = _mm_set1_ps(0.5f);
// global_variable __m128 one_over_255_4x = _mm_div_ps(one_4x, f255_4x);

// global_variable __m128i gr_mask_4x = _mm_setr_epi32(0x80808003, 0x80808007, 0x8080800B, 0x8080800F);
// global_variable __m128i gg_mask_4x = _mm_setr_epi32(0x80808002, 0x80808006, 0x8080800A, 0x8080800E);
// global_variable __m128i gb_mask_4x = _mm_setr_epi32(0x80808001, 0x80808005, 0x80808009, 0x8080800D);
// global_variable __m128i ga_mask_4x = _mm_setr_epi32(0x80808000, 0x80808004, 0x80808008, 0x8080800C);

// __m128i pack_r_mask_4x = _mm_setr_epi32(0x00808080, 0x04808080, 0x08808080, 0x0C808080);
// __m128i pack_g_mask_4x = _mm_setr_epi32(0x80008080, 0x80048080, 0x80088080, 0x800C8080);
// __m128i pack_b_mask_4x = _mm_setr_epi32(0x80800080, 0x80800480, 0x80800880, 0x80800C80);
// __m128i pack_a_mask_4x = _mm_setr_epi32(0x80808000, 0x80808004, 0x80808008, 0x8080800C);

// __m128i unpack_r_mask_4x = _mm_setr_epi32(0x80808003, 0x80808007, 0x8080800B, 0x8080800F);
// __m128i unpack_g_mask_4x = _mm_setr_epi32(0x80808002, 0x80808006, 0x8080800A, 0x8080800E);
// __m128i unpack_b_mask_4x = _mm_setr_epi32(0x80808001, 0x80808005, 0x80808009, 0x8080800D);
// __m128i unpack_a_mask_4x = _mm_setr_epi32(0x80808000, 0x80808004, 0x80808008, 0x8080800C);

// ----------------------------------------------------------------------------------
// -- Vector math.
// ----------------------------------------------------------------------------------

__INLINE__ __m128 sse_v3_len(sse_v3_t *vec)
{
    __m128 len_4x = _mm_sqrt_ps(
                        _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(vec->x, vec->x),
                                _mm_mul_ps(vec->y, vec->y)),
                            _mm_mul_ps(vec->z, vec->z)));

    return len_4x;
}

__INLINE__ void sse_v3_normalize(sse_v3_t *vec)
{
    // __m128 len_4x = _mm_rcp_ps(_mm_sqrt_ps(
    //                     _mm_add_ps(
    //                         _mm_add_ps(
    //                             _mm_mul_ps(vec->x, vec->x),
    //                             _mm_mul_ps(vec->y, vec->y)),
    //                         _mm_mul_ps(vec->z, vec->z))));

    __m128 len_4x = _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(
                        _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(vec->x, vec->x),
                                _mm_mul_ps(vec->y, vec->y)),
                            _mm_mul_ps(vec->z, vec->z))));

    vec->x = _mm_mul_ps(vec->x, len_4x);
    vec->y = _mm_mul_ps(vec->y, len_4x);
    vec->z = _mm_mul_ps(vec->z, len_4x);
}

__INLINE__ __m128 sse_v3_dot(sse_v3_t *v1, sse_v3_t *v2)
{
    __m128 dot = _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(v1->x, v2->x),
                        _mm_mul_ps(v1->y, v2->y)),
                    _mm_mul_ps(v1->z, v2->z));
    return dot;
}

__INLINE__ void sse_v3_muls(sse_v3_t *vec, __m128 s, sse_v3_t *out)
{
    out->x = _mm_mul_ps(vec->x, s);
    out->y = _mm_mul_ps(vec->y, s);
    out->z = _mm_mul_ps(vec->z, s);
}

__INLINE__ void sse_v3_add(sse_v3_t *v1, sse_v3_t *v2, sse_v3_t *out)
{
    out->x = _mm_add_ps(v1->x, v2->x);
    out->y = _mm_add_ps(v1->y, v2->y);
    out->z = _mm_add_ps(v1->z, v2->z);
}

__INLINE__ void sse_v3_sub(sse_v3_t *v1, sse_v3_t *v2, sse_v3_t *out)
{
    out->x = _mm_sub_ps(v1->x, v2->x);
    out->y = _mm_sub_ps(v1->y, v2->y);
    out->z = _mm_sub_ps(v1->z, v2->z);
}

__INLINE__ void sse_v3_cross(sse_v3_t *v1, sse_v3_t *v2, sse_v3_t *out)
{
    out->x = _mm_sub_ps(
                _mm_mul_ps(v1->y, v2->z),
                _mm_mul_ps(v1->z, v2->y));

    out->y = _mm_sub_ps(
                _mm_mul_ps(v1->z, v2->x),
                _mm_mul_ps(v1->x, v2->z));

    out->z = _mm_sub_ps(
                _mm_mul_ps(v1->x, v2->y),
                _mm_mul_ps(v1->y, v2->x));
}

__INLINE__ void sse_v3_inverse(sse_v3_t *vec)
{
    __m128 t = _mm_set1_ps(-1.0f);

    vec->x = _mm_mul_ps(vec->x, t);
    vec->y = _mm_mul_ps(vec->y, t);
    vec->z = _mm_mul_ps(vec->z, t);
}

__INLINE__ void sse_v3_inverse_to(sse_v3_t *vec, sse_v3_t *out)
{
    __m128 t = _mm_set1_ps(-1.0f);

    out->x = _mm_mul_ps(vec->x, t);
    out->y = _mm_mul_ps(vec->y, t);
    out->z = _mm_mul_ps(vec->z, t);
}

__INLINE__ void sse_v3_mix(sse_v3_t *v1, sse_v3_t *v2, r32 t, sse_v3_t *out)
{
    __m128 t4x = _mm_set1_ps(t);

    out->x = _mm_add_ps(v1->x, _mm_mul_ps(_mm_sub_ps(v2->x, v1->x), t4x));
    out->y = _mm_add_ps(v1->y, _mm_mul_ps(_mm_sub_ps(v2->y, v1->y), t4x));
    out->z = _mm_add_ps(v1->z, _mm_mul_ps(_mm_sub_ps(v2->z, v1->z), t4x));
}

__INLINE__ void sse_v3_mix_4(sse_v3_t *v1, sse_v3_t *v2, __m128 *t, sse_v3_t *out)
{
    out->x = _mm_add_ps(v1->x, _mm_mul_ps(_mm_sub_ps(v2->x, v1->x), *t));
    out->y = _mm_add_ps(v1->y, _mm_mul_ps(_mm_sub_ps(v2->y, v1->y), *t));
    out->z = _mm_add_ps(v1->z, _mm_mul_ps(_mm_sub_ps(v2->z, v1->z), *t));
}

__INLINE__ void sse_extract_rgbaf(r32 *s0, r32 *s1, r32 *s2, r32 *s3, sse_color_t *output)
{
    OPTICK_EVENT("sse_extract_rgbaf");

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

#define sse_clamp(val, min, max) _mm_max_ps(_mm_min_ps(val, _mm_set1_ps(max)), _mm_set1_ps(min))
#define sse_wrap(val) _mm_max_ps(_mm_sub_ps(val, _mm_floor_ps(val)), _mm_setzero_ps())
#define SSE_LINEAR_INTERP(r0, r1, r2, r3, s, t) _mm_add_ps(_mm_mul_ps(_mm_add_ps(r0, _mm_mul_ps(_mm_sub_ps(r1, r0), s)), _mm_sub_ps(_mm_set1_ps(1.0f), t)), _mm_mul_ps(_mm_add_ps(r2, _mm_mul_ps(_mm_sub_ps(r3, r2), s)), t))
#define SSE_ABS_PS(val) _mm_and_ps(_mm_castsi128_ps(_mm_srli_epi32(_mm_set1_epi32(-1), 1)), val)
#define SSE_CPYGE_PS(mask, t1, t2) _mm_or_ps(_mm_and_ps(mask, t1), _mm_andnot_ps(mask, t2))

// ----------------------------------------------------------------------------------
// -- Color processing.
// ----------------------------------------------------------------------------------

__INLINE__ __m128i simd_pack_color(sse_color_t *simd_color)
{
    __m128 f255_4x = _mm_set1_ps(255.0f);
    __m128i pack_r_mask_4x = _mm_setr_epi32(0x00808080, 0x04808080, 0x08808080, 0x0C808080);
    __m128i pack_g_mask_4x = _mm_setr_epi32(0x80008080, 0x80048080, 0x80088080, 0x800C8080);
    __m128i pack_b_mask_4x = _mm_setr_epi32(0x80800080, 0x80800480, 0x80800880, 0x80800C80);
    __m128i pack_a_mask_4x = _mm_setr_epi32(0x80808000, 0x80808004, 0x80808008, 0x8080800C);

    __m128i packed = _mm_or_si128(
                        _mm_or_si128(
                            _mm_shuffle_epi8(
                                _mm_cvttps_epi32(_mm_mul_ps(simd_color->r, f255_4x)), pack_r_mask_4x),
                                _mm_shuffle_epi8(
                                    _mm_cvttps_epi32(_mm_mul_ps(simd_color->g, f255_4x)), pack_g_mask_4x)),
                                    _mm_or_si128(
                                        _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(simd_color->b, f255_4x)), pack_b_mask_4x),
                                        _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(simd_color->a, f255_4x)), pack_a_mask_4x)));

    return packed;
}

__INLINE__ void sse_gamma_srgb_to_linear(sse_color_t *simd_color)
{
    simd_color->r = _mm_mul_ps(simd_color->r, simd_color->r);
    simd_color->g = _mm_mul_ps(simd_color->g, simd_color->g);
    simd_color->b = _mm_mul_ps(simd_color->b, simd_color->b);
}

__INLINE__ void sse_gamma_linear_to_srgb(sse_color_t *simd_color)
{
    simd_color->r = _mm_sqrt_ps(simd_color->r);
    simd_color->g = _mm_sqrt_ps(simd_color->g);
    simd_color->b = _mm_sqrt_ps(simd_color->b);
}

#define simd_tone_mapping_clamp(frag_r, frag_g, frag_b) \
{ \
    frag_r = _mm_min_ps(one_4x, _mm_max_ps(zero_4x, frag_r)); \
    frag_g = _mm_min_ps(one_4x, _mm_max_ps(zero_4x, frag_g)); \
    frag_b = _mm_min_ps(one_4x, _mm_max_ps(zero_4x, frag_b)); \
}

#define simd_tone_mapping_reinhard(frag_r, frag_g, frag_b) \
{ \
    frag_r = _mm_div_ps(frag_r, _mm_add_ps(one_4x, frag_r)); \
    frag_g = _mm_div_ps(frag_g, _mm_add_ps(one_4x, frag_g)); \
    frag_b = _mm_div_ps(frag_b, _mm_add_ps(one_4x, frag_b)); \
}

#define simd_tone_mapping_filmic(frag_r, frag_g, frag_b) \
{ \
    frag_r = FILMIC_4x(frag_r); \
    frag_g = FILMIC_4x(frag_g); \
    frag_b = FILMIC_4x(frag_b); \
}

// ----------------------------------------------------------------------------------

#endif