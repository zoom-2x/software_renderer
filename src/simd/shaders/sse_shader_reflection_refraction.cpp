// ----------------------------------------------------------------------------------
// -- File: sse_shader_reflection_refraction.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-17 12:37:22
// -- Modified: 2022-10-17 12:37:22
// ----------------------------------------------------------------------------------

void _sse_shader_reflection_refraction_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;
    gc_blinn_material_t *material = &params->material->blinn;
    gc_light_t *light = params->lights;

    lod_t lod;
    LOD_CLEAR(lod);

    fv3_t cube_vec;

    r32 refk = material->refr_ratio * material->refr_ratio;
    __m128 mat_refr_ratio = _mm_set1_ps(material->refr_ratio);

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        sse_color_t diffuse_color;

        diffuse_color.r = _mm_set1_ps(material->diffuse.c.r);
        diffuse_color.g = _mm_set1_ps(material->diffuse.c.g);
        diffuse_color.b = _mm_set1_ps(material->diffuse.c.b);
        diffuse_color.a = _mm_set1_ps(material->diffuse.c.a);

        sse_v3_t world_normal;
        sse_v3_t world_position;
        sse_v3_t view_vector;

        world_normal.x = _mm_load_ps(fragment->varyings[0]);
        world_normal.y = _mm_load_ps(fragment->varyings[1]);
        world_normal.z = _mm_load_ps(fragment->varyings[2]);

        world_position.x = _mm_load_ps(fragment->varyings[3]);
        world_position.y = _mm_load_ps(fragment->varyings[4]);
        world_position.z = _mm_load_ps(fragment->varyings[5]);

        view_vector.x = _mm_load_ps(fragment->varyings[6]);
        view_vector.y = _mm_load_ps(fragment->varyings[7]);
        view_vector.z = _mm_load_ps(fragment->varyings[8]);

        sse_v3_normalize(&world_normal);
        sse_v3_normalize(&view_vector);

        sse_color_t ambient_reflection_color;
        sse_color_t ambient_refraction_color;
        sse_color_t reflection_refraction;
        shader_color_t tmp_color;

        // ----------------------------------------------------------------------------------
        // -- Ambient reflection/refraction.
        // ----------------------------------------------------------------------------------

        if ((params->shader_flags & (SHADER_FLAG_BLINN_REFLECTION | SHADER_FLAG_BLINN_REFRACTION)) && params->textures[0])
        {
            __m128 vn_dot = sse_v3_dot(&view_vector, &world_normal);

            if (params->shader_flags & SHADER_FLAG_BLINN_REFLECTION)
            {
                sse_v3_t reflection;
                sse_v3_muls(&world_normal, _mm_mul_ps(vn_dot, _mm_set1_ps(2)), &reflection);
                sse_v3_sub(&view_vector, &reflection, &reflection);

                _mm_store_ps(cube_vec.x, reflection.x);
                _mm_store_ps(cube_vec.y, reflection.y);
                _mm_store_ps(cube_vec.z, reflection.z);

                cube_texture_sample((cube_texture_t *) params->textures[0], &cube_vec, &lod, false, &tmp_color);
                SSE_GAMMA_SRGB_TO_LINEAR(&tmp_color);

                ambient_reflection_color.r = _mm_load_ps(tmp_color.r);
                ambient_reflection_color.g = _mm_load_ps(tmp_color.g);
                ambient_reflection_color.b = _mm_load_ps(tmp_color.b);

                ambient_refraction_color.r = _mm_load_ps(tmp_color.r);
                ambient_refraction_color.g = _mm_load_ps(tmp_color.g);
                ambient_refraction_color.b = _mm_load_ps(tmp_color.b);
            }

            if (params->shader_flags & SHADER_FLAG_BLINN_REFRACTION)
            {
                sse_v3_t refraction;

                __m128 ik = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_mul_ps(_mm_set1_ps(refk), _mm_sub_ps(_mm_set1_ps(1.0f), _mm_mul_ps(vn_dot, vn_dot))));
                __m128 tmp = _mm_add_ps(_mm_mul_ps(vn_dot, mat_refr_ratio), _mm_sqrt_ps(ik));

                sse_v3_t v1;
                sse_v3_t v2;

                sse_v3_muls(&view_vector, mat_refr_ratio, &v1);
                sse_v3_muls(&world_normal, tmp, &v2);
                sse_v3_sub(&v1, &v2, &refraction);

                __m128 mask = _mm_cmpge_ps(ik, _mm_setzero_ps());

                refraction.x = _mm_and_ps(refraction.x, mask);
                refraction.y = _mm_and_ps(refraction.y, mask);
                refraction.z = _mm_and_ps(refraction.z, mask);

                _mm_store_ps(cube_vec.x, refraction.x);
                _mm_store_ps(cube_vec.y, refraction.y);
                _mm_store_ps(cube_vec.z, refraction.z);

                cube_texture_sample((cube_texture_t *) params->textures[0], &cube_vec, &lod, false, &tmp_color);
                SSE_GAMMA_SRGB_TO_LINEAR(&tmp_color);

                ambient_refraction_color.r = _mm_load_ps(tmp_color.r);
                ambient_refraction_color.g = _mm_load_ps(tmp_color.g);
                ambient_refraction_color.b = _mm_load_ps(tmp_color.b);

                if (!(params->shader_flags & SHADER_FLAG_BLINN_REFLECTION))
                {
                    ambient_reflection_color.r = _mm_load_ps(tmp_color.r);
                    ambient_reflection_color.g = _mm_load_ps(tmp_color.g);
                    ambient_reflection_color.b = _mm_load_ps(tmp_color.b);
                }
            }

            sse_v3_mix((sse_v3_t *) &ambient_reflection_color,
                        (sse_v3_t *) &ambient_refraction_color,
                        material->rr_ratio,
                        (sse_v3_t *) &reflection_refraction);
        }

        if (params->shader_flags & SHADER_FLAG_BLINN_DIFFUSE)
        {
            sse_v3_mix((sse_v3_t *) &diffuse_color,
                        (sse_v3_t *) &reflection_refraction,
                        material->rr_diffuse_ratio,
                        (sse_v3_t *) &reflection_refraction);
        }

        sse_v3_muls((sse_v3_t *) &reflection_refraction, _mm_set1_ps(light->il), (sse_v3_t *) &reflection_refraction);

        _mm_store_ps(fragment->r, reflection_refraction.r);
        _mm_store_ps(fragment->g, reflection_refraction.g);
        _mm_store_ps(fragment->b, reflection_refraction.b);
        _mm_store_ps(fragment->a, diffuse_color.a);
    }
}