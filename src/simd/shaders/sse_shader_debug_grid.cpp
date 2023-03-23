// ----------------------------------------------------------------------------------
// -- File: sse_shader_debug_grid.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-11-12 14:34:41
// -- Modified: 2022-11-12 14:34:44
// ----------------------------------------------------------------------------------

void _sse_shader_debug_grid_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        _mm_store_ps(fragment->r, _mm_set1_ps(fragment->primitive->base.data[0]));
        _mm_store_ps(fragment->g, _mm_set1_ps(fragment->primitive->base.data[1]));
        _mm_store_ps(fragment->b, _mm_set1_ps(fragment->primitive->base.data[2]));
        _mm_store_ps(fragment->a, _mm_set1_ps(fragment->primitive->base.data[3]));
    }
}