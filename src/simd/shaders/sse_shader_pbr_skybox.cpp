// ----------------------------------------------------------------------------------
// -- File: sse_shader_pbr_skybox.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-17 21:16:55
// -- Modified: 2022-10-17 21:16:56
// ----------------------------------------------------------------------------------

void _sse_shader_pbr_skybox_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_sse_shader_pbr_skybox_fs");

    gc_processed_fragment_t *fragment = 0;
    shader_color_t tmp_color;
    cube_texture_t *environment = (cube_texture_t *) params->textures[0];

    lod_t lod;
    LOD_CLEAR(lod);

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        fv3_t *position = (fv3_t *) fragment->varyings[2];

        if (environment)
        {
            gl_sse_cubemap_compute_lod(environment, fragment, &lod);
            sse_cube_texture_sample(environment, position, &lod, false, &tmp_color);

            _mm_store_ps(fragment->r, _mm_load_ps(tmp_color.r));
            _mm_store_ps(fragment->g, _mm_load_ps(tmp_color.g));
            _mm_store_ps(fragment->b, _mm_load_ps(tmp_color.b));
            _mm_store_ps(fragment->a, _mm_set1_ps(1.0f));
        }
    }
}