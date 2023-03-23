// ----------------------------------------------------------------------------------
// -- FLAT COLOR SHADER.
// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

gc_vec_t shader_flat_color_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);

    gl_mat4_mulvec(GET_MATRIX(M_MODEL_VIEW), &gl_position, &gl_position);
    gl_mat4_mulvec(GET_MATRIX(M_PROJECTION), &gl_position, &gl_position);

    return gl_position;
}

#if defined(GL_PIPE_AVX)

// ----------------------------------------------------------------------------------
// -- AVX fragment shader.
// ----------------------------------------------------------------------------------

void shader_flat_color_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
    // NOTE(gabic): Not implemented !!
}

#elif defined(GC_PIPE_SSE)

// ----------------------------------------------------------------------------------
// -- SSE fragment shader.
// ----------------------------------------------------------------------------------

void shader_flat_color_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
#if 0
    __m128 r_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.r);
    __m128 g_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.g);
    __m128 b_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.b);
    __m128 a_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.a);

    for (u32 i = 0; i < pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *pixels = pack->pixels + i;

        _mm_store_ps(pixels->colorr, r_4x);
        _mm_store_ps(pixels->colorg, g_4x);
        _mm_store_ps(pixels->colorb, b_4x);
        _mm_store_ps(pixels->colora, a_4x);
    }
#endif
}

#else

// ----------------------------------------------------------------------------------
// -- Normal fragment shader.
// ----------------------------------------------------------------------------------

void shader_flat_color_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
#if 0
    for (u32 i = 0; i < pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *pixels = pack->pixels + i;

        for (u32 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            pixels->colorr[j] = uniforms->mesh_uniforms.mesh_color.r;
            pixels->colorg[j] = uniforms->mesh_uniforms.mesh_color.g;
            pixels->colorb[j] = uniforms->mesh_uniforms.mesh_color.b;
            pixels->colora[j] = uniforms->mesh_uniforms.mesh_color.a;
        }
    }
#endif
}

#endif
