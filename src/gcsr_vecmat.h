// ----------------------------------------------------------------------------------
// -- File: gcsr_vecmat.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-02-08 08:24:22
// -- Modified:
// ----------------------------------------------------------------------------------

#ifndef GCSR_ENGINE_VECMAT_H
#define GCSR_ENGINE_VECMAT_H

#include "math.h"

#define DEBUG_V2(name, v) (printf("%s: {%f, %f\n", name, (v).v3.x, (v).v3.y))
#define DEBUG_V3(name, v) (printf("%s: {%f, %f, %f}\n", name, v.v3.x, v.v3.y, v.v3.z))

// ----------------------------------------------------------------------------------
// -- Vector operations.
// ----------------------------------------------------------------------------------

__INLINE__ b8 gl_vec2_equals(gc_vec_t *v1, gc_vec_t *v2) {
    return (v1->data[0] == v2->data[0] && v1->data[1] == v2->data[1]);
}

__INLINE__ r32 gl_vec2_dot(gc_vec_t *v1, gc_vec_t *v2) {
    return v1->data[0] * v2->data[0] + v1->data[1] * v2->data[1];
}

__INLINE__ r32 gl_vec2_inner(gc_vec_t *v1, gc_vec_t *v2) {
    return v1->data[0] * v2->data[0] + v1->data[1] * v2->data[1];
}

__INLINE__ r32 gl_vec2_len(gc_vec_t *v) {
    return sqrt(gl_vec2_inner(v, v));
}

__INLINE__ r32 gl_vec2_len2(gc_vec_t *v) {
    return gl_vec2_inner(v, v);
}

__INLINE__ void gl_vec2_add(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] + v2->data[0];
    out->data[1] = v1->data[1] + v2->data[1];
}

__INLINE__ void gl_vec2_sub(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] - v2->data[0];
    out->data[1] = v1->data[1] - v2->data[1];
}

__INLINE__ void gl_vec2_muls(gc_vec_t *v, r32 s, gc_vec_t *out)
{
    out->data[0] = v->data[0] * s;
    out->data[1] = v->data[1] * s;
}

__INLINE__ void gl_vec2_inv(gc_vec_t *v, gc_vec_t *out)
{
    out->data[0] = v->data[0] * -1;
    out->data[1] = v->data[1] * -1;
}

__INLINE__ void gl_vec2_perp(gc_vec_t *v, gc_vec_t *out)
{
    out->data[0] = -v->data[1];
    out->data[1] = v->data[0];
}

__INLINE__ void gl_vec2_scale(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] * v2->data[0];
    out->data[1] = v1->data[1] * v2->data[1];
}

__INLINE__ r32 gl_vec2_area(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *v3) {
    return (v2->data[0] - v1->data[0]) * (v3->data[1] - v1->data[1]) - (v2->data[1] - v1->data[1]) * (v3->data[0] - v1->data[0]);
}

__INLINE__ void gl_vec2_swap(gc_vec_t *v1, gc_vec_t *v2)
{
    r32 t = v1->data[0];
    v1->data[0] = v2->data[0];
    v2->data[0] = t;

    t = v1->data[1];
    v1->data[1] = v2->data[1];
    v2->data[1] = t;
}

__INLINE__ void gl_vec2_normalize(gc_vec_t *v, gc_vec_t *out)
{
    r32 len = gl_vec2_len(v);
    assert(len);
    r32 one_over_len = 1.0f / len;

    out->data[0] = v->data[0] * one_over_len;
    out->data[1] = v->data[1] * one_over_len;
}

__INLINE__ void gl_vec2_rotate(gc_vec_t *v, r32 angle, gc_vec_t *out)
{
    r32 s = sinf(angle);
    r32 c = cosf(angle);

    out->data[0] = v->data[0] * c - v->data[1] * s;
    out->data[1] = v->data[0] * s - v->data[1] * c;
}

__INLINE__ b8 gl_vec3_equals(gc_vec_t *v1, gc_vec_t *v2) {
    return (v1->data[0] == v2->data[0] && v1->data[1] == v2->data[1] && v1->data[2] == v2->data[2]);
}

__INLINE__ r32 gl_vec3_inner(gc_vec_t *v1, gc_vec_t *v2) {
    return (v1->data[0] * v2->data[0] + v1->data[1] * v2->data[1] + v1->data[2] * v2->data[2]);
}

#define v3_dot(v1, v2) gl_vec3_inner(v1, v2)
#define GL_V3_DOT(v1, v2) (v1.data[0] * v2.data[0] + v1.data[1] * v2.data[1] + v1.data[2] * v2.data[2])

__INLINE__ r32 gl_vec3_len(gc_vec_t *v) {
    return sqrtf(gl_vec3_inner(v, v));
}

#define gl_vec3_len2(v) gl_vec3_inner(v, v)

__INLINE__ void gl_vec3_add(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] + v2->data[0];
    out->data[1] = v1->data[1] + v2->data[1];
    out->data[2] = v1->data[2] + v2->data[2];
}

__INLINE__ void gl_vec3_sub(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] - v2->data[0];
    out->data[1] = v1->data[1] - v2->data[1];
    out->data[2] = v1->data[2] - v2->data[2];
}

// #define GL_V3_ADD(v1, v2) {v1.data[0] + v2.data[0], v1.data[1] + v2.data[1], v1.data[2] + v2.data[2]}
// #define GL_V3_ADDP(v1, v2) {v1->data[0] + v2->data[0], v1->data[1] + v2->data[1], v1->data[2] + v2->data[2]}
// #define GL_V3_SUB(v1, v2) {v1.data[0] - v2.data[0], v1.data[1] - v2.data[1], v1.data[2] - v2.data[2]}
// #define GL_V3_SUBP(v1, v2) {v1->data[0] - v2->data[0], v1->data[1] - v2->data[1], v1->data[2] - v2->data[2]}

__INLINE__ void gl_vec3_muls(gc_vec_t *v, r32 s, gc_vec_t *out)
{
    out->data[0] = v->data[0] * s;
    out->data[1] = v->data[1] * s;
    out->data[2] = v->data[2] * s;
}

// #define GL_V3_MULS(v, s) {v.data[0] * s, v.data[1] * s, v.data[2] * s}
// #define GL_V3_MULSP(v, s) {v->data[0] * s, v->data[1] * s, v->data[2] * s}

__INLINE__ void gl_vec3_inv(gc_vec_t *v)
{
    v->data[0] = -v->data[0];
    v->data[1] = -v->data[1];
    v->data[2] = -v->data[2];
}

// #define GL_V3_INV(v) {v.v3.x *= -1; v.v3.y *= -1; v.v3.z *= -1;}
// #define GL_V3_INVP(v) {v->v3.x *= -1; v->v3.y *= -1; v->v3.z *= -1;}

__INLINE__ void v3_cross(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[1] * v2->data[2] - v1->data[2] * v2->data[1];
    out->data[1] = v1->data[2] * v2->data[0] - v1->data[0] * v2->data[2];
    out->data[2] = v1->data[0] * v2->data[1] - v1->data[1] * v2->data[0];
}

#define GL_V3_CROSS(v1, v2)  \
{ \
    v1.data[1] * v2.data[2] - v1.data[2] * v2.data[1], \
    v1.data[2] * v2.data[0] - v1.data[0] * v2.data[2], \
    v1.data[0] * v2.data[1] - v1.data[1] * v2.data[0] \
}

#define GL_V3_CROSSP(v1, v2)  \
{ \
    v1->data[1] * v2->data[2] - v1->data[2] * v2->data[1], \
    v1->data[2] * v2->data[0] - v1->data[0] * v2->data[2], \
    v1->data[0] * v2->data[1] - v1->data[1] * v2->data[0] \
}

__INLINE__ void gl_vec3_swap(gc_vec_t *v1, gc_vec_t *v2)
{
    r32 t = v1->data[0];
    v1->data[0] = v2->data[0];
    v2->data[0] = t;

    t = v1->data[1];
    v1->data[1] = v2->data[1];
    v2->data[1] = t;

    t = v1->data[2];
    v1->data[2] = v2->data[2];
    v2->data[2] = t;
}

__INLINE__ void v3_normalize(gc_vec_t *v)
{
    r32 len = gl_vec3_len(v);
    // SDL_assert(len);
    r32 one_over_len = 1.0f / len;

    v->data[0] *= one_over_len;
    v->data[1] *= one_over_len;
    v->data[2] *= one_over_len;
}

__INLINE__ r32 gl_vec4_inner(gc_vec_t *v1, gc_vec_t *v2) {
    return (v1->data[0] * v2->data[0] + v1->data[1] * v2->data[1] + v1->data[2] * v2->data[2] + v1->data[3] * v2->data[3]);
}

#define gl_vec4_dot(v1, v2) gl_vec4_inner(v1, v2)

__INLINE__ r32 gl_vec4_len(gc_vec_t *v) {
    return (sqrt(gl_vec4_inner(v, v)));
}

#define gl_vec4_len2(v) _gl_vec_inner_sse(v, v)

__INLINE__ b8 gl_vec4_equals(gc_vec_t *v1, gc_vec_t *v2) {
    return (v1->data[0] == v2->data[0] && v1->data[1] == v2->data[1] && v1->data[2] == v2->data[2] && v1->data[3] == v2->data[3]);
}

__INLINE__ void gl_vec_copy(gc_vec_t *src, gc_vec_t *dest)
{
    dest->data[0] = src->data[0];
    dest->data[1] = src->data[1];
    dest->data[2] = src->data[2];
    dest->data[3] = src->data[3];
}

__INLINE__ void gl_vec4_add(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] + v2->data[0];
    out->data[1] = v1->data[1] + v2->data[1];
    out->data[2] = v1->data[2] + v2->data[2];
    out->data[3] = v1->data[3] + v2->data[3];
}

__INLINE__ void gl_vec4_sub(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] - v2->data[0];
    out->data[1] = v1->data[1] - v2->data[1];
    out->data[2] = v1->data[2] - v2->data[2];
    out->data[3] = v1->data[3] - v2->data[3];
}

__INLINE__ void gl_vec4_muls(gc_vec_t *v, r32 s, gc_vec_t *out)
{
    out->data[0] = v->data[0] * s;
    out->data[1] = v->data[1] * s;
    out->data[2] = v->data[2] * s;
    out->data[3] = v->data[3] * s;
}

__INLINE__ void gl_vec4_inv(gc_vec_t *v, gc_vec_t *out)
{
    out->data[0] = -v->data[0];
    out->data[1] = -v->data[1];
    out->data[2] = -v->data[2];
    out->data[3] = -v->data[3];
}

__INLINE__ void gl_vec4_mulvec(gc_vec_t *v1, gc_vec_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->data[0] * v2->data[0];
    out->data[1] = v1->data[1] * v2->data[1];
    out->data[2] = v1->data[2] * v2->data[2];
    out->data[3] = v1->data[3] * v2->data[3];
}

__INLINE__ void gl_vec4_swap(gc_vec_t *v1, gc_vec_t *v2)
{
    r32 t = v1->data[0];
    v1->data[0] = v2->data[0];
    v2->data[0] = t;

    t = v1->data[1];
    v1->data[1] = v2->data[1];
    v2->data[1] = t;

    t = v1->data[2];
    v1->data[2] = v2->data[2];
    v2->data[2] = t;

    t = v1->data[3];
    v1->data[3] = v2->data[3];
    v2->data[3] = t;
}

__INLINE__ void gl_vec4_normalize(gc_vec_t *v)
{
    r32 len = gl_vec4_len(v);
    assert(len);
    r32 one_over_len = 1.0f / len;

    v->data[0] *= one_over_len;
    v->data[1] *= one_over_len;
    v->data[2] *= one_over_len;
    v->data[3] *= one_over_len;
}

__INLINE__ void v4_max(gc_vec_t *v, r32 s)
{
    v->data[0] = v->data[0] >= s ? v->data[0] : s;
    v->data[1] = v->data[1] >= s ? v->data[1] : s;
    v->data[2] = v->data[2] >= s ? v->data[2] : s;
    v->data[3] = v->data[3] >= s ? v->data[3] : s;
}

__INLINE__ void v4_maxv(gc_vec_t *v, gc_vec_t *s)
{
    v->data[0] = v->data[0] >= s->data[0] ? v->data[0] : s->data[0];
    v->data[1] = v->data[1] >= s->data[1] ? v->data[1] : s->data[1];
    v->data[2] = v->data[2] >= s->data[2] ? v->data[2] : s->data[2];
    v->data[3] = v->data[3] >= s->data[3] ? v->data[3] : s->data[3];
}

// ----------------------------------------------------------------------------------
// -- Vector operations (fragment version).
// ----------------------------------------------------------------------------------

__INLINE__ void fv3_mix(fv3_t *v1, fv3_t *v2, gc_vec_t *t, fv3_t *out)
{
    out->x[0] = v1->x[0] + (v2->x[0] - v1->x[0]) * t->data[0];
    out->x[1] = v1->x[1] + (v2->x[1] - v1->x[1]) * t->data[1];
    out->x[2] = v1->x[2] + (v2->x[2] - v1->x[2]) * t->data[2];
    out->x[3] = v1->x[3] + (v2->x[3] - v1->x[3]) * t->data[3];

    out->y[0] = v1->y[0] + (v2->y[0] - v1->y[0]) * t->data[0];
    out->y[1] = v1->y[1] + (v2->y[1] - v1->y[1]) * t->data[1];
    out->y[2] = v1->y[2] + (v2->y[2] - v1->y[2]) * t->data[2];
    out->y[3] = v1->y[3] + (v2->y[3] - v1->y[3]) * t->data[3];

    out->z[0] = v1->z[0] + (v2->z[0] - v1->z[0]) * t->data[0];
    out->z[1] = v1->z[1] + (v2->z[1] - v1->z[1]) * t->data[1];
    out->z[2] = v1->z[2] + (v2->z[2] - v1->z[2]) * t->data[2];
    out->z[3] = v1->z[3] + (v2->z[3] - v1->z[3]) * t->data[3];
}

__INLINE__ void fv3_dot(fv3_t *v1, fv3_t *v2, gc_vec_t *out)
{
    out->data[0] = v1->x[0] * v2->x[0] + v1->y[0] * v2->y[0] + v1->z[0] * v2->z[0];
    out->data[1] = v1->x[1] * v2->x[1] + v1->y[1] * v2->y[1] + v1->z[1] * v2->z[1];
    out->data[2] = v1->x[2] * v2->x[2] + v1->y[2] * v2->y[2] + v1->z[2] * v2->z[2];
    out->data[3] = v1->x[3] * v2->x[3] + v1->y[3] * v2->y[3] + v1->z[3] * v2->z[3];
}

__INLINE__ void fv3_dotv(fv3_t *v, gc_vec_t sv, gc_vec_t *out)
{
    out->data[0] = v->x[0] * sv.v3.x + v->y[0] * sv.v3.y + v->z[0] * sv.v3.z;
    out->data[1] = v->x[1] * sv.v3.x + v->y[1] * sv.v3.y + v->z[1] * sv.v3.z;
    out->data[2] = v->x[2] * sv.v3.x + v->y[2] * sv.v3.y + v->z[2] * sv.v3.z;
    out->data[3] = v->x[3] * sv.v3.x + v->y[3] * sv.v3.y + v->z[3] * sv.v3.z;
}

__INLINE__ void fv3_len(fv3_t *v, r32 *len)
{
    gc_vec_t dot;

    fv3_dot(v, v, &dot);

    len[0] = sqrt(dot.data[0]);
    len[1] = sqrt(dot.data[1]);
    len[2] = sqrt(dot.data[2]);
    len[3] = sqrt(dot.data[3]);
}

__INLINE__ void fv3_lenv(fv3_t *v, gc_vec_t *len)
{
    gc_vec_t dot;

    fv3_dot(v, v, &dot);

    len->data[0] = sqrt(dot.data[0]);
    len->data[1] = sqrt(dot.data[1]);
    len->data[2] = sqrt(dot.data[2]);
    len->data[3] = sqrt(dot.data[3]);
}

__INLINE__ void fv3_normalize(fv3_t *v)
{
    r32 len[GC_FRAG_SIZE];
    r32 one_over_len[GC_FRAG_SIZE];

    fv3_len(v, len);

    one_over_len[0] = 1.0f / len[0];
    one_over_len[1] = 1.0f / len[1];
    one_over_len[2] = 1.0f / len[2];
    one_over_len[3] = 1.0f / len[3];

    v->x[0] *= one_over_len[0];
    v->y[0] *= one_over_len[0];
    v->z[0] *= one_over_len[0];

    v->x[1] *= one_over_len[1];
    v->y[1] *= one_over_len[1];
    v->z[1] *= one_over_len[1];

    v->x[2] *= one_over_len[2];
    v->y[2] *= one_over_len[2];
    v->z[2] *= one_over_len[2];

    v->x[3] *= one_over_len[3];
    v->y[3] *= one_over_len[3];
    v->z[3] *= one_over_len[3];
}

__INLINE__ void fv3_add(fv3_t *v1, fv3_t *v2, fv3_t *out)
{
    out->x[0] = v1->x[0] + v2->x[0];
    out->y[0] = v1->y[0] + v2->y[0];
    out->z[0] = v1->z[0] + v2->z[0];

    out->x[1] = v1->x[1] + v2->x[1];
    out->y[1] = v1->y[1] + v2->y[1];
    out->z[1] = v1->z[1] + v2->z[1];

    out->x[2] = v1->x[2] + v2->x[2];
    out->y[2] = v1->y[2] + v2->y[2];
    out->z[2] = v1->z[2] + v2->z[2];

    out->x[3] = v1->x[3] + v2->x[3];
    out->y[3] = v1->y[3] + v2->y[3];
    out->z[3] = v1->z[3] + v2->z[3];
}

__INLINE__ void fv3_sub(fv3_t *v1, fv3_t *v2, fv3_t *out)
{
    out->x[0] = v1->x[0] - v2->x[0];
    out->y[0] = v1->y[0] - v2->y[0];
    out->z[0] = v1->z[0] - v2->z[0];

    out->x[1] = v1->x[1] - v2->x[1];
    out->y[1] = v1->y[1] - v2->y[1];
    out->z[1] = v1->z[1] - v2->z[1];

    out->x[2] = v1->x[2] - v2->x[2];
    out->y[2] = v1->y[2] - v2->y[2];
    out->z[2] = v1->z[2] - v2->z[2];

    out->x[3] = v1->x[3] - v2->x[3];
    out->y[3] = v1->y[3] - v2->y[3];
    out->z[3] = v1->z[3] - v2->z[3];
}

__INLINE__ void fv3_vec_sub(gc_vec_t *v1, fv3_t *v2, fv3_t *out)
{
    out->x[0] = v1->v3.x - v2->x[0];
    out->x[1] = v1->v3.x - v2->x[1];
    out->x[2] = v1->v3.x - v2->x[2];
    out->x[3] = v1->v3.x - v2->x[3];

    out->y[0] = v1->v3.y - v2->y[0];
    out->y[1] = v1->v3.y - v2->y[1];
    out->y[2] = v1->v3.y - v2->y[2];
    out->y[3] = v1->v3.y - v2->y[3];

    out->z[0] = v1->v3.z - v2->z[0];
    out->z[1] = v1->v3.z - v2->z[1];
    out->z[2] = v1->v3.z - v2->z[2];
    out->z[3] = v1->v3.z - v2->z[3];
}

__INLINE__ void fv3_sub_vec(fv3_t *v1, gc_vec_t *v2, fv3_t *out)
{
    out->x[0] = v1->x[0] - v2->v3.x;
    out->x[1] = v1->x[1] - v2->v3.x;
    out->x[2] = v1->x[2] - v2->v3.x;
    out->x[3] = v1->x[3] - v2->v3.x;

    out->y[0] = v1->y[0] - v2->v3.y;
    out->y[1] = v1->y[1] - v2->v3.y;
    out->y[2] = v1->y[2] - v2->v3.y;
    out->y[3] = v1->y[3] - v2->v3.y;

    out->z[0] = v1->z[0] - v2->v3.z;
    out->z[1] = v1->z[1] - v2->v3.z;
    out->z[2] = v1->z[2] - v2->v3.z;
    out->z[3] = v1->z[3] - v2->v3.z;
}

__INLINE__ void fv3_muls(fv3_t *v, gc_vec_t *s, fv3_t *out)
{
    out->x[0] = v->x[0] * s->data[0];
    out->y[0] = v->y[0] * s->data[0];
    out->z[0] = v->z[0] * s->data[0];

    out->x[1] = v->x[1] * s->data[1];
    out->y[1] = v->y[1] * s->data[1];
    out->z[1] = v->z[1] * s->data[1];

    out->x[2] = v->x[2] * s->data[2];
    out->y[2] = v->y[2] * s->data[2];
    out->z[2] = v->z[2] * s->data[2];

    out->x[3] = v->x[3] * s->data[3];
    out->y[3] = v->y[3] * s->data[3];
    out->z[3] = v->z[3] * s->data[3];
}

__INLINE__ void fv3_muls1(fv3_t *v, r32 s, fv3_t *out)
{
    out->x[0] = v->x[0] * s;
    out->y[0] = v->y[0] * s;
    out->z[0] = v->z[0] * s;

    out->x[1] = v->x[1] * s;
    out->y[1] = v->y[1] * s;
    out->z[1] = v->z[1] * s;

    out->x[2] = v->x[2] * s;
    out->y[2] = v->y[2] * s;
    out->z[2] = v->z[2] * s;

    out->x[3] = v->x[3] * s;
    out->y[3] = v->y[3] * s;
    out->z[3] = v->z[3] * s;
}

__INLINE__ void fv3_inv(fv3_t *v)
{
    v->x[0] *= -1;
    v->x[1] *= -1;
    v->x[2] *= -1;
    v->x[3] *= -1;

    v->y[0] *= -1;
    v->y[1] *= -1;
    v->y[2] *= -1;
    v->y[3] *= -1;

    v->z[0] *= -1;
    v->z[1] *= -1;
    v->z[2] *= -1;
    v->z[3] *= -1;
}

__INLINE__ void fv3_inv_to(fv3_t *v, fv3_t *out)
{
    out->x[0] = -v->x[0];
    out->x[1] = -v->x[1];
    out->x[2] = -v->x[2];
    out->x[3] = -v->x[3];

    out->y[0] = -v->y[0];
    out->y[1] = -v->y[1];
    out->y[2] = -v->y[2];
    out->y[3] = -v->y[3];

    out->z[0] = -v->z[0];
    out->z[1] = -v->z[1];
    out->z[2] = -v->z[2];
    out->z[3] = -v->z[3];
}

__INLINE__ void fv3_cross(fv3_t *v1, fv3_t *v2, fv3_t *out)
{
    out->x[0] = v1->y[0] * v2->z[0] - v1->z[0] * v2->y[0];
    out->y[0] = v1->z[0] * v2->x[0] - v1->x[0] * v2->z[0];
    out->z[0] = v1->x[0] * v2->y[0] - v1->y[0] * v2->x[0];

    out->x[1] = v1->y[1] * v2->z[1] - v1->z[1] * v2->y[1];
    out->y[1] = v1->z[1] * v2->x[1] - v1->x[1] * v2->z[1];
    out->z[1] = v1->x[1] * v2->y[1] - v1->y[1] * v2->x[1];

    out->x[2] = v1->y[2] * v2->z[2] - v1->z[2] * v2->y[2];
    out->y[2] = v1->z[2] * v2->x[2] - v1->x[2] * v2->z[2];
    out->z[2] = v1->x[2] * v2->y[2] - v1->y[2] * v2->x[2];

    out->x[3] = v1->y[3] * v2->z[3] - v1->z[3] * v2->y[3];
    out->y[3] = v1->z[3] * v2->x[3] - v1->x[3] * v2->z[3];
    out->z[3] = v1->x[3] * v2->y[3] - v1->y[3] * v2->x[3];
}

__INLINE__ void shader_color_mix(shader_color_t *c1, shader_color_t *c2, r32 t, shader_color_t *out)
{
    out->r[0] = c1->r[0] + (c2->r[0] - c1->r[0]) * t;
    out->g[0] = c1->g[0] + (c2->g[0] - c1->g[0]) * t;
    out->b[0] = c1->b[0] + (c2->b[0] - c1->b[0]) * t;

    out->r[1] = c1->r[1] + (c2->r[1] - c1->r[1]) * t;
    out->g[1] = c1->g[1] + (c2->g[1] - c1->g[1]) * t;
    out->b[1] = c1->b[1] + (c2->b[1] - c1->b[1]) * t;

    out->r[2] = c1->r[2] + (c2->r[2] - c1->r[2]) * t;
    out->g[2] = c1->g[2] + (c2->g[2] - c1->g[2]) * t;
    out->b[2] = c1->b[2] + (c2->b[2] - c1->b[2]) * t;

    out->r[3] = c1->r[3] + (c2->r[3] - c1->r[3]) * t;
    out->g[3] = c1->g[3] + (c2->g[3] - c1->g[3]) * t;
    out->b[3] = c1->b[3] + (c2->b[3] - c1->b[3]) * t;
}

__INLINE__ void shader_color_mixv(shader_color_t *c1, shader_color_t *c2, gc_vec_t *t, shader_color_t *out)
{
    out->r[0] = c1->r[0] + (c2->r[0] - c1->r[0]) * t->data[0];
    out->g[0] = c1->g[0] + (c2->g[0] - c1->g[0]) * t->data[0];
    out->b[0] = c1->b[0] + (c2->b[0] - c1->b[0]) * t->data[0];

    out->r[1] = c1->r[1] + (c2->r[1] - c1->r[1]) * t->data[1];
    out->g[1] = c1->g[1] + (c2->g[1] - c1->g[1]) * t->data[1];
    out->b[1] = c1->b[1] + (c2->b[1] - c1->b[1]) * t->data[1];

    out->r[2] = c1->r[2] + (c2->r[2] - c1->r[2]) * t->data[2];
    out->g[2] = c1->g[2] + (c2->g[2] - c1->g[2]) * t->data[2];
    out->b[2] = c1->b[2] + (c2->b[2] - c1->b[2]) * t->data[2];

    out->r[3] = c1->r[3] + (c2->r[3] - c1->r[3]) * t->data[3];
    out->g[3] = c1->g[3] + (c2->g[3] - c1->g[3]) * t->data[3];
    out->b[3] = c1->b[3] + (c2->b[3] - c1->b[3]) * t->data[3];
}

// ----------------------------------------------------------------------------------
// -- Matrix operations.
// ----------------------------------------------------------------------------------

__INLINE__ void _gl_mat_copy(gc_mat_t *dst, gc_mat_t *src)
{
    __m128 src_r1_4x = _mm_loadu_ps((r32 *) src->data[0]);
    __m128 src_r2_4x = _mm_loadu_ps((r32 *) src->data[1]);
    __m128 src_r3_4x = _mm_loadu_ps((r32 *) src->data[2]);
    __m128 src_r4_4x = _mm_loadu_ps((r32 *) src->data[3]);

    _mm_storeu_ps((r32 *) dst->data[0], src_r1_4x);
    _mm_storeu_ps((r32 *) dst->data[1], src_r2_4x);
    _mm_storeu_ps((r32 *) dst->data[2], src_r3_4x);
    _mm_storeu_ps((r32 *) dst->data[3], src_r4_4x);
}

__INLINE__ void _gl_mat_add(gc_mat_t *m1, gc_mat_t *m2, gc_mat_t *out)
{
    __m128 m1r1_4x = _mm_loadu_ps((r32 *) m1->data[0]);
    __m128 m1r2_4x = _mm_loadu_ps((r32 *) m1->data[1]);
    __m128 m1r3_4x = _mm_loadu_ps((r32 *) m1->data[2]);
    __m128 m1r4_4x = _mm_loadu_ps((r32 *) m1->data[3]);

    __m128 m2r1_4x = _mm_loadu_ps((r32 *) m2->data[0]);
    __m128 m2r2_4x = _mm_loadu_ps((r32 *) m2->data[1]);
    __m128 m2r3_4x = _mm_loadu_ps((r32 *) m2->data[2]);
    __m128 m2r4_4x = _mm_loadu_ps((r32 *) m2->data[3]);

    m1r1_4x = _mm_add_ps(m1r1_4x, m2r1_4x);
    m1r2_4x = _mm_add_ps(m1r2_4x, m2r2_4x);
    m1r3_4x = _mm_add_ps(m1r3_4x, m2r3_4x);
    m1r4_4x = _mm_add_ps(m1r4_4x, m2r4_4x);

    _mm_storeu_ps((r32 *) out->data[0], m1r1_4x);
    _mm_storeu_ps((r32 *) out->data[1], m1r2_4x);
    _mm_storeu_ps((r32 *) out->data[2], m1r3_4x);
    _mm_storeu_ps((r32 *) out->data[3], m1r4_4x);
}

__INLINE__ void gl_mat_empty(gc_mat_t *mat)
{
    mat->m4.r0[0] = 0;
    mat->m4.r0[1] = 0;
    mat->m4.r0[2] = 0;
    mat->m4.r0[3] = 0;

    mat->m4.r1[0] = 0;
    mat->m4.r1[1] = 0;
    mat->m4.r1[2] = 0;
    mat->m4.r1[3] = 0;

    mat->m4.r2[0] = 0;
    mat->m4.r2[1] = 0;
    mat->m4.r2[2] = 0;
    mat->m4.r2[3] = 0;

    mat->m4.r3[0] = 0;
    mat->m4.r3[1] = 0;
    mat->m4.r3[2] = 0;
    mat->m4.r3[3] = 0;
}

__INLINE__ void gl_mat_identity(gc_mat_t *mat)
{
    mat->m4.r0[0] = 1;
    mat->m4.r0[1] = 0;
    mat->m4.r0[2] = 0;
    mat->m4.r0[3] = 0;

    mat->m4.r1[0] = 0;
    mat->m4.r1[1] = 1;
    mat->m4.r1[2] = 0;
    mat->m4.r1[3] = 0;

    mat->m4.r2[0] = 0;
    mat->m4.r2[1] = 0;
    mat->m4.r2[2] = 1;
    mat->m4.r2[3] = 0;

    mat->m4.r3[0] = 0;
    mat->m4.r3[1] = 0;
    mat->m4.r3[2] = 0;
    mat->m4.r3[3] = 1;
}

__INLINE__ void gl_mat2_copy(gc_mat_t *m1, gc_mat_t *m2)
{
    m1->data[0][0] = m2->data[0][0];
    m1->data[0][1] = m2->data[0][1];

    m1->data[1][0] = m2->data[1][0];
    m1->data[1][1] = m2->data[1][1];
}

__INLINE__ void gl_mat3_copy(gc_mat_t *m1, gc_mat_t *m2)
{
    m1->data[0][0] = m2->data[0][0];
    m1->data[0][1] = m2->data[0][1];
    m1->data[0][2] = m2->data[0][2];

    m1->data[1][0] = m2->data[1][0];
    m1->data[1][1] = m2->data[1][1];
    m1->data[1][2] = m2->data[1][2];

    m1->data[2][0] = m2->data[2][0];
    m1->data[2][1] = m2->data[2][1];
    m1->data[2][2] = m2->data[2][2];
}

__INLINE__ void gl_mat4_copy(gc_mat_t *m1, gc_mat_t *m2)
{
    m1->data[0][0] = m2->data[0][0];
    m1->data[0][1] = m2->data[0][1];
    m1->data[0][2] = m2->data[0][2];
    m1->data[0][3] = m2->data[0][3];

    m1->data[1][0] = m2->data[1][0];
    m1->data[1][1] = m2->data[1][1];
    m1->data[1][2] = m2->data[1][2];
    m1->data[1][3] = m2->data[1][3];

    m1->data[2][0] = m2->data[2][0];
    m1->data[2][1] = m2->data[2][1];
    m1->data[2][2] = m2->data[2][2];
    m1->data[2][3] = m2->data[2][3];

    m1->data[3][0] = m2->data[3][0];
    m1->data[3][1] = m2->data[3][1];
    m1->data[3][2] = m2->data[3][2];
    m1->data[3][3] = m2->data[3][3];
}

__INLINE__ void gl_mat2_add(gc_mat_t *m1, gc_mat_t *m2, gc_mat_t *out)
{
    out->data[0][0] = m1->data[0][0] + m2->data[0][0];
    out->data[0][1] = m1->data[0][1] + m2->data[0][1];

    out->data[1][0] = m1->data[1][0] + m2->data[1][0];
    out->data[1][1] = m1->data[1][1] + m2->data[1][1];
}

__INLINE__ void gl_mat3_add(gc_mat_t *m1, gc_mat_t *m2, gc_mat_t *out)
{
    out->data[0][0] = m1->data[0][0] + m2->data[0][0];
    out->data[0][1] = m1->data[0][1] + m2->data[0][1];
    out->data[0][2] = m1->data[0][2] + m2->data[0][2];

    out->data[1][0] = m1->data[1][0] + m2->data[1][0];
    out->data[1][1] = m1->data[1][1] + m2->data[1][1];
    out->data[1][2] = m1->data[1][2] + m2->data[1][2];

    out->data[2][0] = m1->data[2][0] + m2->data[2][0];
    out->data[2][1] = m1->data[2][1] + m2->data[2][1];
    out->data[2][2] = m1->data[2][2] + m2->data[2][2];
}

__INLINE__ void gl_mat4_add(gc_mat_t *m1, gc_mat_t *m2, gc_mat_t *out)
{
    out->data[0][0] = m1->data[0][0] + m2->data[0][0];
    out->data[0][1] = m1->data[0][1] + m2->data[0][1];
    out->data[0][2] = m1->data[0][2] + m2->data[0][2];
    out->data[0][3] = m1->data[0][3] + m2->data[0][3];

    out->data[1][0] = m1->data[1][0] + m2->data[1][0];
    out->data[1][1] = m1->data[1][1] + m2->data[1][1];
    out->data[1][2] = m1->data[1][2] + m2->data[1][2];
    out->data[1][3] = m1->data[1][3] + m2->data[1][3];

    out->data[2][0] = m1->data[2][0] + m2->data[2][0];
    out->data[2][1] = m1->data[2][1] + m2->data[2][1];
    out->data[2][2] = m1->data[2][2] + m2->data[2][2];
    out->data[2][3] = m1->data[2][3] + m2->data[2][3];

    out->data[3][0] = m1->data[3][0] + m2->data[3][0];
    out->data[3][1] = m1->data[3][1] + m2->data[3][1];
    out->data[3][2] = m1->data[3][2] + m2->data[3][2];
    out->data[3][3] = m1->data[3][3] + m2->data[3][3];
}

__INLINE__ void gl_mat2_sub(gc_mat_t *m1, gc_mat_t *m2, gc_mat_t *out)
{
    out->data[0][0] = m1->data[0][0] - m2->data[0][0];
    out->data[0][1] = m1->data[0][1] - m2->data[0][1];

    out->data[1][0] = m1->data[1][0] - m2->data[1][0];
    out->data[1][1] = m1->data[1][1] - m2->data[1][1];
}

__INLINE__ void gl_mat3_sub(gc_mat_t *m1, gc_mat_t *m2, gc_mat_t *out)
{
    out->data[0][0] = m1->data[0][0] - m2->data[0][0];
    out->data[0][1] = m1->data[0][1] - m2->data[0][1];
    out->data[0][2] = m1->data[0][2] - m2->data[0][2];

    out->data[1][0] = m1->data[1][0] - m2->data[1][0];
    out->data[1][1] = m1->data[1][1] - m2->data[1][1];
    out->data[1][2] = m1->data[1][2] - m2->data[1][2];

    out->data[2][0] = m1->data[2][0] - m2->data[2][0];
    out->data[2][1] = m1->data[2][1] - m2->data[2][1];
    out->data[2][2] = m1->data[2][2] - m2->data[2][2];
}

__INLINE__ void gl_mat4_sub(gc_mat_t *m1, gc_mat_t *m2, gc_mat_t *out)
{
    out->data[0][0] = m1->data[0][0] - m2->data[0][0];
    out->data[0][1] = m1->data[0][1] - m2->data[0][1];
    out->data[0][2] = m1->data[0][2] - m2->data[0][2];
    out->data[0][3] = m1->data[0][3] - m2->data[0][3];

    out->data[1][0] = m1->data[1][0] - m2->data[1][0];
    out->data[1][1] = m1->data[1][1] - m2->data[1][1];
    out->data[1][2] = m1->data[1][2] - m2->data[1][2];
    out->data[1][3] = m1->data[1][3] - m2->data[1][3];

    out->data[2][0] = m1->data[2][0] - m2->data[2][0];
    out->data[2][1] = m1->data[2][1] - m2->data[2][1];
    out->data[2][2] = m1->data[2][2] - m2->data[2][2];
    out->data[2][3] = m1->data[2][3] - m2->data[2][3];

    out->data[3][0] = m1->data[3][0] - m2->data[3][0];
    out->data[3][1] = m1->data[3][1] - m2->data[3][1];
    out->data[3][2] = m1->data[3][2] - m2->data[3][2];
    out->data[3][3] = m1->data[3][3] - m2->data[3][3];
}

__INLINE__ void gl_mat2_muls(gc_mat_t *m, r32 s)
{
    m->data[0][0] *= s;
    m->data[0][1] *= s;

    m->data[1][0] *= s;
    m->data[1][1] *= s;
}

__INLINE__ void gl_mat3_muls(gc_mat_t *m, r32 s)
{
    m->data[0][0] *= s;
    m->data[0][1] *= s;
    m->data[0][2] *= s;

    m->data[1][0] *= s;
    m->data[1][1] *= s;
    m->data[1][2] *= s;

    m->data[2][0] *= s;
    m->data[2][1] *= s;
    m->data[2][2] *= s;
}

__INLINE__ void gl_mat4_muls(gc_mat_t *m, r32 s)
{
    m->data[0][0] *= s;
    m->data[0][1] *= s;
    m->data[0][2] *= s;
    m->data[0][3] *= s;

    m->data[1][0] *= s;
    m->data[1][1] *= s;
    m->data[1][2] *= s;
    m->data[1][3] *= s;

    m->data[2][0] *= s;
    m->data[2][1] *= s;
    m->data[2][2] *= s;
    m->data[2][3] *= s;

    m->data[3][0] *= s;
    m->data[3][1] *= s;
    m->data[3][2] *= s;
    m->data[3][3] *= s;
}

__INLINE__ void gl_mat3_mul(gc_mat_t *m1, gc_mat_t * m2, gc_mat_t *out)
{
    gc_mat_t tmp;

    tmp.data[0][0] = m1->data[0][0] * m2->data[0][0] + m1->data[0][1] * m2->data[1][0] + m1->data[0][2] * m2->data[2][0];
    tmp.data[0][1] = m1->data[0][0] * m2->data[0][1] + m1->data[0][1] * m2->data[1][1] + m1->data[0][2] * m2->data[2][1];
    tmp.data[0][2] = m1->data[0][0] * m2->data[0][2] + m1->data[0][1] * m2->data[1][2] + m1->data[0][2] * m2->data[2][2];

    tmp.data[1][0] = m1->data[1][0] * m2->data[0][0] + m1->data[1][1] * m2->data[1][0] + m1->data[1][2] * m2->data[2][0];
    tmp.data[1][1] = m1->data[1][0] * m2->data[0][1] + m1->data[1][1] * m2->data[1][1] + m1->data[1][2] * m2->data[2][1];
    tmp.data[1][2] = m1->data[1][0] * m2->data[0][2] + m1->data[1][1] * m2->data[1][2] + m1->data[1][2] * m2->data[2][2];

    tmp.data[2][0] = m1->data[2][0] * m2->data[0][0] + m1->data[2][1] * m2->data[1][0] + m1->data[2][2] * m2->data[2][0];
    tmp.data[2][1] = m1->data[2][0] * m2->data[0][1] + m1->data[2][1] * m2->data[1][1] + m1->data[2][2] * m2->data[2][1];
    tmp.data[2][2] = m1->data[2][0] * m2->data[0][2] + m1->data[2][1] * m2->data[1][2] + m1->data[2][2] * m2->data[2][2];

    gl_mat3_copy(out, &tmp);
}

__INLINE__ void gl_mat4_mul(gc_mat_t *m1, gc_mat_t * m2, gc_mat_t *out)
{
    gc_mat_t tmp;

    tmp.data[0][0] = m1->data[0][0] * m2->data[0][0] + m1->data[0][1] * m2->data[1][0] + m1->data[0][2] * m2->data[2][0] + m1->data[0][3] * m2->data[3][0];
    tmp.data[0][1] = m1->data[0][0] * m2->data[0][1] + m1->data[0][1] * m2->data[1][1] + m1->data[0][2] * m2->data[2][1] + m1->data[0][3] * m2->data[3][1];
    tmp.data[0][2] = m1->data[0][0] * m2->data[0][2] + m1->data[0][1] * m2->data[1][2] + m1->data[0][2] * m2->data[2][2] + m1->data[0][3] * m2->data[3][2];
    tmp.data[0][3] = m1->data[0][0] * m2->data[0][3] + m1->data[0][1] * m2->data[1][3] + m1->data[0][2] * m2->data[2][3] + m1->data[0][3] * m2->data[3][3];

    tmp.data[1][0] = m1->data[1][0] * m2->data[0][0] + m1->data[1][1] * m2->data[1][0] + m1->data[1][2] * m2->data[2][0] + m1->data[1][3] * m2->data[3][0];
    tmp.data[1][1] = m1->data[1][0] * m2->data[0][1] + m1->data[1][1] * m2->data[1][1] + m1->data[1][2] * m2->data[2][1] + m1->data[1][3] * m2->data[3][1];
    tmp.data[1][2] = m1->data[1][0] * m2->data[0][2] + m1->data[1][1] * m2->data[1][2] + m1->data[1][2] * m2->data[2][2] + m1->data[1][3] * m2->data[3][2];
    tmp.data[1][3] = m1->data[1][0] * m2->data[0][3] + m1->data[1][1] * m2->data[1][3] + m1->data[1][2] * m2->data[2][3] + m1->data[1][3] * m2->data[3][3];

    tmp.data[2][0] = m1->data[2][0] * m2->data[0][0] + m1->data[2][1] * m2->data[1][0] + m1->data[2][2] * m2->data[2][0] + m1->data[2][3] * m2->data[3][0];
    tmp.data[2][1] = m1->data[2][0] * m2->data[0][1] + m1->data[2][1] * m2->data[1][1] + m1->data[2][2] * m2->data[2][1] + m1->data[2][3] * m2->data[3][1];
    tmp.data[2][2] = m1->data[2][0] * m2->data[0][2] + m1->data[2][1] * m2->data[1][2] + m1->data[2][2] * m2->data[2][2] + m1->data[2][3] * m2->data[3][2];
    tmp.data[2][3] = m1->data[2][0] * m2->data[0][3] + m1->data[2][1] * m2->data[1][3] + m1->data[2][2] * m2->data[2][3] + m1->data[2][3] * m2->data[3][3];

    tmp.data[3][0] = m1->data[3][0] * m2->data[0][0] + m1->data[3][1] * m2->data[1][0] + m1->data[3][2] * m2->data[2][0] + m1->data[3][3] * m2->data[3][0];
    tmp.data[3][1] = m1->data[3][0] * m2->data[0][1] + m1->data[3][1] * m2->data[1][1] + m1->data[3][2] * m2->data[2][1] + m1->data[3][3] * m2->data[3][1];
    tmp.data[3][2] = m1->data[3][0] * m2->data[0][2] + m1->data[3][1] * m2->data[1][2] + m1->data[3][2] * m2->data[2][2] + m1->data[3][3] * m2->data[3][2];
    tmp.data[3][3] = m1->data[3][0] * m2->data[0][3] + m1->data[3][1] * m2->data[1][3] + m1->data[3][2] * m2->data[2][3] + m1->data[3][3] * m2->data[3][3];

    gl_mat4_copy(out, &tmp);
}

__INLINE__ void sse_mat4_mul(gc_mat_t *m0, gc_mat_t *m1, gc_mat_t *out)
{
    __m128 r0 = _mm_load_ps(m1->data[0]);
    __m128 r1 = _mm_load_ps(m1->data[1]);
    __m128 r2 = _mm_load_ps(m1->data[2]);
    __m128 r3 = _mm_load_ps(m1->data[3]);

    __m128 c0 = _mm_set1_ps(m0->data[0][0]);
    __m128 c1 = _mm_set1_ps(m0->data[0][1]);
    __m128 c2 = _mm_set1_ps(m0->data[0][2]);
    __m128 c3 = _mm_set1_ps(m0->data[0][3]);

    __m128 res_row_0 = _mm_add_ps(
                            _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                            _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

    c0 = _mm_set1_ps(m0->data[1][0]);
    c1 = _mm_set1_ps(m0->data[1][1]);
    c2 = _mm_set1_ps(m0->data[1][2]);
    c3 = _mm_set1_ps(m0->data[1][3]);

    __m128 res_row_1 = _mm_add_ps(
                            _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                            _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

    c0 = _mm_set1_ps(m0->data[2][0]);
    c1 = _mm_set1_ps(m0->data[2][1]);
    c2 = _mm_set1_ps(m0->data[2][2]);
    c3 = _mm_set1_ps(m0->data[2][3]);

    __m128 res_row_2 = _mm_add_ps(
                            _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                            _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

    c0 = _mm_set1_ps(m0->data[3][0]);
    c1 = _mm_set1_ps(m0->data[3][1]);
    c2 = _mm_set1_ps(m0->data[3][2]);
    c3 = _mm_set1_ps(m0->data[3][3]);

    __m128 res_row_3 = _mm_add_ps(
                            _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                            _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

    _mm_store_ps(out->data[0], res_row_0);
    _mm_store_ps(out->data[1], res_row_1);
    _mm_store_ps(out->data[2], res_row_2);
    _mm_store_ps(out->data[3], res_row_3);
}

__INLINE__ void gl_mat3_mulvec(gc_mat_t *m, gc_vec_t *v, gc_vec_t *out)
{
    VINIT4(tmp, 0, 0, 0, 0);

    tmp.data[0] = m->data[0][0] * v->data[0] + m->data[0][1] * v->data[1] + m->data[0][2] * v->data[2];
    tmp.data[1] = m->data[1][0] * v->data[0] + m->data[1][1] * v->data[1] + m->data[1][2] * v->data[2];
    tmp.data[2] = m->data[2][0] * v->data[0] + m->data[2][1] * v->data[1] + m->data[2][2] * v->data[2];

    gl_vec_copy(&tmp, out);
}

__INLINE__ void gl_mat4_mulvec(gc_mat_t *m, gc_vec_t *v, gc_vec_t *out)
{
    VINIT4(tmp, 0, 0, 0, 0);

    tmp.data[0] = m->data[0][0] * v->data[0] + m->data[0][1] * v->data[1] + m->data[0][2] * v->data[2] + m->data[0][3] * v->data[3];
    tmp.data[1] = m->data[1][0] * v->data[0] + m->data[1][1] * v->data[1] + m->data[1][2] * v->data[2] + m->data[1][3] * v->data[3];
    tmp.data[2] = m->data[2][0] * v->data[0] + m->data[2][1] * v->data[1] + m->data[2][2] * v->data[2] + m->data[2][3] * v->data[3];
    tmp.data[3] = m->data[3][0] * v->data[0] + m->data[3][1] * v->data[1] + m->data[3][2] * v->data[2] + m->data[3][3] * v->data[3];

    gl_vec_copy(&tmp, out);
}

__INLINE__ void gl_mat4_mul_vector(gc_mat_t *m, gc_vec_t *v, gc_vec_t *out)
{
    VINIT4(tmp, 0, 0, 0, 0);

    tmp.data[0] = m->data[0][0] * v->data[0] + m->data[0][1] * v->data[1] + m->data[0][2] * v->data[2];
    tmp.data[1] = m->data[1][0] * v->data[0] + m->data[1][1] * v->data[1] + m->data[1][2] * v->data[2];
    tmp.data[2] = m->data[2][0] * v->data[0] + m->data[2][1] * v->data[1] + m->data[2][2] * v->data[2];
    tmp.data[3] = m->data[3][0] * v->data[0] + m->data[3][1] * v->data[1] + m->data[3][2] * v->data[2];

    gl_vec_copy(&tmp, out);
}

__INLINE__ void gl_mat4_mul_point(gc_mat_t *m, gc_vec_t *v, gc_vec_t *out)
{
    VINIT4(tmp, 0, 0, 0, 0);

    tmp.data[0] = m->data[0][0] * v->data[0] + m->data[0][1] * v->data[1] + m->data[0][2] * v->data[2] + m->data[0][3];
    tmp.data[1] = m->data[1][0] * v->data[0] + m->data[1][1] * v->data[1] + m->data[1][2] * v->data[2] + m->data[1][3];
    tmp.data[2] = m->data[2][0] * v->data[0] + m->data[2][1] * v->data[1] + m->data[2][2] * v->data[2] + m->data[2][3];
    tmp.data[3] = m->data[3][0] * v->data[0] + m->data[3][1] * v->data[1] + m->data[3][2] * v->data[2] + m->data[3][3];

    gl_vec_copy(&tmp, out);
}

__INLINE__ void gl_mat3_transpose(gc_mat_t *m, gc_mat_t *out)
{
    out->data[0][0] = m->data[0][0];
    out->data[0][1] = m->data[1][0];
    out->data[0][2] = m->data[2][0];

    out->data[1][0] = m->data[0][1];
    out->data[1][1] = m->data[1][1];
    out->data[1][2] = m->data[2][1];

    out->data[2][0] = m->data[0][2];
    out->data[2][1] = m->data[1][2];
    out->data[2][2] = m->data[2][2];
}

__INLINE__ void gl_mat4_transpose(gc_mat_t *m, gc_mat_t *out)
{
    out->data[0][0] = m->data[0][0];
    out->data[0][1] = m->data[1][0];
    out->data[0][2] = m->data[2][0];
    out->data[0][3] = m->data[3][0];

    out->data[1][0] = m->data[0][1];
    out->data[1][1] = m->data[1][1];
    out->data[1][2] = m->data[2][1];
    out->data[1][3] = m->data[3][1];

    out->data[2][0] = m->data[0][2];
    out->data[2][1] = m->data[1][2];
    out->data[2][2] = m->data[2][2];
    out->data[2][3] = m->data[3][2];

    out->data[3][0] = m->data[0][3];
    out->data[3][1] = m->data[1][3];
    out->data[3][2] = m->data[2][3];
    out->data[3][3] = m->data[3][3];
}

__INLINE__ r32 gl_mat3_det(gc_mat_t *mat)
{
    r32 result = mat->data[0][0] * (mat->data[1][1] * mat->data[2][2] - mat->data[1][2] * mat->data[2][1]) -
                 mat->data[0][1] * (mat->data[1][0] * mat->data[2][2] - mat->data[1][2] * mat->data[2][0]) +
                 mat->data[0][2] * (mat->data[1][0] * mat->data[2][1] - mat->data[1][1] * mat->data[2][0]);

    return result;
}

__INLINE__ r32 gl_mat4_det(gc_mat_t *mat)
{
    // (0,0) minor.
    r32 det00 = mat->data[1][1] * (mat->data[2][2] * mat->data[3][3] - mat->data[2][3] * mat->data[3][2]) -
                mat->data[1][2] * (mat->data[2][1] * mat->data[3][3] - mat->data[2][3] * mat->data[3][1]) +
                mat->data[1][3] * (mat->data[2][1] * mat->data[3][2] - mat->data[2][2] * mat->data[3][1]);

    // (0,1) minor.
    r32 det01 = mat->data[1][0] * (mat->data[2][2] * mat->data[3][3] - mat->data[2][3] * mat->data[3][2]) -
                mat->data[1][2] * (mat->data[2][0] * mat->data[3][3] - mat->data[2][3] * mat->data[3][0]) +
                mat->data[1][3] * (mat->data[2][0] * mat->data[3][2] - mat->data[2][2] * mat->data[3][0]);

    // (0,2) minor.
    r32 det02 = mat->data[1][0] * (mat->data[2][1] * mat->data[3][3] - mat->data[2][3] * mat->data[3][1]) -
                mat->data[1][1] * (mat->data[2][0] * mat->data[3][3] - mat->data[2][3] * mat->data[3][0]) +
                mat->data[1][3] * (mat->data[2][0] * mat->data[3][1] - mat->data[2][1] * mat->data[3][0]);

    // (0,3) minor.
    r32 det03 = mat->data[1][0] * (mat->data[2][1] * mat->data[3][2] - mat->data[2][2] * mat->data[3][1]) -
                mat->data[1][1] * (mat->data[2][0] * mat->data[3][2] - mat->data[2][2] * mat->data[3][0]) +
                mat->data[1][2] * (mat->data[2][0] * mat->data[3][1] - mat->data[2][1] * mat->data[3][0]);

    r32 result = mat->data[0][0] * det00 -
                 mat->data[0][1] * det01 +
                 mat->data[0][2] * det02 -
                 mat->data[0][3] * det03;

    return result;
}

__INLINE__ void gl_mat3_inv(gc_mat_t *mat, gc_mat_t *inv)
{
    // Assume mat is invertible.
    r32 one_over_det = 1.0f / gl_mat3_det(mat);

    inv->data[0][0] = (mat->data[1][1] * mat->data[2][2] - mat->data[1][2] * mat->data[2][1]) * one_over_det;
    inv->data[0][1] = -(1 * (mat->data[0][1] * mat->data[2][2] - mat->data[0][2] * mat->data[2][1])) * one_over_det;
    inv->data[0][2] = (mat->data[0][1] * mat->data[1][2] - mat->data[0][2] * mat->data[1][1]) * one_over_det;
    inv->data[1][0] = -(1 * (mat->data[1][0] * mat->data[2][2] - mat->data[1][2] * mat->data[2][0])) * one_over_det;
    inv->data[1][1] = (mat->data[0][0] * mat->data[2][2] - mat->data[0][2] * mat->data[2][0]) * one_over_det;
    inv->data[1][2] = -(1 * (mat->data[0][0] * mat->data[1][2] - mat->data[0][2] * mat->data[1][0])) * one_over_det;
    inv->data[2][0] = (mat->data[1][0] * mat->data[2][1] - mat->data[1][1] * mat->data[2][0]) * one_over_det;
    inv->data[2][1] = -(1 * (mat->data[0][0] * mat->data[2][1] - mat->data[0][1] * mat->data[2][0])) * one_over_det;
    inv->data[2][2] = (mat->data[0][0] * mat->data[1][1] - mat->data[0][1] * mat->data[1][0]) * one_over_det;
}

__INLINE__ void gc_mat4_inv(gc_mat_t *mat, gc_mat_t *inv)
{
    r32 one_over_det = 1.0f / gl_mat4_det(mat);
    r32 tmp[36];

    tmp[0] = mat->data[2][2] * mat->data[3][3];
    tmp[1] = mat->data[2][3] * mat->data[3][2];
    tmp[2] = mat->data[2][1] * mat->data[3][3];
    tmp[3] = mat->data[2][3] * mat->data[3][1];
    tmp[4] = mat->data[2][1] * mat->data[3][2];
    tmp[5] = mat->data[2][2] * mat->data[3][1];
    tmp[6] = mat->data[2][0] * mat->data[3][3];
    tmp[7] = mat->data[2][3] * mat->data[3][0];
    tmp[8] = mat->data[2][0] * mat->data[3][2];
    tmp[9] = mat->data[2][2] * mat->data[3][0];
    tmp[10] = mat->data[2][0] * mat->data[3][1];
    tmp[11] = mat->data[2][1] * mat->data[3][0];
    tmp[12] = mat->data[1][2] * mat->data[3][3];
    tmp[13] = mat->data[1][3] * mat->data[3][2];
    tmp[14] = mat->data[1][1] * mat->data[3][3];
    tmp[15] = mat->data[1][3] * mat->data[3][1];
    tmp[16] = mat->data[1][1] * mat->data[3][2];
    tmp[17] = mat->data[1][2] * mat->data[3][1];
    tmp[18] = mat->data[1][0] * mat->data[3][3];
    tmp[19] = mat->data[1][3] * mat->data[3][0];
    tmp[20] = mat->data[1][0] * mat->data[3][2];
    tmp[21] = mat->data[1][2] * mat->data[3][0];
    tmp[22] = mat->data[1][0] * mat->data[3][1];
    tmp[23] = mat->data[1][1] * mat->data[3][0];
    tmp[24] = mat->data[1][2] * mat->data[2][3];
    tmp[25] = mat->data[1][3] * mat->data[2][2];
    tmp[26] = mat->data[1][1] * mat->data[2][3];
    tmp[27] = mat->data[1][3] * mat->data[2][1];
    tmp[28] = mat->data[1][1] * mat->data[2][2];
    tmp[29] = mat->data[1][2] * mat->data[2][1];
    tmp[30] = mat->data[1][0] * mat->data[2][3];
    tmp[31] = mat->data[1][3] * mat->data[2][0];
    tmp[32] = mat->data[1][0] * mat->data[2][2];
    tmp[33] = mat->data[1][2] * mat->data[2][0];
    tmp[34] = mat->data[1][0] * mat->data[2][1];
    tmp[35] = mat->data[1][1] * mat->data[2][0];

    r32 e11 = mat->data[1][1] * (tmp[0] - tmp[1]) - mat->data[1][2] * (tmp[2] - tmp[3]) + mat->data[1][3] * (tmp[4] - tmp[5]);
    r32 e12 = mat->data[1][0] * (tmp[0] - tmp[1]) - mat->data[1][2] * (tmp[6] - tmp[7]) + mat->data[1][3] * (tmp[8] - tmp[9]);
    r32 e13 = mat->data[1][0] * (tmp[2] - tmp[3]) - mat->data[1][1] * (tmp[6] - tmp[7]) + mat->data[1][3] * (tmp[10] - tmp[11]);
    r32 e14 = mat->data[1][0] * (tmp[4] - tmp[5]) - mat->data[1][1] * (tmp[8] - tmp[9]) + mat->data[1][2] * (tmp[10] - tmp[11]);
    r32 e21 = mat->data[0][1] * (tmp[0] - tmp[1]) - mat->data[0][2] * (tmp[2] - tmp[3]) + mat->data[0][3] * (tmp[4] - tmp[5]);
    r32 e22 = mat->data[0][0] * (tmp[0] - tmp[1]) - mat->data[0][2] * (tmp[6] - tmp[7]) + mat->data[0][3] * (tmp[8] - tmp[9]);
    r32 e23 = mat->data[0][0] * (tmp[2] - tmp[3]) - mat->data[0][1] * (tmp[6] - tmp[7]) + mat->data[0][3] * (tmp[10] - tmp[11]);
    r32 e24 = mat->data[0][0] * (tmp[4] - tmp[5]) - mat->data[0][1] * (tmp[8] - tmp[9]) + mat->data[0][2] * (tmp[10] - tmp[11]);
    r32 e31 = mat->data[0][1] * (tmp[12] - tmp[13]) - mat->data[0][2] * (tmp[14] - tmp[15]) + mat->data[0][3] * (tmp[16] - tmp[17]);
    r32 e32 = mat->data[0][0] * (tmp[12] - tmp[13]) - mat->data[0][2] * (tmp[18] - tmp[19]) + mat->data[0][3] * (tmp[20] - tmp[21]);
    r32 e33 = mat->data[0][0] * (tmp[14] - tmp[15]) - mat->data[0][1] * (tmp[18] - tmp[19]) + mat->data[0][3] * (tmp[22] - tmp[23]);
    r32 e34 = mat->data[0][0] * (tmp[16] - tmp[17]) - mat->data[0][1] * (tmp[20] - tmp[21]) + mat->data[0][2] * (tmp[22] - tmp[23]);
    r32 e41 = mat->data[0][1] * (tmp[24] - tmp[25]) - mat->data[0][2] * (tmp[26] - tmp[27]) + mat->data[0][3] * (tmp[28] - tmp[29]);
    r32 e42 = mat->data[0][0] * (tmp[24] - tmp[25]) - mat->data[0][2] * (tmp[30] - tmp[31]) + mat->data[0][3] * (tmp[32] - tmp[33]);
    r32 e43 = mat->data[0][0] * (tmp[26] - tmp[27]) - mat->data[0][1] * (tmp[30] - tmp[31]) + mat->data[0][3] * (tmp[34] - tmp[35]);
    r32 e44 = mat->data[0][0] * (tmp[28] - tmp[29]) - mat->data[0][1] * (tmp[32] - tmp[33]) + mat->data[0][2] * (tmp[34] - tmp[35]);

    inv->data[0][0] = one_over_det * e11;
    inv->data[0][1] = -one_over_det * e21;
    inv->data[0][2] = one_over_det * e31;
    inv->data[0][3] = -one_over_det * e41;

    inv->data[1][0] = -one_over_det * e12;
    inv->data[1][1] = one_over_det * e22;
    inv->data[1][2] = -one_over_det * e32;
    inv->data[1][3] = one_over_det * e42;

    inv->data[2][0] = one_over_det * e13;
    inv->data[2][1] = -one_over_det * e23;
    inv->data[2][2] = one_over_det * e33;
    inv->data[2][3] = -one_over_det * e43;

    inv->data[3][0] = -one_over_det * e14;
    inv->data[3][1] = one_over_det * e24;
    inv->data[3][2] = -one_over_det * e34;
    inv->data[3][3] = one_over_det * e44;
}

__INLINE__ void gl_mat4_adjoint(gc_mat_t *mat, gc_mat_t *out)
{
    r32 tmp[36];

    tmp[0] = mat->data[2][2] * mat->data[3][3];
    tmp[1] = mat->data[2][3] * mat->data[3][2];
    tmp[2] = mat->data[2][1] * mat->data[3][3];
    tmp[3] = mat->data[2][3] * mat->data[3][1];
    tmp[4] = mat->data[2][1] * mat->data[3][2];
    tmp[5] = mat->data[2][2] * mat->data[3][1];
    tmp[6] = mat->data[2][0] * mat->data[3][3];
    tmp[7] = mat->data[2][3] * mat->data[3][0];
    tmp[8] = mat->data[2][0] * mat->data[3][2];
    tmp[9] = mat->data[2][2] * mat->data[3][0];
    tmp[10] = mat->data[2][0] * mat->data[3][1];
    tmp[11] = mat->data[2][1] * mat->data[3][0];
    tmp[12] = mat->data[1][2] * mat->data[3][3];
    tmp[13] = mat->data[1][3] * mat->data[3][2];
    tmp[14] = mat->data[1][1] * mat->data[3][3];
    tmp[15] = mat->data[1][3] * mat->data[3][1];
    tmp[16] = mat->data[1][1] * mat->data[3][2];
    tmp[17] = mat->data[1][2] * mat->data[3][1];
    tmp[18] = mat->data[1][0] * mat->data[3][3];
    tmp[19] = mat->data[1][3] * mat->data[3][0];
    tmp[20] = mat->data[1][0] * mat->data[3][2];
    tmp[21] = mat->data[1][2] * mat->data[3][0];
    tmp[22] = mat->data[1][0] * mat->data[3][1];
    tmp[23] = mat->data[1][1] * mat->data[3][0];
    tmp[24] = mat->data[1][2] * mat->data[2][3];
    tmp[25] = mat->data[1][3] * mat->data[2][2];
    tmp[26] = mat->data[1][1] * mat->data[2][3];
    tmp[27] = mat->data[1][3] * mat->data[2][1];
    tmp[28] = mat->data[1][1] * mat->data[2][2];
    tmp[29] = mat->data[1][2] * mat->data[2][1];
    tmp[30] = mat->data[1][0] * mat->data[2][3];
    tmp[31] = mat->data[1][3] * mat->data[2][0];
    tmp[32] = mat->data[1][0] * mat->data[2][2];
    tmp[33] = mat->data[1][2] * mat->data[2][0];
    tmp[34] = mat->data[1][0] * mat->data[2][1];
    tmp[35] = mat->data[1][1] * mat->data[2][0];

    r32 e11 = mat->data[1][1] * (tmp[0] - tmp[1]) - mat->data[1][2] * (tmp[2] - tmp[3]) + mat->data[1][3] * (tmp[4] - tmp[5]);
    r32 e12 = mat->data[1][0] * (tmp[0] - tmp[1]) - mat->data[1][2] * (tmp[6] - tmp[7]) + mat->data[1][3] * (tmp[8] - tmp[9]);
    r32 e13 = mat->data[1][0] * (tmp[2] - tmp[3]) - mat->data[1][1] * (tmp[6] - tmp[7]) + mat->data[1][3] * (tmp[10] - tmp[11]);
    r32 e14 = mat->data[1][0] * (tmp[4] - tmp[5]) - mat->data[1][1] * (tmp[8] - tmp[9]) + mat->data[1][2] * (tmp[10] - tmp[11]);
    r32 e21 = mat->data[0][1] * (tmp[0] - tmp[1]) - mat->data[0][2] * (tmp[2] - tmp[3]) + mat->data[0][3] * (tmp[4] - tmp[5]);
    r32 e22 = mat->data[0][0] * (tmp[0] - tmp[1]) - mat->data[0][2] * (tmp[6] - tmp[7]) + mat->data[0][3] * (tmp[8] - tmp[9]);
    r32 e23 = mat->data[0][0] * (tmp[2] - tmp[3]) - mat->data[0][1] * (tmp[6] - tmp[7]) + mat->data[0][3] * (tmp[10] - tmp[11]);
    r32 e24 = mat->data[0][0] * (tmp[4] - tmp[5]) - mat->data[0][1] * (tmp[8] - tmp[9]) + mat->data[0][2] * (tmp[10] - tmp[11]);
    r32 e31 = mat->data[0][1] * (tmp[12] - tmp[13]) - mat->data[0][2] * (tmp[14] - tmp[15]) + mat->data[0][3] * (tmp[16] - tmp[17]);
    r32 e32 = mat->data[0][0] * (tmp[12] - tmp[13]) - mat->data[0][2] * (tmp[18] - tmp[19]) + mat->data[0][3] * (tmp[20] - tmp[21]);
    r32 e33 = mat->data[0][0] * (tmp[14] - tmp[15]) - mat->data[0][1] * (tmp[18] - tmp[19]) + mat->data[0][3] * (tmp[22] - tmp[23]);
    r32 e34 = mat->data[0][0] * (tmp[16] - tmp[17]) - mat->data[0][1] * (tmp[20] - tmp[21]) + mat->data[0][2] * (tmp[22] - tmp[23]);
    r32 e41 = mat->data[0][1] * (tmp[24] - tmp[25]) - mat->data[0][2] * (tmp[26] - tmp[27]) + mat->data[0][3] * (tmp[28] - tmp[29]);
    r32 e42 = mat->data[0][0] * (tmp[24] - tmp[25]) - mat->data[0][2] * (tmp[30] - tmp[31]) + mat->data[0][3] * (tmp[32] - tmp[33]);
    r32 e43 = mat->data[0][0] * (tmp[26] - tmp[27]) - mat->data[0][1] * (tmp[30] - tmp[31]) + mat->data[0][3] * (tmp[34] - tmp[35]);
    r32 e44 = mat->data[0][0] * (tmp[28] - tmp[29]) - mat->data[0][1] * (tmp[32] - tmp[33]) + mat->data[0][2] * (tmp[34] - tmp[35]);

    out->data[0][0] = e11;
    out->data[0][1] = -e21;
    out->data[0][2] = e31;
    out->data[0][3] = -e41;

    out->data[1][0] = -e12;
    out->data[1][1] = e22;
    out->data[1][2] = -e32;
    out->data[1][3] = e42;

    out->data[2][0] = e13;
    out->data[2][1] = -e23;
    out->data[2][2] = e33;
    out->data[2][3] = -e43;

    out->data[3][0] = -e14;
    out->data[3][1] = e24;
    out->data[3][2] = -e34;
    out->data[3][3] = e44;
}

// ----------------------------------------------------------------------------------
// -- Transformations.
// ----------------------------------------------------------------------------------

__INLINE__ void gl_mat3_translation(r32 x, r32 y, gc_mat_t *out)
{
    gl_mat_identity(out);

    out->data[0][2] = x;
    out->data[1][2] = y;
}

__INLINE__ void gl_mat3_translation_vec(gc_vec_t *v, gc_mat_t *out)
{
    gl_mat_identity(out);

    out->data[0][2] = v->data[0];
    out->data[1][2] = v->data[1];
}

__INLINE__ void gl_mat4_translation(r32 x, r32 y, r32 z, gc_mat_t *out)
{
    gl_mat_identity(out);

    out->data[0][3] = x;
    out->data[1][3] = y;
    out->data[2][3] = z;
}

__INLINE__ void gl_mat4_translation_vec(gc_vec_t *v, gc_mat_t *out)
{
    gl_mat_identity(out);

    out->data[0][3] = v->data[0];
    out->data[1][3] = v->data[1];
    out->data[2][3] = v->data[2];
}

__INLINE__ void gl_mat3_rotation(r32 angle, gc_mat_t *out)
{
    gl_mat_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->data[0][0] = cos_t;
    out->data[0][1] = -sin_t;
    out->data[1][0] = sin_t;
    out->data[1][1] = cos_t;
}

__INLINE__ void gl_mat3_rotation_around_point(gc_vec_t *point, r32 angle, gc_mat_t *out)
{
    gl_mat_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->data[0][0] = cos_t;
    out->data[0][1] = -sin_t;
    out->data[1][0] = sin_t;
    out->data[1][1] = cos_t;

    r32 tmp0 = 1 - out->data[0][0];
    r32 tmp1 = 0 - out->data[0][1];
    r32 tmp2 = 0 - out->data[0][0];
    r32 tmp3 = 1 - out->data[0][1];

    out->data[0][2] = tmp0 * point->data[0] + tmp1 * point->data[1];
    out->data[0][2] = tmp2 * point->data[0] + tmp3 * point->data[1];
}

__INLINE__ void gl_mat4_rotation_x(r32 angle, gc_mat_t *out)
{
    gl_mat_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->data[1][1] = cos_t;
    out->data[1][2] = -sin_t;
    out->data[2][1] = sin_t;
    out->data[2][2] = cos_t;
}

__INLINE__ void gl_mat4_rotation_y(r32 angle, gc_mat_t *out)
{
    gl_mat_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->data[0][0] = cos_t;
    // out->data[1][0] = 0;
    out->data[2][0] = -sin_t;

    // out->data[0][1] = 0;
    // out->data[1][1] = 1;
    // out->data[2][1] = 0;

    out->data[0][2] = sin_t;
    // out->data[1][2] = 0;
    out->data[2][2] = cos_t;
}

__INLINE__ void gl_mat4_rotation_z(r32 angle, gc_mat_t *out)
{
    gl_mat_identity(out);

    r32 cos_t = cosf(angle);
    r32 sin_t = sinf(angle);

    out->data[0][0] = cos_t;
    out->data[1][0] = sin_t;
    // out->data[2][0] = 0;

    out->data[0][1] = -sin_t;
    out->data[1][1] = cos_t;
    // out->data[2][1] = 0;

    // out->data[0][2] = 0;
    // out->data[1][2] = 0;
    // out->data[2][2] = 1;
}

__INLINE__ void gl_mat4_rotation_around_axis(gc_vec_t *r, r32 angle, gc_mat_t *out)
{
    gl_mat_identity(out);

    r32 c = cosf(angle);
    r32 s = sinf(angle);
    r32 t = 1 - c;

    out->data[0][0] = t * r->data[0] * r->data[0] + c;
    out->data[0][1] = t * r->data[0] * r->data[1] - s * r->data[2];
    out->data[0][2] = t * r->data[0] * r->data[2] + s * r->data[1];
    out->data[1][0] = t * r->data[0] * r->data[1] + s * r->data[2];
    out->data[1][1] = t * r->data[1] * r->data[1] + c;
    out->data[1][2] = t * r->data[1] * r->data[2] - s * r->data[0];
    out->data[2][0] = t * r->data[0] * r->data[2] - s * r->data[1];
    out->data[2][1] = t * r->data[1] * r->data[2] + s * r->data[0];
    out->data[2][2] = t * r->data[2] * r->data[2] + c;
}

__INLINE__ void gl_mat4_rotation_around_point(gc_vec_t *point, gc_mat_t *rotation, gc_mat_t *out)
{
    gc_mat_t tmp;
    gl_mat_identity(&tmp);

    tmp.data[0][0] = rotation->data[0][0];
    tmp.data[0][1] = rotation->data[0][1];
    tmp.data[0][2] = rotation->data[0][2];
    tmp.data[0][3] = (1 - rotation->data[0][0]) * point->data[0] - rotation->data[0][1] * point->data[1] - rotation->data[0][2] * point->data[2];

    tmp.data[1][0] = rotation->data[1][0];
    tmp.data[1][1] = rotation->data[1][1];
    tmp.data[1][2] = rotation->data[1][2];
    tmp.data[1][3] = - rotation->data[1][0] * point->data[0] + (1 - rotation->data[1][1]) * point->data[1] - rotation->data[1][2] * point->data[2];

    tmp.data[2][0] = rotation->data[2][0];
    tmp.data[2][1] = rotation->data[2][1];
    tmp.data[2][2] = rotation->data[2][2];
    tmp.data[2][3] = - rotation->data[2][0] * point->data[0] - rotation->data[2][1] * point->data[1] + (1 - rotation->data[2][2]) * point->data[2];

    gl_mat4_copy(out, &tmp);
}

__INLINE__ void gl_mat3_scale(r32 sx, r32 sy, gc_mat_t *out)
{
    gl_mat_identity(out);

    out->data[0][0] = sx;
    out->data[1][1] = sy;
}

__INLINE__ void gl_mat4_scale(r32 sx, r32 sy, r32 sz, gc_mat_t *out)
{
    gl_mat_identity(out);

    out->data[0][0] = sx;
    out->data[1][1] = sy;
    out->data[2][2] = sz;
}

__INLINE__ void gl_mat4_scale_vec(gc_vec_t *vec, gc_mat_t *out)
{
    gl_mat_identity(out);

    out->data[0][0] = vec->data[0];
    out->data[1][1] = vec->data[1];
    out->data[2][2] = vec->data[2];
}

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
__INLINE__ void gc_mat4_perspective(r32 aspect, r32 fov, r32 f_near, r32 f_far, gc_mat_t *out)
{
    gl_mat_empty(out);

    r32 d = 1.0f / tan(fov * 0.5f);
    r32 s = f_near - f_far;

    out->data[0][0] = d / aspect;
    out->data[1][1] = d;
    out->data[2][2] = (f_near + f_far) / s;
    out->data[3][2] = -1;
    out->data[2][3] = 2 * f_near * f_far / s;
}

__INLINE__ void gc_mat4_orthographic(r32 f_right, r32 f_left, r32 f_top, r32 f_bottom, r32 f_near, r32 f_far, gc_mat_t *out)
{
    gl_mat_empty(out);

    r32 rml = f_right - f_left;
    r32 fmn = f_far - f_near;
    r32 tmb = f_top - f_bottom;

    out->data[0][0] = 2 / rml;
    out->data[1][1] = 2 / tmb;
    out->data[2][2] = -2 / fmn;
    out->data[3][3] = 1;
    out->data[0][3] = -(f_right + f_left) / rml;
    out->data[1][3] = -(f_top + f_bottom) / tmb;
    out->data[2][3] = -(f_far + f_near) / fmn;
}

__INLINE__ void gc_mat4_viewport(r32 ws, r32 hs, r32 sx, r32 sy, r32 ds, gc_mat_t *out)
{
    gl_mat_empty(out);

    r32 wh = ws / 2.0f;
    r32 hh = hs / 2.0f;
    r32 dh = ds / 2.0f;

    out->data[0][0] = wh;
    out->data[0][3] = wh + sx;
    out->data[1][1] = -hh;
    out->data[1][3] = hh + sy;
    out->data[2][2] = dh;
    out->data[2][3] = dh;
    out->data[3][3] = 1;
}

// Computes the pitch angle relative to the -oz axis and
// the heading angle relative to the oy axis (camera orientation).
gc_euler_xyz_t v3_polar_angles_basic(gc_vec_t *v)
{
    gc_euler_xyz_t res;

    r32 p = sqrtf(v->data[0] * v->data[0] + v->data[1] * v->data[1]);

    res.x = 0;
    res.y = RAD2DEG(atan2(p, -v->data[2]));
    res.z = RAD2DEG(atan2(v->data[0], v->data[1]));

    return res;
};

// Computes the pitch angle relative to the oz axis and
// the heading angle relative to the -oy axis (camera orientation).
gc_euler_xyz_t v3_polar_angles_focus(gc_vec_t *v)
{
    gc_euler_xyz_t res;

    r32 p = sqrtf(v->data[0] * v->data[0] + v->data[1] * v->data[1]);

    res.x = 0;
    res.y = RAD2DEG(atan2(p, v->data[2]));
    res.z = RAD2DEG(atan2(v->data[0], -v->data[1]));

    return res;
};

// Computes the pitch angle relative to the ox axis
// (positive below the axis, negative above).
gc_euler_xyz_t v3_polar_angles(gc_vec_t *v)
{
    gc_euler_xyz_t res;

    r32 p = sqrtf(v->data[0] * v->data[0] + v->data[1] * v->data[1]);

    res.x = 0;
    res.y = -RAD2DEG(atan2(v->data[2], p));
    res.z = RAD2DEG(atan2(v->data[1], v->data[0]));

    return res;
};

__INLINE__ void gc_mat4_lookat(gc_vec_t *eye, gc_vec_t *target, gc_vec_t *up, gc_mat_t *out)
{
    gl_mat_identity(out);

    gc_vec_t z_axis;
    gc_vec_t x_axis;
    gc_vec_t y_axis;

    gl_vec3_sub(eye, target, &z_axis);
    v3_normalize(&z_axis);

    v3_cross(up, &z_axis, &x_axis);
    v3_normalize(&x_axis);

    v3_cross(&z_axis, &x_axis, &y_axis);
    v3_normalize(&y_axis);

    // orthogonal axis => inverse = transpose.
    out->data[0][0] = x_axis.v3.x;
    out->data[0][1] = x_axis.v3.y;
    out->data[0][2] = x_axis.v3.z;

    out->data[1][0] = y_axis.v3.x;
    out->data[1][1] = y_axis.v3.y;
    out->data[1][2] = y_axis.v3.z;

    out->data[2][0] = z_axis.v3.x;
    out->data[2][1] = z_axis.v3.y;
    out->data[2][2] = z_axis.v3.z;

    out->data[0][3] = -v3_dot(&x_axis, eye);
    out->data[1][3] = -v3_dot(&y_axis, eye);
    out->data[2][3] = -v3_dot(&z_axis, eye);
}

#define gl_viewspace(mat, vertex, out) \
{ \
    out.data[0] = mat->data[0][0] * vertex->pos[0] + mat->data[0][1] * vertex->pos[1] + mat->data[0][2] * vertex->pos[2] + mat->data[0][3]; \
    out.data[1] = mat->data[1][0] * vertex->pos[0] + mat->data[1][1] * vertex->pos[1] + mat->data[1][2] * vertex->pos[2] + mat->data[1][3]; \
    out.data[2] = mat->data[2][0] * vertex->pos[0] + mat->data[2][1] * vertex->pos[1] + mat->data[2][2] * vertex->pos[2] + mat->data[2][3]; \
    out.data[3] = 1; \
}

#define gl_viewspace_vec(mat, vec, out) \
{ \
    out.data[0] = mat->data[0][0] * vec->pos[0] + mat->data[0][1] * vec->pos[1] + mat->data[0][2] * vec->pos[2]; \
    out.data[1] = mat->data[1][0] * vec->pos[0] + mat->data[1][1] * vec->pos[1] + mat->data[1][2] * vec->pos[2]; \
    out.data[2] = mat->data[2][0] * vec->pos[0] + mat->data[2][1] * vec->pos[1] + mat->data[2][2] * vec->pos[2]; \
    out.data[3] = 0; \
}

#define gl_project(projection, point) \
{ \
    r32 _tmpz_ = point.data[2]; \
    point.data[0] *= projection->data[0][0]; \
    point.data[1] *= projection->data[1][1]; \
    point.data[2] = projection->data[2][2] * _tmpz_ + projection->data[2][3]; \
    point.data[3] = projection->data[3][2] * _tmpz_ + projection->data[3][3] * point.data[3]; \
}

#define GL_MIX(v1, v2, t) ((v1) + ((v2) - (v1)) * t)
#define GL_V3_MIX(v1, v2, t) { \
    GL_MIX((v1).v3.x, (v2).v3.x, t), \
    GL_MIX((v1).v3.y, (v2).v3.y, t), \
    GL_MIX((v1).v3.z, (v2).v3.z, t) \
}

#endif