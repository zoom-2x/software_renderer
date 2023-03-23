// ----------------------------------------------------------------------------------
// -- File: gcsr_animation.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-16 20:43:57
// -- Modified: 2022-12-16 20:43:58
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

s32 frame_loop_clamp(s32 frame, s32 max_frames)
{
    frame--;
    max_frames--;

    frame = frame % max_frames;

    if (frame < 0)
        frame += max_frames;

    frame++;

    return frame;
}

__INLINE__ s32 frame_offset(s32 frame, s32 max_frames, s32 offset) {
    return frame_loop_clamp(frame + offset, max_frames);
}

__INLINE__ r32 _smoothstep(r32 t)
{
    if (t < 0)
        t = 0;

    if (t > 1)
        t = 1;

    return t * t * (3 - 2 * t);
}

r32 _compute_animation_keyframe(keyframe_t *keyframes, u32 count, s32 frame)
{
    r32 res = 0;

    for (u32 i = 0; i < count - 1; ++i)
    {
        keyframe_t *current = keyframes + i;
        keyframe_t *next = keyframes + i + 1;

        if (frame >= current->frame && frame <= next->frame)
        {
            if (next->frame != current->frame)
            {
                r32 t = (r32) (frame - current->frame) / (next->frame - current->frame);
                // t = _smoothstep(t);
                res = current->value * (1 - t) + next->value * t;
            }
            else
                res = current->value;

            break;
        }
    }

    return res;
}

void _update_camera_animation(animation_t *animation)
{
    s32 frame = frame_offset(animation->frame, animation->frames, animation->frame_offset);

    b8 is_manual = GCSR.gl->camera.track_target || FLAG(animation->flags, ANIMATION_FOLLOW_PATH);

    // Offset keyframe.
    if (animation->offset_keyframes > 1)
    {
        r32 offset_t_value = _compute_animation_keyframe(animation->offset, animation->offset_keyframes, frame);

        if (FLAG(animation->curve->flags, BEZIER_SYNCHRONIZE))
            offset_t_value = bezier_sync_to_lut_from_tg(animation->curve, offset_t_value);

        gc_vec_t new_position = bezier_compute_point(animation->curve, offset_t_value);

        GCSR.gl->camera.eye.v4.x = new_position.v3.x;
        GCSR.gl->camera.eye.v4.y = new_position.v3.y;
        GCSR.gl->camera.eye.v4.z = new_position.v3.z;
        GCSR.gl->camera.eye.v4.w = 1;

        apply_transform(&animation->curve->transform, &GCSR.gl->camera.eye);

        // This model is tracked by the camera so we need to compute it's transformed
        // origin position.

        if (GCSR.gl->camera.track_target)
        {
            gc_vec_t target_origin = {0, 0, 0, 1};
            apply_transforms(&GCSR.gl->camera.track_target->object.transforms, &target_origin);

            GCSR.gl->camera.target.v3.x = target_origin.v3.x;
            GCSR.gl->camera.target.v3.y = target_origin.v3.y;
            GCSR.gl->camera.target.v3.z = target_origin.v3.z;
        }
        else if (FLAG(animation->flags, ANIMATION_FOLLOW_PATH))
        {
            gc_vec_t tangent = bezier_compute_first_derivative(animation->curve, offset_t_value);
            v3_normalize(&tangent);

            GCSR.gl->camera.target.v3.x = GCSR.gl->camera.eye.v3.x + tangent.v3.x * 3;
            GCSR.gl->camera.target.v3.y = GCSR.gl->camera.eye.v3.y + tangent.v3.y * 3;
            GCSR.gl->camera.target.v3.z = GCSR.gl->camera.eye.v3.z + tangent.v3.z * 3;
        }

        GCSR.gl->camera.manual_orientation = true;
    }

    if (!GCSR.gl->camera.track_target && animation->roll_keyframes > 1)
    {
        r32 roll_angle = _compute_animation_keyframe(animation->roll, animation->roll_keyframes, frame);
        GCSR.gl->camera.rotation.roll = roll_angle;
    }

    if ((!GCSR.gl->camera.track_target && !FLAG(animation->flags, ANIMATION_FOLLOW_PATH)))
    {
        if (animation->pitch_keyframes > 1)
        {
            r32 pitch_angle = _compute_animation_keyframe(animation->pitch, animation->pitch_keyframes, frame);
            GCSR.gl->camera.manual_orientation = false;
            GCSR.gl->camera.type = GC_CAMERA_BASIC;
        }

        if (animation->heading_keyframes > 1)
        {
            r32 heading_angle = _compute_animation_keyframe(animation->heading, animation->heading_keyframes, frame);
            GCSR.gl->camera.rotation.yaw = heading_angle;
            GCSR.gl->camera.manual_orientation = false;
            GCSR.gl->camera.type = GC_CAMERA_BASIC;
        }
    }
}

void _update_base_animation(animation_t *animation)
{
    for (u32 i = 0; i < animation->target_count; ++i)
    {
        animation_target_t *target = animation->targets + i;
        s32 frame = frame_offset(animation->frame, animation->frames, target->offset + animation->frame_offset);

        transform_t animation_transform;

        animation_transform.translation.v3.x = 0;
        animation_transform.translation.v3.y = 0;
        animation_transform.translation.v3.z = 0;

        animation_transform.rotation.v3.x = 0;
        animation_transform.rotation.v3.y = 0;
        animation_transform.rotation.v3.z = 0;

        animation_transform.scaling.v3.x = 1;
        animation_transform.scaling.v3.y = 1;
        animation_transform.scaling.v3.z = 1;

        // Transform order: curve - animation - model.
        // Push the curve transformation.

        MODEL_TRANSFORM_RESET(target->model);
        model_transform_push(target->model,
                             &animation->curve->transform.translation,
                             &animation->curve->transform.rotation,
                             &animation->curve->transform.scaling,
                             EULER_XYZ);

        // Offset keyframe.
        if (animation->offset_keyframes > 1)
        {
            r32 offset_t_value = _compute_animation_keyframe(animation->offset, animation->offset_keyframes, frame);

            if (animation->curve->flags & BEZIER_SYNCHRONIZE)
                offset_t_value = bezier_sync_to_lut_from_tg(animation->curve, offset_t_value);

            // r32 tc = bezier_tc(animation->curve->section_count, offset_t_value);
            gc_vec_t new_position = bezier_compute_point(animation->curve, offset_t_value);

            animation_transform.translation.data[0] = new_position.data[0];
            animation_transform.translation.data[1] = new_position.data[1];
            animation_transform.translation.data[2] = new_position.data[2];

            if (FLAG(animation->flags, ANIMATION_FOLLOW_PATH))
            {
                gc_vec_t tangent = bezier_compute_first_derivative(animation->curve, offset_t_value);
                v3_normalize(&tangent);
                gc_euler_xyz_t polar_angles = v3_polar_angles(&tangent);

                // The curve rotation angles are applied to the base model transformation.
                target->model->object.rotation.data[0] = 0;
                target->model->object.rotation.data[1] = polar_angles.y;
                target->model->object.rotation.data[2] = polar_angles.z;
            }
        }

        if (FLAG(animation->flags, ANIMATION_FOLLOW_PATH) && animation->roll_keyframes > 1)
        {
            r32 roll_angle = _compute_animation_keyframe(animation->roll, animation->roll_keyframes, frame);
            // The curve rotation angles are applied to the base model transformation.
            target->model->object.rotation.data[0] = roll_angle;
        }

        if (animation->horizontal_keyframes > 1)
        {}

        if (animation->vertical_keyframes > 1)
        {}

        if (animation->scaling_x_keyframes > 1)
        {}

        if (animation->scaling_y_keyframes > 1)
        {}

        // Push the animation transformation.
        model_transform_push(target->model,
                             &animation_transform.translation,
                             &animation_transform.rotation,
                             &animation_transform.scaling,
                             EULER_XYZ);
        MODEL_PUSH_BASE_TRANSFORM(target->model);
    }
}

void update_animation(animation_t *animation, r32 delta)
{
    if (!animation ||
        !animation->curve ||
        FLAG(animation->flags, ANIMATION_DISABLED) ||
        (!FLAG(animation->flags, ANIMATION_LOOP) && (animation->frame == animation->frames)))
        return;

    // Frame update.
    if (animation->current_ms >= animation->frame_ms)
    {
        r32 frame_count_elapsed = floorf(animation->current_ms * animation->one_over_frame_ms);
        animation->current_ms -= frame_count_elapsed * animation->frame_ms;
        animation->frame += frame_count_elapsed;
    }

    if (FLAG(animation->flags, ANIMATION_USE_CAMERA))
        _update_camera_animation(animation);
    else
        _update_base_animation(animation);

    if (FLAG(animation->flags, ANIMATION_LOOP))
        animation->frame %= animation->frames;
    else if (animation->frame >= animation->frames)
        animation->frame = animation->frames;

    animation->current_ms += delta;
}