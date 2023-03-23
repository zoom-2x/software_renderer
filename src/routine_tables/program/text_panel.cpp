// ----------------------------------------------------------------------------------
// -- File: text_panel.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-11-20 12:02:59
// -- Modified: 2022-11-21 19:17:00
// ----------------------------------------------------------------------------------

#include "fonts/zxspectrum.h"

#define PANEL_CHARACTER_CELLS 64
#define PANEL_MODEL_SHIFT 6

#define PANEL_STATE_NEXT_CHARACTER 1
#define PANEL_STATE_PUSH_CHARACTER 2
#define PANEL_STATE_END_DELAY 3

typedef __ALIGN__ struct
{
    u16 mask[8];
    u8 shift;
} tpanel_char_t;

typedef struct
{
    char input_text[255];
    u32 text_size;

    u8 capacity;
    u32 state;

    s32 rows;
    s32 cols;
    u32 input_char_index;
    r32 current_ms;

    gc_vec_t scaling;
    r32 cube_size;

    r32 spacing;
    u32 delay;
    r32 anim_ms;
    r32 height;
    r32 z_scaling;

    tpanel_char_t input_buffer;
    tpanel_char_t *characters;

    gc_level_t *level;

    u32 model_count;
    gc_model_t *models;
} text_panel_t;

void *_setup_text_panel(gc_level_t *level)
{
    memory_manager_t *manager = &GCSR.state->manager;
    text_panel_t *text_panel = (text_panel_t *) stack_push(manager->stack, sizeof(text_panel_t));

    // ----------------------------------------------------------------------------------
    // -- Default panel settings.
    // ----------------------------------------------------------------------------------

    text_panel->level = level;

    text_panel->capacity = 8;
    text_panel->state = PANEL_STATE_NEXT_CHARACTER;
    text_panel->scaling.v3.x = 0.25f;
    text_panel->scaling.v3.y = 0.25f;
    text_panel->scaling.v3.z = 0.5f;
    text_panel->cube_size = 2.0f;
    text_panel->spacing = 0.1f;
    text_panel->height = 0.0f;
    text_panel->z_scaling = text_panel->scaling.v3.z;

    strncpy(text_panel->input_text, "Demonstration", 255);

    text_panel->input_char_index = 0;
    text_panel->anim_ms = 60.0f;
    text_panel->current_ms = 0;
    text_panel->input_buffer.shift = 0;
    text_panel->delay = 3;

    // ----------------------------------------------------------------------------------
    // -- Read the configuration.
    // ----------------------------------------------------------------------------------

    if (level->program_settings)
    {
        JSON_VALUE_OBJECT_LOOP(level->program_settings->value, pi)
        {
            struct _json_object_entry *setting = JSON_OBJECT_PROPERTY(level->program_settings->value, pi);

            if (JSON_PROPERTY_COMPARE(setting, "capacity", json_integer))
                text_panel->capacity = JSON_PROPERTY_VALUE_INTEGER(setting);
            else if (JSON_PROPERTY_COMPARE(setting, "scaling", json_array) && JSON_ARRAY_LENGTH(setting->value) == 3)
                _json_prop_extract_v3(&text_panel->scaling, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "spacing", json_double))
                _json_prop_extract_float(&text_panel->spacing, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "delay", json_integer))
                text_panel->delay = JSON_PROPERTY_VALUE_INTEGER(setting);
            else if (JSON_PROPERTY_COMPARE(setting, "anim_ms", json_double))
                _json_prop_extract_float(&text_panel->anim_ms, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "anim_text", json_string))
            {
                char *tmp = JSON_PROPERTY_VALUE_STRING(setting);
                strncpy(text_panel->input_text, tmp, 255);
            }
            else if (JSON_PROPERTY_COMPARE(setting, "height", json_double))
                _json_prop_extract_float(&text_panel->height, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "z_scaling", json_double))
                _json_prop_extract_float(&text_panel->z_scaling, setting);
        }
    }

    if (text_panel->z_scaling == 0)
        text_panel->z_scaling = text_panel->scaling.v3.z;

    u32 len = (u32) strlen(text_panel->input_text);
    text_panel->text_size = len + text_panel->delay;
    text_panel->characters = (tpanel_char_t *) stack_push(manager->stack, sizeof(tpanel_char_t) * text_panel->capacity);

    for (u8 i = 0; i < text_panel->delay; ++i)
    {
        if (len + i < 255)
            text_panel->input_text[len + i] = ' ';
    }

    text_panel->rows = 8;
    text_panel->cols = text_panel->capacity * 8;

    // ----------------------------------------------------------------------------------
    // -- Init the panel models.
    // ----------------------------------------------------------------------------------

    text_panel->model_count = text_panel->capacity * PANEL_CHARACTER_CELLS;
    size_t model_bytes = sizeof(gc_model_t) * text_panel->model_count;
    text_panel->models = (gc_model_t *) stack_push(manager->stack, model_bytes);

    r32 step_x = text_panel->cube_size * text_panel->scaling.v3.x + text_panel->spacing;
    r32 step_y = text_panel->cube_size * text_panel->scaling.v3.y + text_panel->spacing;

    s32 row_end = -(text_panel->rows >> 1);
    s32 row_start = row_end + text_panel->rows - 1;

    s32 col_start = -(text_panel->cols >> 1);
    // s32 col_end = col_start + text_panel->cols - 1;

    u32 model_index = 0;

    for (u8 i = 0; i < text_panel->capacity; ++i)
    {
        s32 char_col_start = col_start + i * 8;
        s32 char_col_end = char_col_start + 7;

        for (s32 row = row_start; row >= row_end; --row)
        {
            for (s32 col = char_col_start; col <= char_col_end; ++col)
            {
                gc_model_t *model = text_panel->models + model_index++;

                model->meshes[0] = (mesh_t *) level->meshes[0];
                model->meshes[1] = (mesh_t *) level->meshes[1];
                model->meshes[1] = 0;

                model->material = level->materials + 0;
                model->shader_id = SHADER_NONE;
                // model->shader = 0;

                model->object.position.v4.x = col * step_x;
                model->object.position.v4.y = row * step_y;
                model->object.position.v4.z = 0;
                model->object.position.v4.w = 1;

                model->object.rotation.v3.x = 0;
                model->object.rotation.v3.y = 0;
                model->object.rotation.v3.z = 0;

                model->object.scaling.v3.x = text_panel->scaling.v3.x;
                model->object.scaling.v3.y = text_panel->scaling.v3.y;
                model->object.scaling.v3.z = text_panel->scaling.v3.z;

                // model->overwrites.uv_scaling.v.v2.x = 1;
                // model->overwrites.uv_scaling.v.v2.y = 1;

                model->flags = MOD_BACKFACE_CULL;

                PUSH_TRIANGLE(model);
            }
        }
    }

    GCSR.state->update_params = text_panel;

    return text_panel;
}

void _next_character(text_panel_t *text_panel)
{
    char input_char = text_panel->input_text[text_panel->input_char_index++];
    u32 zx_index = input_char - 32;
    zxspectrum_char_t *zxchar = zxspectrum_font + zx_index;

    text_panel->input_buffer.mask[0] = zxchar->data[0];
    text_panel->input_buffer.mask[1] = zxchar->data[1];
    text_panel->input_buffer.mask[2] = zxchar->data[2];
    text_panel->input_buffer.mask[3] = zxchar->data[3];
    text_panel->input_buffer.mask[4] = zxchar->data[4];
    text_panel->input_buffer.mask[5] = zxchar->data[5];
    text_panel->input_buffer.mask[6] = zxchar->data[6];
    text_panel->input_buffer.mask[7] = zxchar->data[7];

    text_panel->input_char_index %= text_panel->text_size;
}

void _update_text_panel(r32 delta, void *data)
{
    text_panel_t *text_panel = (text_panel_t *) data;

    if (!text_panel || !text_panel->text_size)
        return;

    // ----------------------------------------------------------------------------------
    // -- Get the next character.
    // ----------------------------------------------------------------------------------

    if (text_panel->state == PANEL_STATE_NEXT_CHARACTER)
    {
        _next_character(text_panel);
        text_panel->state = PANEL_STATE_PUSH_CHARACTER;
    }

    // ----------------------------------------------------------------------------------
    // -- Push the characters.
    // ----------------------------------------------------------------------------------

    if (text_panel->state == PANEL_STATE_PUSH_CHARACTER)
    {
        text_panel->current_ms += delta;

        if (text_panel->current_ms >= text_panel->anim_ms)
        {
            u32 required_shifts = (u32) (text_panel->current_ms / text_panel->anim_ms);
            text_panel->current_ms -= text_panel->anim_ms * required_shifts;

            for (u32 k = 0; k < required_shifts; ++k)
            {
                u16 pushed_bit[8];
                u16 previous_bit[8];

                text_panel->input_buffer.shift++;

                pushed_bit[0] = (text_panel->input_buffer.mask[0] & 0b10000000) >> 7;
                pushed_bit[1] = (text_panel->input_buffer.mask[1] & 0b10000000) >> 7;
                pushed_bit[2] = (text_panel->input_buffer.mask[2] & 0b10000000) >> 7;
                pushed_bit[3] = (text_panel->input_buffer.mask[3] & 0b10000000) >> 7;
                pushed_bit[4] = (text_panel->input_buffer.mask[4] & 0b10000000) >> 7;
                pushed_bit[5] = (text_panel->input_buffer.mask[5] & 0b10000000) >> 7;
                pushed_bit[6] = (text_panel->input_buffer.mask[6] & 0b10000000) >> 7;
                pushed_bit[7] = (text_panel->input_buffer.mask[7] & 0b10000000) >> 7;

                text_panel->input_buffer.mask[0] = (text_panel->input_buffer.mask[0] << 1) & 0xff;
                text_panel->input_buffer.mask[1] = (text_panel->input_buffer.mask[1] << 1) & 0xff;
                text_panel->input_buffer.mask[2] = (text_panel->input_buffer.mask[2] << 1) & 0xff;
                text_panel->input_buffer.mask[3] = (text_panel->input_buffer.mask[3] << 1) & 0xff;
                text_panel->input_buffer.mask[4] = (text_panel->input_buffer.mask[4] << 1) & 0xff;
                text_panel->input_buffer.mask[5] = (text_panel->input_buffer.mask[5] << 1) & 0xff;
                text_panel->input_buffer.mask[6] = (text_panel->input_buffer.mask[6] << 1) & 0xff;
                text_panel->input_buffer.mask[7] = (text_panel->input_buffer.mask[7] << 1) & 0xff;

                // Push all the characters in the display buffer and insert the new bits.
                for (u8 i = text_panel->capacity; i > 0; --i)
                {
                    tpanel_char_t *ch = text_panel->characters + i - 1;

                    previous_bit[0] = (ch->mask[0] & 0b10000000) >> 7;
                    previous_bit[1] = (ch->mask[1] & 0b10000000) >> 7;
                    previous_bit[2] = (ch->mask[2] & 0b10000000) >> 7;
                    previous_bit[3] = (ch->mask[3] & 0b10000000) >> 7;
                    previous_bit[4] = (ch->mask[4] & 0b10000000) >> 7;
                    previous_bit[5] = (ch->mask[5] & 0b10000000) >> 7;
                    previous_bit[6] = (ch->mask[6] & 0b10000000) >> 7;
                    previous_bit[7] = (ch->mask[7] & 0b10000000) >> 7;

                    ch->mask[0] = ((ch->mask[0] << 1) & 0xff) | pushed_bit[0];
                    ch->mask[1] = ((ch->mask[1] << 1) & 0xff) | pushed_bit[1];
                    ch->mask[2] = ((ch->mask[2] << 1) & 0xff) | pushed_bit[2];
                    ch->mask[3] = ((ch->mask[3] << 1) & 0xff) | pushed_bit[3];
                    ch->mask[4] = ((ch->mask[4] << 1) & 0xff) | pushed_bit[4];
                    ch->mask[5] = ((ch->mask[5] << 1) & 0xff) | pushed_bit[5];
                    ch->mask[6] = ((ch->mask[6] << 1) & 0xff) | pushed_bit[6];
                    ch->mask[7] = ((ch->mask[7] << 1) & 0xff) | pushed_bit[7];

                    pushed_bit[0] = previous_bit[0];
                    pushed_bit[1] = previous_bit[1];
                    pushed_bit[2] = previous_bit[2];
                    pushed_bit[3] = previous_bit[3];
                    pushed_bit[4] = previous_bit[4];
                    pushed_bit[5] = previous_bit[5];
                    pushed_bit[6] = previous_bit[6];
                    pushed_bit[7] = previous_bit[7];
                }

                if (text_panel->input_buffer.shift == 8)
                {
                    text_panel->input_buffer.shift = 0;
                    _next_character(text_panel);
                    // text_panel->state = PANEL_STATE_NEXT_CHARACTER;
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Update the panel cells (animation).
    // ----------------------------------------------------------------------------------

    u32 model_index = 0;

    for (u8 i = 0; i < text_panel->capacity; ++i)
    {
        tpanel_char_t *ch = text_panel->characters + i;
        model_index = i * PANEL_CHARACTER_CELLS;

        for (u32 r = 0; r < 8; ++r)
        {
            u16 mask = ch->mask[r];

            for (u32 c = 0; c < 8; ++c)
            {
                gc_model_t *model = text_panel->models + model_index++;
                model->object.scaling.v3.z = text_panel->scaling.v3.z;
                model->object.position.v3.z = 0;

                if (cmask[c] & mask)
                {
                    model->object.scaling.v3.z = text_panel->z_scaling;
                    model->object.position.v3.z = text_panel->height;
                    model->material = text_panel->level->materials + 1;
                }
                else
                    model->material = text_panel->level->materials + 0;
            }
        }
    }
}