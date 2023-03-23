// ----------------------------------------------------------------------------------
// -- File: shader_shadow.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-05-20 22:30:02
// -- Modified: 2022-05-20 22:30:11
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_shadow.cpp"
#define shader_shadow_fs _avx_shader_shadow_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_shadow.cpp"
#define shader_shadow_fs _sse_shader_shadow_fs
#else
#define shader_shadow_fs _shader_shadow_fs
#endif

void shader_shadow_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    gc_vec_t gl_position;
    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_shadow_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        gc_processed_fragment_t *fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        for (u8 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            fragment->r[j] = fragment->z[j];
            fragment->g[j] = fragment->z[j] * fragment->z[j];
            fragment->b[j] = 0;
            fragment->a[j] = 0;
        }
    }
}