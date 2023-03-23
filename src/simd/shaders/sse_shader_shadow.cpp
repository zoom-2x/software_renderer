// ----------------------------------------------------------------------------------
// -- File: sse_shader_shadow.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-12 19:19:38
// -- Modified: 2022-10-12 19:19:39
// ----------------------------------------------------------------------------------

void _sse_shader_shadow_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        gc_processed_fragment_t *fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        __m128 depth = _mm_load_ps(fragment->z);
        __m128 double_4x = _mm_mul_ps(depth, depth);

        _mm_store_ps(fragment->r, depth);
        _mm_store_ps(fragment->g, double_4x);
        _mm_store_ps(fragment->b, _mm_setzero_ps());
        _mm_store_ps(fragment->a, _mm_setzero_ps());
    }
}