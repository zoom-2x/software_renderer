// ----------------------------------------------------------------------------------
// -- File: shader_cartoon.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-02-10 19:35:40
// -- Modified: 2022-02-10 19:36:01
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
// -- CARTOON SHADER.
// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

#define CARTOON_COLOR_1 {1, 0.79f, 0.8f}
#define CARTOON_COLOR_2 {0.81f, 0.58f, 0.70f}
#define CARTOON_COLOR_3 {0.62f, 0.48f, 0.65f}
#define CARTOON_COLOR_4 {0.56f, 0.38f, 0.59f}
#define CARTOON_COLOR_5 {0.41f, 0.31f, 0.50f}

gc_vec_t shader_cartoon_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);
    VINIT3(vertex_normal, vertex->data[2], vertex->data[3], vertex->data[4]);

    gc_vec_t world_position;
    gc_vec_t world_normal;

    gc_mat_t *model_world = GET_MATRIX(M_MODEL_WORLD);
    gc_mat_t *normal_world = GET_MATRIX(M_NORMAL_WORLD);

    gl_mat4_mulvec(model_world, &gl_position, &world_position);
    gl_mat4_mulvec(normal_world, &vertex_normal, &world_normal);
    v3_normalize(&world_normal);

    u32 component_index = 4;

    for (u32 i = 0; i < uniforms->light_count; ++i)
    {
        gc_light_t *light = uniforms->lights + i;
        r32 light_angle = 0;

        if (light->type == GC_SUN_LIGHT)
            light_angle = v3_dot(&light->directional.direction, &world_normal);
        else if (light->type == GC_POINT_LIGHT)
        {
            gc_vec_t light_dir;

            gl_vec3_sub(&light->object.position, &world_position, &light_dir);
            v3_normalize(&light_dir);
            light_angle = v3_dot(&light_dir, &world_normal);
        }

        light_angle = clamp(0.0f, 1.0f, light_angle);
        r32 min = 0.32f;
        r32 max = 1.0f;
        r32 intensity = min + light_angle * (max - min);

        vertex->data[component_index++] = intensity;
    }

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);

    return gl_position;
}

#if GL_PIPE_AVX

// ----------------------------------------------------------------------------------
// -- AVX fragment shader.
// ----------------------------------------------------------------------------------

void shader_cartoon_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{}

#elif GC_PIPE_SSE

// ----------------------------------------------------------------------------------
// -- SSE fragment shader.
// ----------------------------------------------------------------------------------

void shader_cartoon_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
}

#else

// ----------------------------------------------------------------------------------
// -- Normal fragment shader.
// ----------------------------------------------------------------------------------

void shader_cartoon_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
#if 0
    gc_light_t *lights = uniforms->lights;
    u32 light_count = uniforms->light_count;

    for (u32 i = 0; i < pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *pixels = pack->pixels + i;
        gl_fragment_varyings_t *varyings = pack->varyings + i;

        for (u32 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            vec3 final_color;
            u32 component_index = 3;

            for (u32 k = 0; k < light_count; ++k)
            {
                gc_light_t *light = lights + k;
                r32 intensity = varyings->data[component_index++][j];
                vec3 intensity_color;

                if (intensity > 0.90f)
                    intensity_color = CARTOON_COLOR_1;
                else if (intensity > 0.75f)
                    intensity_color = CARTOON_COLOR_2;
                else if (intensity > 0.6f)
                    intensity_color = CARTOON_COLOR_3;
                else if (intensity > 0.45f)
                    intensity_color = CARTOON_COLOR_4;
                else if (intensity > 0.3f)
                    intensity_color = CARTOON_COLOR_5;

                gl_gamma_srgb_to_linear(&intensity_color);

                final_color.r += intensity_color.r * light->il * light->color.r;
                final_color.g += intensity_color.g * light->il * light->color.g;
                final_color.b += intensity_color.b * light->il * light->color.b;
            }

            pixels->colorr[j] = final_color.r;
            pixels->colorg[j] = final_color.g;
            pixels->colorb[j] = final_color.b;
            pixels->colora[j] = 1.0f;
        }
    }
#endif
}

#endif