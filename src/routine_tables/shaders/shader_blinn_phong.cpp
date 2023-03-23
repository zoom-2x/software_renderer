// ----------------------------------------------------------------------------------
// -- File: shader_blinn_phong.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-02-10 19:36:34
// -- Modified: 2022-05-20 22:29:56
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;
#define EMISSION_THRESHOLD 0.01f

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_blinn_phong.cpp"
#define shader_blinn_phong_fs _sse_shader_blinn_phong_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_blinn_phong.cpp"
#define shader_blinn_phong_fs _sse_shader_blinn_phong_fs
#else
#define shader_blinn_phong_fs _shader_blinn_phong_fs
#endif

// ----------------------------------------------------------------------------------
// -- BLINN-PHONG SHADER.
// ----------------------------------------------------------------------------------
// -- Shader setup.
// ----------------------------------------------------------------------------------

void shader_blinn_phong_setup(gc_material_t *material)
{
    GCSR.gl->pipeline.params.material = material;
    GCSR.gl->pipeline.params.shader_flags = 0;

    if (GCSR.gl->pipeline.flags & GC_MODE_RENDERED)
    {
        if (FLAG(material->components, SHADER_FLAG_BLINN_SPECULAR))
        GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_SPECULAR;

        if (FLAG(material->components, SHADER_FLAG_BLINN_FRESNEL))
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_FRESNEL;

        if (PIPE_FLAG(GC_SHADOW) && GCSR.gl->pipeline.params.lights) {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_SHADOW;
        }

        if (material->blinn.diffuse_map && FLAG(material->components, SHADER_FLAG_BLINN_DIFFUSE))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_DIFFUSE;
            GCSR.gl->pipeline.params.textures[0] = material->blinn.diffuse_map;
        }

        if (material->blinn.normal_map && FLAG(material->components, SHADER_FLAG_BLINN_NORMAL))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_NORMAL;
            GCSR.gl->pipeline.params.textures[1] = material->blinn.normal_map;
        }

        if (material->blinn.specular_map) {
            GCSR.gl->pipeline.params.textures[2] = material->blinn.specular_map;
        }

        if (material->blinn.emission_map && FLAG(material->components, SHADER_FLAG_BLINN_EMISSION))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_EMISSION;
            GCSR.gl->pipeline.params.textures[3] = material->blinn.emission_map;
        }

        if (material->blinn.ao_map && FLAG(material->components, SHADER_FLAG_BLINN_AO))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_AO;
            GCSR.gl->pipeline.params.textures[4] = material->blinn.ao_map;
        }

        if (material->blinn.rr_map && FLAG(material->components, SHADER_FLAG_BLINN_RR))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_RR;
            GCSR.gl->pipeline.params.textures[6] = material->blinn.rr_map;
        }

        if (material->blinn.cubemap)
        {
            if (FLAG(material->components, SHADER_FLAG_BLINN_REFLECTION))
                GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_REFLECTION;

            if (FLAG(material->components, SHADER_FLAG_BLINN_REFRACTION))
                GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_REFRACTION;

            GCSR.gl->pipeline.params.textures[5] = material->blinn.cubemap;
        }
    }
    else if (PIPE_FLAG(GC_MODE_SOLID))
    {
        if (PIPE_FLAG(GC_SHADOW) && GCSR.gl->pipeline.params.lights) {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_SHADOW;
        }

        if (material->blinn.ao_map && FLAG(material->components, SHADER_FLAG_BLINN_AO))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_AO;
            GCSR.gl->pipeline.params.textures[4] = material->blinn.ao_map;
        }
    }
    else if (GCSR.gl->pipeline.flags & GC_MODE_MATERIAL)
    {
        if (material->blinn.diffuse_map && FLAG(material->components, SHADER_FLAG_BLINN_DIFFUSE))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_DIFFUSE;
            GCSR.gl->pipeline.params.textures[0] = material->blinn.diffuse_map;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

void shader_blinn_phong_vs(gc_vertex_t *vertex, gc_shader_params_t *params)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);
    VINIT3(vertex_normal, vertex->data[2], vertex->data[3], vertex->data[4]);
    VINIT3(vertex_tangent, vertex->data[5], vertex->data[6], vertex->data[7]);

    gc_vec_t world_position;
    gc_vec_t shadow_space_position;
    gc_vec_t world_normal;
    gc_vec_t view_dir;

    r32 _tangent_w = vertex->data[8];

    gl_mat4_mulvec(GET_MATRIX(M_MODEL_WORLD), (gc_vec_t *) &gl_position, &world_position);
    gl_mat4_mulvec(GET_MATRIX(M_NORMAL_WORLD), &vertex_normal, &world_normal);
    v3_normalize(&world_normal);

    gl_vec3_sub(&params->world_camera_position, &world_position, &view_dir);

    gl_mat4_mulvec(GET_MATRIX(M_NORMAL_WORLD), &vertex_tangent, &vertex_tangent);
    v3_normalize(&vertex_tangent);

    // ----------------------------------------------------------------------------------
    // -- Attributes sent for interpolation.
    // ----------------------------------------------------------------------------------

    // world normal.
    vertex->data[2] = world_normal.v3.x;
    vertex->data[3] = world_normal.v3.y;
    vertex->data[4] = world_normal.v3.z;

    // tangent.
    vertex->data[5] = vertex_tangent.v3.x;
    vertex->data[6] = vertex_tangent.v3.y;
    vertex->data[7] = vertex_tangent.v3.z;
    vertex->data[8] = _tangent_w;

    // world position.
    vertex->data[9] = world_position.v3.x;
    vertex->data[10] = world_position.v3.y;
    vertex->data[11] = world_position.v3.z;

    // view vector.
    vertex->data[12] = view_dir.v3.x;
    vertex->data[13] = view_dir.v3.y;
    vertex->data[14] = view_dir.v3.z;

    // shadow space position (sunlight).
    if (PIPE_FLAG(GC_SHADOW) && params->sun_light)
    {
        world_position.v3.x += world_normal.v3.x * params->sun_light->shadow.normal_offset;
        world_position.v3.y += world_normal.v3.y * params->sun_light->shadow.normal_offset;
        world_position.v3.z += world_normal.v3.z * params->sun_light->shadow.normal_offset;

        gc_mat_t *shadow_matrix = GET_MATRIX(M_SUN_LIGHT);
        gl_mat4_mulvec(shadow_matrix, &world_position, &shadow_space_position);

        vertex->data[15] = shadow_space_position.v4.x * 0.5f + 0.5f;
        vertex->data[16] = shadow_space_position.v4.y * 0.5f + 0.5f;
        vertex->data[17] = shadow_space_position.v4.z * 0.5f + 0.5f;
        vertex->data[18] = shadow_space_position.v4.w;
    }

    // ----------------------------------------------------------------------------------

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

// ----------------------------------------------------------------------------------
// -- Normal fragment shader.
// ----------------------------------------------------------------------------------

void _shader_blinn_phong_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;
    gc_blinn_material_t *material = &params->material->blinn;
    VINIT4(specular, material->specular, material->specular, material->specular, material->specular);

    r32 shininess = material->shininess;
    r32 min_fresnel = material->min_fresnel;
    r32 max_fresnel = material->max_fresnel;

    lod_t lod;
    LOD_CLEAR(lod);

    gc_vec_t material_ambient;
    VCPY4(material_ambient, material->ambient);

    b8 is_uv_scaling = PIPE_PARAM_VECTOR(2, uv_scaling, 0) || PIPE_PARAM_VECTOR(2, uv_scaling, 1);

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        SHADER_COLOR_VAR_SET(diffuse_color, material->diffuse.c.r, material->diffuse.c.g, material->diffuse.c.b, material->diffuse.c.a);

        if (PIPE_FLAG(GC_MODE_SOLID))
        {
            diffuse_color.r[0] = PIPE_PARAM_VECTOR(1, solid_color, 0);
            diffuse_color.r[1] = PIPE_PARAM_VECTOR(1, solid_color, 0);
            diffuse_color.r[2] = PIPE_PARAM_VECTOR(1, solid_color, 0);
            diffuse_color.r[3] = PIPE_PARAM_VECTOR(1, solid_color, 0);

            diffuse_color.g[0] = PIPE_PARAM_VECTOR(1, solid_color, 1);
            diffuse_color.g[1] = PIPE_PARAM_VECTOR(1, solid_color, 1);
            diffuse_color.g[2] = PIPE_PARAM_VECTOR(1, solid_color, 1);
            diffuse_color.g[3] = PIPE_PARAM_VECTOR(1, solid_color, 1);

            diffuse_color.b[0] = PIPE_PARAM_VECTOR(1, solid_color, 2);
            diffuse_color.b[1] = PIPE_PARAM_VECTOR(1, solid_color, 2);
            diffuse_color.b[2] = PIPE_PARAM_VECTOR(1, solid_color, 2);
            diffuse_color.b[3] = PIPE_PARAM_VECTOR(1, solid_color, 2);

            diffuse_color.a[0] = PIPE_PARAM_VECTOR(1, solid_color, 3);
            diffuse_color.a[1] = PIPE_PARAM_VECTOR(1, solid_color, 3);
            diffuse_color.a[2] = PIPE_PARAM_VECTOR(1, solid_color, 3);
            diffuse_color.a[3] = PIPE_PARAM_VECTOR(1, solid_color, 3);
        }

        SHADER_COLOR_VAR_SET(ao_component, 1, 1, 1, 1);
        SHADER_COLOR_VAR_SET(emission_component, 0, 0, 0, 0);

        fv2_t *texcoord = (fv2_t *) fragment->varyings[0];
        fv3_t *world_normal = (fv3_t *) fragment->varyings[2];
        fv4_t *varying_tangent = (fv4_t *) fragment->varyings[5];
        fv3_t *world_position = (fv3_t *) fragment->varyings[9];
        fv3_t *view_vector = (fv3_t *) fragment->varyings[12];
        fv3_t final_normal;

        if (is_uv_scaling)
        {
            texcoord->x[0] *= PIPE_PARAM_VECTOR(2, uv_scaling, 0);
            texcoord->x[1] *= PIPE_PARAM_VECTOR(2, uv_scaling, 0);
            texcoord->x[2] *= PIPE_PARAM_VECTOR(2, uv_scaling, 0);
            texcoord->x[3] *= PIPE_PARAM_VECTOR(2, uv_scaling, 0);

            texcoord->y[0] *= PIPE_PARAM_VECTOR(2, uv_scaling, 1);
            texcoord->y[1] *= PIPE_PARAM_VECTOR(2, uv_scaling, 1);
            texcoord->y[2] *= PIPE_PARAM_VECTOR(2, uv_scaling, 1);
            texcoord->y[3] *= PIPE_PARAM_VECTOR(2, uv_scaling, 1);
        }

        fv3_normalize(world_normal);
        fv3_normalize(view_vector);

        // ----------------------------------------------------------------------------------
        // -- Normal.
        // ----------------------------------------------------------------------------------

        final_normal.x[0] = world_normal->x[0];
        final_normal.x[1] = world_normal->x[1];
        final_normal.x[2] = world_normal->x[2];
        final_normal.x[3] = world_normal->x[3];

        final_normal.y[0] = world_normal->y[0];
        final_normal.y[1] = world_normal->y[1];
        final_normal.y[2] = world_normal->y[2];
        final_normal.y[3] = world_normal->y[3];

        final_normal.z[0] = world_normal->z[0];
        final_normal.z[1] = world_normal->z[1];
        final_normal.z[2] = world_normal->z[2];
        final_normal.z[3] = world_normal->z[3];

        if (params->shader_flags & SHADER_FLAG_BLINN_NORMAL)
        {
            shader_color_t tsp_normal;

            // gc_texture_compute_lod(((texture2d_t *) params->textures[1]), texcoord->x, texcoord->y, &lod);
            gc_texture_compute_lod(((texture2d_t *) params->textures[1]), fragment, &lod);
            texture_sample(((texture2d_t *) params->textures[1]), texcoord->x, texcoord->y, &lod, &tsp_normal);

            tsp_normal.r[0] = tsp_normal.r[0] * 2 - 1;
            tsp_normal.g[0] = tsp_normal.g[0] * 2 - 1;
            tsp_normal.b[0] = tsp_normal.b[0] * 2 - 1;

            tsp_normal.r[1] = tsp_normal.r[1] * 2 - 1;
            tsp_normal.g[1] = tsp_normal.g[1] * 2 - 1;
            tsp_normal.b[1] = tsp_normal.b[1] * 2 - 1;

            tsp_normal.r[2] = tsp_normal.r[2] * 2 - 1;
            tsp_normal.g[2] = tsp_normal.g[2] * 2 - 1;
            tsp_normal.b[2] = tsp_normal.b[2] * 2 - 1;

            tsp_normal.r[3] = tsp_normal.r[3] * 2 - 1;
            tsp_normal.g[3] = tsp_normal.g[3] * 2 - 1;
            tsp_normal.b[3] = tsp_normal.b[3] * 2 - 1;

            // -- Gram-Schmidt orthogonalization.

            gc_vec_t dot;
            fv3_t _tmp_vec;
            fv3_t tangent;
            fv3_t btangent;

            fv3_dot((fv3_t *) varying_tangent, world_normal, &dot);
            fv3_muls(world_normal, &dot, &_tmp_vec);
            fv3_sub((fv3_t *) varying_tangent, &_tmp_vec, &tangent);
            fv3_normalize(&tangent);
            fv3_cross(world_normal, &tangent, &btangent);
            fv3_muls(&btangent, (gc_vec_t *) varying_tangent->w, &btangent);

            fv3_t vec1;
            fv3_t vec2;
            fv3_t vec3;

            fv3_muls(&tangent, (gc_vec_t *) tsp_normal.r, &vec1);
            fv3_muls(&btangent, (gc_vec_t *) tsp_normal.g, &vec2);
            fv3_muls(world_normal, (gc_vec_t *) tsp_normal.b, &vec3);

            // fv3_add(&vec1, &vec2, &final_normal);
            // fv3_add(&final_normal, &vec3, &final_normal);

            final_normal.x[0] = vec1.x[0] + vec2.x[0] + vec3.x[0];
            final_normal.y[0] = vec1.y[0] + vec2.y[0] + vec3.y[0];
            final_normal.z[0] = vec1.z[0] + vec2.z[0] + vec3.z[0];

            final_normal.x[1] = vec1.x[1] + vec2.x[1] + vec3.x[1];
            final_normal.y[1] = vec1.y[1] + vec2.y[1] + vec3.y[1];
            final_normal.z[1] = vec1.z[1] + vec2.z[1] + vec3.z[1];

            final_normal.x[2] = vec1.x[2] + vec2.x[2] + vec3.x[2];
            final_normal.y[2] = vec1.y[2] + vec2.y[2] + vec3.y[2];
            final_normal.z[2] = vec1.z[2] + vec2.z[2] + vec3.z[2];

            final_normal.x[3] = vec1.x[3] + vec2.x[3] + vec3.x[3];
            final_normal.y[3] = vec1.y[3] + vec2.y[3] + vec3.y[3];
            final_normal.z[3] = vec1.z[3] + vec2.z[3] + vec3.z[3];

            fv3_normalize(&final_normal);
        }

        if (fragment->primitive->is_backface)
            fv3_inv(&final_normal);

        if (params->shader_flags & SHADER_FLAG_BLINN_DIFFUSE)
        {
            gc_texture_compute_lod((texture2d_t *) params->textures[0], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod, &diffuse_color);
            gc_gamma_srgb_to_linear_frag(&diffuse_color);
        }

        diffuse_color.r[0] *= material->diffuse_multiplier.v3.x;
        diffuse_color.r[1] *= material->diffuse_multiplier.v3.x;
        diffuse_color.r[2] *= material->diffuse_multiplier.v3.x;
        diffuse_color.r[3] *= material->diffuse_multiplier.v3.x;

        diffuse_color.g[0] *= material->diffuse_multiplier.v3.y;
        diffuse_color.g[1] *= material->diffuse_multiplier.v3.y;
        diffuse_color.g[2] *= material->diffuse_multiplier.v3.y;
        diffuse_color.g[3] *= material->diffuse_multiplier.v3.y;

        diffuse_color.b[0] *= material->diffuse_multiplier.v3.z;
        diffuse_color.b[1] *= material->diffuse_multiplier.v3.z;
        diffuse_color.b[2] *= material->diffuse_multiplier.v3.z;
        diffuse_color.b[3] *= material->diffuse_multiplier.v3.z;

        if (params->shader_flags & SHADER_FLAG_BLINN_EMISSION)
        {
            gc_texture_compute_lod((texture2d_t *) params->textures[3], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[3], texcoord->x, texcoord->y, &lod, &emission_component);
            gc_gamma_srgb_to_linear_frag(&emission_component);
        }

        if (params->shader_flags & SHADER_FLAG_BLINN_AO)
        {
            gc_texture_compute_lod((texture2d_t *) params->textures[4], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[4], texcoord->x, texcoord->y, &lod, &ao_component);
            gc_gamma_srgb_to_linear_frag(&ao_component);
        }

        gc_vec_t vn_dot;
        fv3_t inv_view_vector;

        fv3_dot(view_vector, &final_normal, &vn_dot);
        fv3_inv_to(view_vector, &inv_view_vector);

        // ----------------------------------------------------------------------------------
        // -- Ambient reflection/refraction.
        // ----------------------------------------------------------------------------------

        if (params->shader_flags & (SHADER_FLAG_BLINN_REFLECTION | SHADER_FLAG_BLINN_REFLECTION))
        {
            gc_vec_t vn_dot2;
            fv3_dot(&inv_view_vector, &final_normal, &vn_dot2);

            shader_color_t ambient_reflection_color;
            shader_color_t ambient_refraction_color;
            shader_color_t reflection_refraction;

            shader_color_t *refl_color = &ambient_reflection_color;
            shader_color_t *refr_color = &ambient_reflection_color;

            if (params->shader_flags & SHADER_FLAG_BLINN_REFLECTION)
            {
                fv3_t reflection;
                fv3_muls(&final_normal, &vn_dot2, &reflection);
                fv3_muls1(&reflection, 2, &reflection);
                fv3_sub(&inv_view_vector, &reflection, &reflection);

                lod_t reflection_lod;
                LOD_CLEAR(reflection_lod);

                // gl_cubemap_compute_lod((cube_texture_t *) params->textures[5], texcoord->x, texcoord->y, &reflection_lod);
                gl_cubemap_compute_lod((cube_texture_t *) params->textures[5], fragment, &reflection_lod);
                cube_texture_sample((cube_texture_t *) params->textures[5], &reflection, &reflection_lod, false, &ambient_reflection_color);
                gc_gamma_srgb_to_linear_frag(&ambient_reflection_color);
            }

            if (params->shader_flags & SHADER_FLAG_BLINN_REFRACTION)
            {
                fv3_t refraction;
                r32 refr2 = material->refr_ratio * material->refr_ratio;

                VINIT4(ik,
                    1.0f - refr2 * (1.0f - vn_dot2.data[0] * vn_dot2.data[0]),
                    1.0f - refr2 * (1.0f - vn_dot2.data[1] * vn_dot2.data[1]),
                    1.0f - refr2 * (1.0f - vn_dot2.data[2] * vn_dot2.data[2]),
                    1.0f - refr2 * (1.0f - vn_dot2.data[3] * vn_dot2.data[3]));

                VINIT4(tmp,
                    material->refr_ratio * vn_dot2.data[0] + sqrtf(ik.data[0]),
                    material->refr_ratio * vn_dot2.data[1] + sqrtf(ik.data[1]),
                    material->refr_ratio * vn_dot2.data[2] + sqrtf(ik.data[2]),
                    material->refr_ratio * vn_dot2.data[3] + sqrtf(ik.data[3]));

                fv3_t v1;
                fv3_t v2;

                fv3_muls1(&inv_view_vector, material->refr_ratio, &v1);
                fv3_muls(&final_normal, &tmp, &v2);
                fv3_sub(&v1, &v2, &refraction);

                if (ik.data[0] < 0.0f)
                {
                    refraction.x[0] = 0;
                    refraction.y[0] = 0;
                    refraction.z[0] = 0;
                }

                if (ik.data[1] < 0.0f)
                {
                    refraction.x[1] = 0;
                    refraction.y[1] = 0;
                    refraction.z[1] = 0;
                }

                if (ik.data[2] < 0.0f)
                {
                    refraction.x[2] = 0;
                    refraction.y[2] = 0;
                    refraction.z[2] = 0;
                }

                if (ik.data[3] < 0.0f)
                {
                    refraction.x[3] = 0;
                    refraction.y[3] = 0;
                    refraction.z[3] = 0;
                }

                lod_t refraction_lod;
                LOD_CLEAR(refraction_lod);

                gl_cubemap_compute_lod((cube_texture_t *) params->textures[5], fragment, &refraction_lod);
                cube_texture_sample((cube_texture_t *) params->textures[5], &refraction, &refraction_lod, false, &ambient_refraction_color);
                gc_gamma_srgb_to_linear_frag(&ambient_refraction_color);

                refr_color = &ambient_refraction_color;

                if (!(params->shader_flags & SHADER_FLAG_BLINN_REFLECTION))
                    refl_color = &ambient_refraction_color;
            }

            shader_color_mix(refl_color, refr_color, material->rr_ratio, &reflection_refraction);

            VINIT4(rr_mult,
                   material->rr_diffuse_ratio,
                   material->rr_diffuse_ratio,
                   material->rr_diffuse_ratio,
                   material->rr_diffuse_ratio);

            if (params->shader_flags & SHADER_FLAG_BLINN_RR)
            {
                shader_color_t rr_sample;
                gc_texture_compute_lod((texture2d_t *) params->textures[6], fragment, &lod);
                texture_sample((texture2d_t *) params->textures[6], texcoord->x, texcoord->y, &lod, &rr_sample);
                gc_gamma_srgb_to_linear_frag(&rr_sample);

                rr_mult.data[0] *= rr_sample.r[0];
                rr_mult.data[1] *= rr_sample.r[1];
                rr_mult.data[2] *= rr_sample.r[2];
                rr_mult.data[3] *= rr_sample.r[3];
            }

            shader_color_mixv(&diffuse_color, &reflection_refraction, &rr_mult, &diffuse_color);
        }

        shader_color_t constant_material_ambient;

        constant_material_ambient.r[0] = diffuse_color.r[0] * PIPE_PARAM_VECTOR(1, ambient_color, 0) * ao_component.r[0];
        constant_material_ambient.r[1] = diffuse_color.r[1] * PIPE_PARAM_VECTOR(1, ambient_color, 0) * ao_component.r[1];
        constant_material_ambient.r[2] = diffuse_color.r[2] * PIPE_PARAM_VECTOR(1, ambient_color, 0) * ao_component.r[2];
        constant_material_ambient.r[3] = diffuse_color.r[3] * PIPE_PARAM_VECTOR(1, ambient_color, 0) * ao_component.r[3];

        constant_material_ambient.g[0] = diffuse_color.g[0] * PIPE_PARAM_VECTOR(1, ambient_color, 1) * ao_component.g[0];
        constant_material_ambient.g[1] = diffuse_color.g[1] * PIPE_PARAM_VECTOR(1, ambient_color, 1) * ao_component.g[1];
        constant_material_ambient.g[2] = diffuse_color.g[2] * PIPE_PARAM_VECTOR(1, ambient_color, 1) * ao_component.g[2];
        constant_material_ambient.g[3] = diffuse_color.g[3] * PIPE_PARAM_VECTOR(1, ambient_color, 1) * ao_component.g[3];

        constant_material_ambient.b[0] = diffuse_color.b[0] * PIPE_PARAM_VECTOR(1, ambient_color, 2) * ao_component.b[0];
        constant_material_ambient.b[1] = diffuse_color.b[1] * PIPE_PARAM_VECTOR(1, ambient_color, 2) * ao_component.b[1];
        constant_material_ambient.b[2] = diffuse_color.b[2] * PIPE_PARAM_VECTOR(1, ambient_color, 2) * ao_component.b[2];
        constant_material_ambient.b[3] = diffuse_color.b[3] * PIPE_PARAM_VECTOR(1, ambient_color, 2) * ao_component.b[3];

        // ----------------------------------------------------------------------------------
        // -- Lights.
        // ----------------------------------------------------------------------------------

        SHADER_COLOR_VAR(light_diffuse_color);
        SHADER_COLOR_VAR(light_specular_color);
        SHADER_COLOR_VAR(light_ambient_color);
        SHADER_COLOR_VAR(light_emission_color);

        if (!(GCSR.gl->pipeline.flags & GC_MODE_MATERIAL))
        {
            for (u32 k = 0; k < params->light_count; ++k)
            {
                gc_light_t *light = params->lights + k;

                fv2_t light_texcoord;
                fv3_t light_dir;
                gc_vec_t ln_dot;
                gc_vec_t distance;
                r32 attenuation[GC_FRAG_SIZE];

                attenuation[0] = 1;
                attenuation[1] = 1;
                attenuation[2] = 1;
                attenuation[3] = 1;

                // The fragment color is calculated for each light, including the shadow visibility.

                if (light->type == GC_SUN_LIGHT)
                {
                    light_dir.x[0] = light->directional.direction.v3.x;
                    light_dir.x[1] = light->directional.direction.v3.x;
                    light_dir.x[2] = light->directional.direction.v3.x;
                    light_dir.x[3] = light->directional.direction.v3.x;

                    light_dir.y[0] = light->directional.direction.v3.y;
                    light_dir.y[1] = light->directional.direction.v3.y;
                    light_dir.y[2] = light->directional.direction.v3.y;
                    light_dir.y[3] = light->directional.direction.v3.y;

                    light_dir.z[0] = light->directional.direction.v3.z;
                    light_dir.z[1] = light->directional.direction.v3.z;
                    light_dir.z[2] = light->directional.direction.v3.z;
                    light_dir.z[3] = light->directional.direction.v3.z;

                    fv3_dot(&light_dir, &final_normal, &ln_dot);
                    v4_max(&ln_dot, 0);

                    if (params->shader_flags & SHADER_FLAG_SHADOW)
                    {
                        // Fragment shadow uv calculation.
                        for (u8 ti = 0; ti < GC_FRAG_SIZE; ++ti)
                        {
                            r32 num = 1.0f / fragment->varyings[18][ti];
                            distance.data[ti] = fragment->varyings[17][ti] * num - light->shadow.depth_bias;

                            light_texcoord.x[ti] = fragment->varyings[15][ti] * num;
                            light_texcoord.y[ti] = 1.0f - (fragment->varyings[16][ti] * num);
                        }

                        light->shadow.sun_shadow_visibility_r((texture2d_t *) light->shadow_texture, light_texcoord.x, light_texcoord.y, distance.data, fragment->shadow);
                    }
                }
                else if (light->type == GC_POINT_LIGHT)
                {
                    fv3_vec_sub(&light->object.position, world_position, &light_dir);
                    fv3_lenv(&light_dir, &distance);
                    fv3_normalize(&light_dir);

                    fv3_dot(&light_dir, &final_normal, &ln_dot);
                    v4_max(&ln_dot, 0);

                    attenuation[0] = 1.0f / (light->point.kc + light->point.kl * distance.data[0] + light->point.kq * distance.data[0] * distance.data[0]);
                    attenuation[1] = 1.0f / (light->point.kc + light->point.kl * distance.data[1] + light->point.kq * distance.data[1] * distance.data[1]);
                    attenuation[2] = 1.0f / (light->point.kc + light->point.kl * distance.data[2] + light->point.kq * distance.data[2] * distance.data[2]);
                    attenuation[3] = 1.0f / (light->point.kc + light->point.kl * distance.data[3] + light->point.kq * distance.data[3] * distance.data[3]);

                    // ----------------------------------------------------------------------------------
                    // -- Shadow visibility.
                    // ----------------------------------------------------------------------------------

                    if (params->shader_flags & SHADER_FLAG_SHADOW)
                    {
                        gc_vec_t compare;

                        // Determine the direction vector used to sample the shadow cubemap.
                        fv3_t direction;

                        A4SET(direction.x,
                              world_position->x[0] - light->object.position.v3.x,
                              world_position->x[1] - light->object.position.v3.x,
                              world_position->x[2] - light->object.position.v3.x,
                              world_position->x[3] - light->object.position.v3.x);

                        A4SET(direction.y,
                              world_position->y[0] - light->object.position.v3.y,
                              world_position->y[1] - light->object.position.v3.y,
                              world_position->y[2] - light->object.position.v3.y,
                              world_position->y[3] - light->object.position.v3.y);

                        A4SET(direction.z,
                              world_position->z[0] - light->object.position.v3.z,
                              world_position->z[1] - light->object.position.v3.z,
                              world_position->z[2] - light->object.position.v3.z,
                              world_position->z[3] - light->object.position.v3.z);

                        fv3_lenv(&direction, &compare);

                        compare.data[0] = compare.data[0] * light->shadow.f_len_inv - light->shadow.depth_bias;
                        compare.data[1] = compare.data[1] * light->shadow.f_len_inv - light->shadow.depth_bias;
                        compare.data[2] = compare.data[2] * light->shadow.f_len_inv - light->shadow.depth_bias;
                        compare.data[3] = compare.data[3] * light->shadow.f_len_inv - light->shadow.depth_bias;

                        light->shadow.point_shadow_visibility_r((cube_texture_t *) light->shadow_texture, &direction, compare.data, light->shadow.radius, fragment->shadow);
                    }
                }
                else
                    continue;

                // ----------------------------------------------------------------------------------
                // -- Light diffuse.
                // ----------------------------------------------------------------------------------

                attenuation[0] *= fragment->shadow[0];
                attenuation[1] *= fragment->shadow[1];
                attenuation[2] *= fragment->shadow[2];
                attenuation[3] *= fragment->shadow[3];

                r32 lc[4];

                A4SET(lc,
                      ln_dot.data[0] * attenuation[0],
                      ln_dot.data[1] * attenuation[1],
                      ln_dot.data[2] * attenuation[2],
                      ln_dot.data[3] * attenuation[3]);

                light_diffuse_color.r[0] += diffuse_color.r[0] * light->color.c.r * lc[0];
                light_diffuse_color.g[0] += diffuse_color.g[0] * light->color.c.g * lc[0];
                light_diffuse_color.b[0] += diffuse_color.b[0] * light->color.c.b * lc[0];

                light_diffuse_color.r[1] += diffuse_color.r[1] * light->color.c.r * lc[1];
                light_diffuse_color.g[1] += diffuse_color.g[1] * light->color.c.g * lc[1];
                light_diffuse_color.b[1] += diffuse_color.b[1] * light->color.c.b * lc[1];

                light_diffuse_color.r[2] += diffuse_color.r[2] * light->color.c.r * lc[2];
                light_diffuse_color.g[2] += diffuse_color.g[2] * light->color.c.g * lc[2];
                light_diffuse_color.b[2] += diffuse_color.b[2] * light->color.c.b * lc[2];

                light_diffuse_color.r[3] += diffuse_color.r[3] * light->color.c.r * lc[3];
                light_diffuse_color.g[3] += diffuse_color.g[3] * light->color.c.g * lc[3];
                light_diffuse_color.b[3] += diffuse_color.b[3] * light->color.c.b * lc[3];

                // ----------------------------------------------------------------------------------
                // -- Light specular.
                // ----------------------------------------------------------------------------------

                if (params->shader_flags & SHADER_FLAG_BLINN_SPECULAR)
                {
                    gc_vec_t fresnel = {1.0f, 1.0f, 1.0f, 1.0f};

                    if (params->textures[2])
                    {
                        shader_color_t specular_sample;
                        // gc_texture_compute_lod((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod);
                        gc_texture_compute_lod((texture2d_t *) params->textures[2], fragment, &lod);
                        texture_sample((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod, &specular_sample);

                        specular.data[0] = specular_sample.r[0] * material->specular;
                        specular.data[1] = specular_sample.r[1] * material->specular;
                        specular.data[2] = specular_sample.r[2] * material->specular;
                        specular.data[3] = specular_sample.r[3] * material->specular;
                    }

                    if (params->shader_flags & SHADER_FLAG_BLINN_FRESNEL)
                    {
                        fresnel.data[0] = clamp(0, 1, vn_dot.data[0]);
                        fresnel.data[1] = clamp(0, 1, vn_dot.data[1]);
                        fresnel.data[2] = clamp(0, 1, vn_dot.data[2]);
                        fresnel.data[3] = clamp(0, 1, vn_dot.data[3]);

                        fresnel.data[0] = 1.0f - fresnel.data[0];
                        fresnel.data[1] = 1.0f - fresnel.data[1];
                        fresnel.data[2] = 1.0f - fresnel.data[2];
                        fresnel.data[3] = 1.0f - fresnel.data[3];

                        fresnel.data[0] = clamp(0, 1, fresnel.data[0] * fresnel.data[0] * fresnel.data[0] * fresnel.data[0] * fresnel.data[0]);
                        fresnel.data[1] = clamp(0, 1, fresnel.data[1] * fresnel.data[1] * fresnel.data[1] * fresnel.data[1] * fresnel.data[1]);
                        fresnel.data[2] = clamp(0, 1, fresnel.data[2] * fresnel.data[2] * fresnel.data[2] * fresnel.data[2] * fresnel.data[2]);
                        fresnel.data[3] = clamp(0, 1, fresnel.data[3] * fresnel.data[3] * fresnel.data[3] * fresnel.data[3] * fresnel.data[3]);

                        fresnel.data[0] = min_fresnel + (max_fresnel - min_fresnel) * fresnel.data[0];
                        fresnel.data[1] = min_fresnel + (max_fresnel - min_fresnel) * fresnel.data[1];
                        fresnel.data[2] = min_fresnel + (max_fresnel - min_fresnel) * fresnel.data[2];
                        fresnel.data[3] = min_fresnel + (max_fresnel - min_fresnel) * fresnel.data[3];
                    }

                    gc_vec_t spec_dot;
                    fv3_t half_vector;

                    fv3_add(&light_dir, view_vector, &half_vector);
                    fv3_normalize(&half_vector);
                    fv3_dot(&half_vector, &final_normal, &spec_dot);

                    if (spec_dot.data[0] > 0)
                    {
                        r32 spec_term = exp(-shininess * (1 - spec_dot.data[0])) * light->il * specular.data[0] * fresnel.data[0] * ln_dot.data[0];

                        light_specular_color.r[0] += spec_term * light->color.c.r * attenuation[0];
                        light_specular_color.g[0] += spec_term * light->color.c.g * attenuation[0];
                        light_specular_color.b[0] += spec_term * light->color.c.b * attenuation[0];
                    }

                    if (spec_dot.data[1] > 0)
                    {
                        r32 spec_term = exp(-shininess * (1 - spec_dot.data[1])) * light->il * specular.data[1] * fresnel.data[1] * ln_dot.data[1];

                        light_specular_color.r[1] += spec_term * light->color.c.r * attenuation[1];
                        light_specular_color.g[1] += spec_term * light->color.c.g * attenuation[1];
                        light_specular_color.b[1] += spec_term * light->color.c.b * attenuation[1];
                    }

                    if (spec_dot.data[2] > 0)
                    {
                        r32 spec_term = exp(-shininess * (1 - spec_dot.data[2])) * light->il * specular.data[2] * fresnel.data[2] * ln_dot.data[2];

                        light_specular_color.r[2] += spec_term * light->color.c.r * attenuation[2];
                        light_specular_color.g[2] += spec_term * light->color.c.g * attenuation[2];
                        light_specular_color.b[2] += spec_term * light->color.c.b * attenuation[2];
                    }

                    if (spec_dot.data[3] > 0)
                    {
                        r32 spec_term = exp(-shininess * (1 - spec_dot.data[3])) * light->il * specular.data[3] * fresnel.data[3] * ln_dot.data[3];

                        light_specular_color.r[3] += spec_term * light->color.c.r * attenuation[3];
                        light_specular_color.g[3] += spec_term * light->color.c.g * attenuation[3];
                        light_specular_color.b[3] += spec_term * light->color.c.b * attenuation[3];
                    }
                }

                // ----------------------------------------------------------------------------------
                // -- Light ambient.
                // ----------------------------------------------------------------------------------

                light_ambient_color.r[0] += constant_material_ambient.r[0];
                light_ambient_color.g[0] += constant_material_ambient.g[0];
                light_ambient_color.b[0] += constant_material_ambient.b[0];

                light_ambient_color.r[1] += constant_material_ambient.r[1];
                light_ambient_color.g[1] += constant_material_ambient.g[1];
                light_ambient_color.b[1] += constant_material_ambient.b[1];

                light_ambient_color.r[2] += constant_material_ambient.r[2];
                light_ambient_color.g[2] += constant_material_ambient.g[2];
                light_ambient_color.b[2] += constant_material_ambient.b[2];

                light_ambient_color.r[3] += constant_material_ambient.r[3];
                light_ambient_color.g[3] += constant_material_ambient.g[3];
                light_ambient_color.b[3] += constant_material_ambient.b[3];

                // ----------------------------------------------------------------------------------
                // -- Light emission.
                // ----------------------------------------------------------------------------------

                light_emission_color.r[0] += emission_component.r[0];
                light_emission_color.g[0] += emission_component.g[0];
                light_emission_color.b[0] += emission_component.b[0];

                light_emission_color.r[1] += emission_component.r[1];
                light_emission_color.g[1] += emission_component.g[1];
                light_emission_color.b[1] += emission_component.b[1];

                light_emission_color.r[2] += emission_component.r[2];
                light_emission_color.g[2] += emission_component.g[2];
                light_emission_color.b[2] += emission_component.b[2];

                light_emission_color.r[3] += emission_component.r[3];
                light_emission_color.g[3] += emission_component.g[3];
                light_emission_color.b[3] += emission_component.b[3];
            }
        }
        else
        {
            light_diffuse_color.r[0] = diffuse_color.r[0];
            light_diffuse_color.g[0] = diffuse_color.g[0];
            light_diffuse_color.b[0] = diffuse_color.b[0];

            light_diffuse_color.r[1] = diffuse_color.r[1];
            light_diffuse_color.g[1] = diffuse_color.g[1];
            light_diffuse_color.b[1] = diffuse_color.b[1];

            light_diffuse_color.r[2] = diffuse_color.r[2];
            light_diffuse_color.g[2] = diffuse_color.g[2];
            light_diffuse_color.b[2] = diffuse_color.b[2];

            light_diffuse_color.r[3] = diffuse_color.r[3];
            light_diffuse_color.g[3] = diffuse_color.g[3];
            light_diffuse_color.b[3] = diffuse_color.b[3];
        }

        fragment->r[0] = (light_diffuse_color.r[0] + light_specular_color.r[0] + light_ambient_color.r[0] + light_emission_color.r[0]) * ao_component.r[0];
        fragment->g[0] = (light_diffuse_color.g[0] + light_specular_color.g[0] + light_ambient_color.g[0] + light_emission_color.g[0]) * ao_component.g[0];
        fragment->b[0] = (light_diffuse_color.b[0] + light_specular_color.b[0] + light_ambient_color.b[0] + light_emission_color.b[0]) * ao_component.b[0];

        fragment->r[1] = (light_diffuse_color.r[1] + light_specular_color.r[1] + light_ambient_color.r[1] + light_emission_color.r[1]) * ao_component.r[1];
        fragment->g[1] = (light_diffuse_color.g[1] + light_specular_color.g[1] + light_ambient_color.g[1] + light_emission_color.g[1]) * ao_component.g[1];
        fragment->b[1] = (light_diffuse_color.b[1] + light_specular_color.b[1] + light_ambient_color.b[1] + light_emission_color.b[1]) * ao_component.b[1];

        fragment->r[2] = (light_diffuse_color.r[2] + light_specular_color.r[2] + light_ambient_color.r[2] + light_emission_color.r[2]) * ao_component.r[2];
        fragment->g[2] = (light_diffuse_color.g[2] + light_specular_color.g[2] + light_ambient_color.g[2] + light_emission_color.g[2]) * ao_component.g[2];
        fragment->b[2] = (light_diffuse_color.b[2] + light_specular_color.b[2] + light_ambient_color.b[2] + light_emission_color.b[2]) * ao_component.b[2];

        fragment->r[3] = (light_diffuse_color.r[3] + light_specular_color.r[3] + light_ambient_color.r[3] + light_emission_color.r[3]) * ao_component.r[3];
        fragment->g[3] = (light_diffuse_color.g[3] + light_specular_color.g[3] + light_ambient_color.g[3] + light_emission_color.g[3]) * ao_component.g[3];
        fragment->b[3] = (light_diffuse_color.b[3] + light_specular_color.b[3] + light_ambient_color.b[3] + light_emission_color.b[3]) * ao_component.b[3];

        fragment->a[0] = diffuse_color.a[0];
        fragment->a[1] = diffuse_color.a[1];
        fragment->a[2] = diffuse_color.a[2];
        fragment->a[3] = diffuse_color.a[3];
    }
}
