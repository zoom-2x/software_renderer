// ----------------------------------------------------------------------------------
// -- File: gcsr_shadow.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-08-26 09:38:37
// -- Modified: 2022-08-26 09:38:38
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

// ----------------------------------------------------------------------------------

void gc_compute_shadow_map_directional(gc_level_t *level, gc_light_t *light)
{
    u32 shadow_map_size = GCSR.gl->current_framebuffer->color->mips->header->width;

    r32 orthographic_distance = light->shadow.camera_fov;
    r32 light_distance_mult = light->shadow.camera_distance;
    r32 shadows_near = light->shadow.f_near;
    r32 shadows_far = light->shadow.f_far;

    PIPE_FLAG_ENABLE(GC_SHADOW_PASS);
    SET_FRAMEBUFFER(SHADOWS_BUFFER);

    VINIT4(sun_position,
           light->directional.direction.v3.x * light_distance_mult,
           light->directional.direction.v3.y * light_distance_mult,
           light->directional.direction.v3.z * light_distance_mult,
           1);

    light->object.position.v3.x = sun_position.v3.x;
    light->object.position.v3.y = sun_position.v3.y;
    light->object.position.v3.z = sun_position.v3.z;

    VINIT3(sun_focus, 0, 0, 0);
    VINIT3(sun_vector_up, 0, 0, 1);

    gc_projection_orthografic(orthographic_distance,
                              -orthographic_distance,
                              orthographic_distance,
                              -orthographic_distance,
                              shadows_near,
                              shadows_far);

    gc_mat4_lookat(&sun_position, &sun_focus, &sun_vector_up, GET_MATRIX(M_VIEW));
    gc_viewport(shadow_map_size, shadow_map_size, 0, 0);

    MATRIX_BUFFER_RESET();
    gc_push_matrix(GET_MATRIX(M_PROJECTION));
    gc_push_matrix(GET_MATRIX(M_VIEW));
    gc_compose_matrix(GET_MATRIX(M_SUN_LIGHT));

    // PUSH_PROGRAM(SHADER_SHADOW);
    PIPE_FLAG_ENABLE(GC_SHADOW_PASS);
    render_triangle_queue(SHADER_SHADOW, GC_MODE_NORMAL);
    // level_pass(level, GC_MODE_NORMAL);
    PIPE_FLAG_DISABLE(GC_SHADOW_PASS);
    // POP_PROGRAM();

    #ifdef GL_DEBUG_SHADOW_MAP
    GCSR.gl->current_framebuffer->flags |= FB_FLAG_COPY;
    #endif
}

void gc_compute_point_shadow_map(gc_level_t *level, gc_light_t *light, gl_cube_faces_t face)
{
    gc_vec_t target;
    gc_vec_t up = {0, 0, 1};

    if (face == CUBE_LEFT)
    {
        target.v3.x = light->object.position.v3.x - 1;
        target.v3.y = light->object.position.v3.y;
        target.v3.z = light->object.position.v3.z;
    }
    else if (face == CUBE_RIGHT)
    {
        target.v3.x = light->object.position.v3.x + 1;
        target.v3.y = light->object.position.v3.y;
        target.v3.z = light->object.position.v3.z;
    }
    else if (face == CUBE_FRONT)
    {
        target.v3.x = light->object.position.v3.x;
        target.v3.y = light->object.position.v3.y + 1;
        target.v3.z = light->object.position.v3.z;
    }
    else if (face == CUBE_BACK)
    {
        target.v3.x = light->object.position.v3.x;
        target.v3.y = light->object.position.v3.y - 1;
        target.v3.z = light->object.position.v3.z;
    }
    else if (face == CUBE_TOP)
    {
        target.v3.x = light->object.position.v3.x;
        target.v3.y = light->object.position.v3.y;
        target.v3.z = light->object.position.v3.z + 1;

        up.v3.x = 0;
        up.v3.y = -1;
        up.v3.z = 0;
    }
    else if (face == CUBE_BOTTOM)
    {
        target.v3.x = light->object.position.v3.x;
        target.v3.y = light->object.position.v3.y;
        target.v3.z = light->object.position.v3.z - 1;

        up.v3.x = 0;
        up.v3.y = 1;
        up.v3.z = 0;
    }

    gc_projection_perspective(GCSR.gl->current_framebuffer->aspect, DEG2RAD(90), light->shadow.f_near, light->shadow.f_far);
    gc_mat4_lookat(&light->object.position, &target, &up, GET_MATRIX(M_VIEW));
    gc_viewport(GCSR.gl->current_framebuffer->tiled_width, GCSR.gl->current_framebuffer->tiled_height, 0, 0);

    // MATRIX_BUFFER_RESET();
    // gc_push_matrix(GET_MATRIX(M_PROJECTION));
    // gc_push_matrix(GET_MATRIX(M_VIEW));
    // gc_compose_matrix(GET_MATRIX(M_SUN_LIGHT));

    GCSR.gl->pipeline.params.current_light = light;

    // PUSH_PROGRAM(SHADER_POINT_SHADOW);
    PIPE_FLAG_ENABLE(GC_SHADOW_PASS);
    render_triangle_queue(SHADER_POINT_SHADOW, GC_MODE_NORMAL);
    // level_pass(level, GC_MODE_NORMAL);
    PIPE_FLAG_DISABLE(GC_SHADOW_PASS);
    // POP_PROGRAM();

    GCSR.gl->pipeline.params.current_light = 0;

    #ifdef GL_DEBUG_SHADOW_MAP
    GCSR.gl->current_framebuffer->flags |= FB_FLAG_COPY;
    #endif
}
