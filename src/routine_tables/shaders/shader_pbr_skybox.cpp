// ----------------------------------------------------------------------------------
// -- File: shader_skybox.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-06-22 15:24:32
// -- Modified: 2022-06-22 15:24:33
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_pbr_skybox.cpp"
#define shader_pbr_skybox_fs _sse_shader_pbr_skybox_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_pbr_skybox.cpp"
#define shader_pbr_skybox_fs _sse_shader_pbr_skybox_fs
#else
#define shader_pbr_skybox_fs _shader_pbr_skybox_fs
#endif

// ----------------------------------------------------------------------------------
// -- SKYBOX SHADER.
// ----------------------------------------------------------------------------------
// -- Shader setup.
// ----------------------------------------------------------------------------------

void shader_pbr_skybox_setup(gc_material_t *material)
{
    GCSR.gl->pipeline.params.material = material;
    GCSR.gl->pipeline.params.shader_flags = 0;

    if (material->skybox.input)
        GCSR.gl->pipeline.params.textures[0] = material->skybox.input;
}

void shader_pbr_skybox_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    gc_vec_t gl_position;
    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    vertex->data[2] = vertex->pos[0];
    vertex->data[3] = vertex->pos[1];
    vertex->data[4] = vertex->pos[2];

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_pbr_skybox_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;
    shader_color_t cube_sample;
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
            gl_cubemap_compute_lod(environment, fragment, &lod);
            cube_texture_sample(environment, position, &lod, false, &cube_sample);

            // DEBUG_MIP(cube_sample, lod.low);

            fragment->r[0] = cube_sample.r[0];
            fragment->g[0] = cube_sample.g[0];
            fragment->b[0] = cube_sample.b[0];
            fragment->a[0] = 1.0f;

            fragment->r[1] = cube_sample.r[1];
            fragment->g[1] = cube_sample.g[1];
            fragment->b[1] = cube_sample.b[1];
            fragment->a[1] = 1.0f;

            fragment->r[2] = cube_sample.r[2];
            fragment->g[2] = cube_sample.g[2];
            fragment->b[2] = cube_sample.b[2];
            fragment->a[2] = 1.0f;

            fragment->r[3] = cube_sample.r[3];
            fragment->g[3] = cube_sample.g[3];
            fragment->b[3] = cube_sample.b[3];
            fragment->a[3] = 1.0f;
        }
    }
}