// ----------------------------------------------------------------------------------
// -- File: sse_shader_blinn_phong.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-11 16:19:28
// -- Modified: 2022-10-11 16:19:28
// ----------------------------------------------------------------------------------

void _sse_shader_blinn_phong_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_sse_shader_blinn_phong_fs");

    gc_processed_fragment_t *fragment = 0;
    gc_blinn_material_t *material = &params->material->blinn;

    r32 shininess = material->shininess;

    lod_t lod;
    LOD_CLEAR(lod);

    gc_vec_t material_ambient;
    VCPY4(material_ambient, material->ambient);

    b8 is_uv_scaling = PIPE_PARAM_VECTOR(2, uv_scaling, 0) || PIPE_PARAM_VECTOR(2, uv_scaling, 1);

    __ALIGN__ fv4_t tmp_buffer;
    shader_color_t tmp_color;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        SSE_COLOR_VAR(computed_color);
        SSE_COLOR_VAR(emission_color);
        SSE_COLOR_VAR_SET(diffuse_color, material->diffuse.c.r, material->diffuse.c.g, material->diffuse.c.b, material->diffuse.c.a);
        sse_color_t ao_component;
        sse_color_t constant_material_ambient;

        __m128 specular = _mm_set1_ps(material->specular);

        if (PIPE_FLAG(GC_MODE_SOLID))
        {
            diffuse_color.r = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 0));
            diffuse_color.g = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 1));
            diffuse_color.b = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 2));
            diffuse_color.a = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 3));
        }

        ao_component.r = _mm_set1_ps(1.0f);

        sse_v3_t world_normal;
        sse_v4_t varying_tangent;
        sse_v3_t world_position;
        sse_v3_t view_vector;

        world_normal.x = _mm_load_ps(fragment->varyings[2]);
        world_normal.y = _mm_load_ps(fragment->varyings[3]);
        world_normal.z = _mm_load_ps(fragment->varyings[4]);

        varying_tangent.x = _mm_load_ps(fragment->varyings[5]);
        varying_tangent.y = _mm_load_ps(fragment->varyings[6]);
        varying_tangent.z = _mm_load_ps(fragment->varyings[7]);
        varying_tangent.w = _mm_load_ps(fragment->varyings[8]);

        world_position.x = _mm_load_ps(fragment->varyings[9]);
        world_position.y = _mm_load_ps(fragment->varyings[10]);
        world_position.z = _mm_load_ps(fragment->varyings[11]);

        view_vector.x = _mm_load_ps(fragment->varyings[12]);
        view_vector.y = _mm_load_ps(fragment->varyings[13]);
        view_vector.z = _mm_load_ps(fragment->varyings[14]);

        sse_v3_normalize(&world_normal);
        sse_v3_normalize(&view_vector);

        if (is_uv_scaling)
        {
            __m128 u_4x = _mm_load_ps(fragment->varyings[0]);
            __m128 v_4x = _mm_load_ps(fragment->varyings[1]);

            u_4x = _mm_mul_ps(u_4x, _mm_set1_ps(PIPE_PARAM_VECTOR(2, uv_scaling, 0)));
            v_4x = _mm_mul_ps(v_4x, _mm_set1_ps(PIPE_PARAM_VECTOR(2, uv_scaling, 1)));

            _mm_store_ps(fragment->varyings[0], u_4x);
            _mm_store_ps(fragment->varyings[1], v_4x);
        }

        // ----------------------------------------------------------------------------------
        // -- Normal.
        // ----------------------------------------------------------------------------------

        if (params->shader_flags & SHADER_FLAG_BLINN_NORMAL)
        {
            sse_v3_t tmp;
            sse_v3_t tangent;
            sse_v3_t btangent;

            sse_v3_t vec1;
            sse_v3_t vec2;
            sse_v3_t vec3;

            // gc_texture_compute_lod(((texture2d_t *) params->textures[1]), fragment->varyings[0], fragment->varyings[1], &lod);
            gc_texture_compute_lod(((texture2d_t *) params->textures[1]), fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[1], fragment->varyings[0], fragment->varyings[1], &lod, &tmp_color);

            __m128 tsp_normal_x = _mm_load_ps(tmp_color.r);
            __m128 tsp_normal_y = _mm_load_ps(tmp_color.g);
            __m128 tsp_normal_z = _mm_load_ps(tmp_color.b);

            __m128 two = _mm_set1_ps(2.0f);
            __m128 one = _mm_set1_ps(1.0f);

            tsp_normal_x = _mm_sub_ps(_mm_mul_ps(tsp_normal_x, two), one);
            tsp_normal_y = _mm_sub_ps(_mm_mul_ps(tsp_normal_y, two), one);
            tsp_normal_z = _mm_sub_ps(_mm_mul_ps(tsp_normal_z, two), one);

            // Gram-Schmidt orthogonalization (tangent space basis vectors).
            __m128 dot = sse_v3_dot((sse_v3_t *) &varying_tangent, &world_normal);
            sse_v3_muls(&world_normal, dot, &tmp);
            sse_v3_sub((sse_v3_t *) &varying_tangent, &tmp, &tangent);
            sse_v3_normalize(&tangent);
            sse_v3_cross(&world_normal, &tangent, &btangent);
            sse_v3_muls(&btangent, varying_tangent.w, &btangent);

            sse_v3_muls(&tangent, tsp_normal_x, &vec1);
            sse_v3_muls(&btangent, tsp_normal_y, &vec2);
            sse_v3_muls(&world_normal, tsp_normal_z, &vec3);

            world_normal.x = _mm_add_ps(_mm_add_ps(vec1.x, vec2.x), vec3.x);
            world_normal.y = _mm_add_ps(_mm_add_ps(vec1.y, vec2.y), vec3.y);
            world_normal.z = _mm_add_ps(_mm_add_ps(vec1.z, vec2.z), vec3.z);

            sse_v3_normalize(&world_normal);
        }

        if (fragment->primitive->is_backface)
            sse_v3_inverse(&world_normal);

        if (params->shader_flags & SHADER_FLAG_BLINN_DIFFUSE)
        {
            // gc_texture_compute_lod((texture2d_t *) params->textures[0], fragment->varyings[0], fragment->varyings[1], &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[0], fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[0], fragment->varyings[0], fragment->varyings[1], &lod, &tmp_color);
            SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

            diffuse_color.r = _mm_load_ps(tmp_color.r);
            diffuse_color.g = _mm_load_ps(tmp_color.g);
            diffuse_color.b = _mm_load_ps(tmp_color.b);
            diffuse_color.a = _mm_load_ps(tmp_color.a);
        }

        diffuse_color.r = _mm_mul_ps(diffuse_color.r, _mm_set1_ps(material->diffuse_multiplier.v3.x));
        diffuse_color.g = _mm_mul_ps(diffuse_color.g, _mm_set1_ps(material->diffuse_multiplier.v3.y));
        diffuse_color.b = _mm_mul_ps(diffuse_color.b, _mm_set1_ps(material->diffuse_multiplier.v3.z));

        if (params->shader_flags & SHADER_FLAG_BLINN_EMISSION)
        {
            // gc_texture_compute_lod((texture2d_t *) params->textures[3], fragment->varyings[0], fragment->varyings[1], &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[3], fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[3], fragment->varyings[0], fragment->varyings[1], &lod, &tmp_color);
            SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

            emission_color.r = _mm_load_ps(tmp_color.r);
            emission_color.g = _mm_load_ps(tmp_color.g);
            emission_color.b = _mm_load_ps(tmp_color.b);
            emission_color.a = _mm_load_ps(tmp_color.a);
        }

        if (params->shader_flags & SHADER_FLAG_BLINN_AO)
        {
            // gc_texture_compute_lod((texture2d_t *) params->textures[4], fragment->varyings[0], fragment->varyings[1], &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[4], fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[4], fragment->varyings[0], fragment->varyings[1], &lod, &tmp_color);
            SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

            ao_component.r = _mm_load_ps(tmp_color.r);
        }

        __m128 vn_dot = sse_v3_dot(&view_vector, &world_normal);
        vn_dot = _mm_max_ps(_mm_setzero_ps(), vn_dot);

        // ----------------------------------------------------------------------------------
        // -- TODO(gabic): reflexie / refractie ?
        // ----------------------------------------------------------------------------------

        constant_material_ambient.r = _mm_mul_ps(_mm_mul_ps(diffuse_color.r, _mm_set1_ps(PIPE_PARAM_VECTOR(1, ambient_color, 0))), ao_component.r);
        constant_material_ambient.g = _mm_mul_ps(_mm_mul_ps(diffuse_color.g, _mm_set1_ps(PIPE_PARAM_VECTOR(1, ambient_color, 1))), ao_component.r);
        constant_material_ambient.b = _mm_mul_ps(_mm_mul_ps(diffuse_color.b, _mm_set1_ps(PIPE_PARAM_VECTOR(1, ambient_color, 2))), ao_component.r);

        // ----------------------------------------------------------------------------------
        // -- Lights.
        // ----------------------------------------------------------------------------------

        SSE_COLOR_VAR(light_diffuse_color);
        SSE_COLOR_VAR(light_specular_color);
        SSE_COLOR_VAR(light_ambient_color);
        SSE_COLOR_VAR(light_emission_color);

        light_diffuse_color.r = _mm_setzero_ps();
        light_diffuse_color.g = _mm_setzero_ps();
        light_diffuse_color.b = _mm_setzero_ps();

        if (!(GCSR.gl->pipeline.flags & GC_MODE_MATERIAL))
        {
            for (u32 k = 0; k < params->light_count; ++k)
            {
                gc_light_t *light = params->lights + k;
                sse_v3_t light_dir;

                __m128 ln_dot;
                __m128 attenuation = _mm_set1_ps(1.0f);

                if (light->type == GC_SUN_LIGHT)
                {
                    light_dir.x = _mm_set1_ps(light->directional.direction.v3.x);
                    light_dir.y = _mm_set1_ps(light->directional.direction.v3.y);
                    light_dir.z = _mm_set1_ps(light->directional.direction.v3.z);

                    ln_dot = sse_v3_dot(&light_dir, &world_normal);
                    ln_dot = _mm_min_ps(_mm_set1_ps(1.0f), ln_dot);
                    ln_dot = _mm_max_ps(_mm_setzero_ps(), ln_dot);

                    if (FLAG(params->shader_flags, SHADER_FLAG_SHADOW))
                    {
                        __m128 num_4x = _mm_div_ps(_mm_set1_ps(1.0f), _mm_load_ps(fragment->varyings[18]));
                        __m128 light_u_4x = _mm_mul_ps(_mm_load_ps(fragment->varyings[15]), num_4x);
                        __m128 light_v_4x = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_mul_ps(_mm_load_ps(fragment->varyings[16]), num_4x));
                        __m128 distance_4x = _mm_sub_ps(_mm_mul_ps(_mm_load_ps(fragment->varyings[17]), num_4x), _mm_set1_ps(light->shadow.depth_bias));

                        _mm_store_ps(tmp_buffer.x, light_u_4x);
                        _mm_store_ps(tmp_buffer.y, light_v_4x);
                        _mm_store_ps(tmp_buffer.z, distance_4x);

                        light->shadow.sun_shadow_visibility_r((texture2d_t *) light->shadow_texture, tmp_buffer.x, tmp_buffer.y, tmp_buffer.z, fragment->shadow);
                    }
                }
                else if (light->type == GC_POINT_LIGHT)
                {
                    light_dir.x = _mm_sub_ps(_mm_set1_ps(light->object.position.v3.x), world_position.x);
                    light_dir.y = _mm_sub_ps(_mm_set1_ps(light->object.position.v3.y), world_position.y);
                    light_dir.z = _mm_sub_ps(_mm_set1_ps(light->object.position.v3.z), world_position.z);

                    __m128 distance_4x = sse_v3_len(&light_dir);
                    sse_v3_normalize(&light_dir);

                    ln_dot = sse_v3_dot(&light_dir, &world_normal);
                    ln_dot = _mm_min_ps(_mm_set1_ps(1.0f), ln_dot);
                    ln_dot = _mm_max_ps(_mm_setzero_ps(), ln_dot);

                    attenuation = _mm_rcp_ps(
                                    _mm_add_ps(
                                        _mm_add_ps(_mm_set1_ps(light->point.kc),
                                            _mm_mul_ps(_mm_set1_ps(light->point.kl), distance_4x)),
                                            _mm_mul_ps(_mm_set1_ps(light->point.kq), _mm_mul_ps(distance_4x, distance_4x))));

                    if (FLAG(params->shader_flags, SHADER_FLAG_SHADOW))
                    {
                        // NOTE(gabic): Aici trebuie sa verific la final daca nu e mai bine sa calculez
                        // treaba asta mai sus -> light_dir.

                        sse_v3_t local_dir;

                        // base_normal.x = _mm_load_ps(fragment->varyings[2]);
                        // base_normal.y = _mm_load_ps(fragment->varyings[3]);
                        // base_normal.z = _mm_load_ps(fragment->varyings[4]);

                        local_dir.x = _mm_sub_ps(world_position.x, _mm_set1_ps(light->object.position.v3.x));
                        local_dir.y = _mm_sub_ps(world_position.y, _mm_set1_ps(light->object.position.v3.y));
                        local_dir.z = _mm_sub_ps(world_position.z, _mm_set1_ps(light->object.position.v3.z));

                        __m128 compare_4x = sse_v3_len(&local_dir);
                        // __m128 slope_scale_bias = _mm_mul_ps(
                        //                             _mm_sqrt_ps(_mm_sub_ps(_mm_set1_ps(1.0f), _mm_mul_ps(ln_dot, ln_dot))),
                        //                             _mm_rcp_ps(ln_dot));
                        // compare_4x = _mm_sub_ps(_mm_mul_ps(compare_4x, _mm_set1_ps(light->shadow.f_len_inv)), _mm_mul_ps(_mm_set1_ps(light->shadow.depth_bias), slope_scale_bias));
                        compare_4x = _mm_sub_ps(_mm_mul_ps(compare_4x, _mm_set1_ps(light->shadow.f_len_inv)), _mm_set1_ps(light->shadow.depth_bias));

                        _mm_store_ps(tmp_buffer.x, local_dir.x);
                        _mm_store_ps(tmp_buffer.y, local_dir.y);
                        _mm_store_ps(tmp_buffer.z, local_dir.z);
                        _mm_store_ps(tmp_buffer.w, compare_4x);

                        light->shadow.point_shadow_visibility_r((cube_texture_t *) light->shadow_texture, (fv3_t *) &tmp_buffer, tmp_buffer.w, light->shadow.radius, fragment->shadow);
                    }
                }
                else
                    continue;

                // ----------------------------------------------------------------------------------
                // -- Light diffuse.
                // ----------------------------------------------------------------------------------

                attenuation = _mm_mul_ps(attenuation, _mm_load_ps(fragment->shadow));
                __m128 lc_4x = _mm_mul_ps(ln_dot, attenuation);

                light_diffuse_color.r = _mm_add_ps(light_diffuse_color.r, _mm_mul_ps(_mm_mul_ps(diffuse_color.r, _mm_set1_ps(light->color.c.r)), lc_4x));
                light_diffuse_color.g = _mm_add_ps(light_diffuse_color.g, _mm_mul_ps(_mm_mul_ps(diffuse_color.g, _mm_set1_ps(light->color.c.g)), lc_4x));
                light_diffuse_color.b = _mm_add_ps(light_diffuse_color.b, _mm_mul_ps(_mm_mul_ps(diffuse_color.b, _mm_set1_ps(light->color.c.b)), lc_4x));

                // ----------------------------------------------------------------------------------
                // -- Light specular.
                // ----------------------------------------------------------------------------------

                if (params->shader_flags & SHADER_FLAG_BLINN_SPECULAR)
                {
                    __m128 fresnel = _mm_set1_ps(1.0f);

                    if (params->textures[2])
                    {
                        // gc_texture_compute_lod((texture2d_t *) params->textures[2], fragment->varyings[0], fragment->varyings[1], &lod);
                        gc_texture_compute_lod((texture2d_t *) params->textures[2], fragment, &lod);
                        sse_texture_sample((texture2d_t *) params->textures[2], fragment->varyings[0], fragment->varyings[1], &lod, &tmp_color);

                        specular = _mm_mul_ps(_mm_load_ps(tmp_color.r), _mm_set1_ps(material->specular));
                    }

                    if (params->shader_flags & SHADER_FLAG_BLINN_FRESNEL)
                    {
                        __m128 min_fresnel = _mm_set1_ps(material->min_fresnel);
                        __m128 max_fresnel = _mm_set1_ps(material->max_fresnel);
                        __m128 diff_fresnel = _mm_sub_ps(max_fresnel, min_fresnel);

                        fresnel = _mm_sub_ps(_mm_set1_ps(1.0f), sse_clamp(vn_dot, 0, 1));
                        fresnel = _mm_mul_ps(fresnel, _mm_mul_ps(fresnel, _mm_mul_ps(fresnel, _mm_mul_ps(fresnel, fresnel))));
                        fresnel = sse_clamp(fresnel, 0, 1);
                        fresnel = _mm_add_ps(min_fresnel, _mm_mul_ps(diff_fresnel, fresnel));
                    }

                    sse_v3_t half_vector;
                    sse_v3_add(&light_dir, &view_vector, &half_vector);
                    sse_v3_normalize(&half_vector);

                    sse_color_t specular_color_term;

                    __m128 spec_term = sse_v3_dot(&half_vector, &world_normal);
                    spec_term = _mm_max_ps(spec_term, _mm_setzero_ps());
                    __m128 spec_mask = _mm_cmpgt_ps(spec_term, _mm_setzero_ps());

                    _mm_store_ps(tmp_buffer.x, spec_term);

                    // optimization: exp(shininess * (1 - cos(theta)))
                    tmp_buffer.x[0] = exp(-shininess * (1 - tmp_buffer.x[0]));
                    tmp_buffer.x[1] = exp(-shininess * (1 - tmp_buffer.x[1]));
                    tmp_buffer.x[2] = exp(-shininess * (1 - tmp_buffer.x[2]));
                    tmp_buffer.x[3] = exp(-shininess * (1 - tmp_buffer.x[3]));

                    spec_term = _mm_load_ps(tmp_buffer.x);
                    spec_term = _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(_mm_mul_ps(spec_term, _mm_set1_ps(light->il)), specular), fresnel), ln_dot);

                    specular_color_term.r = _mm_mul_ps(_mm_mul_ps(spec_term, attenuation), _mm_set1_ps(light->color.c.r));
                    specular_color_term.g = _mm_mul_ps(_mm_mul_ps(spec_term, attenuation), _mm_set1_ps(light->color.c.g));
                    specular_color_term.b = _mm_mul_ps(_mm_mul_ps(spec_term, attenuation), _mm_set1_ps(light->color.c.b));

                    light_specular_color.r = _mm_add_ps(light_specular_color.r, _mm_and_ps(spec_mask, specular_color_term.r));
                    light_specular_color.g = _mm_add_ps(light_specular_color.g, _mm_and_ps(spec_mask, specular_color_term.g));
                    light_specular_color.b = _mm_add_ps(light_specular_color.b, _mm_and_ps(spec_mask, specular_color_term.b));
                }

                // ----------------------------------------------------------------------------------
                // -- Light ambient.
                // ----------------------------------------------------------------------------------

                light_ambient_color.r = _mm_add_ps(light_ambient_color.r, constant_material_ambient.r);
                light_ambient_color.g = _mm_add_ps(light_ambient_color.g, constant_material_ambient.g);
                light_ambient_color.b = _mm_add_ps(light_ambient_color.b, constant_material_ambient.b);

                // ----------------------------------------------------------------------------------
                // -- Light emission.
                // ----------------------------------------------------------------------------------

                light_emission_color.r = _mm_add_ps(light_emission_color.r, emission_color.r);
                light_emission_color.g = _mm_add_ps(light_emission_color.g, emission_color.g);
                light_emission_color.b = _mm_add_ps(light_emission_color.b, emission_color.b);
            }
        }
        else
        {
            light_diffuse_color.r = diffuse_color.r;
            light_diffuse_color.g = diffuse_color.g;
            light_diffuse_color.b = diffuse_color.b;
        }

        computed_color.r = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(light_diffuse_color.r, light_specular_color.r), light_ambient_color.r), light_emission_color.r), ao_component.r);
        computed_color.g = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(light_diffuse_color.g, light_specular_color.g), light_ambient_color.g), light_emission_color.g), ao_component.r);
        computed_color.b = _mm_mul_ps(_mm_add_ps(_mm_add_ps(_mm_add_ps(light_diffuse_color.b, light_specular_color.b), light_ambient_color.b), light_emission_color.b), ao_component.r);

        _mm_store_ps(fragment->r, computed_color.r);
        _mm_store_ps(fragment->g, computed_color.g);
        _mm_store_ps(fragment->b, computed_color.b);
        _mm_store_ps(fragment->a, diffuse_color.a);
    }
}