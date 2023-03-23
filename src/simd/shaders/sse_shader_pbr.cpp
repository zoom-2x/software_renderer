// ----------------------------------------------------------------------------------
// -- File: sse_shader_pbr.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-17 14:59:16
// -- Modified: 2022-11-07 20:44:42
// ----------------------------------------------------------------------------------

__INLINE__ void sse_fresnel_schlick(__m128 hv_dot, sse_v3_t *f0, sse_v3_t *out)
{
    __m128 fc = _mm_sub_ps(_mm_set1_ps(1.0f), hv_dot);
    fc = _mm_mul_ps(fc, _mm_mul_ps(_mm_mul_ps(fc, fc), _mm_mul_ps(fc, fc)));

    // ks, reflected energy.
    out->x = _mm_add_ps(f0->x, _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), f0->x), fc));
    out->y = _mm_add_ps(f0->y, _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), f0->y), fc));
    out->z = _mm_add_ps(f0->z, _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), f0->z), fc));
}

__INLINE__ void sse_fresnel_schlick_roughness(__m128 hv_dot, sse_v3_t *f0, __m128 roughness, sse_v3_t *out)
{
    __m128 fc = _mm_sub_ps(_mm_set1_ps(1.0f), hv_dot);
    fc = _mm_mul_ps(fc, _mm_mul_ps(_mm_mul_ps(fc, fc), _mm_mul_ps(fc, fc)));

    __m128 tmp = _mm_sub_ps(_mm_set1_ps(1.0f), roughness);
    __m128 mask_x = _mm_cmpge_ps(tmp, f0->x);
    __m128 mask_y = _mm_cmpge_ps(tmp, f0->y);
    __m128 mask_z = _mm_cmpge_ps(tmp, f0->z);

    __m128 tf0_x = _mm_or_ps(_mm_and_ps(mask_x, tmp), _mm_andnot_ps(mask_x, f0->x));
    __m128 tf0_y = _mm_or_ps(_mm_and_ps(mask_y, tmp), _mm_andnot_ps(mask_y, f0->y));
    __m128 tf0_z = _mm_or_ps(_mm_and_ps(mask_z, tmp), _mm_andnot_ps(mask_z, f0->z));

    // ks, reflected energy.
    out->x = _mm_add_ps(f0->x, _mm_mul_ps(_mm_sub_ps(tf0_x, f0->x), fc));
    out->y = _mm_add_ps(f0->y, _mm_mul_ps(_mm_sub_ps(tf0_y, f0->y), fc));
    out->z = _mm_add_ps(f0->z, _mm_mul_ps(_mm_sub_ps(tf0_z, f0->z), fc));
}

__INLINE__ __m128 sse_distribution_ggx(__m128 nh_dot, __m128 roughness)
{
    __m128 roughness_x4 = _mm_mul_ps(_mm_mul_ps(roughness, roughness), _mm_mul_ps(roughness, roughness));
    __m128 denom = _mm_add_ps(_mm_mul_ps(_mm_mul_ps(nh_dot, nh_dot), _mm_sub_ps(roughness_x4, _mm_set1_ps(1.0f))), _mm_set1_ps(1.0f));

    denom = _mm_rcp_ps(_mm_mul_ps(_mm_set1_ps(PI), _mm_mul_ps(denom, denom)));
    __m128 distribution = _mm_mul_ps(roughness_x4, denom);

    return distribution;
}

__INLINE__ __m128 sse_geometry_smith(__m128 vn_dot, __m128 ln_dot, __m128 k)
{
    __m128 t = _mm_sub_ps(_mm_set1_ps(1.0f), k);
    __m128 denom = _mm_rcp_ps(_mm_add_ps(_mm_mul_ps(vn_dot, t), k));
    __m128 g1 = _mm_mul_ps(vn_dot, denom);

    denom = _mm_rcp_ps(_mm_add_ps(_mm_mul_ps(ln_dot, t), k));
    __m128 g2 = _mm_mul_ps(ln_dot, denom);
    __m128 res = _mm_add_ps(_mm_mul_ps(g1, g2), _mm_set1_ps(0.0005f));

    return res;
}

// ----------------------------------------------------------------------------------

void _sse_shader_pbr_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    OPTICK_EVENT("_sse_shader_pbr_fs");

    gc_processed_fragment_t *fragment = 0;
    gc_pbr_material_t *material = &params->material->pbr;
    pbr_multipliers_t multipliers = material->multipliers;

    lod_t lod;
    LOD_CLEAR(lod);

    b8 is_uv_scaling = PIPE_PARAM_VECTOR(2, uv_scaling, 0) || PIPE_PARAM_VECTOR(2, uv_scaling, 1);

    __ALIGN__ shader_color_t tmp_color;
    __ALIGN__ fv4_t tmp_buffer;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        sse_color_t computed_light_color;
        sse_color_t albedo;
        sse_color_t emission;
        sse_color_t albedo_over_pi;
        sse_color_t mf0;

        emission.r = _mm_setzero_ps();
        emission.g = _mm_setzero_ps();
        emission.b = _mm_setzero_ps();

        computed_light_color.r = _mm_setzero_ps();
        computed_light_color.g = _mm_setzero_ps();
        computed_light_color.b = _mm_setzero_ps();

        albedo.r = _mm_set1_ps(material->albedo.c.r);
        albedo.g = _mm_set1_ps(material->albedo.c.g);
        albedo.b = _mm_set1_ps(material->albedo.c.b);
        albedo.a = _mm_set1_ps(material->albedo.c.a);

        if (PIPE_FLAG(GC_MODE_SOLID))
        {
            albedo.r = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 0));
            albedo.g = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 1));
            albedo.b = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 2));
            albedo.a = _mm_set1_ps(PIPE_PARAM_VECTOR(1, solid_color, 3));
        }

        mf0.r = _mm_set1_ps(material->f0.v3.x);
        mf0.g = _mm_set1_ps(material->f0.v3.y);
        mf0.b = _mm_set1_ps(material->f0.v3.z);

        __m128 metalness = _mm_mul_ps(_mm_set1_ps(material->metalness), _mm_set1_ps(multipliers.metalness));
        __m128 roughness = _mm_mul_ps(_mm_set1_ps(material->roughness), _mm_set1_ps(multipliers.roughness));
        __m128 ao = _mm_mul_ps(_mm_set1_ps(material->ao), _mm_set1_ps(multipliers.ao));

        fv2_t *texcoord = (fv2_t *) fragment->varyings[0];

        sse_v3_t world_normal;
        sse_v4_t varying_tangent;
        sse_v3_t world_position;
        sse_v3_t view_vector;
        sse_v3_t final_normal;

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

        if (params->shader_flags & SHADER_FLAG_PBR_ALBEDO)
        {
            // gl_sse_texture_compute_lod((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod);
            gl_sse_texture_compute_lod((texture2d_t *) params->textures[0], fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod, &tmp_color);
            SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

            albedo.r = _mm_load_ps(tmp_color.r);
            albedo.g = _mm_load_ps(tmp_color.g);
            albedo.b = _mm_load_ps(tmp_color.b);
            albedo.a = _mm_load_ps(tmp_color.a);
        }

        albedo.r = _mm_mul_ps(albedo.r, _mm_set1_ps(multipliers.albedo.v3.x));
        albedo.g = _mm_mul_ps(albedo.g, _mm_set1_ps(multipliers.albedo.v3.y));
        albedo.b = _mm_mul_ps(albedo.b, _mm_set1_ps(multipliers.albedo.v3.z));

        if (params->shader_flags & SHADER_FLAG_PBR_EMISSION)
        {
            // gl_sse_texture_compute_lod((texture2d_t *) params->textures[6], texcoord->x, texcoord->y, &lod);
            gl_sse_texture_compute_lod((texture2d_t *) params->textures[6], fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[6], texcoord->x, texcoord->y, &lod, &tmp_color);
            SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

            emission.r = _mm_load_ps(tmp_color.r);
            emission.g = _mm_load_ps(tmp_color.g);
            emission.b = _mm_load_ps(tmp_color.b);
        }

        if (params->shader_flags & SHADER_FLAG_PBR_AO_ROUGHNESS_METALNESS)
        {
            // gl_sse_texture_compute_lod((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod);
            gl_sse_texture_compute_lod((texture2d_t *) params->textures[2], fragment, &lod);
            sse_texture_sample((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod, &tmp_color);
            // SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

            ao = _mm_mul_ps(_mm_load_ps(tmp_color.r), _mm_set1_ps(multipliers.ao));
            roughness = _mm_mul_ps(_mm_load_ps(tmp_color.g), _mm_set1_ps(multipliers.roughness));
            metalness = _mm_mul_ps(_mm_load_ps(tmp_color.b), _mm_set1_ps(multipliers.metalness));
        }
        else
        {
            if (params->shader_flags & SHADER_FLAG_PBR_METALNESS)
            {
                // gl_sse_texture_compute_lod((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod);
                gl_sse_texture_compute_lod((texture2d_t *) params->textures[2], fragment, &lod);
                sse_texture_sample((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod, &tmp_color);
                // SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

                metalness = _mm_mul_ps(_mm_load_ps(tmp_color.r), _mm_set1_ps(multipliers.metalness));
            }

            if (params->shader_flags & SHADER_FLAG_PBR_ROUGHNESS)
            {
                // gl_sse_texture_compute_lod((texture2d_t *) params->textures[3], texcoord->x, texcoord->y, &lod);
                gl_sse_texture_compute_lod((texture2d_t *) params->textures[3], fragment, &lod);
                sse_texture_sample((texture2d_t *) params->textures[3], texcoord->x, texcoord->y, &lod, &tmp_color);
                // SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

                roughness = _mm_mul_ps(_mm_load_ps(tmp_color.r), _mm_set1_ps(multipliers.roughness));
            }

            if (params->shader_flags & SHADER_FLAG_PBR_AO)
            {
                // gl_sse_texture_compute_lod((texture2d_t *) params->textures[4], texcoord->x, texcoord->y, &lod);
                gl_sse_texture_compute_lod((texture2d_t *) params->textures[4], fragment, &lod);
                sse_texture_sample((texture2d_t *) params->textures[4], texcoord->x, texcoord->y, &lod, &tmp_color);
                // SSE_GAMMA_SRGB_TO_LINEAR(tmp_color);

                ao = _mm_mul_ps(_mm_load_ps(tmp_color.r), _mm_set1_ps(multipliers.ao));
            }
        }

        albedo_over_pi.r = _mm_mul_ps(albedo.r, _mm_set1_ps(ONE_OVER_PI));
        albedo_over_pi.g = _mm_mul_ps(albedo.g, _mm_set1_ps(ONE_OVER_PI));
        albedo_over_pi.b = _mm_mul_ps(albedo.b, _mm_set1_ps(ONE_OVER_PI));

        if (params->shader_flags & SHADER_FLAG_PBR_NORMAL)
        {
            sse_v3_t tmp;
            sse_v3_t tangent;
            sse_v3_t btangent;

            sse_v3_t vec1;
            sse_v3_t vec2;
            sse_v3_t vec3;

            // gl_sse_texture_compute_lod(((texture2d_t *) params->textures[1]), fragment->varyings[0], fragment->varyings[1], &lod);
            gl_sse_texture_compute_lod(((texture2d_t *) params->textures[1]), fragment, &lod);
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

            final_normal.x = _mm_add_ps(_mm_add_ps(vec1.x, vec2.x), vec3.x);
            final_normal.y = _mm_add_ps(_mm_add_ps(vec1.y, vec2.y), vec3.y);
            final_normal.z = _mm_add_ps(_mm_add_ps(vec1.z, vec2.z), vec3.z);
        }
        else
        {
            final_normal.x = world_normal.x;
            final_normal.y = world_normal.y;
            final_normal.z = world_normal.z;
        }

        sse_v3_normalize(&final_normal);

        if (fragment->primitive->is_backface)
            sse_v3_inverse(&world_normal);

        __m128 vn_dot = sse_v3_dot(&view_vector, &final_normal);
        vn_dot = _mm_max_ps(_mm_setzero_ps(), vn_dot);

        sse_color_t f0;
        sse_v3_mix_4((sse_v3_t *) &mf0, (sse_v3_t *) &albedo, &metalness, (sse_v3_t *) &f0);
        __m128 one_minus_metalness = _mm_sub_ps(_mm_set1_ps(1.0f), metalness);

        __m128 tmp = _mm_add_ps(roughness, _mm_set1_ps(1.0f));
        __m128 k = _mm_mul_ps(_mm_mul_ps(tmp, tmp), _mm_set1_ps(ONE_OVER_8));

        // ----------------------------------------------------------------------------------
        // -- Lights.
        // ----------------------------------------------------------------------------------

        sse_color_t ambient;

        ambient.r = _mm_setzero_ps();
        ambient.g = _mm_setzero_ps();
        ambient.b = _mm_setzero_ps();

        if (!(params->shader_flags & SHADER_FLAG_PBR_UNLIT) && !(GCSR.gl->pipeline.flags & GC_MODE_MATERIAL))
        {
            for (u32 l = 0; l < params->light_count; ++l)
            {
                gc_light_t *light = params->lights + l;

                __m128 attenuation = _mm_set1_ps(1.0f);
                __m128 ln_dot;

                sse_v3_t light_vector;
                sse_v3_t half_vector;

                if (light->type == GC_SUN_LIGHT)
                {
                    light_vector.x = _mm_set1_ps(light->directional.direction.v3.x);
                    light_vector.y = _mm_set1_ps(light->directional.direction.v3.y);
                    light_vector.z = _mm_set1_ps(light->directional.direction.v3.z);

                    ln_dot = sse_v3_dot(&light_vector, &final_normal);
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
                    light_vector.x = _mm_sub_ps(_mm_set1_ps(light->object.position.v3.x), world_position.x);
                    light_vector.y = _mm_sub_ps(_mm_set1_ps(light->object.position.v3.y), world_position.y);
                    light_vector.z = _mm_sub_ps(_mm_set1_ps(light->object.position.v3.z), world_position.z);

                    __m128 distance_4x = sse_v3_len(&light_vector);
                    sse_v3_normalize(&light_vector);

                    ln_dot = sse_v3_dot(&light_vector, &final_normal);
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

                        local_dir.x = _mm_sub_ps(world_position.x, _mm_set1_ps(light->object.position.v3.x));
                        local_dir.y = _mm_sub_ps(world_position.y, _mm_set1_ps(light->object.position.v3.y));
                        local_dir.z = _mm_sub_ps(world_position.z, _mm_set1_ps(light->object.position.v3.z));

                        __m128 compare_4x = sse_v3_len(&local_dir);
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

                sse_v3_add(&light_vector, &view_vector, &half_vector);
                sse_v3_normalize(&half_vector);

                __m128 hv_dot = sse_v3_dot(&half_vector, &view_vector);
                hv_dot = _mm_max_ps(_mm_setzero_ps(), hv_dot);

                sse_v3_t radiance;

                radiance.x = _mm_set1_ps(light->color.c.r);
                radiance.y = _mm_set1_ps(light->color.c.g);
                radiance.z = _mm_set1_ps(light->color.c.b);

                sse_v3_muls(&radiance, ln_dot, &radiance);

                // ----------------------------------------------------------------------------------
                // -- Cook-Torrance specular BRDF term.
                // ----------------------------------------------------------------------------------
                // -- Fresnel-Schlick.
                // ----------------------------------------------------------------------------------

                sse_v3_t fresnel;
                sse_fresnel_schlick(hv_dot, (sse_v3_t *) &f0, &fresnel);

                // ----------------------------------------------------------------------------------
                // -- Distribution function
                // ----------------------------------------------------------------------------------

                __m128 nh_dot = sse_v3_dot(&final_normal, &half_vector);
                nh_dot = _mm_max_ps(_mm_setzero_ps(), nh_dot);
                __m128 distribution = sse_distribution_ggx(nh_dot, roughness);

                // ----------------------------------------------------------------------------------
                // -- Geometry function.
                // ----------------------------------------------------------------------------------

                __m128 geometry = sse_geometry_smith(vn_dot, ln_dot, k);

                // ----------------------------------------------------------------------------------

                __m128 denom = _mm_rcp_ps(_mm_add_ps(_mm_mul_ps(_mm_set1_ps(4.0f), _mm_mul_ps(vn_dot, ln_dot)), _mm_set1_ps(0.001f)));
                __m128 specular_temp = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(multipliers.specular), _mm_mul_ps(distribution, geometry)), denom);

                sse_v3_t light_specular;
                sse_v3_t kd;

                light_specular.x = _mm_mul_ps(specular_temp, fresnel.x);
                light_specular.y = _mm_mul_ps(specular_temp, fresnel.y);
                light_specular.z = _mm_mul_ps(specular_temp, fresnel.z);

                kd.x = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), fresnel.x), one_minus_metalness);
                kd.y = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), fresnel.y), one_minus_metalness);
                kd.z = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), fresnel.z), one_minus_metalness);

                __m128 fragment_shadow = _mm_load_ps(fragment->shadow);
                computed_light_color.r = _mm_add_ps(computed_light_color.r,
                                            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kd.x, albedo_over_pi.r), light_specular.x),
                                                       _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(radiance.x, ln_dot), fragment_shadow), attenuation)));

                computed_light_color.g = _mm_add_ps(computed_light_color.g,
                                            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kd.y, albedo_over_pi.g), light_specular.y),
                                                       _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(radiance.y, ln_dot), fragment_shadow), attenuation)));

                computed_light_color.b = _mm_add_ps(computed_light_color.b,
                                            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(kd.z, albedo_over_pi.b), light_specular.z),
                                                       _mm_mul_ps(_mm_mul_ps(_mm_mul_ps(radiance.z, ln_dot), fragment_shadow), attenuation)));
            }

            // ----------------------------------------------------------------------------------
            // -- Ambient component.
            // ----------------------------------------------------------------------------------

            if (params->shader_flags & SHADER_FLAG_PBR_AMBIENT)
            {
                sse_color_t ambient_specular;
                sse_v3_t ks;
                sse_v3_t kd;
                sse_v2_t brdf_lut;

                brdf_lut.x = _mm_set1_ps(1.0f);
                brdf_lut.y = _mm_set1_ps(1.0f);

                sse_v3_t inv_view_vector;
                sse_v3_t reflection_vector;
                r32 max_reflection_lod = PBR_PREFILTERED_MIP_LEVELS - 1;

                __m128 roughness_lod = _mm_mul_ps(roughness, _mm_set1_ps(max_reflection_lod));

                sse_v3_inverse_to(&view_vector, &inv_view_vector);
                __m128 vn_dot2 = sse_v3_dot(&inv_view_vector, &final_normal);
                sse_v3_muls(&final_normal, vn_dot2, &reflection_vector);
                sse_v3_muls(&reflection_vector, _mm_set1_ps(2), &reflection_vector);
                sse_v3_sub(&inv_view_vector, &reflection_vector, &reflection_vector);

                sse_fresnel_schlick_roughness(vn_dot, (sse_v3_t *) &f0, roughness, &ks);

                kd.x = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), ks.x), one_minus_metalness);
                kd.y = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), ks.y), one_minus_metalness);
                kd.z = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(1.0f), ks.z), one_minus_metalness);

                fv3_t _normal;

                _mm_store_ps(_normal.x, final_normal.x);
                _mm_store_ps(_normal.y, final_normal.y);
                _mm_store_ps(_normal.z, final_normal.z);

                // ----------------------------------------------------------------------------------

                pbr_ambient_texture_t *ambient_texture = (pbr_ambient_texture_t *) params->textures[5];

                lod_t tlod;
                LOD_CLEAR(tlod);

                lod_t prefiltered_lod;
                LOD_CLEAR(prefiltered_lod);

                sse_cube_texture_sample((cube_texture_t *) ambient_texture->irradiance, &_normal, &tlod, true, &tmp_color);

                ambient.r = _mm_load_ps(tmp_color.r);
                ambient.g = _mm_load_ps(tmp_color.g);
                ambient.b = _mm_load_ps(tmp_color.b);

                roughness_lod = _mm_max_ps(roughness_lod, _mm_setzero_ps());
                roughness_lod = _mm_min_ps(roughness_lod, _mm_set1_ps(ambient_texture->prefiltered->mip_count - 1));

                __m128 low_4x = _mm_floor_ps(roughness_lod);
                __m128 high_4x = _mm_ceil_ps(roughness_lod);

                _mm_store_si128((__m128i *) prefiltered_lod.low, _mm_cvttps_epi32(low_4x));
                _mm_store_si128((__m128i *) prefiltered_lod.high, _mm_cvttps_epi32(high_4x));
                _mm_store_ps(prefiltered_lod.interp, _mm_sub_ps(roughness_lod, low_4x));

                // ----------------------------------------------------------------------------------

                _mm_store_ps(_normal.x, reflection_vector.x);
                _mm_store_ps(_normal.y, reflection_vector.y);
                _mm_store_ps(_normal.z, reflection_vector.z);

                sse_cube_texture_sample((cube_texture_t *) ambient_texture->prefiltered, &_normal, &prefiltered_lod, true, &tmp_color);

                ambient_specular.r = _mm_load_ps(tmp_color.r);
                ambient_specular.g = _mm_load_ps(tmp_color.g);
                ambient_specular.b = _mm_load_ps(tmp_color.b);

                gc_vec_t _dot;
                gc_vec_t _roughness;

                _mm_store_ps(_dot.data, vn_dot);
                _mm_store_ps(_roughness.data, roughness);

                // ----------------------------------------------------------------------------------

                fv2_t _brdf_lut;
                brdf_lut_sample(ambient_texture->brdf_lut, &_dot, &_roughness, &_brdf_lut);

                brdf_lut.x = _mm_load_ps(_brdf_lut.x);
                brdf_lut.y = _mm_load_ps(_brdf_lut.y);

                // ----------------------------------------------------------------------------------

                ambient_specular.r = _mm_mul_ps(ambient_specular.r, _mm_add_ps(_mm_mul_ps(ks.x, brdf_lut.x), brdf_lut.y));
                ambient_specular.g = _mm_mul_ps(ambient_specular.g, _mm_add_ps(_mm_mul_ps(ks.y, brdf_lut.x), brdf_lut.y));
                ambient_specular.b = _mm_mul_ps(ambient_specular.b, _mm_add_ps(_mm_mul_ps(ks.z, brdf_lut.x), brdf_lut.y));

                ambient.r = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_mul_ps(kd.x, ambient.r), albedo.r), ambient_specular.r), ao);
                ambient.g = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_mul_ps(kd.y, ambient.g), albedo.g), ambient_specular.g), ao);
                ambient.b = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_mul_ps(kd.z, ambient.b), albedo.b), ambient_specular.b), ao);
            }
            else
            {
                ambient.r = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(PIPE_PARAM_VECTOR(1, ambient_color, 0)), albedo.r), ao);
                ambient.g = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(PIPE_PARAM_VECTOR(1, ambient_color, 1)), albedo.g), ao);
                ambient.b = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(PIPE_PARAM_VECTOR(1, ambient_color, 2)), albedo.b), ao);
            }
        }
        else
        {
            computed_light_color.r = albedo.r;
            computed_light_color.g = albedo.g;
            computed_light_color.b = albedo.b;
        }

        // _mm_store_ps(fragment->r, _mm_mul_ps(_mm_add_ps(_mm_add_ps(computed_light_color.r, ambient.r), emission.r), _mm_set1_ps(multipliers.albedo.v3.x)));
        // _mm_store_ps(fragment->g, _mm_mul_ps(_mm_add_ps(_mm_add_ps(computed_light_color.g, ambient.g), emission.g), _mm_set1_ps(multipliers.albedo.v3.y)));
        // _mm_store_ps(fragment->b, _mm_mul_ps(_mm_add_ps(_mm_add_ps(computed_light_color.b, ambient.b), emission.b), _mm_set1_ps(multipliers.albedo.v3.z)));

        _mm_store_ps(fragment->r, _mm_add_ps(_mm_add_ps(computed_light_color.r, ambient.r), emission.r));
        _mm_store_ps(fragment->g, _mm_add_ps(_mm_add_ps(computed_light_color.g, ambient.g), emission.g));
        _mm_store_ps(fragment->b, _mm_add_ps(_mm_add_ps(computed_light_color.b, ambient.b), emission.b));
        _mm_store_ps(fragment->a, albedo.a);
    }
}