// ----------------------------------------------------------------------------------
// -- File: gcsr_camera.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-07-20 10:58:21
// -- Modified: 2021-12-28 22:15:47
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#define CAMERA_START_EYE {0.0f, 0.0f, 1.0f}
#define CAMERA_START_TARGET {0.0f, 0.0f, 0.0f}
#define CAMERA_PITCH_VECTOR {0.0f, 0.0f, 1.0f}
#define CAMERA_YAW_VECTOR {0.0f, -1.0f, 0.0f}

void default_camera(gc_camera_t *camera)
{
    if (camera->type == GC_NONE)
        camera->type = GC_CAMERA_FOCUS_POINT;

    camera->track_target = 0;
    camera->manual_orientation = false;

    camera->projection.type = GC_PROJECTION_PERSPECTIVE;
    camera->projection.perspective.f_near = 0.1f;
    camera->projection.perspective.f_far = 100.0f;
    camera->projection.fov = 50;

    camera->translation.v3.x = 0.0f;
    camera->translation.v3.y = 0.0f;
    camera->translation.v3.z = 0.0f;

    camera->rotation.pitch = 0.0f;
    camera->rotation.yaw = 0.0f;
    camera->rotation.roll = 0.0f;

    camera->dampening = 0.5f;
    camera->h_sens = 1.0f;
    camera->v_sens = 1.0f;
    camera->wheel_sens = 1.0f;
    camera->orbit_h_sens = 0.7f;
    camera->orbit_v_sens = 0.7f;
    camera->speed = 0.2f;
    camera->global_sens = 1.0f;
}

void update_camera_orientation(gc_camera_t *camera)
{
    // camera->rotation.roll = 45;

    gl_mat4_rotation_x(DEG2RAD(camera->rotation.pitch), &camera->pitch_rotation_matrix);
    gl_mat4_rotation_z(DEG2RAD(camera->rotation.roll), &camera->roll_rotation_matrix);
    gl_mat4_rotation_z(DEG2RAD(camera->rotation.yaw), &camera->yaw_rotation_matrix);

    camera->side.v3.x = -1.0f;
    camera->side.v3.y = 0.0f;
    camera->side.v3.z = 0.0f;

    camera->up.v3.x = 0.0f;
    camera->up.v3.y = 1.0f;
    camera->up.v3.z = 0.0f;

    camera->forward.v3.x = 0.0f;
    camera->forward.v3.y = 0.0f;
    camera->forward.v3.z = -1.0f;

    // -- Apply the rotations.

    gl_mat3_mulvec(&camera->roll_rotation_matrix, &camera->up, &camera->up);
    gl_mat3_mulvec(&camera->pitch_rotation_matrix, &camera->up, &camera->up);
    gl_mat3_mulvec(&camera->yaw_rotation_matrix, &camera->up, &camera->up);

    gl_mat3_mulvec(&camera->roll_rotation_matrix, &camera->side, &camera->side);
    gl_mat3_mulvec(&camera->pitch_rotation_matrix, &camera->side, &camera->side);
    gl_mat3_mulvec(&camera->yaw_rotation_matrix, &camera->side, &camera->side);

    gl_mat3_mulvec(&camera->roll_rotation_matrix, &camera->forward, &camera->forward);
    gl_mat3_mulvec(&camera->pitch_rotation_matrix, &camera->forward, &camera->forward);
    gl_mat3_mulvec(&camera->yaw_rotation_matrix, &camera->forward, &camera->forward);

    v3_normalize(&camera->side);
    v3_normalize(&camera->up);
    v3_normalize(&camera->forward);
}

void camera_compute_angles(gc_camera_t *camera)
{
    gc_vec_t direction;

    direction.v3.x = camera->eye.v3.x - camera->target.v3.x;
    direction.v3.y = camera->eye.v3.y - camera->target.v3.y;
    direction.v3.z = camera->eye.v3.z - camera->target.v3.z;

    camera->zoom = gl_vec3_len(&direction);

    if (direction.v3.x == 0 && direction.v3.y == 0 && direction.v3.z == 0)
        printf("[ERROR]: Camera {eye} and {target} should be different !");

    gc_euler_xyz_t angles = v3_polar_angles_focus(&direction);

    camera->rotation.pitch = angles.pitch;
    camera->rotation.yaw = angles.heading;

    update_camera_orientation(camera);
}

__INLINE__ void camera_reset(gc_camera_t *camera)
{
    camera->eye = camera->origin_eye;
    camera->target = camera->origin_target;

    camera_compute_angles(camera);
}

void _update_manual_orientation(gc_camera_t *camera)
{
    gc_vec_t direction;

    direction.v3.x = camera->eye.v3.x - camera->target.v3.x;
    direction.v3.y = camera->eye.v3.y - camera->target.v3.y;
    direction.v3.z = camera->eye.v3.z - camera->target.v3.z;

    if (direction.v3.x == 0 && direction.v3.y == 0 && direction.v3.z == 0)
        printf("[ERROR]: Camera {eye} and {target} should be different !");

    gc_euler_xyz_t angles = v3_polar_angles_focus(&direction);

    camera->rotation.pitch = angles.pitch;
    camera->rotation.yaw = angles.heading;

    update_camera_orientation(camera);
    camera->manual_orientation = false;
}

void init_camera(gc_camera_t *camera)
{
    DEBUG_SET_CAMERA_EYE(camera);
    DEBUG_SET_CAMERA_TARGET(camera);

    if (camera->dampening > 1.0f)
        camera->dampening = 1.0f;

    if (camera->dampening <= 0.2f)
        camera->dampening = 0.2f;

    camera->eye = camera->origin_eye;
    camera->target = camera->origin_target;

    // -- Determine the starting rotation angles.
    camera_compute_angles(camera);

    camera->t_mult_x = GCSR.gl->framebuffers[PRIMARY_BUFFER]->aspect * 2.0f / GCSR.gl->framebuffers[PRIMARY_BUFFER]->width;
    camera->t_mult_y = 2.0f / GCSR.gl->framebuffers[PRIMARY_BUFFER]->height;
}

void _update_camera_basic(gc_camera_t *camera)
{
    r32 frames_elapsed = GCSR.gl->pipeline.params.delta * ONE_OVER_TARGET_FRAME_TIME;
    r32 frame_dampening = camera->dampening * camera->dampening * 0.5f * frames_elapsed;

    if (frame_dampening > 1.0f)
        frame_dampening = 1.0f;

    // ----------------------------------------------------------------------------------
    // -- Process the rotation.
    // ----------------------------------------------------------------------------------

    r32 pitch_to_apply = camera->movement.rotation.pitch * frame_dampening;
    r32 yaw_to_apply = camera->movement.rotation.yaw * frame_dampening;

    camera->movement.rotation.pitch -= pitch_to_apply;
    camera->movement.rotation.yaw -= yaw_to_apply;

    camera->rotation.pitch += pitch_to_apply;
    camera->rotation.yaw += yaw_to_apply;

    // -- Pitch limits.

    if (camera->rotation.pitch < 0)
        camera->rotation.pitch = 0;

    if (camera->rotation.pitch > 180)
        camera->rotation.pitch = 180;

    update_camera_orientation(camera);

    // ----------------------------------------------------------------------------------
    // -- Process the translation.
    // ----------------------------------------------------------------------------------

    r32 ht_to_apply = camera->movement.h_translation * frame_dampening;
    r32 vt_to_apply = camera->movement.v_translation * frame_dampening;

    VINIT3(world_axis_z, 0.0f, 0.0f, 1.0f);

    camera->movement.h_translation -= ht_to_apply;
    camera->movement.v_translation -= vt_to_apply;

    // gc_vec_t t_vec;

    camera->eye.v3.x += camera->side.v3.x * ht_to_apply;
    camera->eye.v3.y += camera->side.v3.y * ht_to_apply;
    camera->eye.v3.z += camera->side.v3.z * ht_to_apply;

    // camera->translation.v3.x += t_vec.v3.x;
    // camera->translation.v3.y += t_vec.v3.y;
    // camera->translation.v3.z += t_vec.v3.z;

    camera->eye.v3.x += camera->up.v3.x * vt_to_apply;
    camera->eye.v3.y += camera->up.v3.y * vt_to_apply;
    camera->eye.v3.z += camera->up.v3.z * vt_to_apply;

    // camera->translation.v3.x += t_vec.v3.x;
    // camera->translation.v3.y += t_vec.v3.y;
    // camera->translation.v3.z += t_vec.v3.z;

    // -- Apply the zoom (the zoom is applied as a single value, the z coordinate,
    // -- since the initial camera orientation assumes that the "eye" and the
    // -- "target" are on the pitch reference vector, which is the "Oz" world vector).

    r32 zoom_to_apply = camera->movement.zoom * frame_dampening;
    camera->movement.zoom -= zoom_to_apply;

    if (camera->projection.type == GC_PROJECTION_ORTHOGRAPHIC)
    {
        camera->projection.fov -= zoom_to_apply;

        if (camera->projection.fov < 0.1f)
            camera->projection.fov = 0.1f;
    }
    else
    {
        camera->eye.v3.x += camera->forward.v3.x * zoom_to_apply;
        camera->eye.v3.y += camera->forward.v3.y * zoom_to_apply;
        camera->eye.v3.z += camera->forward.v3.z * zoom_to_apply;

        // camera->target.v3.x += zoom_vec.v3.x;
        // camera->target.v3.y += zoom_vec.v3.y;
        // camera->target.v3.z += zoom_vec.v3.z;
    }

    // ----------------------------------------------------------------------------------
    // -- Apply all the transformations.
    // ----------------------------------------------------------------------------------

    // Starting camera eye.
    // camera->eye.v3.x = 0;
    // camera->eye.v3.y = 0;
    // camera->eye.v3.z = camera->zoom;

    // gl_mat3_mulvec(&camera->pitch_rotation_matrix, &camera->eye, &camera->eye);
    // gl_mat3_mulvec(&camera->yaw_rotation_matrix, &camera->eye, &camera->eye);

    camera->target.v3.x = camera->eye.v3.x + camera->forward.v3.x * 5;
    camera->target.v3.y = camera->eye.v3.y + camera->forward.v3.y * 5;
    camera->target.v3.z = camera->eye.v3.z + camera->forward.v3.z * 5;
}

void _update_camera_focus(gc_camera_t *camera)
{
    r32 frames_elapsed = GCSR.gl->pipeline.params.delta * ONE_OVER_TARGET_FRAME_TIME;
    r32 frame_dampening = camera->dampening * camera->dampening * 0.5f * frames_elapsed;

    if (frame_dampening > 1.0f)
        frame_dampening = 1.0f;

    // ----------------------------------------------------------------------------------
    // -- Process the rotation.
    // ----------------------------------------------------------------------------------

    r32 pitch_to_apply = camera->movement.rotation.pitch * frame_dampening;
    r32 yaw_to_apply = camera->movement.rotation.yaw * frame_dampening;

    camera->movement.rotation.pitch -= pitch_to_apply;
    camera->movement.rotation.yaw -= yaw_to_apply;

    camera->rotation.pitch += pitch_to_apply;
    camera->rotation.yaw += yaw_to_apply;

    // -- Pitch limits.

    if (camera->rotation.pitch < 0)
        camera->rotation.pitch = 0;

    if (camera->rotation.pitch > 180)
        camera->rotation.pitch = 180;

    update_camera_orientation(camera);

    // ----------------------------------------------------------------------------------
    // -- Process the translation.
    // ----------------------------------------------------------------------------------

    r32 ht_to_apply = camera->movement.h_translation * frame_dampening;
    r32 vt_to_apply = camera->movement.v_translation * frame_dampening;

    camera->movement.h_translation -= ht_to_apply;
    camera->movement.v_translation -= vt_to_apply;

    camera->target.v3.x += camera->side.v3.x * ht_to_apply;
    camera->target.v3.y += camera->side.v3.y * ht_to_apply;
    camera->target.v3.z += camera->side.v3.z * ht_to_apply;

    camera->target.v3.x += camera->up.v3.x * vt_to_apply;
    camera->target.v3.y += camera->up.v3.y * vt_to_apply;
    camera->target.v3.z += camera->up.v3.z * vt_to_apply;

    r32 zoom_to_apply = camera->movement.zoom * frame_dampening;
    camera->movement.zoom -= zoom_to_apply;
    camera->eye = camera->origin_eye;

    if (camera->projection.type == GC_PROJECTION_ORTHOGRAPHIC)
    {
        camera->projection.fov += zoom_to_apply;

        if (camera->projection.fov < 0.1f)
            camera->projection.fov = 0.1f;
    }
    else
    {
        // -- Apply the zoom (the zoom is applied as a single value, the z coordinate,
        // -- since the initial camera orientation assumes that the "eye" and the
        // -- "target" are on the pitch reference vector, which is the "Oz" world vector).

        camera->zoom += zoom_to_apply;

        // r32 eye_distance = gl_vec3_len(&camera->origin_eye);
        r32 max_zoom = 0.2f;

        if (camera->zoom < max_zoom)
        {
            // camera->zoom = -(eye_distance - max_zoom);
            camera->zoom = 0.2f;
            camera->movement.zoom = 0;
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Apply all the transformations.
    // ----------------------------------------------------------------------------------

    // Starting camera eye.
    camera->eye.v3.x = 0;
    camera->eye.v3.y = 0;
    camera->eye.v3.z = camera->zoom;

    gl_mat3_mulvec(&camera->pitch_rotation_matrix, &camera->eye, &camera->eye);
    gl_mat3_mulvec(&camera->yaw_rotation_matrix, &camera->eye, &camera->eye);

    camera->eye.v3.x += camera->target.v3.x;
    camera->eye.v3.y += camera->target.v3.y;
    camera->eye.v3.z += camera->target.v3.z;
}

void gl_reset_camera(gc_camera_t *camera)
{}

void process_camera_basic_input(gc_camera_t *camera)
{
    input_bindings_t *bindings = get_input_bindings();

    if (bindings->actions[MOUSE_RIGHT_CLICK].is_active)
    {
        if (bindings->actions[MOUSE_MOVE_LEFT].is_active)
        {
            r32 t = -bindings->actions[MOUSE_MOVE_LEFT].mouse_x_rel * camera->t_mult_x * camera->h_sens * camera->global_sens;
            camera->movement.rotation.yaw += CAMERA_ANGULAR_SPEED * t;
        }

        if (bindings->actions[MOUSE_MOVE_RIGHT].is_active)
        {
            r32 t = -bindings->actions[MOUSE_MOVE_RIGHT].mouse_x_rel * camera->t_mult_x * camera->h_sens * camera->global_sens;
            camera->movement.rotation.yaw += CAMERA_ANGULAR_SPEED * t;
        }

        if (bindings->actions[MOUSE_MOVE_UP].is_active)
        {
            r32 t = -bindings->actions[MOUSE_MOVE_UP].mouse_y_rel * camera->t_mult_y * camera->v_sens * camera->global_sens;
            camera->movement.rotation.pitch += CAMERA_ANGULAR_SPEED * t;
        }

        if (bindings->actions[MOUSE_MOVE_DOWN].is_active)
        {
            r32 t = -bindings->actions[MOUSE_MOVE_DOWN].mouse_y_rel * camera->t_mult_y * camera->v_sens * camera->global_sens;
            camera->movement.rotation.pitch += CAMERA_ANGULAR_SPEED * t;
        }
    }

    if (bindings->actions[MOVE_CAMERA_LEFT].is_active)
        camera->movement.h_translation += camera->speed * camera->global_sens;

    if (bindings->actions[MOVE_CAMERA_RIGHT].is_active)
        camera->movement.h_translation -= camera->speed * camera->global_sens;

    if (bindings->actions[MOVE_CAMERA_UP].is_active)
        camera->movement.v_translation += camera->speed * camera->global_sens;

    if (bindings->actions[MOVE_CAMERA_DOWN].is_active)
        camera->movement.v_translation -= camera->speed * camera->global_sens;

    if (bindings->actions[MOVE_CAMERA_FORWARD].is_active)
        camera->movement.zoom += camera->speed * camera->global_sens;

    if (bindings->actions[MOVE_CAMERA_BACKWARD].is_active)
        camera->movement.zoom -= camera->speed * camera->global_sens;
}

void process_camera_focus_input(gc_camera_t *camera)
{
    input_bindings_t *bindings = get_input_bindings();

    if (bindings->actions[MOUSE_LEFT_CLICK].is_active)
    {
        if (!camera->orbiting_mode)
        {
            camera->orbiting_mode = true;
            SDL_GetMouseState(&camera->start_point.x, &camera->start_point.y);
        }
    }
    else
    {
        if (camera->orbiting_mode)
            camera->orbiting_mode = false;
    }

    if (bindings->actions[MOUSE_WHEEL_SCROLL].is_active)
    {
        r32 zoom_offset = bindings->actions[MOUSE_WHEEL_SCROLL].mouse_y_rel * camera->wheel_sens;
        camera->movement.zoom -= zoom_offset;
        bindings->actions[MOUSE_WHEEL_SCROLL].is_active = false;
    }

    // ----------------------------------------------------------------------------------
    // -- Right click focus point movement.
    // ----------------------------------------------------------------------------------

    if (bindings->actions[MOUSE_RIGHT_CLICK].is_active)
    {
        if (!bindings->actions[CAMERA_FOCUS_LIMIT_VERTICAL].is_active)
        {
            if (bindings->actions[MOUSE_MOVE_LEFT].is_active)
                camera->movement.h_translation += bindings->actions[MOUSE_MOVE_LEFT].mouse_x_rel * camera->t_mult_x * camera->h_sens;

            if (bindings->actions[MOUSE_MOVE_RIGHT].is_active)
                camera->movement.h_translation += bindings->actions[MOUSE_MOVE_RIGHT].mouse_x_rel * camera->t_mult_x * camera->h_sens;
        }

        if (!bindings->actions[CAMERA_FOCUS_LIMIT_HORIZONTAL].is_active)
        {
            if (bindings->actions[MOUSE_MOVE_UP].is_active)
                camera->movement.v_translation += bindings->actions[MOUSE_MOVE_UP].mouse_y_rel * camera->t_mult_y * camera->v_sens;

            if (bindings->actions[MOUSE_MOVE_DOWN].is_active)
                camera->movement.v_translation += bindings->actions[MOUSE_MOVE_DOWN].mouse_y_rel * camera->t_mult_y * camera->v_sens;
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Orbiting.
    // ----------------------------------------------------------------------------------

    if (camera->orbiting_mode)
    {
        vec2i current_point;
        SDL_GetMouseState(&current_point.x, &current_point.y);
        vec2i dir = vec2i_sub(current_point, camera->start_point);
        camera->start_point = current_point;

        if (!bindings->actions[CAMERA_FOCUS_LIMIT_VERTICAL].is_active)
        {
            if (dir.x > 0)
            {
                r32 t = dir.x * camera->t_mult_x * camera->orbit_h_sens;
                camera->movement.rotation.yaw -= 180 * t;
            }
            else
            {
                r32 t = -dir.x * camera->t_mult_x * camera->orbit_h_sens;
                camera->movement.rotation.yaw += 180 * t;
            }
        }

        if (!bindings->actions[CAMERA_FOCUS_LIMIT_HORIZONTAL].is_active)
        {
            if (dir.y > 0)
            {
                r32 t = dir.y * camera->t_mult_y * camera->orbit_v_sens;
                camera->movement.rotation.pitch -= 180 * t;
            }
            else
            {
                r32 t = -dir.y * camera->t_mult_y * camera->orbit_v_sens;
                camera->movement.rotation.pitch += 180 * t;
            }
        }
    }
}

// #define init_camera(camera) \
// { \
//     if ((camera)->type == GC_CAMERA_BASIC) \
//         _init_camera_basic(camera); \
//     else if ((camera)->type == GC_CAMERA_FOCUS_POINT) \
//         _init_camera_focus(camera); \
// }

#define update_camera(camera) \
{ \
    if ((camera)->manual_orientation) \
        _update_manual_orientation(camera); \
    else if ((camera)->type == GC_CAMERA_BASIC) \
        _update_camera_basic(camera); \
    else if ((camera)->type == GC_CAMERA_FOCUS_POINT) \
        _update_camera_focus(camera); \
}

#define process_camera_input(camera) \
{ \
    OPTICK_EVENT("process_camera_input"); \
\
    if ((camera)->type == GC_CAMERA_BASIC) \
        process_camera_basic_input(camera); \
    else if ((camera)->type == GC_CAMERA_FOCUS_POINT) \
        process_camera_focus_input(camera); \
}