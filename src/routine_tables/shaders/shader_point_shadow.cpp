// ----------------------------------------------------------------------------------
// -- File: shader_point_shadow.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-08-28 21:28:04
// -- Modified: 2022-08-28 21:28:05
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_point_shadow.cpp"
#define shader_point_shadow_fs _sse_shader_point_shadow_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_point_shadow.cpp"
#define shader_point_shadow_fs _sse_shader_point_shadow_fs
#else
#define shader_point_shadow_fs _shader_point_shadow_fs
#endif

void shader_point_shadow_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);
    gc_vec_t world_position;

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_mat4_mulvec(GET_MATRIX(M_MODEL_WORLD), (gc_vec_t *) &gl_position, &world_position);

    // world position.
    vertex->data[2] = world_position.v3.x;
    vertex->data[3] = world_position.v3.y;
    vertex->data[4] = world_position.v3.z;

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_point_shadow_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_light_t *light = params->current_light;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        gc_processed_fragment_t *fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        fv3_t *world_position = (fv3_t *) fragment->varyings[2];

        // r32 dx2 = fragment->primitive->triangle.interp_z.x * fragment->primitive->triangle.interp_z.x;
        // r32 dy2 = fragment->primitive->triangle.interp_z.y * fragment->primitive->triangle.interp_z.y;

        for (u8 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            VINIT4(direction,
                   world_position->x[j] - light->object.position.v3.x,
                   world_position->y[j] - light->object.position.v3.y,
                   world_position->z[j] - light->object.position.v3.z,
                   0);

            r32 len = gl_vec3_len(&direction);
            len *= light->shadow.f_len_inv;

            fragment->r[j] = len;
            fragment->g[j] = len * len;
            fragment->b[j] = 0;
            fragment->a[j] = 0;
        }
    }
}