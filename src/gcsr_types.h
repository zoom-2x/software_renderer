#ifndef GCSR_ENGINE_TYPES_H
#define GCSR_ENGINE_TYPES_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------------
// -- primitive data types.
// ---------------------------------------------------------------------------------

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int6;
typedef int32_t int32;
typedef int64_t int64;

typedef u32 b32;
typedef u32 bool32;

typedef u8 b8;
typedef u8 bool8;

typedef float r32;
typedef double r64;

typedef float real32;
typedef double real64;

#if 1
#ifndef true
    #define true 1
#endif
#ifndef false
    #define false 0
#endif
#endif

union _vec2
{
    struct
    {
        r32 x;
        r32 y;
    };

    struct
    {
        r32 u;
        r32 v;
    };

    r32 E[2];
    u32 Ei[2];
};

union _vec3
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
    };

    struct
    {
        r32 u;
        r32 v;
        r32 w;
    };

    struct
    {
        r32 r;
        r32 g;
        r32 b;
    };

    r32 E[3];
    u32 Ei[3];
};

union _vec4
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
        r32 w;
    };

    struct
    {
        r32 r;
        r32 g;
        r32 b;
        r32 a;
    };

    r32 E[4];
    u32 Ei[4];
};

union _vec2i
{
    struct
    {
        s32 x;
        s32 y;
    };

    s32 data[2];
};

union _vec2u
{
    struct
    {
        u32 x;
        u32 y;
    };

    u32 data[2];
};

union _vec3i
{
    struct
    {
        s32 x;
        s32 y;
        s32 z;
    };

    struct
    {
        s32 r;
        s32 g;
        s32 b;
    };

    s32 data[3];
};

union _vec3u
{
    struct
    {
        u32 x;
        u32 y;
        u32 z;
    };

    struct
    {
        u32 r;
        u32 g;
        u32 b;
    };

    s32 data[3];
};

union _vec4i
{
    struct
    {
        s32 x;
        s32 y;
        s32 z;
        s32 w;
    };

    struct
    {
        s32 r;
        s32 g;
        s32 b;
        s32 a;
    };

    s32 data[4];
};

typedef struct {r32 x; r32 y;} vec2_t;
typedef struct {s32 x; s32 y;} vec2i_t;
typedef struct {r32 x; r32 y; r32 z;} vec3_t;
typedef struct {s32 x; s32 y; s32 z;} vec3i_t;
typedef struct {r32 x; r32 y; r32 z; r32 w;} vec4_t;
typedef struct {r32 r; r32 g; r32 b; r32 a;} color_t;

typedef struct {r32 r0[2]; r32 r1[2];} mat2_t;
typedef struct {r32 r0[3]; r32 r1[3]; r32 r2[3];} mat3_t;
typedef struct {r32 r0[4]; r32 r1[4]; r32 r2[4]; r32 r3[4];} mat4_t;

typedef __ALIGN__ union
{
    r32 data[4];
    vec2_t v2;
    vec2i_t v2i;
    vec3_t v3;
    vec3i_t v3i;
    vec4_t v4;
    color_t c;
} gc_vec_t;

#define VINIT2(name, px, py) \
    gc_vec_t name; \
    name.v4.x = px; \
    name.v4.y = py; \
    name.v4.z = 0; \
    name.v4.w = 0

#define VINIT3(name, px, py, pz) \
    gc_vec_t name; \
    name.v4.x = px; \
    name.v4.y = py; \
    name.v4.z = pz; \
    name.v4.w = 0

#define VINIT4(name, px, py, pz, pw) \
    gc_vec_t name; \
    name.v4.x = px; \
    name.v4.y = py; \
    name.v4.z = pz; \
    name.v4.w = pw

#define VSET2(name, px, py) \
    name.v2.x = px; \
    name.v2.y = py

#define VSET3(name, px, py, pz) \
    name.v3.x = px; \
    name.v3.y = py; \
    name.v3.z = pz

#define VSET4(name, px, py, pz, pw) \
    name.v4.x = px; \
    name.v4.y = py; \
    name.v4.z = pz; \
    name.v4.w = pw

#define VCPY2(dest, src) \
    dest.v2.x = (src).v2.x; \
    dest.v2.y = (src).v2.y

#define VCPY3(dest, src) \
    dest.v3.x = (src).v3.x; \
    dest.v3.y = (src).v3.y; \
    dest.v3.z = (src).v3.z

#define VCPY4(dest, src) \
    dest.v4.x = (src).v4.x; \
    dest.v4.y = (src).v4.y; \
    dest.v4.z = (src).v4.z; \
    dest.v4.w = (src).v4.w

typedef __ALIGN__ struct
{
    r32 x[GC_FRAG_SIZE];
    r32 y[GC_FRAG_SIZE];
} fv2_t;

typedef __ALIGN__ struct
{
    r32 x[GC_FRAG_SIZE];
    r32 y[GC_FRAG_SIZE];
    r32 z[GC_FRAG_SIZE];
} fv3_t;

typedef __ALIGN__ struct
{
    r32 x[GC_FRAG_SIZE];
    r32 y[GC_FRAG_SIZE];
    r32 z[GC_FRAG_SIZE];
    r32 w[GC_FRAG_SIZE];
} fv4_t;

typedef __ALIGN__ union
{
    r32 data[4][4];
    mat2_t m2;
    mat3_t m3;
    mat4_t m4;
} gc_mat_t;

// typedef vec4 Color;

// typedef struct
// {
//     r32 r;
//     r32 g;
//     r32 b;
//     r32 a;
// } gl_color_t;

typedef struct
{
    u32 A;
    u32 B;
    u32 C;
    u32 D;
} bilinear_sample_t;

typedef struct
{
    u32 bytes;
} var_char_t;

// typedef r32 mat2[2][2];
// typedef r32 mat3[3][3];
// typedef r32 mat4[4][4];

typedef struct {r32 x; r32 y; r32 z; r32 w;} quaternion;
typedef struct {r32 m[2][2];} mat2;
typedef struct {r32 m[3][3];} mat3;
typedef struct {r32 m[4][4];} mat4;

typedef __ALIGN__ struct
{
    r32 r[GC_FRAG_SIZE];
    r32 g[GC_FRAG_SIZE];
    r32 b[GC_FRAG_SIZE];
    r32 a[GC_FRAG_SIZE];
} shader_color_t;

#define SHADER_COLOR_VAR(name) \
    shader_color_t name; \
    name.r[0] = 0; \
    name.r[1] = 0; \
    name.r[2] = 0; \
    name.r[3] = 0; \
    name.g[0] = 0; \
    name.g[1] = 0; \
    name.g[2] = 0; \
    name.g[3] = 0; \
    name.b[0] = 0; \
    name.b[1] = 0; \
    name.b[2] = 0; \
    name.b[3] = 0; \
    name.a[0] = 0; \
    name.a[1] = 0; \
    name.a[2] = 0; \
    name.a[3] = 0

#define SHADER_COLOR_VAR_SET(name, dr, dg, db, da) \
    shader_color_t name; \
    name.r[0] = dr; \
    name.r[1] = dr; \
    name.r[2] = dr; \
    name.r[3] = dr; \
    name.g[0] = dg; \
    name.g[1] = dg; \
    name.g[2] = dg; \
    name.g[3] = dg; \
    name.b[0] = db; \
    name.b[1] = db; \
    name.b[2] = db; \
    name.b[3] = db; \
    name.a[0] = da; \
    name.a[1] = da; \
    name.a[2] = da; \
    name.a[3] = da

typedef union _vec2 vec2;
typedef union _vec3 vec3;
typedef union _vec4 vec4;
typedef union _vec2u vec2u;
typedef union _vec2i vec2i;
typedef union _vec3u vec3u;
typedef union _vec3i vec3i;
typedef union _vec4i vec4i;

#define A3SET(name, v1, v2, v3) \
    name[0] = v1; \
    name[1] = v2; \
    name[2] = v3

#define A4SET(name, v1, v2, v3, v4) \
    name[0] = v1; \
    name[1] = v2; \
    name[2] = v3; \
    name[3] = v4

typedef struct
{
    vec2i min;
    vec2i max;
} gc_screen_rect_t;

#define SCREEN_RECT_SET(name, minx, miny, maxx, maxy) \
    gc_screen_rect_t name; \
    name.min.x = minx; \
    name.min.y = miny; \
    name.max.x = maxx; \
    name.max.y = maxy

typedef struct
{
    r32 pitch;
    r32 yaw;
    r32 roll;
} gc_rotation_t;

typedef union
{
    struct {
        r32 x;
        r32 y;
        r32 z;
    };

    struct {
        r32 roll;
        r32 pitch;
        r32 heading;
    };
} gc_euler_xyz_t;

typedef struct
{
    gc_vec_t translation;
    gc_vec_t rotation;
    gc_vec_t scaling;
} gc_transform_t;

// ----------------------------------------------------------------------------------
// -- SIMD.
// ----------------------------------------------------------------------------------

typedef __ALIGN__ struct
{
    __m128 x;
    __m128 y;
} sse_v2_t;

typedef __ALIGN__ struct
{
    __m128 x;
    __m128 y;
    __m128 z;
} sse_v3_t;

typedef __ALIGN__ struct
{
    __m128 x;
    __m128 y;
    __m128 z;
    __m128 w;
} sse_v4_t;

typedef struct
{
    __m128 r;
    __m128 g;
    __m128 b;
    __m128 a;
} sse_color_t;

#define SSE_COLOR_VAR(name) \
    sse_color_t name; \
    name.r = _mm_setzero_ps(); \
    name.g = _mm_setzero_ps(); \
    name.b = _mm_setzero_ps(); \
    name.a = _mm_setzero_ps()

#define SSE_COLOR_VAR_SET(name, dr, dg, db, da) \
    sse_color_t name; \
    name.r = _mm_set1_ps(dr); \
    name.g = _mm_set1_ps(dg); \
    name.b = _mm_set1_ps(db); \
    name.a = _mm_set1_ps(da)

#endif