// ----------------------------------------------------------------------------------
// -- File: shader_point.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-17 09:31:28
// -- Modified: 2022-12-12 21:36:33
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_point.cpp"
#define shader_point_fs _sse_shader_point_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_point.cpp"
#define shader_point_fs _sse_shader_point_fs
#else
#define shader_point_fs _shader_point_fs
#endif

void shader_point_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_point_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_shader_point_fs");

    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        fragment->r[0] = 1;
        fragment->r[1] = 1;
        fragment->r[2] = 1;
        fragment->r[3] = 1;

        fragment->g[0] = 1;
        fragment->g[1] = 1;
        fragment->g[2] = 1;
        fragment->g[3] = 1;

        fragment->b[0] = 1;
        fragment->b[1] = 1;
        fragment->b[2] = 1;
        fragment->b[3] = 1;

        fragment->a[0] = 1.0f;
        fragment->a[1] = 1.0f;
        fragment->a[2] = 1.0f;
        fragment->a[3] = 1.0f;
    }
}
