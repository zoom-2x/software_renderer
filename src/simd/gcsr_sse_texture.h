// ----------------------------------------------------------------------------------
// -- File: gcsr_sse_texture.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-07 09:53:40
// -- Modified: 2022-10-07 09:53:41
// ----------------------------------------------------------------------------------

#ifndef GCSR_SSE_TEXTURE_H
#define GCSR_SSE_TEXTURE_H

#define SSE_TEX_OFFSET(x, y, pitch) _mm_add_epi32(_mm_mullo_epi32(y, pitch), x)

#if 0
__INLINE__ void sse_unpack_color(__m128i packed_4x, sse_color_t *simd_color)
{
    simd_color->r = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(packed_4x, unpack_r_mask_4x)), one_over_255_4x); \
    simd_color->g = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(packed_4x, unpack_g_mask_4x)), one_over_255_4x); \
    simd_color->b = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(packed_4x, unpack_b_mask_4x)), one_over_255_4x); \
    simd_color->a = _mm_mul_ps(_mm_cvtepi32_ps(_mm_shuffle_epi8(packed_4x, unpack_a_mask_4x)), one_over_255_4x); \
}
#else
__INLINE__ void sse_unpack_color(__m128i packed_4x, sse_color_t *simd_color)
{
    // NOTE(gabic): Needed to shift into "green position" to avoid a signed conversion to float.
    simd_color->r = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(_mm_and_si128(packed_4x, _mm_set1_epi32(GC_PIXEL_FORMAT_RMASK)), 8)), _mm_set1_ps(GC_PIXEL_FORMAT_OVERG)); \
    simd_color->g = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(packed_4x, _mm_set1_epi32(GC_PIXEL_FORMAT_GMASK))), _mm_set1_ps(GC_PIXEL_FORMAT_OVERG)); \
    simd_color->b = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(packed_4x, _mm_set1_epi32(GC_PIXEL_FORMAT_BMASK))), _mm_set1_ps(GC_PIXEL_FORMAT_OVERB)); \
    simd_color->a = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(packed_4x, _mm_set1_epi32(GC_PIXEL_FORMAT_AMASK))), _mm_set1_ps(GC_PIXEL_FORMAT_OVERA)); \
}
#endif

#define SSE_GAMMA_SRGB_TO_LINEAR(color) \
{ \
    __m128 _r_4x = _mm_load_ps((r32 *) color.r); \
    __m128 _g_4x = _mm_load_ps((r32 *) color.g); \
    __m128 _b_4x = _mm_load_ps((r32 *) color.b); \
    _r_4x = _mm_mul_ps(_r_4x, _r_4x); \
    _g_4x = _mm_mul_ps(_g_4x, _g_4x); \
    _b_4x = _mm_mul_ps(_b_4x, _b_4x); \
    _mm_store_ps((r32 *) color.r, _r_4x); \
    _mm_store_ps((r32 *) color.g, _g_4x); \
    _mm_store_ps((r32 *) color.b, _b_4x); \
}

#define SSE_GAMMA_LINEAR_TO_SRGB(sse_color) \
{ \
    sse_color.r = _mm_sqrt_ps(sse_color.r); \
    sse_color.g = _mm_sqrt_ps(sse_color.g); \
    sse_color.b = _mm_sqrt_ps(sse_color.b); \
}

#define SSE_PACK_CONST() \
__m128i pack_r_mask_4x = _mm_setr_epi32(0x00808080, 0x04808080, 0x08808080, 0x0C808080); \
__m128i pack_g_mask_4x = _mm_setr_epi32(0x80008080, 0x80048080, 0x80088080, 0x800C8080); \
__m128i pack_b_mask_4x = _mm_setr_epi32(0x80800080, 0x80800480, 0x80800880, 0x80800C80); \
__m128i pack_a_mask_4x = _mm_setr_epi32(0x80808000, 0x80808004, 0x80808008, 0x8080800C)

#define SSE_PACK_COLOR(sse_color) \
    _mm_or_si128( \
        _mm_or_si128( \
            _mm_shuffle_epi8( \
                _mm_cvttps_epi32(_mm_mul_ps(sse_color.r, _mm_set1_ps(255.0f))), pack_r_mask_4x), \
                _mm_shuffle_epi8( \
                    _mm_cvttps_epi32(_mm_mul_ps(sse_color.g, _mm_set1_ps(255.0f))), pack_g_mask_4x)), \
                    _mm_or_si128( \
                        _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(sse_color.b, _mm_set1_ps(255.0f))), pack_b_mask_4x), \
                        _mm_shuffle_epi8(_mm_cvttps_epi32(_mm_mul_ps(sse_color.a, _mm_set1_ps(255.0f))), pack_a_mask_4x)))


#endif