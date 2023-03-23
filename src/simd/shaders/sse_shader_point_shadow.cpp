// ----------------------------------------------------------------------------------
// -- File: sse_shader_point_shadow.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-14 10:01:25
// -- Modified: 2022-10-14 10:01:25
// ----------------------------------------------------------------------------------

void _sse_shader_point_shadow_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_light_t *light = params->current_light;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        gc_processed_fragment_t *fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        sse_v3_t direction;

        direction.x = _mm_load_ps(fragment->varyings[2]);
        direction.y = _mm_load_ps(fragment->varyings[3]);
        direction.z = _mm_load_ps(fragment->varyings[4]);

        direction.x = _mm_sub_ps(direction.x, _mm_set1_ps(light->object.position.v3.x));
        direction.y = _mm_sub_ps(direction.y, _mm_set1_ps(light->object.position.v3.y));
        direction.z = _mm_sub_ps(direction.z, _mm_set1_ps(light->object.position.v3.z));

        __m128 distance_4x = sse_v3_len(&direction);
        distance_4x = _mm_mul_ps(distance_4x, _mm_set1_ps(light->shadow.f_len_inv));
        __m128 distance2_4x = _mm_mul_ps(distance_4x, distance_4x);

        _mm_store_ps(fragment->r, distance_4x);
        _mm_store_ps(fragment->g, distance2_4x);
        _mm_store_ps(fragment->b, _mm_setzero_ps());
        _mm_store_ps(fragment->a, _mm_setzero_ps());
    }
}