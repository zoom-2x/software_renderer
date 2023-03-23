// ----------------------------------------------------------------------------------
// -- File: sse_shader_point.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-13 12:42:38
// -- Modified: 2022-12-13 12:42:39
// ----------------------------------------------------------------------------------

void _sse_shader_point_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_shader_point_fs");

    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        _mm_store_ps(fragment->r, _mm_set1_ps(1));
        _mm_store_ps(fragment->g, _mm_set1_ps(1));
        _mm_store_ps(fragment->b, _mm_set1_ps(1));
        _mm_store_ps(fragment->a, _mm_set1_ps(1));
    }
}