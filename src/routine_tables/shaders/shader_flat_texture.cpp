// ----------------------------------------------------------------------------------
// -- FLAT TEXTURE SHADER.
// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

r32 mip_colors[][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 0.55f, 0.8f},
    {0.39f, 0.0f, 0.8f},
    {0.8f, 0.0f, 0.74f},
    {0.8f, 0.55f, 0.0f},
    {0.94f, 0.94f, 0.0f},
    {0.37f, 0.83f, 0.0f},
    {1.0f, 0.22f, 0.7f},
    {0.81f, 0.68f, 1.0f},
};

// #define MIP_COLORS 1

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_flat_texture.cpp"
#define shader_flat_texture_fs _avx_shader_flat_texture_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_flat_texture.cpp"
#define shader_flat_texture_fs _sse_shader_flat_texture_fs
#else
#define shader_flat_texture_fs _shader_flat_texture_fs
#endif

void shader_flat_texture_setup(gc_material_t *material)
{
    GCSR.gl->pipeline.params.material = material;
    GCSR.gl->pipeline.params.shader_flags = 0;

    if (material->blinn.diffuse_map && FLAG(material->components, SHADER_FLAG_BLINN_DIFFUSE))
    {
        GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_DIFFUSE;
        GCSR.gl->pipeline.params.textures[0] = material->blinn.diffuse_map;
    }
}

void shader_flat_texture_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    OPTICK_EVENT("shader_flat_texture_vs");

    gc_vec_t gl_position;

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_flat_texture_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_shader_flat_texture_fs");

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

        if (params->shader_flags & SHADER_FLAG_BLINN_DIFFUSE)
        {
            // gc_texture_compute_lod((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[0], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod, &diffuse_color);
            gc_gamma_srgb_to_linear_frag(&diffuse_color);

            DEBUG_MIP(diffuse_color, lod.low);

            fragment->r[0] = diffuse_color.r[0];
            fragment->g[0] = diffuse_color.g[0];
            fragment->b[0] = diffuse_color.b[0];
            fragment->a[0] = diffuse_color.a[0];

            fragment->r[1] = diffuse_color.r[1];
            fragment->g[1] = diffuse_color.g[1];
            fragment->b[1] = diffuse_color.b[1];
            fragment->a[1] = diffuse_color.a[1];

            fragment->r[2] = diffuse_color.r[2];
            fragment->g[2] = diffuse_color.g[2];
            fragment->b[2] = diffuse_color.b[2];
            fragment->a[2] = diffuse_color.a[2];

            fragment->r[3] = diffuse_color.r[3];
            fragment->g[3] = diffuse_color.g[3];
            fragment->b[3] = diffuse_color.b[3];
            fragment->a[3] = diffuse_color.a[3];
        }
    }
}
