// ----------------------------------------------------------------------------------
// -- File: shader_reflection_refraction.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-06-26 12:41:54
// -- Modified: 2022-10-17 12:39:15
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "../simd/shaders/avx_shader_reflection_refraction.cpp"
#define shader_reflection_refraction_fs _sse_shader_reflection_refraction_fs
#elif defined(GC_PIPE_SSE)
#include "../simd/shaders/sse_shader_reflection_refraction.cpp"
#define shader_reflection_refraction_fs _sse_shader_reflection_refraction_fs
#else
#define shader_reflection_refraction_fs _shader_reflection_refraction_fs
#endif

// ----------------------------------------------------------------------------------
// -- REFLECTION-REFRACTION SHADER.
// ----------------------------------------------------------------------------------
// -- Shader setup.
// ----------------------------------------------------------------------------------

void shader_reflection_refraction_setup(gc_material_t *material)
{
    GCSR.gl->pipeline.params.material = material;
    GCSR.gl->pipeline.params.shader_flags = 0;

    if (FLAG(material->components, SHADER_FLAG_BLINN_DIFFUSE))
        GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_DIFFUSE;

    if (FLAG(material->components, SHADER_FLAG_BLINN_REFLECTION))
        GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_REFLECTION;

    if (FLAG(material->components, SHADER_FLAG_BLINN_REFRACTION))
        GCSR.gl->pipeline.params.shader_flags |= SHADER_FLAG_BLINN_REFRACTION;

    if (material->blinn.cubemap )
        GCSR.gl->pipeline.params.textures[0] = material->blinn.cubemap;
}

// ----------------------------------------------------------------------------------
// -- Vertex shader.
// ----------------------------------------------------------------------------------

void shader_reflection_refraction_vs(gc_vertex_t *vertex, gc_shader_params_t *uniforms)
{
    VINIT4(gl_position, vertex->pos[0], vertex->pos[1], vertex->pos[2], 1);
    VINIT3(vertex_normal, vertex->data[2], vertex->data[3], vertex->data[4]);

    gc_vec_t world_position;
    gc_vec_t world_normal;
    gc_vec_t view_dir;


    gl_mat4_mulvec(GET_MATRIX(M_MODEL_WORLD), (gc_vec_t *) &gl_position, &world_position);
    gl_mat4_mulvec(GET_MATRIX(M_NORMAL_WORLD), &vertex_normal, &world_normal);
    v3_normalize(&world_normal);

    gl_vec3_sub(&world_position, &uniforms->world_camera_position, &view_dir);

    // ----------------------------------------------------------------------------------
    // -- Attributes sent for interpolation.
    // ----------------------------------------------------------------------------------

    // world normal.
    vertex->data[0] = world_normal.v3.x;
    vertex->data[1] = world_normal.v3.y;
    vertex->data[2] = world_normal.v3.z;

    // world position.
    vertex->data[3] = world_position.v3.x;
    vertex->data[4] = world_position.v3.y;
    vertex->data[5] = world_position.v3.z;

    // view vector.
    vertex->data[6] = view_dir.v3.x;
    vertex->data[7] = view_dir.v3.y;
    vertex->data[8] = view_dir.v3.z;

    // ----------------------------------------------------------------------------------

    gc_mat_t *model_view = GET_MATRIX(M_MODEL_VIEW);
    gc_mat_t *projection = GET_MATRIX(M_PROJECTION);

    gl_viewspace(model_view, vertex, gl_position);
    gl_project(projection, gl_position);
    gc_copy_position(vertex, gl_position);
}

void _shader_reflection_refraction_fs(gc_fragments_array_t *fragments_array, gc_shader_params_t *params)
{
    gc_processed_fragment_t *fragment = 0;
    gc_blinn_material_t *material = &params->material->blinn;
    gc_light_t *light = params->lights;

    lod_t lod;
    LOD_CLEAR(lod);

    shader_color_t diffuse_color;

    A4SET(diffuse_color.r, material->diffuse.c.r, material->diffuse.c.r, material->diffuse.c.r, material->diffuse.c.r);
    A4SET(diffuse_color.g, material->diffuse.c.g, material->diffuse.c.g, material->diffuse.c.g, material->diffuse.c.g);
    A4SET(diffuse_color.b, material->diffuse.c.b, material->diffuse.c.b, material->diffuse.c.b, material->diffuse.c.b);
    A4SET(diffuse_color.a, material->diffuse.c.a, material->diffuse.c.a, material->diffuse.c.a, material->diffuse.c.a);

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;

        if (fragment->discarded)
            continue;

        shader_color_t *computed_color = (shader_color_t *) fragment;

        fv3_t *world_normal = (fv3_t *) fragment->varyings[0];
        fv3_t *view_vector = (fv3_t *) fragment->varyings[6];

        fv3_normalize(world_normal);
        fv3_normalize(view_vector);

        // ----------------------------------------------------------------------------------
        // -- Ambient reflection/refraction.
        // ----------------------------------------------------------------------------------

        shader_color_t ambient_reflection_color;
        shader_color_t ambient_refraction_color;

        if ((params->shader_flags & (SHADER_FLAG_BLINN_REFLECTION | SHADER_FLAG_BLINN_REFRACTION)) && params->textures[0])
        {
            shader_color_t *refl_color = &ambient_reflection_color;
            shader_color_t *refr_color = &ambient_reflection_color;

            gc_vec_t vn_dot;
            // fv3_t view_vector;

            // fv3_inv_to(view_vector, &view_vector);
            fv3_dot(view_vector, world_normal, &vn_dot);

            if (params->shader_flags & SHADER_FLAG_BLINN_REFLECTION)
            {
                fv3_t reflection;
                fv3_muls(world_normal, &vn_dot, &reflection);
                fv3_muls1(&reflection, 2, &reflection);
                fv3_sub(view_vector, &reflection, &reflection);

                cube_texture_sample((cube_texture_t *) params->textures[0], &reflection, &lod, false, &ambient_reflection_color);
                gc_gamma_srgb_to_linear_frag(&ambient_reflection_color);
            }

            if (params->shader_flags & SHADER_FLAG_BLINN_REFRACTION)
            {
                fv3_t refraction;
                r32 refr2 = material->refr_ratio * material->refr_ratio;

                VINIT4(ik,
                       1.0f - refr2 * (1.0f - vn_dot.data[0] * vn_dot.data[0]),
                       1.0f - refr2 * (1.0f - vn_dot.data[1] * vn_dot.data[1]),
                       1.0f - refr2 * (1.0f - vn_dot.data[2] * vn_dot.data[2]),
                       1.0f - refr2 * (1.0f - vn_dot.data[3] * vn_dot.data[3]));

                VINIT4(tmp,
                       material->refr_ratio * vn_dot.data[0] + sqrtf(ik.data[0]),
                       material->refr_ratio * vn_dot.data[1] + sqrtf(ik.data[1]),
                       material->refr_ratio * vn_dot.data[2] + sqrtf(ik.data[2]),
                       material->refr_ratio * vn_dot.data[3] + sqrtf(ik.data[3]));

                fv3_t v1;
                fv3_t v2;

                fv3_muls1(view_vector, material->refr_ratio, &v1);
                fv3_muls(world_normal, &tmp, &v2);
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

                cube_texture_sample((cube_texture_t *) params->textures[0], &refraction, &lod, false, &ambient_refraction_color);
                gc_gamma_srgb_to_linear_frag(&ambient_refraction_color);

                refr_color = &ambient_refraction_color;

                if (!(params->shader_flags & SHADER_FLAG_BLINN_REFLECTION))
                    refl_color = &ambient_refraction_color;
            }

            shader_color_mix(refl_color, refr_color, material->rr_ratio, computed_color);
        }

        if (params->shader_flags & SHADER_FLAG_BLINN_DIFFUSE)
            shader_color_mix(&diffuse_color, computed_color, material->rr_diffuse_ratio, computed_color);

        fv3_muls1((fv3_t *) computed_color, light->il, (fv3_t *) computed_color);

        fragment->a[0] = 1.0f;
        fragment->a[1] = 1.0f;
        fragment->a[2] = 1.0f;
        fragment->a[3] = 1.0f;
    }
}