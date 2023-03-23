// ----------------------------------------------------------------------------------
// -- File: gcsr_level.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-08-26 09:43:55
// -- Modified: 2022-12-28 11:06:36
// ----------------------------------------------------------------------------------

void overwrites_clear(pipe_param_input_table_t *table)
{
    table->tone_mapping.overwrite = false;
    table->postprocessing.overwrite = false;
    table->saturation.overwrite = false;
    table->tint_color.overwrite = false;
    table->solid_color.overwrite = false;
    table->ambient_color.overwrite = false;
    table->background_color.overwrite = false;
    table->wireframe_color.overwrite = false;
    table->point_color.overwrite = false;
    table->uv_scaling.overwrite = false;
    table->forced_opacity.overwrite = false;
    table->point_radius.overwrite = false;
}

void model_init(gc_model_t *model,
                shader_id_t shader,
                r32 x, r32 y, r32 z,
                r32 roll, r32 pitch, r32 heading,
                r32 sx, r32 sy, r32 sz,
                mesh_t *triangles,
                mesh_t *lines,
                mesh_t *points)
{
    model->shader_id = shader;
    model->object.transforms.count = 0;
    model->material = 0;

    model->meshes[0] = triangles;
    model->meshes[1] = lines;
    model->meshes[2] = points;

    model->object.position.v4.x = x;
    model->object.position.v4.y = y;
    model->object.position.v4.z = z;
    model->object.position.v4.w = 1;

    model->object.rtype = EULER_XYZ;
    model->object.rotation.v3.x = roll;
    model->object.rotation.v3.y = pitch;
    model->object.rotation.v3.z = heading;

    model->object.scaling.v3.x = sx;
    model->object.scaling.v3.y = sy;
    model->object.scaling.v3.z = sz;

    overwrites_clear(&model->overwrites);

    model->flags = MOD_BACKFACE_CULL;
}

// Applies a transformation to a single point.
void apply_transform(transform_t *transform, gc_vec_t *point)
{
    gc_mat_t tmp;

    MATRIX_BUFFER_RESET();

    gc_mat_t *current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
    gl_mat4_translation_vec(&transform->translation, current);

    if (transform->rtype == EULER_ZYX)
    {
        current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_rotation_x(DEG2RAD(transform->rotation.v3.x), current);

        current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_rotation_y(DEG2RAD(transform->rotation.v3.y), current);

        current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_rotation_z(DEG2RAD(transform->rotation.v3.z), current);
    }
    else
    {
        current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_rotation_z(DEG2RAD(transform->rotation.v3.z), current);

        current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_rotation_y(DEG2RAD(transform->rotation.v3.y), current);

        current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_rotation_x(DEG2RAD(transform->rotation.v3.x), current);
    }

    current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
    gl_mat4_scale_vec(&transform->scaling, current);

    sse_matrix_stack_compose(&tmp);
    gl_mat4_mulvec(&tmp, point, point);
}

__INLINE__ void apply_transforms(transform_stack_t *transforms, gc_vec_t *point)
{
    transform_t *transform = 0;

    transform = transforms->data + 2;
    apply_transform(transform, point);

    transform = transforms->data + 1;
    apply_transform(transform, point);

    transform = transforms->data + 0;
    apply_transform(transform, point);
}

// ----------------------------------------------------------------------------------
// -- Model transforms.
// ----------------------------------------------------------------------------------

__INLINE__ transform_t *model_transform_push_exp(gc_model_t *model, r32 x, r32 y, r32 z, r32 rx, r32 ry, r32 rz, r32 sx, r32 sy, r32 sz, rotation_type_t rtype)
{
    u32 current_pointer = model->object.transforms.count++;
    SDL_assert(model->object.transforms.count <= 3);

    transform_t *transform = model->object.transforms.data + current_pointer;

    transform->translation.data[0] = x;
    transform->translation.data[1] = y;
    transform->translation.data[2] = z;
    transform->translation.data[3] = 0;

    transform->rtype = rtype;
    transform->rotation.data[0] = rx;
    transform->rotation.data[1] = ry;
    transform->rotation.data[2] = rz;
    transform->rotation.data[3] = 0;

    transform->scaling.data[0] = sx;
    transform->scaling.data[1] = sy;
    transform->scaling.data[2] = sz;
    transform->scaling.data[3] = 0;

    return transform;
}

__INLINE__ transform_t *model_transform_push(gc_model_t *model, gc_vec_t *position, gc_vec_t *rotation, gc_vec_t *scaling, rotation_type_t rtype)
{
    u32 current_pointer = model->object.transforms.count++;
    SDL_assert(model->object.transforms.count <= 3);

    transform_t *transform = model->object.transforms.data + current_pointer;

    transform->translation = *position;
    transform->rtype = rtype;
    transform->rotation = *rotation;
    transform->scaling = *scaling;

    return transform;
}

void gc_pipeline_model_setup(gc_model_t *model)
{
    PIPE_FLAG_DISABLE(GC_BACKFACE_CULL | GC_TRANSPARENCY | GC_FORCE_DEPTH_ONE);
    pipe_param_merged_table_t *overwrites = &GCSR.gl->pipeline.params.overwrites;

    PIPE_PARAM_COPY(1, 2);

    if (model->overwrites.wireframe_color.overwrite)
        overwrites->wireframe_color[2] = &model->overwrites.wireframe_color.value;

    if (model->overwrites.point_color.overwrite)
        overwrites->point_color[2] = &model->overwrites.point_color.value;

    if (model->overwrites.uv_scaling.overwrite)
        overwrites->uv_scaling[2] = &model->overwrites.uv_scaling.value;

    if (model->overwrites.forced_opacity.overwrite)
        overwrites->forced_opacity[2] = &model->overwrites.forced_opacity.value;

    if (FLAG(model->flags, MOD_BACKFACE_CULL))
        PIPE_FLAG_ENABLE(GC_BACKFACE_CULL);

    if (model->shader_id)
        SET_SHADER(model->shader_id);

    if (model->material && model->material->transparency)
        PIPE_FLAG_ENABLE(GC_TRANSPARENCY);
}

void gl_update_lights(gc_light_t *lights, u32 count)
{
    for (u32 i = 0; i < count; ++i)
    {
        gc_light_t *light = lights + i;

        if (light->type == GC_SUN_LIGHT)
        {
            gc_vec_t yaw_vector = {0, 0, 1};
            gc_vec_t side_transformed;

            // v3_cross(&yaw_vector, &light->position, &pitch_vector);
            // v3_normalize(&pitch_vector);

            gc_mat_t yaw_rotation_matrix;
            gc_mat_t pitch_rotation_matrix;

            gl_mat4_rotation_around_axis(&yaw_vector, DEG2RAD(light->object.rotation.v3.z), &yaw_rotation_matrix);
            gl_mat4_mulvec(&yaw_rotation_matrix, &light->object.axis.side, &side_transformed);

            gl_mat4_rotation_around_axis(&side_transformed, DEG2RAD(light->object.rotation.v3.y), &pitch_rotation_matrix);

            gl_mat4_mulvec(&yaw_rotation_matrix, &light->object.axis.forward, &light->directional.direction);
            gl_mat4_mulvec(&pitch_rotation_matrix, &light->directional.direction, &light->directional.direction);

            gl_vec3_inv(&light->directional.direction);
            v3_normalize(&light->directional.direction);
        }
        else if (light->type == GC_POINT_LIGHT)
        {}
    }
}

void gl_update_model_to_world(gc_model_t *model)
{
    gc_mat_t tmp;
    gc_mat_t *model_world_matrix = GET_MATRIX(M_MODEL_WORLD);

    if (!model->object.transforms.count)
    {
        MATRIX_BUFFER_RESET();

        gc_mat_t *current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_translation_vec(&model->object.position, current);

        if (model->object.rtype == EULER_ZYX)
        {
            current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_rotation_x(DEG2RAD(model->object.rotation.v3.x), current);

            current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_rotation_y(DEG2RAD(model->object.rotation.v3.y), current);

            current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_rotation_z(DEG2RAD(model->object.rotation.v3.z), current);
        }
        else
        {
            current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_rotation_z(DEG2RAD(model->object.rotation.v3.z), current);

            current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_rotation_y(DEG2RAD(model->object.rotation.v3.y), current);

            current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_rotation_x(DEG2RAD(model->object.rotation.v3.x), current);
        }

        current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
        gl_mat4_scale_vec(&model->object.scaling, current);

        sse_matrix_stack_compose(model_world_matrix);
    }
    else
    {
        for (u32 i = 0; i < model->object.transforms.count; ++i)
        {
            transform_t *transform = model->object.transforms.data + i;

            MATRIX_BUFFER_RESET();

            gc_mat_t *current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_translation_vec(&transform->translation, current);

            if (transform->rtype == EULER_ZYX)
            {
                current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
                gl_mat4_rotation_x(DEG2RAD(transform->rotation.v3.x), current);

                current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
                gl_mat4_rotation_y(DEG2RAD(transform->rotation.v3.y), current);

                current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
                gl_mat4_rotation_z(DEG2RAD(transform->rotation.v3.z), current);
            }
            else
            {
                current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
                gl_mat4_rotation_z(DEG2RAD(transform->rotation.v3.z), current);

                current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
                gl_mat4_rotation_y(DEG2RAD(transform->rotation.v3.y), current);

                current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
                gl_mat4_rotation_x(DEG2RAD(transform->rotation.v3.x), current);
            }

            current = MATRIX_BUFFER_NEXT(); MATRIX_BUFFER_CHECK();
            gl_mat4_scale_vec(&transform->scaling, current);

            if (i == 0)
                sse_matrix_stack_compose(model_world_matrix);
            else
            {
                sse_matrix_stack_compose(&tmp);
                sse_mat4_mul(model_world_matrix, &tmp, model_world_matrix);
            }
        }
    }
}

void gl_set_projection_from_level(gc_level_t *level)
{
    r32 aspect = GCSR.gl->current_framebuffer->aspect;

    if (GCSR.gl->camera.projection.type == GC_PROJECTION_PERSPECTIVE)
    {
        GCSR.gl->pipeline.params.f_near = GCSR.gl->camera.projection.perspective.f_near;
        GCSR.gl->pipeline.params.f_far = GCSR.gl->camera.projection.perspective.f_far;

        gc_projection_perspective(GCSR.gl->current_framebuffer->aspect,
                                  DEG2RAD(GCSR.gl->camera.projection.fov),
                                  GCSR.gl->camera.projection.perspective.f_near,
                                  GCSR.gl->camera.projection.perspective.f_far);
    }
    else if (GCSR.gl->camera.projection.type == GC_PROJECTION_ORTHOGRAPHIC)
    {
        // r32 orthographic_zoom = GCSR.gl->camera.orthographic_zoom;
        r32 fov = GCSR.gl->camera.projection.fov;

        GCSR.gl->pipeline.params.f_near = GCSR.gl->camera.projection.orthographic.f_near;
        GCSR.gl->pipeline.params.f_far = GCSR.gl->camera.projection.orthographic.f_far;

        gc_projection_orthografic(fov * 1 * aspect,
                                  fov * -1 * aspect,
                                  fov * 1,
                                  fov * -1,
                                  GCSR.gl->camera.projection.orthographic.f_near,
                                  GCSR.gl->camera.projection.orthographic.f_far);
    }
}

void level_debug_grid_pass()
{
    gc_model_t *model = &GCSR.gl->debug_grid;
    mesh_t *mesh = model->meshes[1];
    SET_SHADER(GCSR.gl->debug_grid.shader_id);
    gc_pipeline_model_setup(model);

    PIPE_FLAG_DISABLE(GC_BACKFACE_CULL | GC_FORCE_DEPTH_ONE);
    PIPE_FLAG_ENABLE(GC_TRANSPARENCY);

    // ----------------------------------------------------------------------------------
    // -- Matrix setup.
    // ----------------------------------------------------------------------------------

    gl_update_model_to_world(model);
    MATRIX_BUFFER_RESET();

    gc_push_matrix(GET_MATRIX(M_VIEW));
    gc_push_matrix(GET_MATRIX(M_MODEL_WORLD));
    gc_compose_matrix(GET_MATRIX(M_MODEL_VIEW));

    gc_mat_t tmp;
    gc_mat4_inv(GET_MATRIX(M_MODEL_VIEW), &tmp);
    gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL));

    gl_mat_empty(&tmp);
    // gl_mat4_adjoint(&GCSR.gl->matrices->model_world_matrix, &tmp);
    gc_mat4_inv(GET_MATRIX(M_MODEL_WORLD), &tmp);
    gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL_WORLD));

    // ----------------------------------------------------------------------------------
    // -- Draw the buffers.
    // ----------------------------------------------------------------------------------

    // Reset the texture slots.
    for (u8 k = 0; k < SHADER_TEXTURE_SLOTS; ++k) {
        GCSR.gl->pipeline.params.textures[k] = 0;
    }

    if (GCSR.gl->pipeline.shader->setup)
        GCSR.gl->pipeline.shader->setup(model->material);

    gl_draw_arrays(mesh->indices, mesh->vertices, mesh->indices_count, mesh->type);
}

// ----------------------------------------------------------------------------------
// -- Render the lights for debugging (for now only point lights).
// ----------------------------------------------------------------------------------

void level_debug_lights_pass(gc_level_t *level)
{
    gc_camera_t *camera = &GCSR.gl->camera;

    for (u32 i = 0; i < level->light_count; ++i)
    {
        gc_light_t *light = level->lights + i;

        if (light->type == GC_SUN_LIGHT)
        {
            gc_model_t light_model;
            gc_material_t light_material;

            light_material.blinn.diffuse.data[0] = light->color.data[0] / light->il;
            light_material.blinn.diffuse.data[1] = light->color.data[1] / light->il;
            light_material.blinn.diffuse.data[2] = light->color.data[2] / light->il;
            light_material.blinn.diffuse.data[3] = light->color.data[3] / light->il;

            r32 dir_len = 5;

            model_init(&light_model, SHADER_NONE,
                       light->directional.direction.v3.x * dir_len,
                       light->directional.direction.v3.y * dir_len,
                       light->directional.direction.v3.z * dir_len,
                       camera->rotation.pitch, 0, camera->rotation.yaw,
                       0.05f, 0.05f, 0.05f,
                       GCSR.gl->debug_light_mesh, 0, 0);

            light_model.material = &light_material;
            GCSR.gl->pipeline.params.material = &light_material;

            gc_pipeline_model_setup(&light_model);

            // ----------------------------------------------------------------------------------
            // -- Matrix setup.
            // ----------------------------------------------------------------------------------

            gl_update_model_to_world(&light_model);

            MATRIX_BUFFER_RESET();
            gc_push_matrix(GET_MATRIX(M_VIEW));
            gc_push_matrix(GET_MATRIX(M_MODEL_WORLD));
            gc_compose_matrix(GET_MATRIX(M_MODEL_VIEW));

            SET_SHADER(SHADER_DEBUG_LIGHT);
            gl_draw_arrays(light_model.meshes[0]->indices, light_model.meshes[0]->vertices, light_model.meshes[0]->indices_count, light_model.meshes[0]->type);

            // ----------------------------------------------------------------------------------
            // -- Light direction vector.
            // ----------------------------------------------------------------------------------

            __ALIGN__ u32 dir_indices[2];
            __ALIGN__ asset_vertex_t dir_vertices[2];

            asset_vertex_t *vertex = dir_vertices + 0;

            vertex->pos[0] = 0;
            vertex->pos[1] = 0;
            vertex->pos[2] = 0;
            vertex->pos[3] = 1;

            vertex->data[0] = 1 * 0.1f;
            vertex->data[1] = 1 * 0.1f;
            vertex->data[2] = 1 * 0.1f;
            vertex->data[3] = 0.1f;

            vertex = dir_vertices + 1;

            vertex->pos[0] = light_model.object.position.data[0];
            vertex->pos[1] = light_model.object.position.data[1];
            vertex->pos[2] = light_model.object.position.data[2];
            vertex->pos[3] = 1;

            vertex->data[0] = 1 * 0.1f;
            vertex->data[1] = 1 * 0.1f;
            vertex->data[2] = 1 * 0.1f;
            vertex->data[3] = 0.1f;

            gc_model_t dir_model;
            model_init(&dir_model, SHADER_NONE, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);

            dir_indices[0] = 0;
            dir_indices[1] = 1;

            gc_pipeline_model_setup(&dir_model);

            // ----------------------------------------------------------------------------------
            // -- Matrix setup.
            // ----------------------------------------------------------------------------------

            gl_update_model_to_world(&dir_model);

            MATRIX_BUFFER_RESET();
            gc_push_matrix(GET_MATRIX(M_VIEW));
            gc_push_matrix(GET_MATRIX(M_MODEL_WORLD));
            gc_compose_matrix(GET_MATRIX(M_MODEL_VIEW));

            PIPE_FLAG_ENABLE(GC_TRANSPARENCY);

            // SET_SHADER(SHADER_WIREFRAME);
            SET_SHADER(SHADER_DEBUG_GRID);
            gl_draw_arrays(dir_indices, dir_vertices, 2, GL_MESH_LINE);
        }
        else if (light->type == GC_POINT_LIGHT)
        {
            gc_model_t light_model;
            gc_material_t light_material;

            light_material.blinn.diffuse.data[0] = light->color.data[0] / light->il;
            light_material.blinn.diffuse.data[1] = light->color.data[1] / light->il;
            light_material.blinn.diffuse.data[2] = light->color.data[2] / light->il;
            light_material.blinn.diffuse.data[3] = light->color.data[3] / light->il;

            model_init(&light_model,
                       SHADER_NONE,
                       light->object.position.v4.x, light->object.position.v4.y, light->object.position.v4.z,
                       camera->rotation.pitch, 0, camera->rotation.yaw,
                       0.05f, 0.05f, 0.05f,
                       GCSR.gl->debug_light_mesh, 0, 0);

            light_model.material = &light_material;
            GCSR.gl->pipeline.params.material = &light_material;

            gc_pipeline_model_setup(&light_model);

            // ----------------------------------------------------------------------------------
            // -- Matrix setup.
            // ----------------------------------------------------------------------------------

            gl_update_model_to_world(&light_model);
            MATRIX_BUFFER_RESET();

            gc_push_matrix(GET_MATRIX(M_VIEW));
            gc_push_matrix(GET_MATRIX(M_MODEL_WORLD));
            gc_compose_matrix(GET_MATRIX(M_MODEL_VIEW));

            SET_SHADER(SHADER_DEBUG_LIGHT);
            gl_draw_arrays(light_model.meshes[0]->indices, light_model.meshes[0]->vertices, light_model.meshes[0]->indices_count, light_model.meshes[0]->type);
        }
        else
            continue;
    }
}

void render_triangle_queue(shader_id_t shader_id, u32 mode)
{
    dynamic_array_header_t *queue = da_header(GCSR.gl->triangle_queue);
    b8 is_shadow_pass = PIPE_FLAG(GC_SHADOW_PASS);

    if (shader_id == SHADER_NONE)
    {
        printf("Missing level shader\n");
        return;
    }

    for (u32 i = 0; i < queue->length; ++i)
    {
        gc_model_t *model = GCSR.gl->triangle_queue[i];
        mesh_t *mesh = 0;

        if (model->disabled)
            continue;

        #ifdef DEBUG_MESH
        if (i != DEBUG_MESH)
            continue;
        #endif

        if (is_shadow_pass && FLAG(model->flags, MOD_EXCLUDE_FROM_SHADOW))
            continue;

        // Normal mode.
        if (mode == GC_MODE_NORMAL)
        {
            mesh = model->meshes[0];
            SET_SHADER(shader_id);
        }

        else if (mode == GC_MODE_WIREFRAME)
        {
            mesh = model->meshes[1];
            SET_SHADER(SHADER_WIREFRAME);
        }

        else if (mode == GC_MODE_POINT)
        {
            mesh = model->meshes[2];
            SET_SHADER(SHADER_POINT);
        }

        if (!mesh)
            continue;

        gc_pipeline_model_setup(model);

        if (mode == GC_MODE_WIREFRAME || mode == GC_MODE_POINT)
            PIPE_FLAG_ENABLE(GC_TRANSPARENCY);

        if (model->shader_id == SHADER_SKYBOX || model->shader_id == SHADER_SKYBOX_PBR)
        {
            model->object.position = GCSR.gl->camera.eye;
            PIPE_FLAG_DISABLE(GC_BACKFACE_CULL);
            PIPE_FLAG_ENABLE(GC_FORCE_DEPTH_ONE);
        }

        // ----------------------------------------------------------------------------------
        // -- Matrix setup.
        // ----------------------------------------------------------------------------------

        gl_update_model_to_world(model);
        MATRIX_BUFFER_RESET();

        gc_push_matrix(GET_MATRIX(M_VIEW));
        gc_push_matrix(GET_MATRIX(M_MODEL_WORLD));
        gc_compose_matrix(GET_MATRIX(M_MODEL_VIEW));

        gc_mat_t tmp;
        gc_mat4_inv(GET_MATRIX(M_MODEL_VIEW), &tmp);
        gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL));

        gl_mat_empty(&tmp);
        // gl_mat4_adjoint(&GCSR.gl->matrices->model_world_matrix, &tmp);
        gc_mat4_inv(GET_MATRIX(M_MODEL_WORLD), &tmp);
        gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL_WORLD));

        // ----------------------------------------------------------------------------------
        // -- Draw the buffers.
        // ----------------------------------------------------------------------------------

        // Reset the texture slots.
        for (u8 k = 0; k < SHADER_TEXTURE_SLOTS; ++k) {
            GCSR.gl->pipeline.params.textures[k] = 0;
        }

        if (GCSR.gl->pipeline.shader->setup)
            GCSR.gl->pipeline.shader->setup(model->material);

        gl_draw_arrays(mesh->indices, mesh->vertices, mesh->indices_count, mesh->type);
    }
}

void render_line_queue()
{
    dynamic_array_header_t *queue = da_header(GCSR.gl->line_queue);

    for (u32 i = 0; i < queue->length; ++i)
    {
        gc_model_t *model = GCSR.gl->line_queue[i];
        mesh_t *mesh = model->meshes[1];

        if (model->disabled || !mesh)
            continue;

        SET_SHADER(SHADER_WIREFRAME);
        gc_pipeline_model_setup(model);
        PIPE_FLAG_ENABLE(GC_TRANSPARENCY);

        // ----------------------------------------------------------------------------------
        // -- Matrix setup.
        // ----------------------------------------------------------------------------------

        gl_update_model_to_world(model);
        MATRIX_BUFFER_RESET();

        gc_push_matrix(GET_MATRIX(M_VIEW));
        gc_push_matrix(GET_MATRIX(M_MODEL_WORLD));
        gc_compose_matrix(GET_MATRIX(M_MODEL_VIEW));

        gc_mat_t tmp;
        gc_mat4_inv(GET_MATRIX(M_MODEL_VIEW), &tmp);
        gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL));

        gl_mat_empty(&tmp);
        // gl_mat4_adjoint(&GCSR.gl->matrices->model_world_matrix, &tmp);
        gc_mat4_inv(GET_MATRIX(M_MODEL_WORLD), &tmp);
        gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL_WORLD));

        // ----------------------------------------------------------------------------------
        // -- Draw the buffers.
        // ----------------------------------------------------------------------------------

        // Reset the texture slots.
        for (u8 k = 0; k < SHADER_TEXTURE_SLOTS; ++k) {
            GCSR.gl->pipeline.params.textures[k] = 0;
        }

        if (GCSR.gl->pipeline.shader->setup)
            GCSR.gl->pipeline.shader->setup(model->material);

        gl_draw_arrays(mesh->indices, mesh->vertices, mesh->indices_count, mesh->type);
    }
}

void render_point_queue()
{
    // SET_SHADER(SHADER_POINT);
    dynamic_array_header_t *queue = da_header(GCSR.gl->point_queue);

    for (u32 i = 0; i < queue->length; ++i)
    {
        gc_model_t *model = GCSR.gl->point_queue[i];
        mesh_t *mesh = model->meshes[2];

        if (model->disabled || !mesh)
            continue;

        SET_SHADER(SHADER_POINT);
        gc_pipeline_model_setup(model);
        PIPE_FLAG_ENABLE(GC_TRANSPARENCY);

        // ----------------------------------------------------------------------------------
        // -- Matrix setup.
        // ----------------------------------------------------------------------------------

        gl_update_model_to_world(model);
        MATRIX_BUFFER_RESET();

        gc_push_matrix(GET_MATRIX(M_VIEW));
        gc_push_matrix(GET_MATRIX(M_MODEL_WORLD));
        gc_compose_matrix(GET_MATRIX(M_MODEL_VIEW));

        gc_mat_t tmp;
        gc_mat4_inv(GET_MATRIX(M_MODEL_VIEW), &tmp);
        gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL));

        gl_mat_empty(&tmp);
        // gl_mat4_adjoint(&GCSR.gl->matrices->model_world_matrix, &tmp);
        gc_mat4_inv(GET_MATRIX(M_MODEL_WORLD), &tmp);
        gl_mat4_transpose(&tmp, GET_MATRIX(M_NORMAL_WORLD));

        // ----------------------------------------------------------------------------------
        // -- Draw the buffers.
        // ----------------------------------------------------------------------------------

        // Reset the texture slots.
        for (u8 k = 0; k < SHADER_TEXTURE_SLOTS; ++k) {
            GCSR.gl->pipeline.params.textures[k] = 0;
        }

        if (GCSR.gl->pipeline.shader->setup)
            GCSR.gl->pipeline.shader->setup(model->material);

        gl_draw_arrays(mesh->indices, mesh->vertices, mesh->indices_count, mesh->type);
    }
}