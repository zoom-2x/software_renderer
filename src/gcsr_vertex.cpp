// ----------------------------------------------------------------------------------
// -- File: gcsr_vertex.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description: Vertex operations.
// -- Created: 2021-03-27 11:30:33
// -- Modified: 2021-12-19 13:37:27
// ----------------------------------------------------------------------------------

__INLINE__ void gl_copy_vertex(gc_vertex_t *dest, gc_vertex_t *src)
{
    platform_api_t *API = get_platform_api();
    API->mem_copy(dest, src, sizeof(gc_vertex_t), sizeof(gc_vertex_t));
}

#if defined(GC_PIPE_SSE)
__INLINE__ void gl_interpolate_buffer(gc_clipping_vertex_buffer_t *buffer,
                                  u32 dest_idx, u32 current_idx, u32 next_idx,
                                  r32 t, u32 interpolation_components)
{
    gc_vertex_t *dest = &buffer->data[dest_idx];
    gc_vertex_t *current = &buffer->data[current_idx];
    gc_vertex_t *next = &buffer->data[next_idx];

    u32 packs = ((4 + interpolation_components) >> 2) + 1;
    __m128 t_4x = _mm_set1_ps(t);

    // -- Position.

    __m128 current_pos_4x = _mm_load_ps(current->pos);
    __m128 next_pos_4x = _mm_load_ps(next->pos);
    __m128 interp_pos_4x =  _mm_add_ps(current_pos_4x, _mm_mul_ps( _mm_sub_ps(next_pos_4x, current_pos_4x), t_4x));
    _mm_store_ps(dest->pos, interp_pos_4x);

    // -- Attributes.

    for (u32 i = 0; i < packs; ++i)
    {
        u32 offset = i * GC_FRAG_SIZE;
        r32 *current_addr = &current->data[offset];
        r32 *next_addr = &next->data[offset];
        r32 *dest_addr = &dest->data[offset];

        __m128 current_4x = _mm_load_ps(current_addr);
        __m128 next_4x = _mm_load_ps(next_addr);
        __m128 dest_4x = _mm_add_ps(
                            current_4x,
                            _mm_mul_ps(
                                _mm_sub_ps(next_4x, current_4x),
                                t_4x));

        _mm_store_ps(dest_addr, dest_4x);
    }
}
#else
__INLINE__ void gl_interpolate_buffer(gc_clipping_vertex_buffer_t *buffer,
                                  u32 dest_idx, u32 current_idx, u32 next_idx,
                                  r32 t, u32 interpolation_components)
{
    gc_vertex_t *dest = &buffer->data[dest_idx];
    gc_vertex_t *current = &buffer->data[current_idx];
    gc_vertex_t *next = &buffer->data[next_idx];

    // The "position" + "w" is also interpolated.
#if 1
    s64 tt = FP_FROM_REAL(t);
    s64 start = FP_FROM_REAL(current->pos[0]);
    s64 end = FP_FROM_REAL(next->pos[0]);

    s64 result = (start << 8) + (end - start) * tt;
    dest->pos[0] = result * FP_ONE_OVER_2DEC_VAL;

    start = FP_FROM_REAL(current->pos[1]);
    end = FP_FROM_REAL(next->pos[1]);

    result = (start << 8) + (end - start) * tt;
    dest->pos[1] = result * FP_ONE_OVER_2DEC_VAL;

    start = FP_FROM_REAL(current->pos[2]);
    end = FP_FROM_REAL(next->pos[2]);

    result = (start << 8) + (end - start) * tt;
    dest->pos[2] = result * FP_ONE_OVER_2DEC_VAL;

    start = FP_FROM_REAL(current->pos[3]);
    end = FP_FROM_REAL(next->pos[3]);

    result = (start << 8) + (end - start) * tt;
    dest->pos[3] = result * FP_ONE_OVER_2DEC_VAL;
#else
    dest->pos[0] = current->pos[0] + (next->pos[0] - current->pos[0]) * t;
    dest->pos[1] = current->pos[1] + (next->pos[1] - current->pos[1]) * t;
    dest->pos[2] = current->pos[2] + (next->pos[2] - current->pos[2]) * t;
    dest->pos[3] = current->pos[3] + (next->pos[3] - current->pos[3]) * t;
#endif
    for (u32 i = 0; i < interpolation_components; ++i) {
        dest->data[i] = current->data[i] + (next->data[i] - current->data[i]) * t;
    }
}
#endif

// ----------------------------------------------------------------------------------
// -- The coordinates are in clip-space.
// ----------------------------------------------------------------------------------

__INLINE__ u8 gl_compute_clip_codes(gc_vertex_t *vertex)
{
    OPTICK_EVENT("gl_compute_clip_codes");

    u8 codes = 0;

    // -w <= x <= w
    // -w <= y <= w
    // -w <= z <= w

    // x.
    if (vertex->pos[0] < -vertex->pos[3])
        codes |= CLIP_LEFT;
    else if (vertex->pos[0] > vertex->pos[3])
        codes |= CLIP_RIGHT;

    // y.
    if (vertex->pos[1] < -vertex->pos[3])
        codes |= CLIP_BOTTOM;
    else if (vertex->pos[1] > vertex->pos[3])
        codes |= CLIP_TOP;

    // z.
    if (vertex->pos[2] < -vertex->pos[3])
        codes |= CLIP_NEAR;
    else if (vertex->pos[2] > vertex->pos[3])
        codes |= CLIP_FAR;

    return codes;
}

#if defined(GC_PIPE_SSE)
__INLINE__ void mat4_perspective_division_vertex(gc_vertex_t *vertex, u32 interpolation_components)
{
    OPTICK_EVENT("mat4_perspective_division_vertex");

    r32 one_over_w = 1.0f / vertex->pos[3];
    __m128 one_over_w_4x = _mm_set1_ps(one_over_w);

    vertex->pos[0] *= one_over_w;
    vertex->pos[1] *= one_over_w;
    vertex->pos[2] *= one_over_w;
    vertex->pos[3] = one_over_w;

    u32 packs = ((4 + interpolation_components) >> 2) + 1;

    for (u32 i = 0; i < packs; ++i)
    {
        u32 offset = i * GC_FRAG_SIZE;
        r32 *addr = &vertex->data[offset];
        __m128 tmp_4x = _mm_load_ps(addr);
        tmp_4x = _mm_mul_ps(tmp_4x, one_over_w_4x);
        _mm_store_ps(addr, tmp_4x);
    }

    // vertex->data[3] = one_over_w;
}
#else
__INLINE__ void mat4_perspective_division_vertex(gc_vertex_t *vertex, u32 interpolation_components)
{
    OPTICK_EVENT("mat4_perspective_division_vertex");

    r32 one_over_w = 1.0f / vertex->pos[3];

    vertex->pos[0] *= one_over_w;
    vertex->pos[1] *= one_over_w;
    vertex->pos[2] *= one_over_w;
    vertex->pos[3] = one_over_w;

    for (u32 i = 0; i < interpolation_components; ++i) {
        vertex->data[i] *= one_over_w;
    }
}
#endif