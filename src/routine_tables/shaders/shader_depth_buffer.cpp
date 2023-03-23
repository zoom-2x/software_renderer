// ----------------------------------------------------------------------------------
// -- Depth buffer shader.
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

void shader_depth_buffer_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

#if GL_PIPE_AVX

// ----------------------------------------------------------------------------------
// -- AVX fragment shader.
// ----------------------------------------------------------------------------------

void shader_depth_buffer_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{}

#elif GC_PIPE_SSE

// ----------------------------------------------------------------------------------
// -- SSE fragment shader.
// ----------------------------------------------------------------------------------

void shader_depth_buffer_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        gc_processed_fragment_t *fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        __m128 z = _mm_load_ps(fragment->z);

        _mm_store_ps(fragment->r, z);
        _mm_store_ps(fragment->g, z);
        _mm_store_ps(fragment->b, z);
        _mm_store_ps(fragment->a, _mm_set1_ps(1.0f));
    }
}

#else

// ----------------------------------------------------------------------------------
// -- Normal fragment shader.
// ----------------------------------------------------------------------------------

void shader_depth_buffer_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        gc_processed_fragment_t *fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        for (u8 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            r32 z = fragment->z[j];

            fragment->r[j] = z;
            fragment->g[j] = z;
            fragment->b[j] = z;
            fragment->a[j] = 1.0f;
        }
    }
}

#endif