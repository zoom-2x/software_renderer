// ----------------------------------------------------------------------------------
// -- File: gcsr_texture.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-07 09:48:46
// -- Modified: 2022-11-16 19:15:52
// ----------------------------------------------------------------------------------

#ifndef GCSR_TEXTURE_H
#define GCSR_TEXTURE_H

kernel_3x3_t weights_3x3 = {
    0.5f, 1.0f, 0.5f,
    1.0f, 1.0f, 1.0f,
    0.5f, 1.0f, 0.5f
};

kernel_5x5_t weights_5x5 = {
    0.0f, 0.5f, 1.0f, 0.5f, 0.0f,
    0.5f, 1.0f, 1.0f, 1.0f, 0.5f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    0.5f, 1.0f, 1.0f, 1.0f, 0.5f,
    0.0f, 0.5f, 1.0f, 0.5f, 0.0f
};

gc_vec_t cube_direction_offset[20] = {
    {1, 1, 1}, {1, -1, 1}, {-1, -1, 1}, {-1, 1, 1},
    {1, 1, -1}, {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1},
    {1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {-1, 1, 0},
    {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
    {0, 1, 1}, {0, -1, 1}, {0, -1, -1}, {0, 1, -1}
};

__ALIGN__ gc_vec_t sse_cube_direction_offset_x[5] = {
    {1, 1, -1, -1},
    {1, 1, -1, -1},
    {1, 1, -1, -1},
    {1, -1, 1, -1},
    {0, 0, 0, 0}
};

__ALIGN__ gc_vec_t sse_cube_direction_offset_y[5] = {
    {1, -1, -1, 1},
    {1, -1, -1, 1},
    {1, -1, -1, 1},
    {0, 0, 0, 0},
    {1, -1, -1, 1}
};

__ALIGN__ gc_vec_t sse_cube_direction_offset_z[5] = {
    {1, 1, 1, 1},
    {-1, -1, -1, -1},
    {0, 0, 0, 0},
    {1, 1, -1, -1},
    {1, 1, -1, -1}
};

__ALIGN__ gc_vec_t sse_stu3x3_offset[3] = {
    {-1, 0, 1, -1},
    {0, 1, -1, 0},
    {-1, 0, 0, 0}
};

__ALIGN__ gc_vec_t sse_stv3x3_offset[3] = {
    {-1, -1, -1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 0}
};

__ALIGN__ gc_vec_t sse_stu5x5_offset[7] = {
    {-2, -1, 0, 1},
    {2, -2, -1, 0},
    {1, 2, -2, -1},
    {0, 1, 2, -2},
    {-1, 0, 1, 2},
    {-2, -1, 0, 1},
    {2, 0, 0, 0},
};

__ALIGN__ gc_vec_t sse_stv5x5_offset[7] = {
    {-2, -2, -2, -2},
    {-2, -1, -1, -1},
    {-1, -1, 0, 0},
    {0, 0, 0, 1},
    {1, 1, 1, 1},
    {2, 2, 2, 2},
    {2, 0, 0, 0},
};

// ----------------------------------------------------------------------------------
// -- Macros.
// ----------------------------------------------------------------------------------

#define TEX_OFFSET(name, width, x, y) u32 name = y * (width) + x
#define FRAG_COLOR(frag, index) {frag->r[index], frag->g[index], frag->b[index], frag->a[index]}

#define gl_pack(color) \
        (((u32) (color.c.r * 255) << GL_PIXEL_FORMAT_RED_SHIFT) | \
         ((u32) (color.c.g * 255) << GL_PIXEL_FORMAT_GREEN_SHIFT) | \
         ((u32) (color.c.b * 255) << GL_PIXEL_FORMAT_BLUE_SHIFT) | \
         ((u32) (color.c.a * 255) << GL_PIXEL_FORMAT_ALPHA_SHIFT))

// #define gl_unpack(packed_color) \
// { \
//     ((packed_color >> GL_PIXEL_FORMAT_RED_SHIFT) & 0xFF) * ONE_OVER_255, \
//     ((packed_color >> GL_PIXEL_FORMAT_GREEN_SHIFT) & 0xFF) * ONE_OVER_255, \
//     ((packed_color >> GL_PIXEL_FORMAT_BLUE_SHIFT) & 0xFF) * ONE_OVER_255, \
//     ((packed_color >> GL_PIXEL_FORMAT_ALPHA_SHIFT) & 0xFF) * ONE_OVER_255 \
// }

#define gl_unpack(dest, packed_color) \
    (dest).r = ((packed_color) & GC_PIXEL_FORMAT_RMASK) * GC_PIXEL_FORMAT_OVERR; \
    (dest).g = ((packed_color) & GC_PIXEL_FORMAT_GMASK) * GC_PIXEL_FORMAT_OVERG; \
    (dest).b = ((packed_color) & GC_PIXEL_FORMAT_BMASK) * GC_PIXEL_FORMAT_OVERB; \
    (dest).a = ((packed_color) & GC_PIXEL_FORMAT_AMASK) * GC_PIXEL_FORMAT_OVERA

#define gl_pack_p4(color) \
    (((u32) (color.r * 255) << GL_PIXEL_FORMAT_RED_SHIFT) | \
     ((u32) (color.g * 255) << GL_PIXEL_FORMAT_GREEN_SHIFT) | \
     ((u32) (color.b * 255) << GL_PIXEL_FORMAT_BLUE_SHIFT) | \
     ((u32) (color.a * 255) << GL_PIXEL_FORMAT_ALPHA_SHIFT))

#define PRE_MULT_ALPHA(color) \
    color.c.r *= color.c.a; \
    color.c.g *= color.c.a; \
    color.c.b *= color.c.a

// ----------------------------------------------------------------------------------
// -- Gamma correction.
// ----------------------------------------------------------------------------------

#define gl_gamma_srgb_to_linear(color) \
{ \
    (color)->c.r *= (color)->c.r; \
    (color)->c.g *= (color)->c.g; \
    (color)->c.b *= (color)->c.b; \
}

#define gl_gamma_linear_to_srgb(color) \
{ \
    (color)->c.r = sqrt((color)->c.r); \
    (color)->c.g = sqrt((color)->c.g); \
    (color)->c.b = sqrt((color)->c.b); \
}
#define gl_gamma_srgb_to_linear_texpixel(pixel) \
{ \
    (pixel)->r *= (pixel)->r; \
    (pixel)->g *= (pixel)->g; \
    (pixel)->b *= (pixel)->b; \
}

#define gl_gamma_linear_to_srgb_texpixel(pixel) \
{ \
    (pixel)->r = sqrt((pixel)->r); \
    (pixel)->g = sqrt((pixel)->g); \
    (pixel)->b = sqrt((pixel)->b); \
}
#define gc_gamma_srgb_to_linear_frag(color) \
{ \
    (color)->r[0] *= (color)->r[0]; \
    (color)->g[0] *= (color)->g[0]; \
    (color)->b[0] *= (color)->b[0]; \
    (color)->r[1] *= (color)->r[1]; \
    (color)->g[1] *= (color)->g[1]; \
    (color)->b[1] *= (color)->b[1]; \
    (color)->r[2] *= (color)->r[2]; \
    (color)->g[2] *= (color)->g[2]; \
    (color)->b[2] *= (color)->b[2]; \
    (color)->r[3] *= (color)->r[3]; \
    (color)->g[3] *= (color)->g[3]; \
    (color)->b[3] *= (color)->b[3]; \
}

// ----------------------------------------------------------------------------------
// -- Filmic tonemapping (https://gdcvault.com/play/1012351/Uncharted-2-HDR).
// ----------------------------------------------------------------------------------

// -- Defaults.

// #define FILMIC_SHOULDER_STRENGTH 0.22f
// #define FILMIC_LINEAR_STRENGTH 0.30f
// #define FILMIC_LINEAR_ANGLE 0.10f
// #define FILMIC_TOE_STRENGTH 0.20f
// #define FILMIC_TOE_NUMERATOR 0.01f
// #define FILMIC_TOE_DENOMINATOR 0.30f
// #define FILMIC_TOE_ANGLE FILMIC_TOE_NUMERATOR / FILMIC_TOE_DENOMINATOR
// #define FILMIC_LINEAR_WHITE_POINT_VALUE 11.2f

#define FILMIC_SHOULDER_STRENGTH 0.38f
#define FILMIC_LINEAR_STRENGTH 0.18f
#define FILMIC_LINEAR_ANGLE 0.15f
#define FILMIC_TOE_STRENGTH 0.25f
#define FILMIC_TOE_NUMERATOR 0.01f
#define FILMIC_TOE_DENOMINATOR 0.30f
#define FILMIC_TOE_ANGLE FILMIC_TOE_NUMERATOR / FILMIC_TOE_DENOMINATOR
#define FILMIC_LINEAR_WHITE_POINT_VALUE 200.0f

#define _FILMIC(x) ((((x) * (FILMIC_SHOULDER_STRENGTH * (x) + FILMIC_LINEAR_ANGLE * FILMIC_LINEAR_STRENGTH) + FILMIC_TOE_STRENGTH * FILMIC_TOE_NUMERATOR) / ((x) * ((x) * FILMIC_SHOULDER_STRENGTH + FILMIC_LINEAR_STRENGTH) + FILMIC_TOE_STRENGTH * FILMIC_TOE_DENOMINATOR)) - FILMIC_TOE_ANGLE)
#define FILMIC(x) _FILMIC(x) / _FILMIC(FILMIC_LINEAR_WHITE_POINT_VALUE)

__INLINE__ __m128 FILMIC_4x(__m128 x_4x)
{
    __m128 FILMIC_SHOULDER_STRENGTH_4x = _mm_set1_ps(FILMIC_SHOULDER_STRENGTH);
    __m128 T1_4x = _mm_set1_ps(FILMIC_LINEAR_ANGLE * FILMIC_LINEAR_STRENGTH);
    __m128 T2_4x = _mm_set1_ps(FILMIC_TOE_STRENGTH * FILMIC_TOE_NUMERATOR);
    __m128 T3_4x = _mm_set1_ps(FILMIC_TOE_STRENGTH * FILMIC_TOE_DENOMINATOR);
    __m128 FILMIC_LINEAR_STRENGTH_4x = _mm_set1_ps(FILMIC_LINEAR_STRENGTH);
    __m128 FILMIC_TOE_ANGLE_4x = _mm_set1_ps(FILMIC_TOE_ANGLE);
    __m128 T4_4x = _mm_set1_ps(_FILMIC(FILMIC_LINEAR_WHITE_POINT_VALUE));

    __m128 tx_4x = _mm_mul_ps(x_4x, FILMIC_SHOULDER_STRENGTH_4x);

    tx_4x = _mm_sub_ps(
                _mm_mul_ps(
                    _mm_add_ps(_mm_mul_ps(x_4x, _mm_add_ps(tx_4x, T1_4x)), T2_4x),
                    _mm_rcp_ps(_mm_add_ps(_mm_mul_ps(x_4x, _mm_add_ps(tx_4x, FILMIC_LINEAR_STRENGTH_4x)), T3_4x))),
                FILMIC_TOE_ANGLE_4x);

    tx_4x = _mm_mul_ps(tx_4x, _mm_rcp_ps(T4_4x));

    return tx_4x;
}

// ----------------------------------------------------------------------------------
// -- ACES aproximation (https://64.github.io/tonemapping/#aces).
// ----------------------------------------------------------------------------------

// #define ACESA_brightness 0.6f
// #define ACESA_a 2.51f
// #define ACESA_b 0.03f
// #define ACESA_c 2.43f
// #define ACESA_d 0.59f
// #define ACESA_e 0.14f

#define ACESA_brightness 0.8f
#define ACESA_a 2.51f
#define ACESA_b 0.03f
#define ACESA_c 2.43f
#define ACESA_d 0.89f
#define ACESA_e 0.14f

__INLINE__ r32 ACES_APROX(r32 comp)
{
    comp *= ACESA_brightness;
    comp = (comp * (ACESA_a * comp + ACESA_b)) / (comp * (ACESA_c * comp + ACESA_d) + ACESA_e);
    comp = clamp(0, 1, comp);

    return comp;
}

__INLINE__ __m128 ACES_APROX_4x(__m128 comp)
{
    __m128 osix = _mm_set1_ps(ACESA_brightness);
    __m128 a = _mm_set1_ps(ACESA_a);
    __m128 b = _mm_set1_ps(ACESA_b);
    __m128 c = _mm_set1_ps(ACESA_c);
    __m128 d = _mm_set1_ps(ACESA_d);
    __m128 e = _mm_set1_ps(ACESA_e);

    comp = _mm_mul_ps(comp, osix);

    __m128 rt1 = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(comp, a), b), comp);
    __m128 rt2 = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(comp, c), d), comp), e);
    comp = _mm_mul_ps(rt1, _mm_rcp_ps(rt2));
    comp = _mm_max_ps(comp, _mm_setzero_ps());
    comp = _mm_min_ps(comp, _mm_set1_ps(1.0f));

    return comp;
}

void texture_sample(texture2d_t *texture, r32 *in_u, r32 *in_v, lod_t *lod, shader_color_t *output);
void cube_uv_from_vec(fv3_t *v, fv2_t *texcoord, u32 *face_index, b8 warp);

#endif