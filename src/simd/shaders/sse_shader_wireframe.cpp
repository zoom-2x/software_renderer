// ----------------------------------------------------------------------------------
// -- File: sse_shader_wireframe.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-17 09:31:28
// -- Modified: 2022-10-17 09:31:29
// ----------------------------------------------------------------------------------

void _sse_shader_wireframe_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_sse_shader_wireframe_fs");

    gc_processed_fragment_t *fragment = 0;
    sse_color_t computed_color;

    computed_color.r = _mm_set1_ps(PIPE_PARAM_VECTOR(2, wireframe_color, 0));
    computed_color.g = _mm_set1_ps(PIPE_PARAM_VECTOR(2, wireframe_color, 1));
    computed_color.b = _mm_set1_ps(PIPE_PARAM_VECTOR(2, wireframe_color, 2));
    computed_color.a = _mm_set1_ps(PIPE_PARAM_VECTOR(2, wireframe_color, 3));

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        _mm_store_ps(fragment->r, computed_color.r);
        _mm_store_ps(fragment->g, computed_color.g);
        _mm_store_ps(fragment->b, computed_color.b);
        _mm_store_ps(fragment->a, computed_color.a);
    }
}