// ----------------------------------------------------------------------------------
// -- FLAT COLOR BLINN-PHONG SHADER.
// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

gc_vec_t shader_flat_color_blinn_phong_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
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

    vertex->data[4] = world_normal.v3.x;
    vertex->data[5] = world_normal.v3.y;
    vertex->data[6] = world_normal.v3.z;

    vertex->data[7] = world_position.v3.x;
    vertex->data[8] = world_position.v3.y;
    vertex->data[9] = world_position.v3.z;

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);

    return gl_position;
}

#if defined(GL_PIPE_AVX)

// ----------------------------------------------------------------------------------
// -- AVX fragment shader.
// ----------------------------------------------------------------------------------

void shader_flat_color_blinn_phong_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
    // NOTE(gabic): Not implemented !!
}

#elif defined(GC_PIPE_SSE)

// ----------------------------------------------------------------------------------
// -- SSE fragment shader.
// ----------------------------------------------------------------------------------

void shader_flat_color_blinn_phong_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
#if 0
    gc_directional_light_t *directional_light = uniforms->directional_light;
    gc_blinn_material_t *material = &uniforms->material->blinn;

    __m128 r_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.r);
    __m128 g_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.g);
    __m128 b_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.b);
    __m128 a_4x = _mm_set1_ps(uniforms->mesh_uniforms.mesh_color.a);

    __m128 light_dir_x_4x = _mm_set1_ps(uniforms->directional_light_dir.x);
    __m128 light_dir_y_4x = _mm_set1_ps(uniforms->directional_light_dir.y);
    __m128 light_dir_z_4x = _mm_set1_ps(uniforms->directional_light_dir.z);

    __m128 light_intensity_4x = _mm_set1_ps(directional_light->intensity);

    __m128 ambient_r_4x = _mm_set1_ps(material->ambient.r);
    __m128 ambient_g_4x = _mm_set1_ps(material->ambient.g);
    __m128 ambient_b_4x = _mm_set1_ps(material->ambient.b);

    __m128 zero_4x = _mm_setzero_ps();
    __m128 one_4x = _mm_set1_ps(1.0f);
    __m128 minus_one_4x = _mm_set1_ps(-1.0f);

    for (u32 i = 0; i < pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *pixels = pack->pixels + i;
        gl_fragment_varyings_t *varyings = pack->varyings + i;

        __m128 normal_x_4x = _mm_load_ps(varyings->data[3]);
        __m128 normal_y_4x = _mm_load_ps(varyings->data[4]);
        __m128 normal_z_4x = _mm_load_ps(varyings->data[5]);

        __m128 light_angle_4x = vec3_dot_sse(light_dir_x_4x, light_dir_y_4x, light_dir_z_4x,
                                             normal_x_4x, normal_y_4x, normal_z_4x);

        if (pixels->primitive->is_backface)
            light_angle_4x = _mm_mul_ps(light_angle_4x, minus_one_4x);

        light_angle_4x = _mm_blendv_ps(zero_4x, light_angle_4x, _mm_cmpge_ps(light_angle_4x, zero_4x));

        __m128 pixel_color_r_4x = _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_mul_ps(light_intensity_r_4x, r_4x),
                                        light_angle_4x),
                                    _mm_mul_ps(ambient_intensity_r_4x, r_4x));

        __m128 pixel_color_g_4x = _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_mul_ps(light_intensity_g_4x, g_4x),
                                        light_angle_4x),
                                    _mm_mul_ps(ambient_intensity_g_4x, g_4x));

        __m128 pixel_color_b_4x = _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_mul_ps(light_intensity_b_4x, b_4x),
                                        light_angle_4x),
                                    _mm_mul_ps(ambient_intensity_b_4x, b_4x));

        pixel_color_r_4x = _mm_blendv_ps(zero_4x, pixel_color_r_4x, _mm_cmpge_ps(pixel_color_r_4x, zero_4x));
        pixel_color_g_4x = _mm_blendv_ps(zero_4x, pixel_color_g_4x, _mm_cmpge_ps(pixel_color_g_4x, zero_4x));
        pixel_color_b_4x = _mm_blendv_ps(zero_4x, pixel_color_b_4x, _mm_cmpge_ps(pixel_color_b_4x, zero_4x));

        pixel_color_r_4x = _mm_blendv_ps(one_4x, pixel_color_r_4x, _mm_cmple_ps(pixel_color_r_4x, one_4x));
        pixel_color_g_4x = _mm_blendv_ps(one_4x, pixel_color_g_4x, _mm_cmple_ps(pixel_color_g_4x, one_4x));
        pixel_color_b_4x = _mm_blendv_ps(one_4x, pixel_color_b_4x, _mm_cmple_ps(pixel_color_b_4x, one_4x));

        _mm_store_ps(pixels->colorr, pixel_color_r_4x);
        _mm_store_ps(pixels->colorg, pixel_color_g_4x);
        _mm_store_ps(pixels->colorb, pixel_color_b_4x);
        _mm_store_ps(pixels->colora, a_4x);
    }
#endif
}

#else

// ----------------------------------------------------------------------------------
// -- Normal fragment shader.
// ----------------------------------------------------------------------------------

void shader_flat_color_blinn_phong_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms)
{
#if 0
    gc_light_t *lights = uniforms->lights;
    gc_blinn_material_t *material = &uniforms->material->blinn;
    u32 light_count = uniforms->light_count;

    vec4 diffuse_color = {
        uniforms->mesh_uniforms.mesh_color.r,
        uniforms->mesh_uniforms.mesh_color.g,
        uniforms->mesh_uniforms.mesh_color.b,
        uniforms->mesh_uniforms.mesh_color.a
    };

    vec3 ambient = material->ambient;

    for (u32 i = 0; i < pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *pixels = pack->pixels + i;
        gl_fragment_varyings_t *varyings = pack->varyings + i;

        for (u32 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            vec3 computed_color;

            vec3 world_normal = {
                varyings->data[3][j],
                varyings->data[4][j],
                varyings->data[5][j],
            };

            vec3 world_position = {
                varyings->data[6][j],
                varyings->data[7][j],
                varyings->data[8][j]
            };

            // ----------------------------------------------------------------------------------
            // -- Light computation.
            // ----------------------------------------------------------------------------------

            for (u32 k = 0; k < light_count; ++k)
            {
                gc_light_t *light = lights + k;
                r32 light_angle = 0;

                if (light->type == GC_SUN_LIGHT) {
                    light_angle = vec3_dot(light->directional.direction, world_normal);
                }
                else if (light->type == GC_POINT_LIGHT)
                {
                    vec3 light_dir = vec3_normalize(vec3_sub(light->position, world_position));
                    light_angle = vec3_dot(light_dir, world_normal);
                }

                if (pixels->primitive->is_backface)
                    light_angle *= -1;

                if (light_angle < 0)
                    light_angle = 0;

                r32 light_term = light->il * light_angle;

                computed_color.r += clamp(0, 1, (diffuse_color.r * light->color.r * light_term) + (ambient.r * light->ambient.r * diffuse_color.r));
                computed_color.g += clamp(0, 1, (diffuse_color.g * light->color.g * light_term) + (ambient.g * light->ambient.g * diffuse_color.g));
                computed_color.b += clamp(0, 1, (diffuse_color.b * light->color.b * light_term) + (ambient.b * light->ambient.b * diffuse_color.b));
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
