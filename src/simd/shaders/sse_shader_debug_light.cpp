// ----------------------------------------------------------------------------------
// -- File: sse_shader_debug_light.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-11-11 20:08:49
// -- Modified: 2022-11-11 20:08:50
// ----------------------------------------------------------------------------------

void _sse_shader_debug_light_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;
    gc_blinn_material_t *material = &params->material->blinn;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        _mm_store_ps(fragment->r, _mm_set1_ps(material->diffuse.data[0]));
        _mm_store_ps(fragment->g, _mm_set1_ps(material->diffuse.data[1]));
        _mm_store_ps(fragment->b, _mm_set1_ps(material->diffuse.data[2]));
        _mm_store_ps(fragment->a, _mm_set1_ps(1.0f));
    }
}