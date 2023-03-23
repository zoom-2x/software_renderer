// ----------------------------------------------------------------------------------
// -- FLAT TEXTURE SHADER (SSE).
// ----------------------------------------------------------------------------------

void _sse_shader_flat_texture_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_sse_shader_flat_texture_fs");

    gc_processed_fragment_t *fragment = 0;
    shader_color_t diffuse_color;

    lod_t lod;
    LOD_CLEAR(lod);

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        fv2_t *texcoord = (fv2_t *) fragment->varyings[0];
        sse_color_t fragment_color;

        if (params->shader_flags & SHADER_FLAG_BLINN_DIFFUSE)
        {
            // gc_texture_compute_lod((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[0], fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod, &diffuse_color);
            SSE_GAMMA_SRGB_TO_LINEAR(diffuse_color);
        }

        fragment_color.r = _mm_load_ps(diffuse_color.r);
        fragment_color.g = _mm_load_ps(diffuse_color.g);
        fragment_color.b = _mm_load_ps(diffuse_color.b);
        fragment_color.a = _mm_load_ps(diffuse_color.a);

        _mm_store_ps(fragment->r, fragment_color.r);
        _mm_store_ps(fragment->g, fragment_color.g);
        _mm_store_ps(fragment->b, fragment_color.b);
        _mm_store_ps(fragment->a, fragment_color.a);
    }
}