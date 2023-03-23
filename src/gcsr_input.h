// ----------------------------------------------------------------------------------
// -- File: gcsr_input.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created:
// -- Modified: 2021-12-28 22:15:25
// ----------------------------------------------------------------------------------

#ifndef GCSR_ENGINE_INPUT_H
#define GCSR_ENGINE_INPUT_H

typedef enum
{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,

    USE,

    MOVE_CAMERA_UP,
    MOVE_CAMERA_DOWN,
    MOVE_CAMERA_LEFT,
    MOVE_CAMERA_RIGHT,
    MOVE_CAMERA_FORWARD,
    MOVE_CAMERA_BACKWARD,

    ROTATE_CAMERA_LEFT,
    ROTATE_CAMERA_RIGHT,

    CAMERA_FOCUS_LIMIT_HORIZONTAL,
    CAMERA_FOCUS_LIMIT_VERTICAL,
    CAMERA_CENTER_ORIGIN,

    MOUSE_MOVE_UP,
    MOUSE_MOVE_DOWN,
    MOUSE_MOVE_LEFT,
    MOUSE_MOVE_RIGHT,

    MOUSE_LEFT_CLICK,
    MOUSE_RIGHT_CLICK,
    MOUSE_WHEEL_SCROLL,

    LIGHT_ROTATE_RIGHT,
    LIGHT_ROTATE_LEFT,
    LIGHT_ROTATE_DOWN,
    LIGHT_ROTATE_UP,

    CHANGE_RENDER_MODE,
    DEBUG_OUTPUT_MODE,

    ACTION_COUNT
} input_action_t;

typedef struct
{
    input_action_t type;
    b32 is_active;
    b32 is_down;
    b32 is_up;

    union
    {
        struct {
            u32 primary_keycode;
            u32 secodary_keycode;
        };

        struct {
            s32 mouse_x;
            s32 mouse_y;
            s32 mouse_x_rel;
            s32 mouse_y_rel;
        };
    };
} binding_t;

typedef struct
{
    b32 enabled;
    b32 is_init;

    binding_t actions[ACTION_COUNT];
} input_bindings_t;

#endif