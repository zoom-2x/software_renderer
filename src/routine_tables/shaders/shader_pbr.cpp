// ----------------------------------------------------------------------------------
// -- File: shader_pbr.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-06-18 14:55:11
// -- Modified: 2022-06-18 14:55:13
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_pbr.cpp"
#define shader_pbr_fs _sse_shader_pbr_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_pbr.cpp"
#define shader_pbr_fs _sse_shader_pbr_fs
#else
#define shader_pbr_fs _shader_pbr_fs
#endif

__INLINE__ void fresnel_schlick(gc_vec_t *hv_dot, fv3_t *f0, fv3_t *out)
{
    VINIT4(fc,
           1.0f - hv_dot->data[0],
           1.0f - hv_dot->data[1],
           1.0f - hv_dot->data[2],
           1.0f - hv_dot->data[3]);

    fc.data[0] = fc.data[0] * fc.data[0] * fc.data[0] * fc.data[0] * fc.data[0];
    fc.data[1] = fc.data[1] * fc.data[1] * fc.data[1] * fc.data[1] * fc.data[1];
    fc.data[2] = fc.data[2] * fc.data[2] * fc.data[2] * fc.data[2] * fc.data[2];
    fc.data[3] = fc.data[3] * fc.data[3] * fc.data[3] * fc.data[3] * fc.data[3];

    // ks, reflected energy.
    out->x[0] = f0->x[0] + (1.0f - f0->x[0]) * fc.data[0];
    out->x[1] = f0->x[1] + (1.0f - f0->x[1]) * fc.data[1];
    out->x[2] = f0->x[2] + (1.0f - f0->x[2]) * fc.data[2];
    out->x[3] = f0->x[3] + (1.0f - f0->x[3]) * fc.data[3];

    out->y[0] = f0->y[0] + (1.0f - f0->y[0]) * fc.data[0];
    out->y[1] = f0->y[1] + (1.0f - f0->y[1]) * fc.data[1];
    out->y[2] = f0->y[2] + (1.0f - f0->y[2]) * fc.data[2];
    out->y[3] = f0->y[3] + (1.0f - f0->y[3]) * fc.data[3];

    out->z[0] = f0->z[0] + (1.0f - f0->z[0]) * fc.data[0];
    out->z[1] = f0->z[1] + (1.0f - f0->z[1]) * fc.data[1];
    out->z[2] = f0->z[2] + (1.0f - f0->z[2]) * fc.data[2];
    out->z[3] = f0->z[3] + (1.0f - f0->z[3]) * fc.data[3];
}

__INLINE__ void fresnel_schlick_roughness(gc_vec_t *hv_dot, fv3_t *f0, gc_vec_t *roughness, fv3_t *out)
{
    VINIT4(fc,
           1.0f - hv_dot->data[0],
           1.0f - hv_dot->data[1],
           1.0f - hv_dot->data[2],
           1.0f - hv_dot->data[3]);

    fc.data[0] = fc.data[0] * fc.data[0] * fc.data[0] * fc.data[0] * fc.data[0];
    fc.data[1] = fc.data[1] * fc.data[1] * fc.data[1] * fc.data[1] * fc.data[1];
    fc.data[2] = fc.data[2] * fc.data[2] * fc.data[2] * fc.data[2] * fc.data[2];
    fc.data[3] = fc.data[3] * fc.data[3] * fc.data[3] * fc.data[3] * fc.data[3];

    VINIT4(tmp,
           1.0f - roughness->data[0],
           1.0f - roughness->data[1],
           1.0f - roughness->data[2],
           1.0f - roughness->data[3]);

    fv3_t tf0;

    tf0.x[0] = tmp.data[0] >= f0->x[0] ? tmp.data[0] : f0->x[0];
    tf0.x[1] = tmp.data[1] >= f0->x[1] ? tmp.data[1] : f0->x[1];
    tf0.x[2] = tmp.data[2] >= f0->x[2] ? tmp.data[2] : f0->x[2];
    tf0.x[3] = tmp.data[3] >= f0->x[3] ? tmp.data[3] : f0->x[3];

    tf0.y[0] = tmp.data[0] >= f0->y[0] ? tmp.data[0] : f0->y[0];
    tf0.y[1] = tmp.data[1] >= f0->y[1] ? tmp.data[1] : f0->y[1];
    tf0.y[2] = tmp.data[2] >= f0->y[2] ? tmp.data[2] : f0->y[2];
    tf0.y[3] = tmp.data[3] >= f0->y[3] ? tmp.data[3] : f0->y[3];

    tf0.z[0] = tmp.data[0] >= f0->z[0] ? tmp.data[0] : f0->z[0];
    tf0.z[1] = tmp.data[1] >= f0->z[1] ? tmp.data[1] : f0->z[1];
    tf0.z[2] = tmp.data[2] >= f0->z[2] ? tmp.data[2] : f0->z[2];
    tf0.z[3] = tmp.data[3] >= f0->z[3] ? tmp.data[3] : f0->z[3];

    // ks, reflected energy.
    out->x[0] = f0->x[0] + (tf0.x[0] - f0->x[0]) * fc.data[0];
    out->x[1] = f0->x[1] + (tf0.x[1] - f0->x[1]) * fc.data[1];
    out->x[2] = f0->x[2] + (tf0.x[2] - f0->x[2]) * fc.data[2];
    out->x[3] = f0->x[3] + (tf0.x[3] - f0->x[3]) * fc.data[3];

    out->y[0] = f0->y[0] + (tf0.y[0] - f0->y[0]) * fc.data[0];
    out->y[1] = f0->y[1] + (tf0.y[1] - f0->y[1]) * fc.data[1];
    out->y[2] = f0->y[2] + (tf0.y[2] - f0->y[2]) * fc.data[2];
    out->y[3] = f0->y[3] + (tf0.y[3] - f0->y[3]) * fc.data[3];

    out->z[0] = f0->z[0] + (tf0.z[0] - f0->z[0]) * fc.data[0];
    out->z[1] = f0->z[1] + (tf0.z[1] - f0->z[1]) * fc.data[1];
    out->z[2] = f0->z[2] + (tf0.z[2] - f0->z[2]) * fc.data[2];
    out->z[3] = f0->z[3] + (tf0.z[3] - f0->z[3]) * fc.data[3];
}

__INLINE__ void distribution_ggx(gc_vec_t *nh_dot, gc_vec_t *roughness, gc_vec_t *distribution)
{
    VINIT4(roughness_x4,
           roughness->data[0] * roughness->data[0] * roughness->data[0] * roughness->data[0],
           roughness->data[1] * roughness->data[1] * roughness->data[1] * roughness->data[1],
           roughness->data[2] * roughness->data[2] * roughness->data[2] * roughness->data[2],
           roughness->data[3] * roughness->data[3] * roughness->data[3] * roughness->data[3]);

    VINIT4(denom,
           (nh_dot->data[0] * nh_dot->data[0]) * (roughness_x4.data[0] - 1.0f) + 1.0f,
           (nh_dot->data[1] * nh_dot->data[1]) * (roughness_x4.data[1] - 1.0f) + 1.0f,
           (nh_dot->data[2] * nh_dot->data[2]) * (roughness_x4.data[2] - 1.0f) + 1.0f,
           (nh_dot->data[3] * nh_dot->data[3]) * (roughness_x4.data[3] - 1.0f) + 1.0f);

    denom.data[0] = PI * denom.data[0] * denom.data[0];
    denom.data[1] = PI * denom.data[1] * denom.data[1];
    denom.data[2] = PI * denom.data[2] * denom.data[2];
    denom.data[3] = PI * denom.data[3] * denom.data[3];

    distribution->data[0] = roughness_x4.data[0] / denom.data[0];
    distribution->data[1] = roughness_x4.data[1] / denom.data[1];
    distribution->data[2] = roughness_x4.data[2] / denom.data[2];
    distribution->data[3] = roughness_x4.data[3] / denom.data[3];
}

__INLINE__ void geometry_smith(gc_vec_t *vn_dot, gc_vec_t *ln_dot, gc_vec_t *k, gc_vec_t *out)
{
    VINIT4(g1,
           vn_dot->data[0] / (vn_dot->data[0] * (1.0f - k->data[0]) + k->data[0]),
           vn_dot->data[1] / (vn_dot->data[1] * (1.0f - k->data[1]) + k->data[1]),
           vn_dot->data[2] / (vn_dot->data[2] * (1.0f - k->data[2]) + k->data[2]),
           vn_dot->data[3] / (vn_dot->data[3] * (1.0f - k->data[3]) + k->data[3]));

    VINIT4(g2,
           ln_dot->data[0] / (ln_dot->data[0] * (1.0f - k->data[0]) + k->data[0]),
           ln_dot->data[1] / (ln_dot->data[1] * (1.0f - k->data[1]) + k->data[1]),
           ln_dot->data[2] / (ln_dot->data[2] * (1.0f - k->data[2]) + k->data[2]),
           ln_dot->data[3] / (ln_dot->data[3] * (1.0f - k->data[3]) + k->data[3]));

    r32 correction = 0.0005f;

    out->data[0] = (g1.data[0] * g2.data[0]) + correction;
    out->data[1] = (g1.data[1] * g2.data[1]) + correction;
    out->data[2] = (g1.data[2] * g2.data[2]) + correction;
    out->data[3] = (g1.data[3] * g2.data[3]) + correction;
}

// ----------------------------------------------------------------------------------
// -- PBR SHADER.
// ----------------------------------------------------------------------------------
// -- Shader setup.
// ----------------------------------------------------------------------------------

void shader_pbr_setup(gc_material_t *material)
{
    GCSR.gl->pipeline.params.material = material;
    GCSR.gl->pipeline.params.shader_flags = 0;

    if (FLAG(material->components, SHADER_FLAG_PBR_UNLIT))
        GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_UNLIT;

    if (GCSR.gl->pipeline.flags & GC_MODE_RENDERED)
    {
        if (PIPE_FLAG(GC_SHADOW) && GCSR.gl->pipeline.params.lights) {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_SHADOW;
        }

        if (material->pbr.albedo_map && FLAG(material->components, SHADER_FLAG_PBR_ALBEDO))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_ALBEDO;
            GCSR.gl->pipeline.params.textures[0] = material->pbr.albedo_map;
        }

        if (material->pbr.normal_map && FLAG(material->components, SHADER_FLAG_PBR_NORMAL))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_NORMAL;
            GCSR.gl->pipeline.params.textures[1] = material->pbr.normal_map;
        }

        if (material->pbr.arm_map && FLAG(material->components, SHADER_FLAG_PBR_AO_ROUGHNESS_METALNESS))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_AO_ROUGHNESS_METALNESS;
            GCSR.gl->pipeline.params.textures[2] = material->pbr.arm_map;
        }
        else
        {
            if (material->pbr.metalness_map && FLAG(material->components, SHADER_FLAG_PBR_METALNESS))
            {
                GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_METALNESS;
                GCSR.gl->pipeline.params.textures[2] = material->pbr.metalness_map;
            }

            if (material->pbr.roughness_map && FLAG(material->components, SHADER_FLAG_PBR_ROUGHNESS))
            {
                GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_ROUGHNESS;
                GCSR.gl->pipeline.params.textures[3] = material->pbr.roughness_map;
            }

            if (material->pbr.ao_map && FLAG(material->components, SHADER_FLAG_PBR_AO))
            {
                GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_AO;
                GCSR.gl->pipeline.params.textures[4] = material->pbr.ao_map;
            }
        }

        if (material->pbr.ambient_map && FLAG(material->components, SHADER_FLAG_PBR_AMBIENT))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_AMBIENT;
            GCSR.gl->pipeline.params.textures[5] = material->pbr.ambient_map;
        }

        if (material->pbr.ambient_map && FLAG(material->components, SHADER_FLAG_PBR_EMISSION))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_EMISSION;
            GCSR.gl->pipeline.params.textures[6] = material->pbr.emission_map;
        }
    }
    else if (PIPE_FLAG(GC_MODE_SOLID))
    {
        if (PIPE_FLAG(GC_SHADOW) && GCSR.gl->pipeline.params.lights) {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_SHADOW;
        }

        // if (material->pbr.ao_map && FLAG(material->components, SHADER_FLAG_PBR_AO))
        // {
        //     GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_AO;
        //     GCSR.gl->pipeline.params.textures[4] = material->pbr.ao_map;
        // }
    }
    else if (GCSR.gl->pipeline.flags & GC_MODE_MATERIAL)
    {
        if (material->pbr.albedo_map && FLAG(material->components, SHADER_FLAG_PBR_ALBEDO))
        {
            GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_PBR_ALBEDO;
            GCSR.gl->pipeline.params.textures[0] = material->pbr.albedo_map;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

void shader_pbr_vs(gc_vertex_t *vertex, gc_shader_params_t *params)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);
    VINIT3(vertex_normal, vertex->data[2], vertex->data[3], vertex->data[4]);
    VINIT3(vertex_tangent, vertex->data[5], vertex->data[6], vertex->data[7]);

    VINIT4(world_position, 0, 0, 0, 0);
    VINIT4(shadow_space_position, 0, 0, 0, 0);
    VINIT4(world_normal, 0, 0, 0, 0);
    VINIT4(view_dir, 0, 0, 0, 0);

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
// -- https://learnopengl.com/PBR/Theory
// ----------------------------------------------------------------------------------

void _shader_pbr_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;
    gc_pbr_material_t *material = &params->material->pbr;
    pbr_multipliers_t multipliers = material->multipliers;

    lod_t lod;
    LOD_CLEAR(lod);

    b8 is_uv_scaling = PIPE_PARAM_VECTOR(2, uv_scaling, 0) || PIPE_PARAM_VECTOR(2, uv_scaling, 1);

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        shader_color_t computed_light_color;

        A4SET(computed_light_color.r, 0, 0, 0, 0);
        A4SET(computed_light_color.g, 0, 0, 0, 0);
        A4SET(computed_light_color.b, 0, 0, 0, 0);

        shader_color_t albedo;

        A4SET(albedo.r, material->albedo.c.r, material->albedo.c.r, material->albedo.c.r, material->albedo.c.r);
        A4SET(albedo.g, material->albedo.c.g, material->albedo.c.g, material->albedo.c.g, material->albedo.c.g);
        A4SET(albedo.b, material->albedo.c.b, material->albedo.c.b, material->albedo.c.b, material->albedo.c.b);
        A4SET(albedo.a, material->albedo.c.a, material->albedo.c.a, material->albedo.c.a, material->albedo.c.a);

        if (PIPE_FLAG(GC_MODE_SOLID))
        {
            r32 sr = PIPE_PARAM_VECTOR(1, solid_color, 0);
            r32 sg = PIPE_PARAM_VECTOR(1, solid_color, 1);
            r32 sb = PIPE_PARAM_VECTOR(1, solid_color, 2);
            r32 sa = PIPE_PARAM_VECTOR(1, solid_color, 3);

            A4SET(albedo.r, sr, sr, sr, sr);
            A4SET(albedo.g, sg, sg, sg, sg);
            A4SET(albedo.b, sb, sb, sb, sb);
            A4SET(albedo.a, sa, sa, sa, sa);
        }

        shader_color_t emission = {
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        };

        shader_color_t mf0;

        A4SET(mf0.r, material->f0.v3.x, material->f0.v3.x, material->f0.v3.x, material->f0.v3.x);
        A4SET(mf0.g, material->f0.v3.y, material->f0.v3.y, material->f0.v3.y, material->f0.v3.y);
        A4SET(mf0.b, material->f0.v3.z, material->f0.v3.z, material->f0.v3.z, material->f0.v3.z);

        VINIT4(metalness,
               material->metalness * multipliers.metalness,
               material->metalness * multipliers.metalness,
               material->metalness * multipliers.metalness,
               material->metalness * multipliers.metalness);

        VINIT4(roughness,
               material->roughness * multipliers.roughness,
               material->roughness * multipliers.roughness,
               material->roughness * multipliers.roughness,
               material->roughness * multipliers.roughness);

        VINIT4(ao,
               material->ao * multipliers.ao,
               material->ao * multipliers.ao,
               material->ao * multipliers.ao,
               material->ao * multipliers.ao);

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

        if (params->shader_flags & SHADER_FLAG_PBR_ALBEDO)
        {
            // gc_texture_compute_lod((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[0], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[0], texcoord->x, texcoord->y, &lod, &albedo);

            gc_gamma_srgb_to_linear_frag(&albedo);
        }

        albedo.r[0] *= multipliers.albedo.v3.x;
        albedo.r[1] *= multipliers.albedo.v3.x;
        albedo.r[2] *= multipliers.albedo.v3.x;
        albedo.r[3] *= multipliers.albedo.v3.x;

        albedo.g[0] *= multipliers.albedo.v3.y;
        albedo.g[1] *= multipliers.albedo.v3.y;
        albedo.g[2] *= multipliers.albedo.v3.y;
        albedo.g[3] *= multipliers.albedo.v3.y;

        albedo.b[0] *= multipliers.albedo.v3.z;
        albedo.b[1] *= multipliers.albedo.v3.z;
        albedo.b[2] *= multipliers.albedo.v3.z;
        albedo.b[3] *= multipliers.albedo.v3.z;

        if (params->shader_flags & SHADER_FLAG_PBR_EMISSION)
        {
            // gc_texture_compute_lod((texture2d_t *) params->textures[6], texcoord->x, texcoord->y, &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[6], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[6], texcoord->x, texcoord->y, &lod, &emission);

            gc_gamma_srgb_to_linear_frag(&emission);
        }

        if (params->shader_flags & SHADER_FLAG_PBR_AO_ROUGHNESS_METALNESS)
        {
            shader_color_t tmp_sample;

            // gc_texture_compute_lod((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[2], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod, &tmp_sample);
            // gc_gamma_srgb_to_linear_frag(&tmp_sample);

            ao.data[0] = tmp_sample.r[0] * multipliers.ao;
            ao.data[1] = tmp_sample.r[1] * multipliers.ao;
            ao.data[2] = tmp_sample.r[2] * multipliers.ao;
            ao.data[3] = tmp_sample.r[3] * multipliers.ao;

            roughness.data[0] = tmp_sample.g[0] * multipliers.roughness;
            roughness.data[1] = tmp_sample.g[1] * multipliers.roughness;
            roughness.data[2] = tmp_sample.g[2] * multipliers.roughness;
            roughness.data[3] = tmp_sample.g[3] * multipliers.roughness;

            metalness.data[0] = tmp_sample.b[0] * multipliers.metalness;
            metalness.data[1] = tmp_sample.b[1] * multipliers.metalness;
            metalness.data[2] = tmp_sample.b[2] * multipliers.metalness;
            metalness.data[3] = tmp_sample.b[3] * multipliers.metalness;
        }
        else
        {
            if (params->shader_flags & SHADER_FLAG_PBR_METALNESS)
            {
                shader_color_t metalness_sample;
                // gc_texture_compute_lod((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod);
                gc_texture_compute_lod((texture2d_t *) params->textures[2], fragment, &lod);
                texture_sample((texture2d_t *) params->textures[2], texcoord->x, texcoord->y, &lod, &metalness_sample);
                // gc_gamma_srgb_to_linear_frag(&metalness_sample);

                metalness.data[0] = metalness_sample.r[0] * multipliers.metalness;
                metalness.data[1] = metalness_sample.r[1] * multipliers.metalness;
                metalness.data[2] = metalness_sample.r[2] * multipliers.metalness;
                metalness.data[3] = metalness_sample.r[3] * multipliers.metalness;
            }

            if (params->shader_flags & SHADER_FLAG_PBR_ROUGHNESS)
            {
                shader_color_t roughness_sample;
                // gc_texture_compute_lod((texture2d_t *) params->textures[3], texcoord->x, texcoord->y, &lod);
                gc_texture_compute_lod((texture2d_t *) params->textures[3], fragment, &lod);
                texture_sample((texture2d_t *) params->textures[3], texcoord->x, texcoord->y, &lod, &roughness_sample);
                // gc_gamma_srgb_to_linear_frag(&roughness_sample);

                roughness.data[0] = roughness_sample.r[0] * multipliers.roughness;
                roughness.data[1] = roughness_sample.r[1] * multipliers.roughness;
                roughness.data[2] = roughness_sample.r[2] * multipliers.roughness;
                roughness.data[3] = roughness_sample.r[3] * multipliers.roughness;
            }

            if (params->shader_flags & SHADER_FLAG_PBR_AO)
            {
                shader_color_t ao_sample;
                // gc_texture_compute_lod((texture2d_t *) params->textures[4], texcoord->x, texcoord->y, &lod);
                gc_texture_compute_lod((texture2d_t *) params->textures[4], fragment, &lod);
                texture_sample((texture2d_t *) params->textures[4], texcoord->x, texcoord->y, &lod, &ao_sample);
                // gc_gamma_srgb_to_linear_frag(&ao_sample);

                ao.data[0] = ao_sample.r[0] * multipliers.ao;
                ao.data[1] = ao_sample.r[1] * multipliers.ao;
                ao.data[2] = ao_sample.r[2] * multipliers.ao;
                ao.data[3] = ao_sample.r[3] * multipliers.ao;
            }
        }

        // gc_vec_t roughness_x4 = {
        //     roughness.data[0] * roughness.data[0] * roughness.data[0] * roughness.data[0],
        //     roughness.data[1] * roughness.data[1] * roughness.data[1] * roughness.data[1],
        //     roughness.data[2] * roughness.data[2] * roughness.data[2] * roughness.data[2],
        //     roughness.data[3] * roughness.data[3] * roughness.data[3] * roughness.data[3]
        // };

        shader_color_t albedo_over_pi;

        A4SET(albedo_over_pi.r, albedo.r[0] * ONE_OVER_PI, albedo.r[1] * ONE_OVER_PI, albedo.r[2] * ONE_OVER_PI, albedo.r[3] * ONE_OVER_PI);
        A4SET(albedo_over_pi.g, albedo.g[0] * ONE_OVER_PI, albedo.g[1] * ONE_OVER_PI, albedo.g[2] * ONE_OVER_PI, albedo.g[3] * ONE_OVER_PI);
        A4SET(albedo_over_pi.b, albedo.b[0] * ONE_OVER_PI, albedo.b[1] * ONE_OVER_PI, albedo.b[2] * ONE_OVER_PI, albedo.b[3] * ONE_OVER_PI);

        // shader_color_t albedo_over_pi = {
        //     { albedo.r[0], albedo.r[1], albedo.r[2], albedo.r[3] },
        //     { albedo.g[0], albedo.g[1], albedo.g[2], albedo.g[3] },
        //     { albedo.b[0], albedo.b[1], albedo.b[2], albedo.b[3] }
        // };

        // ----------------------------------------------------------------------------------
        // -- Normal.
        // ----------------------------------------------------------------------------------

        if (params->shader_flags & SHADER_FLAG_PBR_NORMAL)
        {
            shader_color_t tsp_normal;
            // gc_texture_compute_lod((texture2d_t *) params->textures[1], texcoord->x, texcoord->y, &lod);
            gc_texture_compute_lod((texture2d_t *) params->textures[1], fragment, &lod);
            texture_sample((texture2d_t *) params->textures[1], texcoord->x, texcoord->y, &lod, &tsp_normal);

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
        }
        else
        {
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
        }

        fv3_normalize(&final_normal);

        if (fragment->primitive->is_backface)
            fv3_inv(&final_normal);

        gc_vec_t vn_dot;
        fv3_dot(view_vector, &final_normal, &vn_dot);
        v4_max(&vn_dot, 0);

        fv3_t f0;
        fv3_mix((fv3_t *) &mf0, (fv3_t *) &albedo, &metalness, &f0);

        VINIT4(one_minus_metalness,
               1.0f - metalness.data[0],
               1.0f - metalness.data[1],
               1.0f - metalness.data[2],
               1.0f - metalness.data[3]);

        VINIT4(k,
               ((roughness.data[0] + 1) * (roughness.data[0] + 1)) / 8.0f,
               ((roughness.data[1] + 1) * (roughness.data[1] + 1)) / 8.0f,
               ((roughness.data[2] + 1) * (roughness.data[2] + 1)) / 8.0f,
               ((roughness.data[3] + 1) * (roughness.data[3] + 1)) / 8.0f);

        // ----------------------------------------------------------------------------------
        // -- Lights.
        // ----------------------------------------------------------------------------------

        shader_color_t ambient;

        A4SET(ambient.r, 0, 0, 0, 0);
        A4SET(ambient.g, 0, 0, 0, 0);
        A4SET(ambient.b, 0, 0, 0, 0);

        if (!(params->shader_flags & SHADER_FLAG_PBR_UNLIT) && !(GCSR.gl->pipeline.flags & GC_MODE_MATERIAL))
        {
            for (u32 l = 0; l < params->light_count; ++l)
            {
                gc_light_t *light = params->lights + l;

                r32 attenuation[GC_FRAG_SIZE] = {1, 1, 1, 1};

                fv3_t light_vector;
                fv2_t light_texcoord;

                gc_vec_t ln_dot;
                gc_vec_t hv_dot;
                gc_vec_t distance;

                fv3_t half_vector;

                if (light->type == GC_SUN_LIGHT)
                {
                    light_vector.x[0] = light->directional.direction.v3.x;
                    light_vector.x[1] = light->directional.direction.v3.x;
                    light_vector.x[2] = light->directional.direction.v3.x;
                    light_vector.x[3] = light->directional.direction.v3.x;

                    light_vector.y[0] = light->directional.direction.v3.y;
                    light_vector.y[1] = light->directional.direction.v3.y;
                    light_vector.y[2] = light->directional.direction.v3.y;
                    light_vector.y[3] = light->directional.direction.v3.y;

                    light_vector.z[0] = light->directional.direction.v3.z;
                    light_vector.z[1] = light->directional.direction.v3.z;
                    light_vector.z[2] = light->directional.direction.v3.z;
                    light_vector.z[3] = light->directional.direction.v3.z;

                    fv3_dot(&light_vector, &final_normal, &ln_dot);
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
                    fv3_vec_sub(&light->object.position, world_position, &light_vector);
                    fv3_lenv(&light_vector, &distance);
                    fv3_normalize(&light_vector);

                    fv3_dot(&light_vector, &final_normal, &ln_dot);
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

                        // gc_vec_t slope_scale_bias = {
                        //     sqrtf(1.0f - ln_dot.data[0] * ln_dot.data[0]) / ln_dot.data[0],
                        //     sqrtf(1.0f - ln_dot.data[1] * ln_dot.data[1]) / ln_dot.data[1],
                        //     sqrtf(1.0f - ln_dot.data[2] * ln_dot.data[2]) / ln_dot.data[2],
                        //     sqrtf(1.0f - ln_dot.data[3] * ln_dot.data[3]) / ln_dot.data[3]
                        // };

                        compare.data[0] = compare.data[0] * light->shadow.f_len_inv - light->shadow.depth_bias;
                        compare.data[1] = compare.data[1] * light->shadow.f_len_inv - light->shadow.depth_bias;
                        compare.data[2] = compare.data[2] * light->shadow.f_len_inv - light->shadow.depth_bias;
                        compare.data[3] = compare.data[3] * light->shadow.f_len_inv - light->shadow.depth_bias;

                        light->shadow.point_shadow_visibility_r((cube_texture_t *) light->shadow_texture, &direction, compare.data, light->shadow.radius, fragment->shadow);
                    }
                }
                else
                    continue;

                fv3_add(&light_vector, view_vector, &half_vector);
                fv3_normalize(&half_vector);

                fv3_dot(&half_vector, view_vector, &hv_dot);
                v4_max(&hv_dot, 0);

                fv3_t radiance;

                A4SET(radiance.x, light->color.c.r, light->color.c.r, light->color.c.r, light->color.c.r);
                A4SET(radiance.y, light->color.c.g, light->color.c.g, light->color.c.g, light->color.c.g);
                A4SET(radiance.z, light->color.c.b, light->color.c.b, light->color.c.b, light->color.c.b);

                fv3_muls(&radiance, &ln_dot, &radiance);

                // ----------------------------------------------------------------------------------
                // -- Cook-Torrance specular BRDF term.
                // ----------------------------------------------------------------------------------
                // -- Fresnel-Schlick.
                // ----------------------------------------------------------------------------------

                fv3_t fresnel;
                fresnel_schlick(&hv_dot, &f0, &fresnel);

                // ----------------------------------------------------------------------------------
                // -- Distribution function
                // ----------------------------------------------------------------------------------

                gc_vec_t nh_dot;
                gc_vec_t distribution;

                fv3_dot(&final_normal, &half_vector, &nh_dot);
                v4_max(&nh_dot, 0);
                distribution_ggx(&nh_dot, &roughness, &distribution);

                // ----------------------------------------------------------------------------------
                // -- Geometry function.
                // ----------------------------------------------------------------------------------

                gc_vec_t geometry;
                geometry_smith(&vn_dot, &ln_dot, &k, &geometry);

                // ----------------------------------------------------------------------------------

                VINIT4(specular_temp,
                       multipliers.specular * (distribution.data[0] * geometry.data[0]) / (4 * vn_dot.data[0] * ln_dot.data[0] + 0.001f),
                       multipliers.specular * (distribution.data[1] * geometry.data[1]) / (4 * vn_dot.data[1] * ln_dot.data[1] + 0.001f),
                       multipliers.specular * (distribution.data[2] * geometry.data[2]) / (4 * vn_dot.data[2] * ln_dot.data[2] + 0.001f),
                       multipliers.specular * (distribution.data[3] * geometry.data[3]) / (4 * vn_dot.data[3] * ln_dot.data[3] + 0.001f));

                fv3_t light_specular;

                A4SET(light_specular.x,
                      specular_temp.data[0] * fresnel.x[0],
                      specular_temp.data[1] * fresnel.x[1],
                      specular_temp.data[2] * fresnel.x[2],
                      specular_temp.data[3] * fresnel.x[3]);

                A4SET(light_specular.y,
                      specular_temp.data[0] * fresnel.y[0],
                      specular_temp.data[1] * fresnel.y[1],
                      specular_temp.data[2] * fresnel.y[2],
                      specular_temp.data[3] * fresnel.y[3]);

                A4SET(light_specular.z,
                      specular_temp.data[0] * fresnel.z[0],
                      specular_temp.data[1] * fresnel.z[1],
                      specular_temp.data[2] * fresnel.z[2],
                      specular_temp.data[3] * fresnel.z[3]);

                fv3_t kd;

                A4SET(kd.x,
                      (1.0f - fresnel.x[0]) * one_minus_metalness.data[0],
                      (1.0f - fresnel.x[1]) * one_minus_metalness.data[1],
                      (1.0f - fresnel.x[2]) * one_minus_metalness.data[2],
                      (1.0f - fresnel.x[3]) * one_minus_metalness.data[3]);

                A4SET(kd.y,
                      (1.0f - fresnel.y[0]) * one_minus_metalness.data[0],
                      (1.0f - fresnel.y[1]) * one_minus_metalness.data[1],
                      (1.0f - fresnel.y[2]) * one_minus_metalness.data[2],
                      (1.0f - fresnel.y[3]) * one_minus_metalness.data[3]);

                A4SET(kd.z,
                      (1.0f - fresnel.z[0]) * one_minus_metalness.data[0],
                      (1.0f - fresnel.z[1]) * one_minus_metalness.data[1],
                      (1.0f - fresnel.z[2]) * one_minus_metalness.data[2],
                      (1.0f - fresnel.z[3]) * one_minus_metalness.data[3]);

                computed_light_color.r[0] += (kd.x[0] * albedo_over_pi.r[0] + light_specular.x[0]) * radiance.x[0] * ln_dot.data[0] * fragment->shadow[0] * attenuation[0];
                computed_light_color.r[1] += (kd.x[1] * albedo_over_pi.r[1] + light_specular.x[1]) * radiance.x[1] * ln_dot.data[1] * fragment->shadow[1] * attenuation[1];
                computed_light_color.r[2] += (kd.x[2] * albedo_over_pi.r[2] + light_specular.x[2]) * radiance.x[2] * ln_dot.data[2] * fragment->shadow[2] * attenuation[2];
                computed_light_color.r[3] += (kd.x[3] * albedo_over_pi.r[3] + light_specular.x[3]) * radiance.x[3] * ln_dot.data[3] * fragment->shadow[3] * attenuation[3];

                computed_light_color.g[0] += (kd.y[0] * albedo_over_pi.g[0] + light_specular.y[0]) * radiance.y[0] * ln_dot.data[0] * fragment->shadow[0] * attenuation[0];
                computed_light_color.g[1] += (kd.y[1] * albedo_over_pi.g[1] + light_specular.y[1]) * radiance.y[1] * ln_dot.data[1] * fragment->shadow[1] * attenuation[1];
                computed_light_color.g[2] += (kd.y[2] * albedo_over_pi.g[2] + light_specular.y[2]) * radiance.y[2] * ln_dot.data[2] * fragment->shadow[2] * attenuation[2];
                computed_light_color.g[3] += (kd.y[3] * albedo_over_pi.g[3] + light_specular.y[3]) * radiance.y[3] * ln_dot.data[3] * fragment->shadow[3] * attenuation[3];

                computed_light_color.b[0] += (kd.z[0] * albedo_over_pi.b[0] + light_specular.z[0]) * radiance.z[0] * ln_dot.data[0] * fragment->shadow[0] * attenuation[0];
                computed_light_color.b[1] += (kd.z[1] * albedo_over_pi.b[1] + light_specular.z[1]) * radiance.z[1] * ln_dot.data[1] * fragment->shadow[1] * attenuation[1];
                computed_light_color.b[2] += (kd.z[2] * albedo_over_pi.b[2] + light_specular.z[2]) * radiance.z[2] * ln_dot.data[2] * fragment->shadow[2] * attenuation[2];
                computed_light_color.b[3] += (kd.z[3] * albedo_over_pi.b[3] + light_specular.z[3]) * radiance.z[3] * ln_dot.data[3] * fragment->shadow[3] * attenuation[3];
            }

            // ----------------------------------------------------------------------------------
            // -- Ambient component.
            // ----------------------------------------------------------------------------------

            if (fragment->x == 708 && fragment->y == 454)
            {
                u8 a = 0;
                a++;
            }

            if (params->shader_flags & SHADER_FLAG_PBR_AMBIENT)
            {
                shader_color_t ambient_specular = {
                    {1, 1, 1, 1},
                    {1, 1, 1, 1},
                    {1, 1, 1, 1},
                };
                // brdf_lut_pixel_t brdf_lut[GC_FRAG_SIZE] = {
                //     {1, 1},
                //     {1, 1},
                //     {1, 1},
                //     {1, 1},
                // };

                fv2_t brdf_lut = {
                    {1, 1, 1, 1},
                    {1, 1, 1, 1}
                };

                fv3_t ks;

                gc_vec_t vn_dot2;
                fv3_t inv_view_vector;
                fv3_t reflection_vector;
                r32 max_reflection_lod = PBR_PREFILTERED_MIP_LEVELS - 1;

                r32 roughness_lod[GC_FRAG_SIZE];
                A4SET(roughness_lod,
                      roughness.data[0] * max_reflection_lod,
                      roughness.data[1] * max_reflection_lod,
                      roughness.data[2] * max_reflection_lod,
                      roughness.data[3] * max_reflection_lod);

                fv3_inv_to(view_vector, &inv_view_vector);
                fv3_dot(&inv_view_vector, &final_normal, &vn_dot2);
                fv3_muls(&final_normal, &vn_dot2, &reflection_vector);
                fv3_muls1(&reflection_vector, 2, &reflection_vector);
                fv3_sub(&inv_view_vector, &reflection_vector, &reflection_vector);

                fresnel_schlick_roughness(&vn_dot, &f0, &roughness, &ks);

                fv3_t kd;

                A4SET(kd.x,
                      (1.0f - ks.x[0]) * one_minus_metalness.data[0],
                      (1.0f - ks.x[1]) * one_minus_metalness.data[1],
                      (1.0f - ks.x[2]) * one_minus_metalness.data[2],
                      (1.0f - ks.x[3]) * one_minus_metalness.data[3]);

                A4SET(kd.y,
                      (1.0f - ks.y[0]) * one_minus_metalness.data[0],
                      (1.0f - ks.y[1]) * one_minus_metalness.data[1],
                      (1.0f - ks.y[2]) * one_minus_metalness.data[2],
                      (1.0f - ks.y[3]) * one_minus_metalness.data[3]);

                A4SET(kd.z,
                      (1.0f - ks.z[0]) * one_minus_metalness.data[0],
                      (1.0f - ks.z[1]) * one_minus_metalness.data[1],
                      (1.0f - ks.z[2]) * one_minus_metalness.data[2],
                      (1.0f - ks.z[3]) * one_minus_metalness.data[3]);

                lod_t tlod;
                LOD_CLEAR(tlod);

                pbr_ambient_texture_t *ambient_texture = (pbr_ambient_texture_t *) params->textures[5];
                cube_texture_sample((cube_texture_t *) ambient_texture->irradiance, &final_normal, &tlod, true, &ambient);

                lod_t prefiltered_tlod;
                LOD_CLEAR(prefiltered_tlod);

                for (u8 p = 0; p < GC_FRAG_SIZE; ++p)
                {
                    if (roughness_lod[p] < 0)
                        roughness_lod[p] = 0;
                    else if (roughness_lod[p] > ambient_texture->prefiltered->mip_count - 1)
                        roughness_lod[p] = ambient_texture->prefiltered->mip_count - 1;

                    prefiltered_tlod.low[p] = FAST_FLOOR16(roughness_lod[p]);
                    prefiltered_tlod.high[p] = FAST_CEIL16(roughness_lod[p]);
                    prefiltered_tlod.interp[p] = roughness_lod[p] - prefiltered_tlod.low[p];
                }

                cube_texture_sample((cube_texture_t *) ambient_texture->prefiltered, &reflection_vector, &prefiltered_tlod, false, &ambient_specular);
                brdf_lut_sample(ambient_texture->brdf_lut, &vn_dot, &roughness, &brdf_lut);

    #if 1
                ambient_specular.r[0] *= (ks.x[0] * brdf_lut.x[0] + brdf_lut.y[0]);
                ambient_specular.r[1] *= (ks.x[1] * brdf_lut.x[1] + brdf_lut.y[1]);
                ambient_specular.r[2] *= (ks.x[2] * brdf_lut.x[2] + brdf_lut.y[2]);
                ambient_specular.r[3] *= (ks.x[3] * brdf_lut.x[3] + brdf_lut.y[3]);

                ambient_specular.g[0] *= (ks.y[0] * brdf_lut.x[0] + brdf_lut.y[0]);
                ambient_specular.g[1] *= (ks.y[1] * brdf_lut.x[1] + brdf_lut.y[1]);
                ambient_specular.g[2] *= (ks.y[2] * brdf_lut.x[2] + brdf_lut.y[2]);
                ambient_specular.g[3] *= (ks.y[3] * brdf_lut.x[3] + brdf_lut.y[3]);

                ambient_specular.b[0] *= (ks.z[0] * brdf_lut.x[0] + brdf_lut.y[0]);
                ambient_specular.b[1] *= (ks.z[1] * brdf_lut.x[1] + brdf_lut.y[1]);
                ambient_specular.b[2] *= (ks.z[2] * brdf_lut.x[2] + brdf_lut.y[2]);
                ambient_specular.b[3] *= (ks.z[3] * brdf_lut.x[3] + brdf_lut.y[3]);
    #else
                ambient_specular.r[0] *= (material->f0.c.r * brdf_lut.x[0] + brdf_lut.y[0]);
                ambient_specular.r[1] *= (material->f0.c.r * brdf_lut.x[1] + brdf_lut.y[1]);
                ambient_specular.r[2] *= (material->f0.c.r * brdf_lut.x[2] + brdf_lut.y[2]);
                ambient_specular.r[3] *= (material->f0.c.r * brdf_lut.x[3] + brdf_lut.y[3]);

                ambient_specular.g[0] *= (material->f0.c.g * brdf_lut.x[0] + brdf_lut.y[0]);
                ambient_specular.g[1] *= (material->f0.c.g * brdf_lut.x[1] + brdf_lut.y[1]);
                ambient_specular.g[2] *= (material->f0.c.g * brdf_lut.x[2] + brdf_lut.y[2]);
                ambient_specular.g[3] *= (material->f0.c.g * brdf_lut.x[3] + brdf_lut.y[3]);

                ambient_specular.b[0] *= (material->f0.c.b * brdf_lut.x[0] + brdf_lut.y[0]);
                ambient_specular.b[1] *= (material->f0.c.b * brdf_lut.x[1] + brdf_lut.y[1]);
                ambient_specular.b[2] *= (material->f0.c.b * brdf_lut.x[2] + brdf_lut.y[2]);
                ambient_specular.b[3] *= (material->f0.c.b * brdf_lut.x[3] + brdf_lut.y[3]);
    #endif

                ambient.r[0] = (kd.x[0] * ambient.r[0] * albedo.r[0] + ambient_specular.r[0]) * ao.data[0];
                ambient.r[1] = (kd.x[1] * ambient.r[1] * albedo.r[1] + ambient_specular.r[1]) * ao.data[1];
                ambient.r[2] = (kd.x[2] * ambient.r[2] * albedo.r[2] + ambient_specular.r[2]) * ao.data[2];
                ambient.r[3] = (kd.x[3] * ambient.r[3] * albedo.r[3] + ambient_specular.r[3]) * ao.data[3];

                ambient.g[0] = (kd.y[0] * ambient.g[0] * albedo.g[0] + ambient_specular.g[0]) * ao.data[0];
                ambient.g[1] = (kd.y[1] * ambient.g[1] * albedo.g[1] + ambient_specular.g[1]) * ao.data[1];
                ambient.g[2] = (kd.y[2] * ambient.g[2] * albedo.g[2] + ambient_specular.g[2]) * ao.data[2];
                ambient.g[3] = (kd.y[3] * ambient.g[3] * albedo.g[3] + ambient_specular.g[3]) * ao.data[3];

                ambient.b[0] = (kd.z[0] * ambient.b[0] * albedo.b[0] + ambient_specular.b[0]) * ao.data[0];
                ambient.b[1] = (kd.z[1] * ambient.b[1] * albedo.b[1] + ambient_specular.b[1]) * ao.data[1];
                ambient.b[2] = (kd.z[2] * ambient.b[2] * albedo.b[2] + ambient_specular.b[2]) * ao.data[2];
                ambient.b[3] = (kd.z[3] * ambient.b[3] * albedo.b[3] + ambient_specular.b[3]) * ao.data[3];
            }
            else
            {
                ambient.r[0] = PIPE_PARAM_VECTOR(1, ambient_color, 0) * albedo.r[0] * ao.data[0];
                ambient.r[1] = PIPE_PARAM_VECTOR(1, ambient_color, 0) * albedo.r[1] * ao.data[1];
                ambient.r[2] = PIPE_PARAM_VECTOR(1, ambient_color, 0) * albedo.r[2] * ao.data[2];
                ambient.r[3] = PIPE_PARAM_VECTOR(1, ambient_color, 0) * albedo.r[3] * ao.data[3];

                ambient.g[0] = PIPE_PARAM_VECTOR(1, ambient_color, 1) * albedo.g[0] * ao.data[0];
                ambient.g[1] = PIPE_PARAM_VECTOR(1, ambient_color, 1) * albedo.g[1] * ao.data[1];
                ambient.g[2] = PIPE_PARAM_VECTOR(1, ambient_color, 1) * albedo.g[2] * ao.data[2];
                ambient.g[3] = PIPE_PARAM_VECTOR(1, ambient_color, 1) * albedo.g[3] * ao.data[3];

                ambient.b[0] = PIPE_PARAM_VECTOR(1, ambient_color, 2) * albedo.b[0] * ao.data[0];
                ambient.b[1] = PIPE_PARAM_VECTOR(1, ambient_color, 2) * albedo.b[1] * ao.data[1];
                ambient.b[2] = PIPE_PARAM_VECTOR(1, ambient_color, 2) * albedo.b[2] * ao.data[2];
                ambient.b[3] = PIPE_PARAM_VECTOR(1, ambient_color, 2) * albedo.b[3] * ao.data[3];
            }
        }
        else
        {
            computed_light_color.r[0] = albedo.r[0];
            computed_light_color.g[0] = albedo.g[0];
            computed_light_color.b[0] = albedo.b[0];

            computed_light_color.r[1] = albedo.r[1];
            computed_light_color.g[1] = albedo.g[1];
            computed_light_color.b[1] = albedo.b[1];

            computed_light_color.r[2] = albedo.r[2];
            computed_light_color.g[2] = albedo.g[2];
            computed_light_color.b[2] = albedo.b[2];

            computed_light_color.r[3] = albedo.r[3];
            computed_light_color.g[3] = albedo.g[3];
            computed_light_color.b[3] = albedo.b[3];
        }

        if (fragment->x == 618 && fragment->y == 288)
        {
            u8 a = 0;
            a++;
        }

        fragment->r[0] = computed_light_color.r[0] + ambient.r[0] + emission.r[0];
        fragment->r[1] = computed_light_color.r[1] + ambient.r[1] + emission.r[1];
        fragment->r[2] = computed_light_color.r[2] + ambient.r[2] + emission.r[2];
        fragment->r[3] = computed_light_color.r[3] + ambient.r[3] + emission.r[3];

        fragment->g[0] = computed_light_color.g[0] + ambient.g[0] + emission.g[0];
        fragment->g[1] = computed_light_color.g[1] + ambient.g[1] + emission.g[1];
        fragment->g[2] = computed_light_color.g[2] + ambient.g[2] + emission.g[2];
        fragment->g[3] = computed_light_color.g[3] + ambient.g[3] + emission.g[3];

        fragment->b[0] = computed_light_color.b[0] + ambient.b[0] + emission.b[0];
        fragment->b[1] = computed_light_color.b[1] + ambient.b[1] + emission.b[1];
        fragment->b[2] = computed_light_color.b[2] + ambient.b[2] + emission.b[2];
        fragment->b[3] = computed_light_color.b[3] + ambient.b[3] + emission.b[3];

        fragment->a[0] = albedo.a[0];
        fragment->a[1] = albedo.a[1];
        fragment->a[2] = albedo.a[2];
        fragment->a[3] = albedo.a[3];
    }
}
