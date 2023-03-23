// ----------------------------------------------------------------------------------
// -- File: shader_debug_grid.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-11-11 12:35:37
// -- Modified: 2022-11-11 12:35:40
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_debug_grid.cpp"
#define shader_debug_grid_fs _sse_shader_debug_grid_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_debug_grid.cpp"
#define shader_debug_grid_fs _sse_shader_debug_grid_fs
#else
#define shader_debug_grid_fs _shader_debug_grid_fs
#endif

// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

void shader_debug_grid_vs(gc_vertex_t *vertex, gc_shader_params_t *params)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_debug_grid_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        fragment->r[0] = fragment->primitive->base.data[0];
        fragment->r[1] = fragment->primitive->base.data[0];
        fragment->r[2] = fragment->primitive->base.data[0];
        fragment->r[3] = fragment->primitive->base.data[0];

        fragment->g[0] = fragment->primitive->base.data[1];
        fragment->g[1] = fragment->primitive->base.data[1];
        fragment->g[2] = fragment->primitive->base.data[1];
        fragment->g[3] = fragment->primitive->base.data[1];

        fragment->b[0] = fragment->primitive->base.data[2];
        fragment->b[1] = fragment->primitive->base.data[2];
        fragment->b[2] = fragment->primitive->base.data[2];
        fragment->b[3] = fragment->primitive->base.data[2];

        fragment->a[0] = fragment->primitive->base.data[3];
        fragment->a[1] = fragment->primitive->base.data[3];
        fragment->a[2] = fragment->primitive->base.data[3];
        fragment->a[3] = fragment->primitive->base.data[3];
    }
}