// ---------------------------------------------------------------------------------
// -- File: gcsr_math.h
// ---------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created:
// -- Modified: 2021-12-28 22:15:55
// ---------------------------------------------------------------------------------

#ifndef GCSR_ENGINE_MATH_H
#define GCSR_ENGINE_MATH_H

#define MATH_USE_FUNCTION_MACROS 1
#define ONE_OVER_255 0.00392156862745098039f
#define _ONE_OVER_NINE 0.111111111111f
#define MAX_INT16 65536

#define M(a, i) *((r32 *) &a + i)
#define Mi(a, i) *((u32 *) &a + i)
#define Ms(a, i) *((s32 *) &a + i)
#define mmSquare(a) _mm_mul_ps(a, a)

// #define Matrix(name, size) r32 name[size][size];

#define MATRIX_SIZE_2 4
#define MATRIX_SIZE_3 9
#define MATRIX_SIZE_4 16
#define PI 3.14159265358979323846f
#define TWO_PI 6.28318530717958647693f
#define ONE_OVER_PI 0.31830988618379067154f
#define ONE_OVER_2PI 0.15915494309189533577f
#define GL_EPSILON 0.0000001f
#define GL_ONE_OVER_EPSILON 10000000.0f
#define GL_EPSILON_ZERO 1.0e-5
#define GL_EPSILON_BARY_MINUS -0.0001f
#define GL_EPSILON_BARY_PLUS 0.0001f
#define PI_OVER_ONE80 0.01745329251994329577
#define ONE80_OVER_PI 57.2957795130823208768
#define ONE_OVER_8 0.125
#define HALF_PI 1.570796326f

#define SCALEDEF 3
#define USE_FP_RASTERIZATION 1

#define LERP(a, b, t) (t * a + (1.0f - t) * b)
#define RAD2DEG(angle) ((r32) (angle * ONE80_OVER_PI))
#define DEG2RAD(angle) ((r32) (angle * PI_OVER_ONE80))

#define FAST_FLOOR16(v) (s32) ((v) + MAX_INT16) - MAX_INT16
#define FAST_CEIL16(v) MAX_INT16 - (s32) (MAX_INT16 - (v))

typedef struct
{
    u8 int_bits;
    u8 dec_bits;
    u16 dec_mask;
    u16 dec_val;
    s32 offsetx;
    s32 offsety;
    r32 one_over_dec_val;
} fixed_point_setup_t;

// All these configurations will generate 32 bit signed values.
// 2p + 2 bits required for the edge equation result.
#if SCALEDEF == 1
// [-4096,+4095] guard band
#define FP_INT_BIT 13
#define FP_DEC_BIT 4
#define FP_DEC_MASK 0b1111
#define FP_DEC_MASK_PLUS 0b111111
#define FP_INT_VAL 8092
#define FP_DEC_VAL 16
#define FP_ZERO_POINT_FIVE 0b1000
#define FP_ONE_OVER_DEC_VAL 0.0625f
#define FP_ONE_OVER_2DEC_VAL 0.03125f
#define FP_GUARD_BAND_MIN -4096
#define FP_GUARD_BAND_MAX 4095
#define FP_HALF_ONE 8
#elif SCALEDEF == 2
// [-2048,+2047] guard band
#define FP_INT_BIT 12
#define FP_DEC_BIT 6
#define FP_DEC_MASK 0b111111
#define FP_DEC_MASK_PLUS 0b11111111
#define FP_INT_VAL 4096
#define FP_DEC_VAL 64
#define FP_ZERO_POINT_FIVE 0b100000
#define FP_ONE_OVER_DEC_VAL 0.015625f
#define FP_ONE_OVER_2DEC_VAL 0.0078125f
#define FP_GUARD_BAND_MIN -2048
#define FP_GUARD_BAND_MAX 2047
#define FP_HALF_ONE 32
#elif SCALEDEF == 3
// [-1024,+1023] guard band
#define FP_INT_BIT 11
#define FP_DEC_BIT 8
#define FP_DEC_MASK 0b11111111
#define FP_DEC_MASK_PLUS 0b1111111111
#define FP_INT_VAL 2048
#define FP_DEC_VAL 256
#define FP_ZERO_POINT_FIVE 0b10000000
#define FP_ONE_OVER_DEC_VAL 0.00390625f
#define FP_ONE_OVER_2DEC_VAL 0.0000152587890625f
#define FP_GUARD_BAND_MIN -1024
#define FP_GUARD_BAND_MAX 1023
#define FP_HALF_ONE 128
#endif

#define FP_CONST_MIN FP_GUARD_BAND_MIN * FP_INT_VAL
#define FP_CONST_MAX FP_GUARD_BAND_MAX * FP_INT_VAL
#define FP_ONE_OVER_256 0.00390625f
#define FP_ONE_OVER_65536 0.0000152587890625
// #define FP_ONE_OVER_DEC_VAL (1.0f / FP_DEC_VAL)

#define FP_FROM_REAL(val) floorR32ToS32((val * FP_DEC_VAL) + 0.5f)
#define FP_FROM_REAL8(val) floorR32ToS32((val * 256) + 0.5f)
// #define FP_FROM_REAL(val) ((s32) round((val * FP_DEC_VAL)))
#define FP_FROM_INT(val) (val << FP_DEC_BIT)
#define FP_TO_REAL(val) ((r32) val * FP_ONE_OVER_DEC_VAL)
// #define FP_MULTIPLY(v1, v2) ((v1 >> FP_DEC_BIT) * (v2 >> FP_DEC_BIT))
#define FP_MULTIPLY(v1, v2) ((v1 * v2) >> FP_DEC_BIT)

#define FP_FROM_REAL2(val) floorR32ToS32(val * 256)
#define FP_TO_REAL2(val) floorR32ToS32(val * 0.00390625f)

/*
    vector -> mat2
    0 1
    2 3

    vector -> mat3
    0 1 2
    3 4 5
    6 7 8

    vector -> mat4
     0  1  2  3
     4  5  6  7
     8  9 10 11
    12 13 14 15
*/

__INLINE__ s32 floorR32ToS32(r32 value)
{
    s32 result = (s32) floorf(value);
    return result;
}

__INLINE__ u32 floorR32ToU32(r32 value)
{
    u32 result = (u32) floorf(value);
    return result;
}

__INLINE__ vec4 sRGB_linear1(vec4 Color)
{
    vec4 Result;

    Result.r = Color.r * ONE_OVER_255;
    Result.g = Color.g * ONE_OVER_255;
    Result.b = Color.b * ONE_OVER_255;
    Result.a = Color.a * ONE_OVER_255;

    return Result;
}

__INLINE__ vec4 linear1_sRGB255(vec4 Color)
{
    vec4 Result;

    Result.r = Color.r * 255;
    Result.g = Color.g * 255;
    Result.b = Color.b * 255;
    Result.a = Color.a * 255;

    return Result;
}

__INLINE__ r32 clamp(r32 min, r32 max, r32 value)
{
    r32 result = value;

    if (result < min)
        result = min;

    if (result > max)
        result = max;

    return result;
}

__INLINE__ r32 wrap(r32 min, r32 max, r32 value)
{
    r32 result = value;

    if (result < min)
        result = max - (min - result);

    if (result > max)
        result = min + (result - max);

    return result;
}

__INLINE__ r32 clamp01(r32 value) {
    return clamp(0.0f, 1.0f, value);
}

__INLINE__ s32 clamp_s32(s32 min, s32 max, s32 value)
{
    s32 result = value;

    if (result < min)
        result = min;

    if (result > max)
        result = max;

    return result;
}

// ---------------------------------------------------------------------------------
// -- vec2 operations.
// ---------------------------------------------------------------------------------

__INLINE__ b8 vec2_equals(vec2 v1, vec2 v2) {
    return (v1.x == v2.x && v1.y == v2.y);
}

__INLINE__ vec2 vec2_copy(vec2 src)
{
    vec2 res;

    res.x = src.x;
    res.y = src.y;

    return res;
}

__INLINE__ r32 vec2_dot(vec2 v1, vec2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

__INLINE__ r32 vec2_inner(vec2 v1, vec2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

__INLINE__ r32 vec2_len(vec2 v) {
    return sqrtf(vec2_inner(v, v));
}

__INLINE__ r32 vec2_len2(vec2 v) {
    return vec2_inner(v, v);
}

__INLINE__ vec2 vec2_add(vec2 v1, vec2 v2)
{
    vec2 res;

    res.x = v1.x + v2.x;
    res.y = v1.y + v2.y;

    return res;
}

__INLINE__ vec2 vec2_sub(vec2 v1, vec2 v2)
{
    vec2 res;

    res.x = v1.x - v2.x;
    res.y = v1.y - v2.y;

    return res;
}

__INLINE__ vec2i vec2i_sub(vec2i v1, vec2i v2)
{
    vec2i res;

    res.x = v1.x - v2.x;
    res.y = v1.y - v2.y;

    return res;
}

__INLINE__ vec2 vec2_muls(vec2 v, r32 s)
{
    vec2 res;

    res.x = v.x * s;
    res.y = v.y * s;

    return res;
}

__INLINE__ vec2 vec2_inv(vec2 v)
{
    vec2 res;

    res.x = v.x;
    res.y = -v.y;

    return res;
}

__INLINE__ vec2 vec2_perp(vec2 v)
{
    vec2 res;

    res.x = -v.y;
    res.y = v.x;

    return res;
}

__INLINE__ vec2 vec2_scale(vec2 v1, vec2 v2)
{
    vec2 res;

    res.x = v1.x * v2.x;
    res.y = v1.y * v2.y;

    return res;
}

__INLINE__ r32 vec2_area(vec2 v1, vec2 v2, vec2 v3) {
    return (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x);
}

__INLINE__ void vec2_swap(vec2 *v1, vec2 *v2)
{
    r32 t = v1->x;
    v1->x = v2->x;
    v2->x = t;

    t = v1->y;
    v1->y = v2->y;
    v2->y = t;
}

__INLINE__ vec2 vec2_normalize(vec2 v)
{
    r32 len = vec2_len(v);
    SDL_assert(len);
    r32 one_over_len = 1.0f / len;

    vec2 res;

    res.x = v.x * one_over_len;
    res.y = v.y * one_over_len;

    return res;
}

__INLINE__ vec2 vec2_rotate(vec2 v, r32 angle)
{
    r32 s = sinf(angle);
    r32 c = cosf(angle);

    vec2 res;

    res.x = v.x * c - v.y * s;
    res.y = v.x * s + v.y * c;

    return res;
}

// ---------------------------------------------------------------------------------
// -- vec3 operations.
// ---------------------------------------------------------------------------------

__INLINE__ b8 vec3_equals(vec3 v1, vec3 v2) {
    return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z);
}

__INLINE__ vec3 vec3_copy(vec3 v)
{
    vec3 res;

    res.x = v.x;
    res.y = v.y;
    res.z = v.z;

    return res;
}

__INLINE__ r32 vec3_inner(vec3 v1, vec3 v2) {
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

__INLINE__ r32 vec3_dot(vec3 v1, vec3 v2) {
    return vec3_inner(v1, v2);
}

__INLINE__ r32 vec3_len(vec3 v) {
    return sqrtf(vec3_inner(v, v));
}

__INLINE__ r32 vec3_len2(vec3 v) {
    return vec3_inner(v, v);
}

__INLINE__ vec3 vec3_add(vec3 v1, vec3 v2)
{
    vec3 res;

    res.x = v1.x + v2.x;
    res.y = v1.y + v2.y;
    res.z = v1.z + v2.z;

    return res;
}

__INLINE__ vec3 vec3_sub(vec3 v1, vec3 v2)
{
    vec3 res;

    res.x = v1.x - v2.x;
    res.y = v1.y - v2.y;
    res.z = v1.z - v2.z;

    return res;
}

__INLINE__ vec3 vec3_muls(vec3 v, r32 s)
{
    vec3 res;

    res.x = v.x * s;
    res.y = v.y * s;
    res.z = v.z * s;

    return res;
}

__INLINE__ vec3 vec3_inv(vec3 v)
{
    vec3 res;

    res.x = -v.x;
    res.y = -v.y;
    res.z = -v.z;

    return res;
}

__INLINE__ vec3 vec3_cross(vec3 v1, vec3 v2)
{
    vec3 res;

    res.x = v1.y * v2.z - v1.z * v2.y;
    res.y = v1.z * v2.x - v1.x * v2.z;
    res.z = v1.x * v2.y - v1.y * v2.x;

    return res;
}

__INLINE__ void vec3_swap(vec3 *v1, vec3 *v2)
{
    r32 t = v1->x;
    v1->x = v2->x;
    v2->x = t;

    t = v1->y;
    v1->y = v2->y;
    v2->y = t;

    t = v1->z;
    v1->z = v2->z;
    v2->z = t;
}

__INLINE__ vec3 vec3_normalize(vec3 v)
{
    r32 len = vec3_len(v);
    SDL_assert(len);
    r32 one_over_len = 1.0f / len;

    vec3 res;

    res.x = v.x * one_over_len;
    res.y = v.y * one_over_len;
    res.z = v.z * one_over_len;

    return res;
}

// ---------------------------------------------------------------------------------
// -- vec4 operations.
// ---------------------------------------------------------------------------------

__INLINE__ r32 vec4_inner(vec4 v1, vec4 v2) {
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w);
}

__INLINE__ r32 vec4_dot(vec4 v1, vec4 v2) {
    return vec4_inner(v1, v2);
}

__INLINE__ r32 vec4_len(vec4 v) {
    return (sqrtf(vec4_inner(v, v)));
}

__INLINE__ r32 vec4_len2(vec4 v) {
    return vec4_inner(v, v);
}

__INLINE__ b8 vec4_equals(vec4 v1, vec4 v2) {
    return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w);
}

__INLINE__ vec4 vec4_copy(vec4 v)
{
    vec4 res;

    res.x = v.x;
    res.y = v.y;
    res.z = v.z;
    res.w = v.w;

    return res;
}

__INLINE__ vec4 vec4_add(vec4 v1, vec4 v2)
{
    vec4 res;

    res.x = v1.x + v2.x;
    res.y = v1.y + v2.y;
    res.z = v1.z + v2.z;
    res.w = v1.w + v2.w;

    return res;
}

__INLINE__ vec4 vec4_sub(vec4 v1, vec4 v2)
{
    vec4 res;

    res.x = v1.x - v2.x;
    res.y = v1.y - v2.y;
    res.z = v1.z - v2.z;
    res.w = v1.w - v2.w;

    return res;
}

__INLINE__ vec4 vec4_muls(vec4 v, r32 s)
{
    vec4 res;

    res.x = v.x * s;
    res.y = v.y * s;
    res.z = v.z * s;
    res.w = v.w * s;

    return res;
}

__INLINE__ vec4 vec4_inv(vec4 v)
{
    vec4 res;

    res.x = -v.x;
    res.y = -v.y;
    res.z = -v.z;
    res.w = -v.w;

    return res;
}

__INLINE__ vec4 vec4_mulvec(vec4 v1, vec4 v2)
{
    vec4 res;

    res.r = v1.r * v2.r;
    res.g = v1.g * v2.g;
    res.b = v1.b * v2.b;
    res.a = v1.a * v2.a;

    return res;
}

__INLINE__ void vec4_swap(vec4 *v1, vec4 *v2)
{
    r32 t = v1->x;
    v1->x = v2->x;
    v2->x = t;

    t = v1->y;
    v1->y = v2->y;
    v2->y = t;

    t = v1->z;
    v1->z = v2->z;
    v2->z = t;

    t = v1->w;
    v1->w = v2->w;
    v2->w = t;
}

__INLINE__ vec4 vec4_normalize(vec4 v)
{
    r32 len = vec4_len(v);
    SDL_assert(len);
    r32 one_over_len = 1.0f / len;

    vec4 res;

    res.x = v.x * one_over_len;
    res.y = v.y * one_over_len;
    res.z = v.z * one_over_len;
    res.w = v.w * one_over_len;

    return res;
}

// ----------------------------------------------------------------------------------

// #ifdef MATH_USE_FUNCTION_MACROS

// #define vec3_to_vec4_p(v, w) {v->x, v->y, v->z, w}
// #define vec4_to_vec3_p(v) {v->x, v->y, v->z}
// #define vec3_to_vec2_p(v) {v->x, v->y}
// #define vec3_to_vec4(v, w) {v.x, v.y, v.z, w}
// #define vec4_to_vec3(v) {v.x, v.y, v.z}
// #define vec3_to_vec2(v) {v.x, v.y}

// #else

// __INLINE__ vec4 vec3_to_vec4(vec3 v, r32 w) {
//     return {v.x, v.y, v.z, w};
// }

// __INLINE__ vec3 vec4_to_vec3(vec4 v) {
//     return {v.x, v.y, v.z};
// }

// __INLINE__ vec2 vec3_to_vec2(vec3 v) {
//     return {v.x, v.y};
// }

// #endif

// ---------------------------------------------------------------------------------
// -- Matrix (we're using only square matrices).
// ---------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
// -- mat2 operations.
// ----------------------------------------------------------------------------------

__INLINE__ void mat2_empty(mat2 *mat)
{
    mat->m[0][0] = 0;
    mat->m[0][1] = 0;
    mat->m[1][0] = 0;
    mat->m[1][1] = 0;
}

__INLINE__ void mat2_identity(mat2 *mat)
{
    mat->m[0][0] = 1;
    mat->m[0][1] = 0;
    mat->m[0][1] = 0;
    mat->m[1][1] = 1;
}

__INLINE__ void mat2_copy(mat2 *dest, mat2 *src)
{
    dest->m[0][0] = src->m[0][0];
    dest->m[0][1] = src->m[0][1];
    dest->m[1][0] = src->m[1][0];
    dest->m[1][1] = src->m[1][1];
}

__INLINE__ r32 mat2_det(mat2 *mat) {
    return mat->m[0][0] * mat->m[1][1] - mat->m[0][1] * mat->m[1][0];
}

__INLINE__ void mat2_add(mat2 *m1, mat2 *m2, mat2 *out)
{
    out->m[0][0] = m1->m[0][0] + m2->m[0][0];
    out->m[0][1] = m1->m[0][1] + m2->m[0][1];
    out->m[1][0] = m1->m[1][0] + m2->m[1][0];
    out->m[1][1] = m1->m[1][1] + m2->m[1][1];
}

__INLINE__ void mat2_sub(mat2 *m1, mat2 *m2, mat2 *out)
{
    out->m[0][0] = m1->m[0][0] - m2->m[0][0];
    out->m[0][1] = m1->m[0][1] - m2->m[0][1];
    out->m[1][0] = m1->m[1][0] - m2->m[1][0];
    out->m[1][1] = m1->m[1][1] - m2->m[1][1];
}

__INLINE__ void mat2_mul(mat2 *m1, mat2 *m2, mat2 *out)
{
    out->m[0][0] = m1->m[0][0] * m2->m[0][0] + m1->m[0][1] * m2->m[1][0];
    out->m[0][1] = m1->m[0][0] * m2->m[0][1] + m1->m[0][1] * m2->m[1][1];
    out->m[1][0] = m1->m[1][0] * m2->m[0][0] + m1->m[1][1] * m2->m[1][0];
    out->m[1][1] = m1->m[1][0] * m2->m[0][1] + m1->m[1][1] * m2->m[1][1];
}

__INLINE__ void mat2_muls(mat2 *m, r32 s, mat2 *out)
{
    out->m[0][0] = m->m[0][0] * s;
    out->m[0][1] = m->m[0][1] * s;
    out->m[1][0] = m->m[1][0] * s;
    out->m[1][1] = m->m[1][1] * s;
}

__INLINE__ vec2 mat2_mulvec(mat2 *m, vec2 v)
{
    vec2 result;

    result.x = m->m[0][0] * v.x + m->m[0][1] * v.y;
    result.y = m->m[1][0] * v.x + m->m[1][1] * v.y;

    return result;
}

__INLINE__ void mat2_transpose(mat2 *m, mat2 *out)
{
    out->m[0][0] = m->m[0][0];
    out->m[0][1] = m->m[1][0];
    out->m[1][0] = m->m[0][1];
    out->m[1][1] = m->m[1][1];
}

__INLINE__ void mat2_inverse(mat2 *m, mat2 *out)
{
    r32 one_over_det = 1.0f / mat2_det(m);

    out->m[0][0] = one_over_det * m->m[1][1];
    out->m[0][1] = -1 * one_over_det * m->m[0][1];
    out->m[1][0] = -1 * one_over_det * m->m[1][0];
    out->m[1][1] = one_over_det * m->m[0][0];
}

// ----------------------------------------------------------------------------------
// -- mat3 operations.
// ----------------------------------------------------------------------------------

__INLINE__ void mat3_empty(mat3 *mat)
{
    mat->m[0][0] = 0;
    mat->m[0][1] = 0;
    mat->m[0][2] = 0;

    mat->m[1][0] = 0;
    mat->m[1][1] = 0;
    mat->m[1][2] = 0;

    mat->m[2][0] = 0;
    mat->m[2][1] = 0;
    mat->m[2][2] = 0;
}

__INLINE__ void mat3_identity(mat3 *mat)
{
    mat->m[0][0] = 1;
    mat->m[0][1] = 0;
    mat->m[0][2] = 0;

    mat->m[1][0] = 0;
    mat->m[1][1] = 1;
    mat->m[1][2] = 0;

    mat->m[2][0] = 0;
    mat->m[2][1] = 0;
    mat->m[2][2] = 1;
}

__INLINE__ void mat3_copy(mat3 *dest, mat3 *src)
{
    dest->m[0][0] = src->m[0][0];
    dest->m[0][1] = src->m[0][1];
    dest->m[0][2] = src->m[0][2];

    dest->m[1][0] = src->m[1][0];
    dest->m[1][1] = src->m[1][1];
    dest->m[1][2] = src->m[1][2];

    dest->m[2][0] = src->m[2][0];
    dest->m[2][1] = src->m[2][1];
    dest->m[2][2] = src->m[2][2];
}

__INLINE__ b32 mat3_equals(mat3 *m1, mat3 *m2)
{
    b32 result = 0;

    if (m1->m[0][0] == m2->m[0][0] &&
        m1->m[0][1] == m2->m[0][1] &&
        m1->m[0][2] == m2->m[0][2] &&
        m1->m[1][0] == m2->m[1][0] &&
        m1->m[1][1] == m2->m[1][1] &&
        m1->m[1][2] == m2->m[1][2] &&
        m1->m[2][0] == m2->m[2][0] &&
        m1->m[2][1] == m2->m[2][1] &&
        m1->m[2][2] == m2->m[2][2])
        result = 1;

    return result;
}

__INLINE__ void mat3_add(mat3 *m1, mat3 *m2, mat3 *out)
{
    out->m[0][0] = m1->m[0][0] + m2->m[0][0];
    out->m[0][1] = m1->m[0][1] + m2->m[0][1];
    out->m[0][2] = m1->m[0][2] + m2->m[0][2];

    out->m[1][0] = m1->m[1][0] + m2->m[1][0];
    out->m[1][1] = m1->m[1][1] + m2->m[1][1];
    out->m[1][2] = m1->m[1][2] + m2->m[1][2];

    out->m[2][0] = m1->m[2][0] + m2->m[2][0];
    out->m[2][1] = m1->m[2][1] + m2->m[2][1];
    out->m[2][2] = m1->m[2][2] + m2->m[2][2];
}

__INLINE__ void mat3_sub(mat3 *m1, mat3 *m2, mat3 *out)
{
    out->m[0][0] = m1->m[0][0] - m2->m[0][0];
    out->m[0][1] = m1->m[0][1] - m2->m[0][1];
    out->m[0][2] = m1->m[0][2] - m2->m[0][2];

    out->m[1][0] = m1->m[1][0] - m2->m[1][0];
    out->m[1][1] = m1->m[1][1] - m2->m[1][1];
    out->m[1][2] = m1->m[1][2] - m2->m[1][2];

    out->m[2][0] = m1->m[2][0] - m2->m[2][0];
    out->m[2][1] = m1->m[2][1] - m2->m[2][1];
    out->m[2][2] = m1->m[2][2] - m2->m[2][2];
}

__INLINE__ r32 mat3_det(mat3 *mat)
{
    r32 result = mat->m[0][0] * (mat->m[1][1] * mat->m[2][2] - mat->m[1][2] * mat->m[2][1]) -
                 mat->m[0][1] * (mat->m[1][0] * mat->m[2][2] - mat->m[1][2] * mat->m[2][0]) +
                 mat->m[0][2] * (mat->m[1][0] * mat->m[2][1] - mat->m[1][1] * mat->m[2][0]);

    return result;
}

__INLINE__ void mat3_muls(mat3 *mat, r32 s, mat3 *out)
{
    out->m[0][0] = mat->m[0][0] * s;
    out->m[0][1] = mat->m[0][1] * s;
    out->m[0][2] = mat->m[0][2] * s;

    out->m[1][0] = mat->m[1][0] * s;
    out->m[1][1] = mat->m[1][1] * s;
    out->m[1][2] = mat->m[1][2] * s;

    out->m[2][0] = mat->m[2][0] * s;
    out->m[2][1] = mat->m[2][1] * s;
    out->m[2][2] = mat->m[2][2] * s;
}

__INLINE__ vec3 mat3_mulvec(mat3 *mat, vec3 a)
{
    vec3 result;

    result.x = mat->m[0][0] * a.x + mat->m[0][1] * a.y + mat->m[0][2] * a.z;
    result.y = mat->m[1][0] * a.x + mat->m[1][1] * a.y + mat->m[1][2] * a.z;
    result.z = mat->m[2][0] * a.x + mat->m[2][1] * a.y + mat->m[2][2] * a.z;

    return result;
}

__INLINE__ void mat3_mul(mat3 *m1, mat3 *m2, mat3 *out)
{
    out->m[0][0] = m1->m[0][0] * m2->m[0][0] + m1->m[0][1] * m2->m[1][0] + m1->m[0][2] * m2->m[2][0];
    out->m[0][1] = m1->m[0][0] * m2->m[0][1] + m1->m[0][1] * m2->m[1][1] + m1->m[0][2] * m2->m[2][1];
    out->m[0][2] = m1->m[0][0] * m2->m[0][2] + m1->m[0][1] * m2->m[1][2] + m1->m[0][2] * m2->m[2][2];

    out->m[1][0] = m1->m[1][0] * m2->m[0][0] + m1->m[1][1] * m2->m[1][0] + m1->m[1][2] * m2->m[2][0];
    out->m[1][1] = m1->m[1][0] * m2->m[0][1] + m1->m[1][1] * m2->m[1][1] + m1->m[1][2] * m2->m[2][1];
    out->m[1][2] = m1->m[1][0] * m2->m[0][2] + m1->m[1][1] * m2->m[1][2] + m1->m[1][2] * m2->m[2][2];

    out->m[2][0] = m1->m[2][0] * m2->m[0][0] + m1->m[2][1] * m2->m[1][0] + m1->m[2][2] * m2->m[2][0];
    out->m[2][1] = m1->m[2][0] * m2->m[0][1] + m1->m[2][1] * m2->m[1][1] + m1->m[2][2] * m2->m[2][1];
    out->m[2][2] = m1->m[2][0] * m2->m[0][2] + m1->m[2][1] * m2->m[1][2] + m1->m[2][2] * m2->m[2][2];
}

__INLINE__ void mat3_transpose(mat3 *m, mat3 *out)
{
    out->m[0][0] = m->m[0][0];
    out->m[0][1] = m->m[1][0];
    out->m[0][2] = m->m[2][0];

    out->m[1][0] = m->m[0][1];
    out->m[1][1] = m->m[1][1];
    out->m[1][2] = m->m[2][1];

    out->m[2][0] = m->m[0][2];
    out->m[2][1] = m->m[1][2];
    out->m[2][2] = m->m[2][2];
}

__INLINE__ void mat3_inv(mat3 *mat, mat3 *inv)
{
    // Assume mat is invertible.
    r32 one_over_det = 1.0f / mat3_det(mat);

    inv->m[0][0] = (mat->m[1][1] * mat->m[2][2] - mat->m[1][2] * mat->m[2][1]) * one_over_det;
    inv->m[0][1] = -(1 * (mat->m[0][1] * mat->m[2][2] - mat->m[0][2] * mat->m[2][1])) * one_over_det;
    inv->m[0][2] = (mat->m[0][1] * mat->m[1][2] - mat->m[0][2] * mat->m[1][1]) * one_over_det;
    inv->m[1][0] = -(1 * (mat->m[1][0] * mat->m[2][2] - mat->m[1][2] * mat->m[2][0])) * one_over_det;
    inv->m[1][1] = (mat->m[0][0] * mat->m[2][2] - mat->m[0][2] * mat->m[2][0]) * one_over_det;
    inv->m[1][2] = -(1 * (mat->m[0][0] * mat->m[1][2] - mat->m[0][2] * mat->m[1][0])) * one_over_det;
    inv->m[2][0] = (mat->m[1][0] * mat->m[2][1] - mat->m[1][1] * mat->m[2][0]) * one_over_det;
    inv->m[2][1] = -(1 * (mat->m[0][0] * mat->m[2][1] - mat->m[0][1] * mat->m[2][0])) * one_over_det;
    inv->m[2][2] = (mat->m[0][0] * mat->m[1][1] - mat->m[0][1] * mat->m[1][0]) * one_over_det;
}

__INLINE__ void mat3_create_transform(mat3 *mat, vec2 axisx, vec2 axisy, vec2 position)
{
    mat->m[0][0] = axisx.x;
    mat->m[1][0] = axisx.y;
    mat->m[2][0] = 0;
    mat->m[0][1] = axisy.x;
    mat->m[1][1] = axisy.y;
    mat->m[2][1] = 0;
    mat->m[0][2] = position.x;
    mat->m[1][2] = position.y;
    mat->m[2][2] = 1;
}

// ----------------------------------------------------------------------------------
// -- mat4 operations.
// ----------------------------------------------------------------------------------

__INLINE__ void mat4_empty(mat4 *mat)
{
    mat->m[0][0] = 0;
    mat->m[0][1] = 0;
    mat->m[0][2] = 0;
    mat->m[0][3] = 0;

    mat->m[1][0] = 0;
    mat->m[1][1] = 0;
    mat->m[1][2] = 0;
    mat->m[1][3] = 0;

    mat->m[2][0] = 0;
    mat->m[2][1] = 0;
    mat->m[2][2] = 0;
    mat->m[2][3] = 0;

    mat->m[3][0] = 0;
    mat->m[3][1] = 0;
    mat->m[3][2] = 0;
    mat->m[3][3] = 0;
}

__INLINE__ void mat4_identity(mat4 *mat)
{
    mat->m[0][0] = 1;
    mat->m[0][1] = 0;
    mat->m[0][2] = 0;
    mat->m[0][3] = 0;

    mat->m[1][0] = 0;
    mat->m[1][1] = 1;
    mat->m[1][2] = 0;
    mat->m[1][3] = 0;

    mat->m[2][0] = 0;
    mat->m[2][1] = 0;
    mat->m[2][2] = 1;
    mat->m[2][3] = 0;

    mat->m[3][0] = 0;
    mat->m[3][1] = 0;
    mat->m[3][2] = 0;
    mat->m[3][3] = 1;
}

__INLINE__ void mat4_copy(mat4 *dest, mat4 *src)
{
    dest->m[0][0] = src->m[0][0];
    dest->m[0][1] = src->m[0][1];
    dest->m[0][2] = src->m[0][2];
    dest->m[0][3] = src->m[0][3];

    dest->m[1][0] = src->m[1][0];
    dest->m[1][1] = src->m[1][1];
    dest->m[1][2] = src->m[1][2];
    dest->m[1][3] = src->m[1][3];

    dest->m[2][0] = src->m[2][0];
    dest->m[2][1] = src->m[2][1];
    dest->m[2][2] = src->m[2][2];
    dest->m[2][3] = src->m[2][3];

    dest->m[3][0] = src->m[3][0];
    dest->m[3][1] = src->m[3][1];
    dest->m[3][2] = src->m[3][2];
    dest->m[3][3] = src->m[3][3];
}

__INLINE__ b32 mat4_equals(mat4 *m1, mat4 *m2)
{
    b32 result = 0;

    if (m1->m[0][0] == m2->m[0][0] &&
        m1->m[0][1] == m2->m[0][1] &&
        m1->m[0][2] == m2->m[0][2] &&
        m1->m[0][3] == m2->m[0][3] &&
        m1->m[1][0] == m2->m[1][0] &&
        m1->m[1][1] == m2->m[1][1] &&
        m1->m[1][2] == m2->m[1][2] &&
        m1->m[1][3] == m2->m[1][3] &&
        m1->m[2][0] == m2->m[2][0] &&
        m1->m[2][1] == m2->m[2][1] &&
        m1->m[2][2] == m2->m[2][2] &&
        m1->m[2][3] == m2->m[2][3] &&
        m1->m[3][0] == m2->m[3][0] &&
        m1->m[3][1] == m2->m[3][1] &&
        m1->m[3][2] == m2->m[3][2] &&
        m1->m[3][3] == m2->m[3][3])
        result = 1;

    return result;
}

__INLINE__ r32 mat4_det(mat4 *mat)
{
    // (0,0) minor.
    r32 det00 = mat->m[1][1] * (mat->m[2][2] * mat->m[3][3] - mat->m[2][3] * mat->m[3][2]) -
                mat->m[1][2] * (mat->m[2][1] * mat->m[3][3] - mat->m[2][3] * mat->m[3][1]) +
                mat->m[1][3] * (mat->m[2][1] * mat->m[3][2] - mat->m[2][2] * mat->m[3][1]);

    // (0,1) minor.
    r32 det01 = mat->m[1][0] * (mat->m[2][2] * mat->m[3][3] - mat->m[2][3] * mat->m[3][2]) -
                mat->m[1][2] * (mat->m[2][0] * mat->m[3][3] - mat->m[2][3] * mat->m[3][0]) +
                mat->m[1][3] * (mat->m[2][0] * mat->m[3][2] - mat->m[2][2] * mat->m[3][0]);

    // (0,2) minor.
    r32 det02 = mat->m[1][0] * (mat->m[2][1] * mat->m[3][3] - mat->m[2][3] * mat->m[3][1]) -
                mat->m[1][1] * (mat->m[2][0] * mat->m[3][3] - mat->m[2][3] * mat->m[3][0]) +
                mat->m[1][3] * (mat->m[2][0] * mat->m[3][1] - mat->m[2][1] * mat->m[3][0]);

    // (0,3) minor.
    r32 det03 = mat->m[1][0] * (mat->m[2][1] * mat->m[3][2] - mat->m[2][2] * mat->m[3][1]) -
                mat->m[1][1] * (mat->m[2][0] * mat->m[3][2] - mat->m[2][2] * mat->m[3][0]) +
                mat->m[1][2] * (mat->m[2][0] * mat->m[3][1] - mat->m[2][1] * mat->m[3][0]);

    r32 result = mat->m[0][0] * det00 -
                 mat->m[0][1] * det01 +
                 mat->m[0][2] * det02 -
                 mat->m[0][3] * det03;

    return result;
}

__INLINE__ void mat4_add(mat4 *m1, mat4 *m2, mat4 *out)
{
    out->m[0][0] = m1->m[0][0] + m2->m[0][0];
    out->m[0][1] = m1->m[0][1] + m2->m[0][1];
    out->m[0][2] = m1->m[0][2] + m2->m[0][2];
    out->m[0][3] = m1->m[0][3] + m2->m[0][3];

    out->m[1][0] = m1->m[1][0] + m2->m[1][0];
    out->m[1][1] = m1->m[1][1] + m2->m[1][1];
    out->m[1][2] = m1->m[1][2] + m2->m[1][2];
    out->m[1][3] = m1->m[1][3] + m2->m[1][3];

    out->m[2][0] = m1->m[2][0] + m2->m[2][0];
    out->m[2][1] = m1->m[2][1] + m2->m[2][1];
    out->m[2][2] = m1->m[2][2] + m2->m[2][2];
    out->m[2][3] = m1->m[2][3] + m2->m[2][3];

    out->m[3][0] = m1->m[1][0] + m2->m[3][0];
    out->m[3][1] = m1->m[1][1] + m2->m[3][1];
    out->m[3][2] = m1->m[1][2] + m2->m[3][2];
    out->m[3][3] = m1->m[1][3] + m2->m[3][3];
}

__INLINE__ void mat4_sub(mat4 *m1, mat4 *m2, mat4 *out)
{
    out->m[0][0] = m1->m[0][0] - m2->m[0][0];
    out->m[0][1] = m1->m[0][1] - m2->m[0][1];
    out->m[0][2] = m1->m[0][2] - m2->m[0][2];
    out->m[0][3] = m1->m[0][3] - m2->m[0][3];

    out->m[1][0] = m1->m[1][0] - m2->m[1][0];
    out->m[1][1] = m1->m[1][1] - m2->m[1][1];
    out->m[1][2] = m1->m[1][2] - m2->m[1][2];
    out->m[1][3] = m1->m[1][3] - m2->m[1][3];

    out->m[2][0] = m1->m[2][0] - m2->m[2][0];
    out->m[2][1] = m1->m[2][1] - m2->m[2][1];
    out->m[2][2] = m1->m[2][2] - m2->m[2][2];
    out->m[2][3] = m1->m[2][3] - m2->m[2][3];

    out->m[3][0] = m1->m[1][0] - m2->m[3][0];
    out->m[3][1] = m1->m[1][1] - m2->m[3][1];
    out->m[3][2] = m1->m[1][2] - m2->m[3][2];
    out->m[3][3] = m1->m[1][3] - m2->m[3][3];
}

__INLINE__ void mat4_muls(mat4 *m, r32 s, mat4 *out)
{
    out->m[0][0] = m->m[0][0] * s;
    out->m[0][1] = m->m[0][1] * s;
    out->m[0][2] = m->m[0][2] * s;
    out->m[0][3] = m->m[0][3] * s;

    out->m[1][0] = m->m[1][0] * s;
    out->m[1][1] = m->m[1][1] * s;
    out->m[1][2] = m->m[1][2] * s;
    out->m[1][3] = m->m[1][3] * s;

    out->m[2][0] = m->m[2][0] * s;
    out->m[2][1] = m->m[2][1] * s;
    out->m[2][2] = m->m[2][2] * s;
    out->m[2][3] = m->m[2][3] * s;

    out->m[3][0] = m->m[3][0] * s;
    out->m[3][1] = m->m[3][1] * s;
    out->m[3][2] = m->m[3][2] * s;
    out->m[3][3] = m->m[3][3] * s;
}

// ----------------------------------------------------------------------------------
// -- mat4_mul
// ----------------------------------------------------------------------------------

__INLINE__ void mat4_mul(mat4 *m1, mat4 *m2, mat4 *out)
{
    mat4 tmp;

    tmp.m[0][0] = m1->m[0][0] * m2->m[0][0] + m1->m[0][1] * m2->m[1][0] + m1->m[0][2] * m2->m[2][0] + m1->m[0][3] * m2->m[3][0];
    tmp.m[0][1] = m1->m[0][0] * m2->m[0][1] + m1->m[0][1] * m2->m[1][1] + m1->m[0][2] * m2->m[2][1] + m1->m[0][3] * m2->m[3][1];
    tmp.m[0][2] = m1->m[0][0] * m2->m[0][2] + m1->m[0][1] * m2->m[1][2] + m1->m[0][2] * m2->m[2][2] + m1->m[0][3] * m2->m[3][2];
    tmp.m[0][3] = m1->m[0][0] * m2->m[0][3] + m1->m[0][1] * m2->m[1][3] + m1->m[0][2] * m2->m[2][3] + m1->m[0][3] * m2->m[3][3];

    tmp.m[1][0] = m1->m[1][0] * m2->m[0][0] + m1->m[1][1] * m2->m[1][0] + m1->m[1][2] * m2->m[2][0] + m1->m[1][3] * m2->m[3][0];
    tmp.m[1][1] = m1->m[1][0] * m2->m[0][1] + m1->m[1][1] * m2->m[1][1] + m1->m[1][2] * m2->m[2][1] + m1->m[1][3] * m2->m[3][1];
    tmp.m[1][2] = m1->m[1][0] * m2->m[0][2] + m1->m[1][1] * m2->m[1][2] + m1->m[1][2] * m2->m[2][2] + m1->m[1][3] * m2->m[3][2];
    tmp.m[1][3] = m1->m[1][0] * m2->m[0][3] + m1->m[1][1] * m2->m[1][3] + m1->m[1][2] * m2->m[2][3] + m1->m[1][3] * m2->m[3][3];

    tmp.m[2][0] = m1->m[2][0] * m2->m[0][0] + m1->m[2][1] * m2->m[1][0] + m1->m[2][2] * m2->m[2][0] + m1->m[2][3] * m2->m[3][0];
    tmp.m[2][1] = m1->m[2][0] * m2->m[0][1] + m1->m[2][1] * m2->m[1][1] + m1->m[2][2] * m2->m[2][1] + m1->m[2][3] * m2->m[3][1];
    tmp.m[2][2] = m1->m[2][0] * m2->m[0][2] + m1->m[2][1] * m2->m[1][2] + m1->m[2][2] * m2->m[2][2] + m1->m[2][3] * m2->m[3][2];
    tmp.m[2][3] = m1->m[2][0] * m2->m[0][3] + m1->m[2][1] * m2->m[1][3] + m1->m[2][2] * m2->m[2][3] + m1->m[2][3] * m2->m[3][3];

    tmp.m[3][0] = m1->m[3][0] * m2->m[0][0] + m1->m[3][1] * m2->m[1][0] + m1->m[3][2] * m2->m[2][0] + m1->m[3][3] * m2->m[3][0];
    tmp.m[3][1] = m1->m[3][0] * m2->m[0][1] + m1->m[3][1] * m2->m[1][1] + m1->m[3][2] * m2->m[2][1] + m1->m[3][3] * m2->m[3][1];
    tmp.m[3][2] = m1->m[3][0] * m2->m[0][2] + m1->m[3][1] * m2->m[1][2] + m1->m[3][2] * m2->m[2][2] + m1->m[3][3] * m2->m[3][2];
    tmp.m[3][3] = m1->m[3][0] * m2->m[0][3] + m1->m[3][1] * m2->m[1][3] + m1->m[3][2] * m2->m[2][3] + m1->m[3][3] * m2->m[3][3];

    mat4_copy(out, &tmp);

    // tmp.m[3][0] = m1->m[3][2] * m2->m[2][0];
    // tmp.m[3][1] = m1->m[3][2] * m2->m[2][1];
    // tmp.m[3][2] = m1->m[3][2] * m2->m[2][2];
    // tmp.m[3][3] = m1->m[3][2] * m2->m[2][3] + m1->m[3][3] * m2->m[3][3];
}

__INLINE__ vec3 mat4_mul_vec3(mat4 *m, vec3 v)
{
    vec3 result;

    result.x = m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z;
    result.y = m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z;
    result.z = m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z;

    return result;
}

__INLINE__ vec4 mat4_mulvec(mat4 *m, vec4 v)
{
    vec4 result;

    result.x = m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z + m->m[0][3] * v.w;
    result.y = m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z + m->m[1][3] * v.w;
    result.z = m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z + m->m[2][3] * v.w;
    result.w = m->m[3][0] * v.x + m->m[3][1] * v.y + m->m[3][2] * v.z + m->m[3][3] * v.w;

    return result;
}

__INLINE__ vec3 mat4_mulvec3(mat4 *m, vec4 v)
{
    vec3 result;

    result.x = m->m[0][0] * v.x + m->m[0][1] * v.y + m->m[0][2] * v.z + m->m[0][3] * v.w;
    result.y = m->m[1][0] * v.x + m->m[1][1] * v.y + m->m[1][2] * v.z + m->m[1][3] * v.w;
    result.z = m->m[2][0] * v.x + m->m[2][1] * v.y + m->m[2][2] * v.z + m->m[2][3] * v.w;

    return result;
}

// __INLINE__ vec3 mat4_mulvec3(mat4 *m, vec3 v) {
//     return vec4_to_vec3(mat4_mulvec(m, vec3_to_vec4(v, 0)));
// }

__INLINE__ void mat4_transpose(mat4 *m, mat4 *out)
{
    out->m[0][0] = m->m[0][0];
    out->m[0][1] = m->m[1][0];
    out->m[0][2] = m->m[2][0];
    out->m[0][3] = m->m[3][0];

    out->m[1][0] = m->m[0][1];
    out->m[1][1] = m->m[1][1];
    out->m[1][2] = m->m[2][1];
    out->m[1][3] = m->m[3][1];

    out->m[2][0] = m->m[0][2];
    out->m[2][1] = m->m[1][2];
    out->m[2][2] = m->m[2][2];
    out->m[2][3] = m->m[3][2];

    out->m[3][0] = m->m[0][3];
    out->m[3][1] = m->m[1][3];
    out->m[3][2] = m->m[2][3];
    out->m[3][3] = m->m[3][3];
}

void mat4_inv(mat4 *mat, mat4 *inv)
{
    r32 one_over_det = 1.0f / mat4_det(mat);
    r32 tmp[36];

    tmp[0] = mat->m[2][2] * mat->m[3][3];
    tmp[1] = mat->m[2][3] * mat->m[3][2];
    tmp[2] = mat->m[2][1] * mat->m[3][3];
    tmp[3] = mat->m[2][3] * mat->m[3][1];
    tmp[4] = mat->m[2][1] * mat->m[3][2];
    tmp[5] = mat->m[2][2] * mat->m[3][1];
    tmp[6] = mat->m[2][0] * mat->m[3][3];
    tmp[7] = mat->m[2][3] * mat->m[3][0];
    tmp[8] = mat->m[2][0] * mat->m[3][2];
    tmp[9] = mat->m[2][2] * mat->m[3][0];
    tmp[10] = mat->m[2][0] * mat->m[3][1];
    tmp[11] = mat->m[2][1] * mat->m[3][0];
    tmp[12] = mat->m[1][2] * mat->m[3][3];
    tmp[13] = mat->m[1][3] * mat->m[3][2];
    tmp[14] = mat->m[1][1] * mat->m[3][3];
    tmp[15] = mat->m[1][3] * mat->m[3][1];
    tmp[16] = mat->m[1][1] * mat->m[3][2];
    tmp[17] = mat->m[1][2] * mat->m[3][1];
    tmp[18] = mat->m[1][0] * mat->m[3][3];
    tmp[19] = mat->m[1][3] * mat->m[3][0];
    tmp[20] = mat->m[1][0] * mat->m[3][2];
    tmp[21] = mat->m[1][2] * mat->m[3][0];
    tmp[22] = mat->m[1][0] * mat->m[3][1];
    tmp[23] = mat->m[1][1] * mat->m[3][0];
    tmp[24] = mat->m[1][2] * mat->m[2][3];
    tmp[25] = mat->m[1][3] * mat->m[2][2];
    tmp[26] = mat->m[1][1] * mat->m[2][3];
    tmp[27] = mat->m[1][3] * mat->m[2][1];
    tmp[28] = mat->m[1][1] * mat->m[2][2];
    tmp[29] = mat->m[1][2] * mat->m[2][1];
    tmp[30] = mat->m[1][0] * mat->m[2][3];
    tmp[31] = mat->m[1][3] * mat->m[2][0];
    tmp[32] = mat->m[1][0] * mat->m[2][2];
    tmp[33] = mat->m[1][2] * mat->m[2][0];
    tmp[34] = mat->m[1][0] * mat->m[2][1];
    tmp[35] = mat->m[1][1] * mat->m[2][0];

    r32 e11 = mat->m[1][1] * (tmp[0] - tmp[1]) - mat->m[1][2] * (tmp[2] - tmp[3]) + mat->m[1][3] * (tmp[4] - tmp[5]);
    r32 e12 = mat->m[1][0] * (tmp[0] - tmp[1]) - mat->m[1][2] * (tmp[6] - tmp[7]) + mat->m[1][3] * (tmp[8] - tmp[9]);
    r32 e13 = mat->m[1][0] * (tmp[2] - tmp[3]) - mat->m[1][1] * (tmp[6] - tmp[7]) + mat->m[1][3] * (tmp[10] - tmp[11]);
    r32 e14 = mat->m[1][0] * (tmp[4] - tmp[5]) - mat->m[1][1] * (tmp[8] - tmp[9]) + mat->m[1][2] * (tmp[10] - tmp[11]);
    r32 e21 = mat->m[0][1] * (tmp[0] - tmp[1]) - mat->m[0][2] * (tmp[2] - tmp[3]) + mat->m[0][3] * (tmp[4] - tmp[5]);
    r32 e22 = mat->m[0][0] * (tmp[0] - tmp[1]) - mat->m[0][2] * (tmp[6] - tmp[7]) + mat->m[0][3] * (tmp[8] - tmp[9]);
    r32 e23 = mat->m[0][0] * (tmp[2] - tmp[3]) - mat->m[0][1] * (tmp[6] - tmp[7]) + mat->m[0][3] * (tmp[10] - tmp[11]);
    r32 e24 = mat->m[0][0] * (tmp[4] - tmp[5]) - mat->m[0][1] * (tmp[8] - tmp[9]) + mat->m[0][2] * (tmp[10] - tmp[11]);
    r32 e31 = mat->m[0][1] * (tmp[12] - tmp[13]) - mat->m[0][2] * (tmp[14] - tmp[15]) + mat->m[0][3] * (tmp[16] - tmp[17]);
    r32 e32 = mat->m[0][0] * (tmp[12] - tmp[13]) - mat->m[0][2] * (tmp[18] - tmp[19]) + mat->m[0][3] * (tmp[20] - tmp[21]);
    r32 e33 = mat->m[0][0] * (tmp[14] - tmp[15]) - mat->m[0][1] * (tmp[18] - tmp[19]) + mat->m[0][3] * (tmp[22] - tmp[23]);
    r32 e34 = mat->m[0][0] * (tmp[16] - tmp[17]) - mat->m[0][1] * (tmp[20] - tmp[21]) + mat->m[0][2] * (tmp[22] - tmp[23]);
    r32 e41 = mat->m[0][1] * (tmp[24] - tmp[25]) - mat->m[0][2] * (tmp[26] - tmp[27]) + mat->m[0][3] * (tmp[28] - tmp[29]);
    r32 e42 = mat->m[0][0] * (tmp[24] - tmp[25]) - mat->m[0][2] * (tmp[30] - tmp[31]) + mat->m[0][3] * (tmp[32] - tmp[33]);
    r32 e43 = mat->m[0][0] * (tmp[26] - tmp[27]) - mat->m[0][1] * (tmp[30] - tmp[31]) + mat->m[0][3] * (tmp[34] - tmp[35]);
    r32 e44 = mat->m[0][0] * (tmp[28] - tmp[29]) - mat->m[0][1] * (tmp[32] - tmp[33]) + mat->m[0][2] * (tmp[34] - tmp[35]);

    inv->m[0][0] = one_over_det * e11;
    inv->m[0][1] = -one_over_det * e21;
    inv->m[0][2] = one_over_det * e31;
    inv->m[0][3] = -one_over_det * e41;

    inv->m[1][0] = -one_over_det * e12;
    inv->m[1][1] = one_over_det * e22;
    inv->m[1][2] = -one_over_det * e32;
    inv->m[1][3] = one_over_det * e42;

    inv->m[2][0] = one_over_det * e13;
    inv->m[2][1] = -one_over_det * e23;
    inv->m[2][2] = one_over_det * e33;
    inv->m[2][3] = -one_over_det * e43;

    inv->m[3][0] = -one_over_det * e14;
    inv->m[3][1] = one_over_det * e24;
    inv->m[3][2] = -one_over_det * e34;
    inv->m[3][3] = one_over_det * e44;
}

void mat4_adjoint(mat4 *mat, mat4 *out)
{
    r32 tmp[36];

    tmp[0] = mat->m[2][2] * mat->m[3][3];
    tmp[1] = mat->m[2][3] * mat->m[3][2];
    tmp[2] = mat->m[2][1] * mat->m[3][3];
    tmp[3] = mat->m[2][3] * mat->m[3][1];
    tmp[4] = mat->m[2][1] * mat->m[3][2];
    tmp[5] = mat->m[2][2] * mat->m[3][1];
    tmp[6] = mat->m[2][0] * mat->m[3][3];
    tmp[7] = mat->m[2][3] * mat->m[3][0];
    tmp[8] = mat->m[2][0] * mat->m[3][2];
    tmp[9] = mat->m[2][2] * mat->m[3][0];
    tmp[10] = mat->m[2][0] * mat->m[3][1];
    tmp[11] = mat->m[2][1] * mat->m[3][0];
    tmp[12] = mat->m[1][2] * mat->m[3][3];
    tmp[13] = mat->m[1][3] * mat->m[3][2];
    tmp[14] = mat->m[1][1] * mat->m[3][3];
    tmp[15] = mat->m[1][3] * mat->m[3][1];
    tmp[16] = mat->m[1][1] * mat->m[3][2];
    tmp[17] = mat->m[1][2] * mat->m[3][1];
    tmp[18] = mat->m[1][0] * mat->m[3][3];
    tmp[19] = mat->m[1][3] * mat->m[3][0];
    tmp[20] = mat->m[1][0] * mat->m[3][2];
    tmp[21] = mat->m[1][2] * mat->m[3][0];
    tmp[22] = mat->m[1][0] * mat->m[3][1];
    tmp[23] = mat->m[1][1] * mat->m[3][0];
    tmp[24] = mat->m[1][2] * mat->m[2][3];
    tmp[25] = mat->m[1][3] * mat->m[2][2];
    tmp[26] = mat->m[1][1] * mat->m[2][3];
    tmp[27] = mat->m[1][3] * mat->m[2][1];
    tmp[28] = mat->m[1][1] * mat->m[2][2];
    tmp[29] = mat->m[1][2] * mat->m[2][1];
    tmp[30] = mat->m[1][0] * mat->m[2][3];
    tmp[31] = mat->m[1][3] * mat->m[2][0];
    tmp[32] = mat->m[1][0] * mat->m[2][2];
    tmp[33] = mat->m[1][2] * mat->m[2][0];
    tmp[34] = mat->m[1][0] * mat->m[2][1];
    tmp[35] = mat->m[1][1] * mat->m[2][0];

    r32 e11 = mat->m[1][1] * (tmp[0] - tmp[1]) - mat->m[1][2] * (tmp[2] - tmp[3]) + mat->m[1][3] * (tmp[4] - tmp[5]);
    r32 e12 = mat->m[1][0] * (tmp[0] - tmp[1]) - mat->m[1][2] * (tmp[6] - tmp[7]) + mat->m[1][3] * (tmp[8] - tmp[9]);
    r32 e13 = mat->m[1][0] * (tmp[2] - tmp[3]) - mat->m[1][1] * (tmp[6] - tmp[7]) + mat->m[1][3] * (tmp[10] - tmp[11]);
    r32 e14 = mat->m[1][0] * (tmp[4] - tmp[5]) - mat->m[1][1] * (tmp[8] - tmp[9]) + mat->m[1][2] * (tmp[10] - tmp[11]);
    r32 e21 = mat->m[0][1] * (tmp[0] - tmp[1]) - mat->m[0][2] * (tmp[2] - tmp[3]) + mat->m[0][3] * (tmp[4] - tmp[5]);
    r32 e22 = mat->m[0][0] * (tmp[0] - tmp[1]) - mat->m[0][2] * (tmp[6] - tmp[7]) + mat->m[0][3] * (tmp[8] - tmp[9]);
    r32 e23 = mat->m[0][0] * (tmp[2] - tmp[3]) - mat->m[0][1] * (tmp[6] - tmp[7]) + mat->m[0][3] * (tmp[10] - tmp[11]);
    r32 e24 = mat->m[0][0] * (tmp[4] - tmp[5]) - mat->m[0][1] * (tmp[8] - tmp[9]) + mat->m[0][2] * (tmp[10] - tmp[11]);
    r32 e31 = mat->m[0][1] * (tmp[12] - tmp[13]) - mat->m[0][2] * (tmp[14] - tmp[15]) + mat->m[0][3] * (tmp[16] - tmp[17]);
    r32 e32 = mat->m[0][0] * (tmp[12] - tmp[13]) - mat->m[0][2] * (tmp[18] - tmp[19]) + mat->m[0][3] * (tmp[20] - tmp[21]);
    r32 e33 = mat->m[0][0] * (tmp[14] - tmp[15]) - mat->m[0][1] * (tmp[18] - tmp[19]) + mat->m[0][3] * (tmp[22] - tmp[23]);
    r32 e34 = mat->m[0][0] * (tmp[16] - tmp[17]) - mat->m[0][1] * (tmp[20] - tmp[21]) + mat->m[0][2] * (tmp[22] - tmp[23]);
    r32 e41 = mat->m[0][1] * (tmp[24] - tmp[25]) - mat->m[0][2] * (tmp[26] - tmp[27]) + mat->m[0][3] * (tmp[28] - tmp[29]);
    r32 e42 = mat->m[0][0] * (tmp[24] - tmp[25]) - mat->m[0][2] * (tmp[30] - tmp[31]) + mat->m[0][3] * (tmp[32] - tmp[33]);
    r32 e43 = mat->m[0][0] * (tmp[26] - tmp[27]) - mat->m[0][1] * (tmp[30] - tmp[31]) + mat->m[0][3] * (tmp[34] - tmp[35]);
    r32 e44 = mat->m[0][0] * (tmp[28] - tmp[29]) - mat->m[0][1] * (tmp[32] - tmp[33]) + mat->m[0][2] * (tmp[34] - tmp[35]);

    out->m[0][0] = e11;
    out->m[0][1] = -e21;
    out->m[0][2] = e31;
    out->m[0][3] = -e41;

    out->m[1][0] = -e12;
    out->m[1][1] = e22;
    out->m[1][2] = -e32;
    out->m[1][3] = e42;

    out->m[2][0] = e13;
    out->m[2][1] = -e23;
    out->m[2][2] = e33;
    out->m[2][3] = -e43;

    out->m[3][0] = -e14;
    out->m[3][1] = e24;
    out->m[3][2] = -e34;
    out->m[3][3] = e44;
}

// ----------------------------------------------------------------------------------
// -- 2D Transformations.
// ----------------------------------------------------------------------------------

/**
 * Translation.
 */
void mat3_translation(r32 x, r32 y, mat3 *out)
{
    mat3_identity(out);

    out->m[0][2] = x;
    out->m[1][2] = y;
}

/**
 * Rotation around the origin. The angle is in radians.
 */
void mat3_rotation(r32 angle, mat3 *out)
{
    mat3_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->m[0][0] = cos_t;
    out->m[0][1] = -sin_t;
    out->m[1][0] = sin_t;
    out->m[1][1] = cos_t;
}

/**
 * Rotation around an arbitrary point. The angle is in radians.
 */
void mat3_rotation_around_point(vec2 point, r32 angle, mat3 *out)
{
    mat3_identity(out);
    mat2 tmp;

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->m[0][0] = cos_t;
    out->m[0][1] = -sin_t;
    out->m[1][0] = sin_t;
    out->m[1][1] = cos_t;

    tmp.m[0][0] = 1 - out->m[0][0];
    tmp.m[0][1] = 0 - out->m[0][1];
    tmp.m[1][0] = 0 - out->m[0][0];
    tmp.m[1][1] = 1 - out->m[0][1];

    out->m[0][2] = tmp.m[0][0] * point.x + tmp.m[0][1] * point.y;
    out->m[0][2] = tmp.m[1][0] * point.x + tmp.m[1][1] * point.y;
}

/**
 * Scale by a value.
 */
void mat3_scale(r32 sx, r32 sy, mat3 *out)
{
    mat3_identity(out);

    out->m[0][0] = sx;
    out->m[1][1] = sy;
}

#define mat3_scale_vec(s, out) (mat3_scale(s.x, s.y, out))

// ----------------------------------------------------------------------------------
// -- 3D Transformations.
// ----------------------------------------------------------------------------------

/**
 * Translation.
 */
void mat4_translation(r32 x, r32 y, r32 z, mat4 *out)
{
    mat4_identity(out);

    out->m[0][3] = x;
    out->m[1][3] = y;
    out->m[2][3] = z;
}

#define mat4_translation_vec3(v, out) (mat4_translation(v.x, v.y, v.z, out))
#define mat4_translation_vec4(v, out) (mat4_translation(v.x, v.y, v.z, out))

/**
 * Rotation around the x axis at the origin.
 */
void mat4_rotation_x(r32 angle, mat4 *out)
{
    mat4_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->m[1][1] = cos_t;
    out->m[1][2] = -sin_t;
    out->m[2][1] = sin_t;
    out->m[2][2] = cos_t;
}

/**
 * Rotation around the y axis at the origin.
 */
void mat4_rotation_y(r32 angle, mat4 *out)
{
    mat4_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->m[0][0] = cos_t;
    // out->m[1][0] = 0;
    out->m[2][0] = -sin_t;

    // out->m[0][1] = 0;
    // out->m[1][1] = 1;
    // out->m[2][1] = 0;

    out->m[0][2] = sin_t;
    // out->m[1][2] = 0;
    out->m[2][2] = cos_t;
}

/**
 * Rotation around the z axis at origin.
 */
void mat4_rotation_z(r32 angle, mat4 *out)
{
    mat4_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->m[0][0] = cos_t;
    out->m[1][0] = sin_t;
    // out->m[2][0] = 0;

    out->m[0][1] = -sin_t;
    out->m[1][1] = cos_t;
    // out->m[2][1] = 0;

    // out->m[0][2] = 0;
    // out->m[1][2] = 0;
    // out->m[2][2] = 1;
}

/**
 * Rotation around an arbitrary axis. r is expected
 * to be normalized.
 */
void mat4_rotation_around_axis(vec3 r, r32 angle, mat4 *out)
{
    mat4_identity(out);

    r32 c = cosf(angle);
    r32 s = sinf(angle);
    r32 t = 1 - c;

    out->m[0][0] = t * r.x * r.x + c;
    out->m[0][1] = t * r.x * r.y - s * r.z;
    out->m[0][2] = t * r.x * r.z + s * r.y;
    out->m[1][0] = t * r.x * r.y + s * r.z;
    out->m[1][1] = t * r.y * r.y + c;
    out->m[1][2] = t * r.y * r.z - s * r.x;
    out->m[2][0] = t * r.x * r.z - s * r.y;
    out->m[2][1] = t * r.y * r.z + s * r.x;
    out->m[2][2] = t * r.z * r.z + c;
}

/**
  * Rotation around an arbitrary point.
  */
void mat4_rotation_around_point(vec3 point, mat4 *rotation, mat4 *out)
{
    mat4 tmp;
    mat4_identity(&tmp);

    tmp.m[0][0] = rotation->m[0][0];
    tmp.m[0][1] = rotation->m[0][1];
    tmp.m[0][2] = rotation->m[0][2];
    tmp.m[0][3] = (1 - rotation->m[0][0]) * point.x - rotation->m[0][1] * point.y - rotation->m[0][2] * point.z;

    tmp.m[1][0] = rotation->m[1][0];
    tmp.m[1][1] = rotation->m[1][1];
    tmp.m[1][2] = rotation->m[1][2];
    tmp.m[1][3] = - rotation->m[1][0] * point.x + (1 - rotation->m[1][1]) * point.y - rotation->m[1][2] * point.z;

    tmp.m[2][0] = rotation->m[2][0];
    tmp.m[2][1] = rotation->m[2][1];
    tmp.m[2][2] = rotation->m[2][2];
    tmp.m[2][3] = - rotation->m[2][0] * point.x - rotation->m[2][1] * point.y + (1 - rotation->m[2][2]) * point.z;

    mat4_copy(out, &tmp);
}

/**
 * Scale by a value.
 */
void mat4_scale(r32 sx, r32 sy, r32 sz, mat4 *out)
{
    mat4_identity(out);

    out->m[0][0] = sx;
    out->m[1][1] = sy;
    out->m[2][2] = sz;
}

#define mat4_scale_vec3(s, out) (mat4_scale(s.x, s.y, s.z, out))

// ----------------------------------------------------------------------------------
// -- Projections.
// ----------------------------------------------------------------------------------

/**
 * @param r32 aspect width/height
 * @param r32 fov Angle in radians.
 * @param r32 near
 * @param r32 far
 * @param mat4 out
 */
void mat4_perspective(r32 aspect, r32 fov, r32 f_near, r32 f_far, mat4 *out)
{
    mat4_empty(out);

    r32 d = 1.0f / tan(fov * 0.5f);
    r32 s = f_near - f_far;

    out->m[0][0] = d / aspect;
    out->m[1][1] = d;
    out->m[2][2] = (f_near + f_far) / s;
    out->m[3][2] = -1;
    out->m[2][3] = 2 * f_near * f_far / s;
}

void mat4_orthographic(r32 f_right, r32 f_left, r32 f_top, r32 f_bottom, r32 f_near, r32 f_far, mat4 *out)
{
    mat4_empty(out);

    r32 rml = f_right - f_left;
    r32 fmn = f_far - f_near;
    r32 tmb = f_top - f_bottom;

    out->m[0][0] = 2 / rml;
    out->m[1][1] = 2 / tmb;
    out->m[2][2] = -2 / fmn;
    out->m[3][3] = 1;
    out->m[0][3] = -(f_right + f_left) / rml;
    out->m[1][3] = -(f_top + f_bottom) / tmb;
    out->m[2][3] = -(f_far + f_near) / fmn;
}

void mat4_viewport(r32 ws, r32 hs, r32 sx, r32 sy, r32 ds, mat4 *out)
{
    mat4_empty(out);

    r32 wh = ws / 2.0f;
    r32 hh = hs / 2.0f;
    r32 dh = ds / 2.0f;

    out->m[0][0] = wh;
    out->m[0][3] = wh + sx;
    out->m[1][1] = -hh;
    out->m[1][3] = hh + sy;
    out->m[2][2] = dh;
    out->m[2][3] = dh;
    out->m[3][3] = 1;
}

void mat4_lookat(vec3 eye, vec3 target, vec3 up, mat4 *out)
{
    mat4_identity(out);

    vec3 z_axis = vec3_normalize(vec3_sub(eye, target));
    vec3 x_axis = vec3_normalize(vec3_cross(up, z_axis));
    vec3 y_axis = vec3_normalize(vec3_cross(z_axis, x_axis));

    // orthogonal axis => inverse = transpose.
    out->m[0][0] = x_axis.x;
    out->m[0][1] = x_axis.y;
    out->m[0][2] = x_axis.z;

    out->m[1][0] = y_axis.x;
    out->m[1][1] = y_axis.y;
    out->m[1][2] = y_axis.z;

    out->m[2][0] = z_axis.x;
    out->m[2][1] = z_axis.y;
    out->m[2][2] = z_axis.z;

    out->m[0][3] = -vec3_dot(x_axis, eye);
    out->m[1][3] = -vec3_dot(y_axis, eye);
    out->m[2][3] = -vec3_dot(z_axis, eye);
}

// ----------------------------------------------------------------------------------
// -- General.
// ----------------------------------------------------------------------------------

__INLINE__ vec2i fp_snap_to_pixel_center(vec2i point)
{
    s32 mx = 1;
    s32 my = 1;

    if (point.x < 0)
        mx = -1;

    if (point.y < 0)
        my = -1;

    point.x *= mx;
    point.y *= my;

    if ((point.x & FP_DEC_MASK) >= FP_HALF_ONE)
        point.x += FP_DEC_VAL;

    point.x &= ~FP_DEC_MASK;

    if ((point.y & FP_DEC_MASK) >= FP_HALF_ONE)
        point.y += FP_DEC_VAL;

    point.y &= ~FP_DEC_MASK;

    point.x *= mx;
    point.y *= my;

    return point;
}

// Computes the perpendicular vector to v2 using v1.
vec3 force_perp(vec3 v1, vec3 v2)
{
    r32 s = vec3_dot(v1, v2) / vec3_dot(v2, v2);
    vec3 tmp = vec3_muls(v2, s);
    return vec3_sub(v1, tmp);
}

__INLINE__ r32 shadow_value(r32 shadow_value, r32 strength)
{
    r32 sv3 = shadow_value * shadow_value * shadow_value;
    return sv3 + (1.0f - strength) * (1 - sv3);
}

#define gcsr_min(a, b) (a < b ? a : b)
#define gcsr_max(a, b) (a > b ? a : b)

#define DET_AREA(p1x, p1y, p2x, p2y, p3x, p3y) (p2x - p1x) * (p3y - p1y) - (p2y - p1y) * (p3x - p1x)

#ifdef GC_PIPE_SSE
#include "simd/gcsr_math_sse.h"
#endif

#endif