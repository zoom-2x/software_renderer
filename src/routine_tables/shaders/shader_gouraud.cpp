// ----------------------------------------------------------------------------------
// -- TEXTURE GOURAUD SHADER.
// ----------------------------------------------------------------------------------
// -- Light equation (partial blinn-phong):
// -- (diffuse * light_intensity * light_angle) + (diffuse * ambient_intensity)
// ----------------------------------------------------------------------------------

gc_vec_t shader_gouraud_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    gc_light_t *lights = uniforms->lights;

    gc_vec_t gl_position = {vertex->pos[0], vertex->pos[1], vertex->pos[2], 1};
    gc_vec_t vertex_normal = {vertex->data[2], vertex->data[3], vertex->data[4]};
    gc_vec_t world_position;
    gc_vec_t world_normal;

    gc_mat_t *model_world = GET_MATRIX(M_MODEL_WORLD);
    gc_mat_t *normal_world = GET_MATRIX(M_NORMAL_WORLD);
    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_mat4_mulvec(model_world, &gl_position, &world_position);
    gl_mat4_mulvec(normal_world, &vertex_normal, &world_normal);
    v3_normalize(&world_normal);

    u32 component_index = 6;

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

        vertex->data[component_index++] = light_angle;
    }

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);

    return gl_position;
}

#if defined(GL_PIPE_AVX)

// ----------------------------------------------------------------------------------
// -- AVX fragment shader.
// ----------------------------------------------------------------------------------

void shader_gouraud_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
    // NOTE(gabic): Not implemented !!
}

#elif defined(GC_PIPE_SSE)

// ----------------------------------------------------------------------------------
// -- SSE fragment shader.
// ----------------------------------------------------------------------------------

void shader_gouraud_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
#if 0
    gl_texture_t *diffuse_map = uniforms->material->blinn.diffuse_map;

    __m128 light_intensity_r_4x = _mm_set1_ps(uniforms->directional_light->light_intensity.r);
    __m128 light_intensity_g_4x = _mm_set1_ps(uniforms->directional_light->light_intensity.g);
    __m128 light_intensity_b_4x = _mm_set1_ps(uniforms->directional_light->light_intensity.b);

    __m128 ambient_intensity_r_4x = _mm_set1_ps(uniforms->directional_light->ambient_intensity.r);
    __m128 ambient_intensity_g_4x = _mm_set1_ps(uniforms->directional_light->ambient_intensity.g);
    __m128 ambient_intensity_b_4x = _mm_set1_ps(uniforms->directional_light->ambient_intensity.b);

    __m128 zero_4x = _mm_setzero_ps();
    __m128 one_4x = _mm_set1_ps(1.0f);
    __m128 minus_one_4x = _mm_set1_ps(-1.0f);

    __ALIGN__ u32 diffuse_sample[GC_FRAG_SIZE];

    __ALIGN__ r32 diffuse_r[GC_FRAG_SIZE];
    __ALIGN__ r32 diffuse_g[GC_FRAG_SIZE];
    __ALIGN__ r32 diffuse_b[GC_FRAG_SIZE];
    __ALIGN__ r32 diffuse_a[GC_FRAG_SIZE];

    for (u32 i = 0; i < pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *pixels = pack->pixels + i;
        gl_fragment_varyings_t *varyings = pack->varyings + i;

        gl_sample_frag_4x(diffuse_map, varyings, 3, diffuse_sample, GL_FILTER_NEAREST);
        gl_srgb_to_linear1_frag_4x(diffuse_sample, diffuse_r, diffuse_g, diffuse_b, diffuse_a);

        __m128 diffuse_r_4x = _mm_load_ps(diffuse_r);
        __m128 diffuse_g_4x = _mm_load_ps(diffuse_g);
        __m128 diffuse_b_4x = _mm_load_ps(diffuse_b);
        __m128 diffuse_a_4x = _mm_load_ps(diffuse_a);

        __m128 light_angle_4x = _mm_load_ps(varyings->data[5]);

        if (pixels->is_backface)
            light_angle_4x = _mm_mul_ps(light_angle_4x, minus_one_4x);

        light_angle_4x = _mm_blendv_ps(zero_4x, light_angle_4x, _mm_cmpge_ps(light_angle_4x, zero_4x));

        __m128 pixel_color_r_4x = _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_mul_ps(light_intensity_r_4x, diffuse_r_4x),
                                        light_angle_4x),
                                    _mm_mul_ps(ambient_intensity_r_4x, diffuse_r_4x));

        __m128 pixel_color_g_4x = _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_mul_ps(light_intensity_g_4x, diffuse_g_4x),
                                        light_angle_4x),
                                    _mm_mul_ps(ambient_intensity_g_4x, diffuse_g_4x));

        __m128 pixel_color_b_4x = _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_mul_ps(light_intensity_b_4x, diffuse_b_4x),
                                        light_angle_4x),
                                    _mm_mul_ps(ambient_intensity_b_4x, diffuse_b_4x));

        pixel_color_r_4x = _mm_blendv_ps(zero_4x, pixel_color_r_4x, _mm_cmpge_ps(pixel_color_r_4x, zero_4x));
        pixel_color_g_4x = _mm_blendv_ps(zero_4x, pixel_color_g_4x, _mm_cmpge_ps(pixel_color_g_4x, zero_4x));
        pixel_color_b_4x = _mm_blendv_ps(zero_4x, pixel_color_b_4x, _mm_cmpge_ps(pixel_color_b_4x, zero_4x));

        pixel_color_r_4x = _mm_blendv_ps(one_4x, pixel_color_r_4x, _mm_cmple_ps(pixel_color_r_4x, one_4x));
        pixel_color_g_4x = _mm_blendv_ps(one_4x, pixel_color_g_4x, _mm_cmple_ps(pixel_color_g_4x, one_4x));
        pixel_color_b_4x = _mm_blendv_ps(one_4x, pixel_color_b_4x, _mm_cmple_ps(pixel_color_b_4x, one_4x));

        _mm_store_ps(pixels->colorr, pixel_color_r_4x);
        _mm_store_ps(pixels->colorg, pixel_color_g_4x);
        _mm_store_ps(pixels->colorb, pixel_color_b_4x);
        _mm_store_ps(pixels->colora, diffuse_a_4x);
    }
#endif
}

#else

// ----------------------------------------------------------------------------------
// -- Normal fragment shader.
// ----------------------------------------------------------------------------------

void shader_gouraud_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
#if 0
    gc_light_t *lights = uniforms->lights;
    u32 light_count = uniforms->light_count;

    gc_blinn_material_t *material = &uniforms->material->blinn;
    vec3 ambient = material->ambient;
    gl_texture_t *diffuse_map = material->diffuse_map;

    for (u32 i = 0; i < pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *pixels = pack->pixels + i;
        gl_fragment_varyings_t *varyings = pack->varyings + i;

        for (u32 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            vec3 computed_color;
            u32 component_index = 5;

            vec2 uv = {
                varyings->data[3][j],
                varyings->data[4][j],
            };

            vec4 diffuse_color = gl_sample(diffuse_map, uv);
            gl_gamma_srgb_to_linear(&diffuse_color);

            for (u32 k = 0; k < light_count; ++k)
            {
                gc_light_t *light = lights + k;
                r32 light_angle = varyings->data[component_index++][j];

                if (pixels->primitive->is_backface)
                    light_angle *= -1;

                if (light_angle < 0)
                    light_angle = 0;

                r32 light_term = light->il * light_angle;

                computed_color.r += (light_term * light->color.r * diffuse_color.r) + (ambient.r * diffuse_color.r * light->ambient.r);
                computed_color.g += (light_term * light->color.g * diffuse_color.g) + (ambient.g * diffuse_color.g * light->ambient.g);
                computed_color.b += (light_term * light->color.b * diffuse_color.b) + (ambient.b * diffuse_color.b * light->ambient.b);
            }

#ifdef TONE_MAPPING
            pixels->colorr[j] = clamp(0, 1, computed_color.r / (1 + computed_color.r));
            pixels->colorg[j] = clamp(0, 1, computed_color.g / (1 + computed_color.r));
            pixels->colorb[j] = clamp(0, 1, computed_color.b / (1 + computed_color.r));
#else
            pixels->colorr[j] = clamp(0, 1, computed_color.r);
            pixels->colorg[j] = clamp(0, 1, computed_color.g);
            pixels->colorb[j] = clamp(0, 1, computed_color.b);
#endif
            pixels->colora[j] = diffuse_color.a;
        }
    }
#endif
}

#endif
