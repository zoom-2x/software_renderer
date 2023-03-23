#ifndef GCSR_ENGINE_INTRINSICS_H
#define GCSR_ENGINE_INTRINSICS_H

#include <math.h>
#include <stdlib.h>

__INLINE__ u32 absS32(s32 v)
{
#if 0
    u32 res = 0;

    if (v < 0)
        res = (u32) (v * -1);
    else
        res = v;

    return res;
#else
    return abs(v);
#endif
}

__INLINE__ r32 absR32(r32 v)
{
    r32 res = (r32) fabs(v);

    return res;
}

__INLINE__ u32 absDiff(s32 v1, s32 v2)
{
    u32 res = abs(v1 - v2);
    return res;
}

__INLINE__ r32 square(r32 v)
{
    r32 result = v * v;
    return result;
}

__INLINE__ r32 square_root(r32 value)
{
    r32 result = sqrtf(value);
    return result;
}

// inaccurate.
__INLINE__ double fastPow(double a, double b)
{
    union _tmp {
        double d;
        int x[2];
    } u = { a };

    u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
    u.x[0] = 0;

    return u.d;
}

__INLINE__ r32 power(r32 a,  r32 b)
{
    // r32 result = (r32) powf(a, b);
    r32 result = (r32) fastPow(a, b);
    return result;
}

__INLINE__ s32 roundR32ToS32(r32 value)
{
    s32 result = (s32) roundf(value);
    return result;
}

__INLINE__ u32 roundR32ToU32(r32 value)
{
    u32 result = (u32) roundf(value);
    return result;
}

__INLINE__ r32 t_sin(r32 angle) {
    return sinf(angle);
}

__INLINE__ r32 t_cos(r32 angle)
{
    r32 result = cosf(angle);
    return result;
}

__INLINE__ r32 t_tan(r32 angle)
{
    r32 result = (r32) tan(angle);
    return result;
}

__INLINE__ r32 min2R32(r32 a, r32 b) {
    return a <= b ? a : b;
}

__INLINE__ r32 max2R32(r32 a, r32 b) {
    return a >= b ? a : b;
}

__INLINE__ r32 min3R32(r32 a, r32 b, r32 c)
{
    r32 res = a <= b ? a : b;
    res = res <= c ? res : c;
    return res;
}

__INLINE__ r32 max3R32(r32 a, r32 b, r32 c)
{
    r32 res = a >= b ? a : b;
    res = res >= c ? res : c;
    return res;
}

#endif