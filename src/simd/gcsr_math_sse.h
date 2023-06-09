// ----------------------------------------------------------------------------------
// -- File: gcsr_math_sse.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C
// -- Description:
// -- Created: 2021-12-27 14:26:11
// -- Modified:
// ----------------------------------------------------------------------------------

#define vec3_dot_sse(v1_x, v1_y, v1_z, v2_x, v2_y, v2_z) \
    _mm_add_ps( \
        _mm_add_ps( \
            _mm_mul_ps(v1_x, v2_x), \
            _mm_mul_ps(v1_y, v2_y)), \
        _mm_mul_ps(v1_z, v2_z))
