// ----------------------------------------------------------------------------------
// -- File: gcsr_animation.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-16 20:42:45
// -- Modified: 2022-12-16 20:42:45
// ----------------------------------------------------------------------------------

#ifndef GCSR_ANIMATION_H
#define GCSR_ANIMATION_H

typedef enum
{
    ANIM_CIRCLE,
    ANIM_ELLIPSE,
    ANIM_BEZIER_CURVE,

    ANIM_COUNT
} animation_type_t;

typedef struct
{
    s32 frame;
    r32 value;
} keyframe_t;

typedef struct
{
    gc_model_t *model;
    s32 offset;
} animation_target_t;

typedef enum
{
    ANIMATION_DISABLED = 1,
    ANIMATION_USE_CAMERA = 2,
    ANIMATION_FOLLOW_PATH = 4,
    ANIMATION_LOOP = 8
} animation_flag_t;

typedef struct
{
    animation_type_t type;
    bezier_curve_t *curve;

    u32 flags;

    // b8 disabled;
    // b8 use_camera;
    // b8 loop;

    r32 timescale;

    u32 target_count;
    animation_target_t *targets;

    transform_t transform;

    u32 target_fps;
    r32 frame_ms;
    r32 one_over_frame_ms;
    r32 current_ms;

    s32 frames;
    s32 frame;

    s32 frame_offset;

    u32 offset_keyframes;
    u32 roll_keyframes;
    u32 pitch_keyframes;
    u32 heading_keyframes;
    u32 horizontal_keyframes;
    u32 vertical_keyframes;
    u32 scaling_x_keyframes;
    u32 scaling_y_keyframes;

    // offset = t interpolation => object position
    // tilt = x axis rotation, roll => object rotation
    // horizontal = local object side movement => object position
    // vertical = local object up movement => object position
    // scaling x/y => object scaling
    // ORDER: scaling-rotation-translation + side/up translation - other rotations

    keyframe_t *offset;
    keyframe_t *roll;
    keyframe_t *pitch;
    keyframe_t *heading;
    keyframe_t *horizontal;
    keyframe_t *vertical;
    keyframe_t *scaling_x;
    keyframe_t *scaling_y;
} animation_t;

#endif