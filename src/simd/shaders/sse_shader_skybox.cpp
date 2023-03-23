// ----------------------------------------------------------------------------------
// -- File: sse_shader_skybox.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-17 11:10:00
// -- Modified: 2022-10-17 11:10:01
// ----------------------------------------------------------------------------------

void shader_skybox_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;
    shader_color_t tmp_color;
    gc_light_t *light = params->lights;

    lod_t lod;
    LOD_CLEAR(lod);

    // __m128 il = _mm_set1_ps(light->il);

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        fv3_t *position = (fv3_t *) fragment->varyings[0];
        sse_cube_texture_sample((cube_texture_t *) params->textures[0], position, &lod, false, &tmp_color);
        SSE_GAMMA_SRGB_TO_LINEAR(&tmp_color);

        __m128 cube_sample_r = _mm_load_ps(tmp_color.r);
        __m128 cube_sample_g = _mm_load_ps(tmp_color.g);
        __m128 cube_sample_b = _mm_load_ps(tmp_color.b);

#if 0
        _mm_store_ps(fragment->r, _mm_mul_ps(cube_sample_r, il));
        _mm_store_ps(fragment->g, _mm_mul_ps(cube_sample_g, il));
        _mm_store_ps(fragment->b, _mm_mul_ps(cube_sample_b, il));
#else
        _mm_store_ps(fragment->r, cube_sample_r);
        _mm_store_ps(fragment->g, cube_sample_g);
        _mm_store_ps(fragment->b, cube_sample_b);
#endif
        _mm_store_ps(fragment->a, _mm_set1_ps(1.0f));
    }
}