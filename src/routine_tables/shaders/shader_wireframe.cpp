// ----------------------------------------------------------------------------------
// -- File: shader_wireframe.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-17 09:31:28
// -- Modified: 2022-10-17 09:31:29
// ----------------------------------------------------------------------------------

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_wireframe.cpp"
#define shader_wireframe_fs _avx_shader_wireframe_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_wireframe.cpp"
#define shader_wireframe_fs _sse_shader_wireframe_fs
#else
#define shader_wireframe_fs _shader_wireframe_fs
#endif

// ----------------------------------------------------------------------------------
// -- Basic line shading program.
// ----------------------------------------------------------------------------------

void shader_wireframe_vs(gc_vertex_t *vertex, gc_shader_params_t *params)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_wireframe_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_shader_wireframe_fs");

    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        for (u32 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            // if (fragment->primitive->id == 0)
            // {
            //     fragment->r[j] = 1.0f;
            //     fragment->g[j] = 0.0f;
            //     fragment->b[j] = 0.0f;
            //     fragment->a[j] = 1.0f;
            // }
            // else if (fragment->primitive->id == 1)
            // {
            //     fragment->r[j] = 0.0f;
            //     fragment->g[j] = 0.0f;
            //     fragment->b[j] = 1.0f;
            //     fragment->a[j] = 1.0f;
            // }
            // else
            {
                fragment->r[j] = PIPE_PARAM_VECTOR(2, wireframe_color, 0);
                fragment->g[j] = PIPE_PARAM_VECTOR(2, wireframe_color, 1);
                fragment->b[j] = PIPE_PARAM_VECTOR(2, wireframe_color, 2);
                fragment->a[j] = PIPE_PARAM_VECTOR(2, wireframe_color, 3);
            }
        }
    }
}