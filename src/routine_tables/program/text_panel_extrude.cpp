// ----------------------------------------------------------------------------------
// -- File: text_panel.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-11-19 15:17:24
// -- Modified: 2022-11-19 15:17:24
// ----------------------------------------------------------------------------------

#include "fonts/zxspectrum.h"

#define PANEL_CHARACTER_CELLS 64
#define PANEL_MODEL_SHIFT 6


#define PANEL_ANIMATION_IDLE 0
#define PANEL_ANIMATION_FORWARD 1
#define PANEL_ANIMATION_BACKWARD 2
#define PANEL_ANIMATION_DELAY 3
#define PANEL_ANIMATION_STATES 4

#define PANEL_CELL_ANIMATION_BEGIN 1
#define PANEL_CELL_ANIMATION_RUNNING 2
#define PANEL_CELL_ANIMATION_FINISHED 3

#define PANEL_ANIMATION_POSITION 1
#define PANEL_ANIMATION_SCALING 2

#define PANEL_ANIMATION_OFFSET_VERTICAL 1
#define PANEL_ANIMATION_OFFSET_HORIZONTAL 2

typedef struct varchar_s varchar_t;

struct varchar_s
{
    u32 len;
    char *string;
    struct varchar_s *next;
};

typedef struct
{
    u8 state;

    r32 offset_ms;
    r32 elapsed_ms;
    r32 progress_ms;
} text_panel_cell_t;

typedef __ALIGN__ struct
{
    u16 mask[8];
    text_panel_cell_t cells[PANEL_CHARACTER_CELLS];
} text_panel_char_t;

typedef struct
{
    u8 capacity;

    s32 rows;
    s32 cols;

    gc_vec_t scaling;
    r32 cube_size;
    r32 spacing;

    u32 word_count;
    varchar_t *words;

    u8 anim_state;
    r32 anim_height;
    r32 anim_ms;
    r32 anim_offset_ms;
    r32 anim_end_delay_ms;
    r32 anim_end_delay_check;
    u32 anim_type;
    u32 anim_offset_type;
    u32 anim_word_count;

    varchar_t *current_word;
    u32 animated_cells;

    text_panel_char_t *characters;

    gc_level_t *level;

    u32 model_count;
    gc_model_t *models;
} text_panel_extrude_t;

void *_setup_text_panel_extrude(gc_level_t *level)
{
    memory_manager_t *manager = &GCSR.state->manager;
    text_panel_extrude_t *text_panel = (text_panel_extrude_t *) stack_push(manager->stack, sizeof(text_panel_extrude_t));

    // ----------------------------------------------------------------------------------
    // -- Default panel settings.
    // ----------------------------------------------------------------------------------

    text_panel->level = level;

    text_panel->capacity = 9;
    text_panel->scaling.v3.x = 0.25f;
    text_panel->scaling.v3.y = 0.25f;
    text_panel->scaling.v3.z = 0.5f;
    text_panel->cube_size = 2.0f;
    text_panel->spacing = 0.05f;

    text_panel->words = 0;
    text_panel->word_count = 0;

    text_panel->anim_state = PANEL_ANIMATION_IDLE;
    text_panel->anim_height = 3.0f;
    text_panel->anim_ms = 200.0f;
    text_panel->anim_offset_ms = 80;
    text_panel->anim_end_delay_ms = 0;
    text_panel->anim_end_delay_check = 0;
    text_panel->anim_type = PANEL_ANIMATION_POSITION;
    text_panel->anim_offset_type = PANEL_ANIMATION_OFFSET_VERTICAL;
    text_panel->anim_word_count = 0;

    text_panel->current_word = 0;
    text_panel->animated_cells = 0;

    char input_text[255] = "Gabi are mere si pere!!";

    // ----------------------------------------------------------------------------------
    // -- Read the configuration.
    // ----------------------------------------------------------------------------------

    if (level->program_settings)
    {
        JSON_VALUE_OBJECT_LOOP(level->program_settings->value, si)
        {
            struct _json_object_entry *setting = JSON_OBJECT_PROPERTY(level->program_settings->value, si);

            if (JSON_PROPERTY_COMPARE(setting, "capacity", json_integer))
                text_panel->capacity = JSON_PROPERTY_VALUE_INTEGER(setting);
            else if (JSON_PROPERTY_COMPARE(setting, "scaling", json_array) && JSON_ARRAY_LENGTH(setting->value) == 3)
                _json_prop_extract_v3(&text_panel->scaling, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "spacing", json_double))
                _json_prop_extract_float(&text_panel->spacing, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "anim_text", json_string))
            {
                char *tmp = JSON_PROPERTY_VALUE_STRING(setting);
                strncpy(input_text, tmp, 255);
            }
            else if (JSON_PROPERTY_COMPARE(setting, "anim_height", json_double))
                _json_prop_extract_float(&text_panel->anim_height, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "anim_ms", json_double))
                _json_prop_extract_float(&text_panel->anim_ms, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "anim_offset_ms", json_double))
                _json_prop_extract_float(&text_panel->anim_offset_ms, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "anim_end_delay_ms", json_double))
                _json_prop_extract_float(&text_panel->anim_end_delay_ms, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "anim_type", json_string))
            {
                if (strcmp(JSON_VALUE_STRING(setting->value), "position") == 0)
                    text_panel->anim_type = PANEL_ANIMATION_POSITION;
                else if (strcmp(JSON_VALUE_STRING(setting->value), "scaling") == 0)
                    text_panel->anim_type = PANEL_ANIMATION_SCALING;
            }
            else if (JSON_PROPERTY_COMPARE(setting, "anim_offset_type", json_string))
            {
                if (strcmp(JSON_VALUE_STRING(setting->value), "vertical") == 0)
                    text_panel->anim_offset_type = PANEL_ANIMATION_OFFSET_VERTICAL;
                else if (strcmp(JSON_VALUE_STRING(setting->value), "horizontal") == 0)
                    text_panel->anim_offset_type = PANEL_ANIMATION_OFFSET_HORIZONTAL;
            }
        }
    }

    // NOTE(gabic): A character is represented as a 8x8 pixel map. There are
    // 8 lines and each line represents a byte.
    text_panel->rows = 8;
    text_panel->cols = text_panel->capacity * 8;

    // ----------------------------------------------------------------------------------
    // -- Allocate the text buffer.
    // ----------------------------------------------------------------------------------

    u32 text_len = (u32) strlen(input_text);
    u32 word_len = 0;
    u32 word_start = 0;
    varchar_t *prev_word = 0;

    // Extract the words of the input text.
    for (u32 i = 0; i <= text_len; ++i)
    {
        if (input_text[i] == ' ' || input_text[i] == '\0')
        {
            varchar_t *word = (varchar_t *) stack_push(manager->stack, sizeof(varchar_t) + word_len);

            word->string = 0;
            word->next = 0;

            if (!text_panel->words)
            {
                text_panel->words = word;
                text_panel->current_word = word;
            }

            char *string = (char *) (word + 1);
            word->len = word_len;
            word->string = string;
            text_panel->word_count++;

            if (prev_word)
                prev_word->next = word;

            // Copy the word.
            for (u32 j = word_start; j < word_start + word_len; ++j) {
                *string++ = input_text[j];
            }

            prev_word = word;
            word_start = i + 1;
            word_len = 0;
        }
        else
            word_len++;
    }

    // ----------------------------------------------------------------------------------
    // -- Init the panel characters.
    // ----------------------------------------------------------------------------------

    text_panel->characters = (text_panel_char_t *) stack_push(manager->stack, sizeof(text_panel_char_t) * text_panel->capacity);

    for (u8 i = 0; i < text_panel->capacity; ++i)
    {
        text_panel_char_t *ch = text_panel->characters + i;

        ch->mask[0] = 0;
        ch->mask[1] = 0;
        ch->mask[2] = 0;
        ch->mask[3] = 0;
        ch->mask[4] = 0;
        ch->mask[5] = 0;
        ch->mask[6] = 0;
        ch->mask[7] = 0;

        for (u32 r = 0, char_index = 0; r < 8; ++r)
        {
            for (u32 c = 0; c < 8; ++c)
            {
                text_panel_cell_t *cell = ch->cells + char_index++;

                cell->state = PANEL_CELL_ANIMATION_BEGIN;
                cell->offset_ms = 0;
                cell->elapsed_ms = 0;
                cell->progress_ms = 0;
            }
        }
    }

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
                model->meshes[2] = 0;

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

__INLINE__ r32 ease_in_cubic(r32 x) {
    return (x * x * x);
}

__INLINE__ r32 ease_out_cubic(r32 x)
{
    r32 res = 1 - x;
    res = res * res * res;

    return (1 - res);
}

__INLINE__ r32 ease_in_quart(r32 x) {
    return (x * x * x * x);
}

__INLINE__ r32 ease_out_quart(r32 x)
{
    r32 res = 1 - x;
    res = res * res * res * res;

    return (1 - res);
}

r32 _set_character_offset_vertical(text_panel_extrude_t *text_panel, text_panel_char_t *ch, r32 offset)
{
    for (u32 c = 0; c < 8; ++c)
    {
        b8 has_lit_pixels = false;

        for (u32 r = 0; r < 8; ++r)
        {
            text_panel_cell_t *cell = ch->cells + r * 8 + c;
            cell->offset_ms = 0;

            if (ch->mask[r] & cmask[c])
            {
                has_lit_pixels = true;

                // Begin adding to the offset.
                if (offset < 0)
                    offset = 0;

                cell->offset_ms = offset;
            }
        }

        if (has_lit_pixels && offset >= 0)
            offset += text_panel->anim_offset_ms;
    }

    return offset;
}

r32 _set_character_offset_horizontal(text_panel_extrude_t *text_panel, text_panel_char_t *ch, r32 offset)
{
    u32 cell_index = 0;

    for (u32 r = 0; r < 8; ++r)
    {
        for (u32 c = 0; c < 8; ++c)
        {
            b8 has_lit_pixels = false;

            text_panel_cell_t *cell = ch->cells + cell_index++;
            cell->offset_ms = 0;

            if (ch->mask[r] & cmask[c])
            {
                has_lit_pixels = true;

                // Begin adding to the offset.
                if (offset < 0)
                    offset = 0;

                cell->offset_ms = offset;
                offset += text_panel->anim_offset_ms;
            }
        }
    }

    return offset;
}

void _update_text_panel_extrude(r32 delta, void *data)
{
    text_panel_extrude_t *text_panel = (text_panel_extrude_t *) data;

    if (!text_panel || !text_panel->word_count)
        return;

    // ----------------------------------------------------------------------------------
    // -- Update the panel mask based on the current word.
    // ----------------------------------------------------------------------------------

    if (text_panel->anim_state == PANEL_ANIMATION_IDLE)
    {
        text_panel->anim_state = PANEL_ANIMATION_FORWARD;

        // Reset the text panel mask.

        u32 start_char = 0;
        u32 end_char = 0;

        // The word is bigger than the panel capacity, the word will be trimed.
        if (text_panel->current_word->len > text_panel->capacity)
        {
            end_char = text_panel->capacity - 1;
        }
        // Compute the word's start position on the panel board.
        else
        {
            start_char = (text_panel->capacity - text_panel->current_word->len) >> 1;
            end_char = start_char + text_panel->current_word->len - 1;
        }

        r32 offset = -1;

        for (u8 i = 0; i < text_panel->capacity; ++i)
        {
            text_panel_char_t *ch = text_panel->characters + i;

            if (i >= start_char && i <= end_char)
            {
                u32 index = i - start_char;
                char _c = text_panel->current_word->string[index];

                u32 zx_index = _c - 32;
                zxspectrum_char_t *zxchar = zxspectrum_font + zx_index;

                ch->mask[0] = zxchar->data[0];
                ch->mask[1] = zxchar->data[1];
                ch->mask[2] = zxchar->data[2];
                ch->mask[3] = zxchar->data[3];
                ch->mask[4] = zxchar->data[4];
                ch->mask[5] = zxchar->data[5];
                ch->mask[6] = zxchar->data[6];
                ch->mask[7] = zxchar->data[7];

                if (text_panel->anim_offset_type == PANEL_ANIMATION_OFFSET_HORIZONTAL)
                    offset = _set_character_offset_horizontal(text_panel, ch, offset);
                else if (text_panel->anim_offset_type == PANEL_ANIMATION_OFFSET_VERTICAL)
                    offset = _set_character_offset_vertical(text_panel, ch, offset);
            }
            else
            {
                ch->mask[0] = 0;
                ch->mask[1] = 0;
                ch->mask[2] = 0;
                ch->mask[3] = 0;
                ch->mask[4] = 0;
                ch->mask[5] = 0;
                ch->mask[6] = 0;
                ch->mask[7] = 0;
            }
        }

        text_panel->current_word = text_panel->current_word->next;

        // Switch to the next word or start from the beginning.
        if (!text_panel->current_word)
            text_panel->current_word = text_panel->words;
    }

    u32 model_index = 0;

    // ----------------------------------------------------------------------------------
    // -- Update the panel cells (animation).
    // ----------------------------------------------------------------------------------

    if (text_panel->anim_state == PANEL_ANIMATION_FORWARD || text_panel->anim_state == PANEL_ANIMATION_BACKWARD)
    {
        for (u8 i = 0; i < text_panel->capacity; ++i)
        {
            text_panel_char_t *ch = text_panel->characters + i;
            u32 cell_index = 0;

            for (u32 r = 0; r < 8; ++r)
            {
                u16 mask = ch->mask[r];

                for (u32 c = 0; c < 8; ++c)
                {
                    text_panel_cell_t *cell = ch->cells + cell_index++;
                    gc_model_t *model = text_panel->models + model_index++;

                    if (cmask[c] & mask)
                    {
                        if (cell->state == PANEL_CELL_ANIMATION_BEGIN)
                        {
                            text_panel->animated_cells++;
                            cell->state = PANEL_CELL_ANIMATION_RUNNING;
                        }

                        // Start the forward animation.
                        if (cell->state == PANEL_CELL_ANIMATION_RUNNING && cell->elapsed_ms >= cell->offset_ms)
                        {
                            cell->progress_ms += delta;
                            r32 progress = cell->progress_ms / text_panel->anim_ms;

                            if (progress >= 1.0f)
                                progress = 1.0f;

                            if (text_panel->anim_state == PANEL_ANIMATION_FORWARD)
                            {
                                // progress = ease_out_cubic(progress);
                                progress = ease_out_quart(progress);

                                if (text_panel->anim_type == PANEL_ANIMATION_POSITION)
                                    model->object.position.v3.z = progress * text_panel->anim_height;
                                else if (text_panel->anim_type == PANEL_ANIMATION_SCALING)
                                    model->object.scaling.v3.z = text_panel->scaling.v3.z + progress * text_panel->anim_height;

                                if (cell->state == PANEL_CELL_ANIMATION_RUNNING)
                                    model->material = text_panel->level->materials + 1;

                                if (progress == 1.0f)
                                {
                                    cell->state = PANEL_CELL_ANIMATION_FINISHED;
                                    text_panel->animated_cells--;
                                }
                            }
                            else if (text_panel->anim_state == PANEL_ANIMATION_BACKWARD)
                            {
                                // progress = ease_in_cubic(progress);
                                progress = ease_in_quart(progress);

                                if (text_panel->anim_type == PANEL_ANIMATION_POSITION)
                                    model->object.position.v3.z = (1.0f - progress) * text_panel->anim_height;
                                else if (text_panel->anim_type == PANEL_ANIMATION_SCALING)
                                    model->object.scaling.v3.z = text_panel->scaling.v3.z + (1.0f - progress) * text_panel->anim_height;

                                if (progress == 1.0f)
                                {
                                    cell->state = PANEL_CELL_ANIMATION_FINISHED;
                                    text_panel->animated_cells--;
                                }

                                if (cell->state == PANEL_CELL_ANIMATION_FINISHED)
                                    model->material = text_panel->level->materials + 0;
                            }
                        }
                        else
                            cell->elapsed_ms += delta;
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // -- The animation is finished, reset the cells and go to the next state.
        // ----------------------------------------------------------------------------------

        if (!text_panel->animated_cells)
        {
            // Reset the cells.
            for (u8 i = 0; i < text_panel->capacity; ++i)
            {
                text_panel_char_t *ch = text_panel->characters + i;
                u32 cell_index = 0;

                for (u32 j = 0; j < PANEL_CHARACTER_CELLS; ++j)
                {
                    text_panel_cell_t *cell = ch->cells + cell_index++;

                    cell->state = PANEL_CELL_ANIMATION_BEGIN;
                    cell->elapsed_ms = 0;
                    cell->progress_ms = 0;
                }
            }

            text_panel->anim_state++;
            text_panel->anim_state %= PANEL_ANIMATION_STATES;

            if (text_panel->anim_state == PANEL_ANIMATION_DELAY)
                text_panel->anim_word_count++;
        }
    }

    // ----------------------------------------------------------------------------------
    // -- End animation delay.
    // ----------------------------------------------------------------------------------

    if (text_panel->anim_state == PANEL_ANIMATION_DELAY)
    {
        // Apply the delay after the final word.

        if (text_panel->anim_word_count == text_panel->word_count)
            text_panel->anim_end_delay_check += delta;
        else
            text_panel->anim_end_delay_check = text_panel->anim_end_delay_ms;

        if (text_panel->anim_end_delay_check >= text_panel->anim_end_delay_ms)
        {
            if (text_panel->anim_word_count == text_panel->word_count)
                text_panel->anim_word_count = 0;

            text_panel->anim_end_delay_check = 0;
            text_panel->anim_state++;
            text_panel->anim_state %= PANEL_ANIMATION_STATES;
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Update the shadow maps.
    // ----------------------------------------------------------------------------------

    for (u32 i = 0; i < text_panel->level->light_count; ++i)
    {
        gc_light_t *light = text_panel->level->lights + i;

        if (light->type == GC_SUN_LIGHT || light->type == GC_POINT_LIGHT)
            light->updated = true;
    }
}

