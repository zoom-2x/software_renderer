// ----------------------------------------------------------------------------------
// -- File: wave.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-11-16 21:47:39
// -- Modified: 2022-11-16 21:47:39
// ----------------------------------------------------------------------------------

#define WAVE_ANIMATION_POSITION 1
#define WAVE_ANIMATION_SCALING 2

typedef struct
{
    r32 total_delta;
    u32 animation;

    b8 shadow_plane;
    r32 shadow_plane_z;
    r32 shadow_plane_scaling;

    s32 rows;
    s32 cols;

    gc_vec_t scaling;
    r32 cube_size;
    r32 offset;

    r32 row_offset;
    r32 col_offset;

    r32 row_period_ms;
    r32 row_radian_ms;

    r32 col_period_ms;
    r32 col_radian_ms;

    r32 row_amplitude;
    r32 col_amplitude;

    r32 sin_offset;

    u32 model_count;
    gc_model_t *models;
    gc_level_t *level;
} wave_t;

extern global_vars_t GCSR;

void *_setup_wave(gc_level_t *level)
{
    memory_manager_t *manager = &GCSR.state->manager;

    u32 index = 0;

    wave_t *wave_setup = (wave_t *) stack_push(manager->stack, sizeof(wave_t));

    // ----------------------------------------------------------------------------------
    // -- Default wave settings.
    // ----------------------------------------------------------------------------------

    wave_setup->level = level;
    wave_setup->animation = WAVE_ANIMATION_POSITION;

    wave_setup->shadow_plane = false;
    wave_setup->shadow_plane_z = -4.0f;
    wave_setup->shadow_plane_scaling = 2.0f;

    wave_setup->scaling.v3.x = 0.25f;
    wave_setup->scaling.v3.y = 0.25f;
    wave_setup->scaling.v3.z = 0.25f;

    wave_setup->cube_size = 2.0f;
    wave_setup->offset = 0.1f;

    wave_setup->rows = 10;
    wave_setup->cols = 10;

    wave_setup->row_period_ms = 8000.0f;
    wave_setup->row_radian_ms = 1.0f / (wave_setup->row_period_ms * ONE_OVER_2PI);

    wave_setup->col_period_ms = 4000.0f;
    wave_setup->col_radian_ms = 1.0f / (wave_setup->col_period_ms * ONE_OVER_2PI);

    wave_setup->row_offset = PI / 10.0f;
    wave_setup->col_offset = PI / 12.0f;

    wave_setup->row_amplitude = 1.5f;
    wave_setup->col_amplitude = 0.8f;

    wave_setup->sin_offset = 1;
    wave_setup->total_delta = 0;

    // ----------------------------------------------------------------------------------
    // -- Read the configuration.
    // ----------------------------------------------------------------------------------

    if (level->program_settings)
    {
        JSON_VALUE_OBJECT_LOOP(level->program_settings->value, si)
        {
            struct _json_object_entry *setting = JSON_OBJECT_PROPERTY(level->program_settings->value, si);

            if (JSON_PROPERTY_COMPARE(setting, "animation", json_string))
            {
                if (strcmp(JSON_VALUE_STRING(setting->value), "position") == 0)
                    wave_setup->animation = WAVE_ANIMATION_POSITION;
                else if (strcmp(JSON_VALUE_STRING(setting->value), "scaling") == 0)
                    wave_setup->animation = WAVE_ANIMATION_SCALING;
            }
            else if (JSON_PROPERTY_COMPARE(setting, "rows", json_integer))
                wave_setup->rows = JSON_PROPERTY_VALUE_INTEGER(setting);
            else if (JSON_PROPERTY_COMPARE(setting, "cols", json_integer))
                wave_setup->cols = JSON_PROPERTY_VALUE_INTEGER(setting);
            else if (JSON_PROPERTY_COMPARE(setting, "scaling", json_array) && JSON_ARRAY_LENGTH(setting->value) == 3)
                _json_prop_extract_v3(&wave_setup->scaling, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "scaling", json_array) && JSON_ARRAY_LENGTH(setting->value) == 4)
                _json_prop_extract_v4(&wave_setup->scaling, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "cube_size", json_double))
                _json_prop_extract_float(&wave_setup->cube_size, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "offset", json_double))
                _json_prop_extract_float(&wave_setup->offset, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "row_period_ms", json_double))
            {
                _json_prop_extract_float(&wave_setup->row_period_ms, setting);
                wave_setup->row_radian_ms = 1.0f / (wave_setup->row_period_ms * ONE_OVER_2PI);
            }
            else if (JSON_PROPERTY_COMPARE(setting, "col_period_ms", json_double))
            {
                _json_prop_extract_float(&wave_setup->col_period_ms, setting);
                wave_setup->col_radian_ms = 1.0f / (wave_setup->col_period_ms * ONE_OVER_2PI);
            }
            else if (JSON_PROPERTY_COMPARE(setting, "row_offset", json_double))
            {
                _json_prop_extract_float(&wave_setup->row_offset, setting);
                wave_setup->row_offset = PI / wave_setup->row_offset;
            }
            else if (JSON_PROPERTY_COMPARE(setting, "col_offset", json_double))
            {
                _json_prop_extract_float(&wave_setup->col_offset, setting);
                wave_setup->col_offset = PI / wave_setup->col_offset;
            }
            else if (JSON_PROPERTY_COMPARE(setting, "row_amplitude", json_double))
                _json_prop_extract_float(&wave_setup->row_amplitude, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "col_amplitude", json_double))
                _json_prop_extract_float(&wave_setup->col_amplitude, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "sin_offset", json_double))
                _json_prop_extract_float(&wave_setup->sin_offset, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "shadow_plane", json_boolean) && JSON_PROPERTY_VALUE_BOOL(setting))
                wave_setup->shadow_plane = true;
            else if (JSON_PROPERTY_COMPARE(setting, "shadow_plane_z", json_double))
                _json_prop_extract_float(&wave_setup->shadow_plane_z, setting);
            else if (JSON_PROPERTY_COMPARE(setting, "shadow_plane_scaling", json_double))
                _json_prop_extract_float(&wave_setup->shadow_plane_scaling, setting);
        }
    }

    level->model_count = 0;
    level->models = 0;

    u32 total_elems = wave_setup->rows * wave_setup->cols + 1;
    r32 step_x = wave_setup->cube_size * wave_setup->scaling.v3.x + wave_setup->offset;
    r32 step_y = wave_setup->cube_size * wave_setup->scaling.v3.y + wave_setup->offset;

    if (total_elems)
    {
        s32 row_end = -(wave_setup->rows >> 1);
        s32 row_start = row_end + wave_setup->rows - 1;

        s32 col_start = -(wave_setup->cols >> 1);
        s32 col_end = col_start + wave_setup->cols - 1;

        wave_setup->model_count = total_elems;
        wave_setup->models = (gc_model_t *) stack_push(manager->stack, sizeof(gc_model_t) * total_elems);

        GCSR.state->update_params = wave_setup;

        // ----------------------------------------------------------------------------------
        // -- Generate the cubes.
        // ----------------------------------------------------------------------------------

        for (s32 row = row_start; row >= row_end; --row)
        {
            for (s32 col = col_start; col <= col_end; ++col)
            {
                gc_model_t *model = wave_setup->models + index++;

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

                model->object.scaling.v3.x = wave_setup->scaling.v3.x;
                model->object.scaling.v3.y = wave_setup->scaling.v3.y;
                model->object.scaling.v3.z = wave_setup->scaling.v3.z;

                // model->overwrites.uv_scaling.v.v2.x = 1;
                // model->overwrites.uv_scaling.v.v2.y = 1;

                model->flags = MOD_BACKFACE_CULL;

                PUSH_TRIANGLE(model);
            }
        }

        if (wave_setup->shadow_plane)
        {
            gc_model_t *plane = wave_setup->models + total_elems - 1;

            plane->meshes[0] = (mesh_t *) level->meshes[2];
            plane->meshes[1] = 0;
            plane->meshes[2] = 0;

            plane->material = level->materials + 1;
            plane->shader_id = SHADER_NONE;
            // plane->shader = 0;

            plane->object.position.v4.x = 0;
            plane->object.position.v4.y = 0;
            plane->object.position.v4.z = wave_setup->shadow_plane_z;
            plane->object.position.v4.w = 1;

            plane->object.rotation.v3.x = 0;
            plane->object.rotation.v3.y = 0;
            plane->object.rotation.v3.z = 0;

            plane->object.scaling.v3.x = wave_setup->shadow_plane_scaling;
            plane->object.scaling.v3.y = wave_setup->shadow_plane_scaling;
            plane->object.scaling.v3.z = 1.0f;

            // plane->overwrites.uv_scaling.v.v2.x = 1;
            // plane->overwrites.uv_scaling.v.v2.y = 1;

            plane->flags = MOD_BACKFACE_CULL;

            PUSH_TRIANGLE(plane);
        }
    }

    return wave_setup;
}

void _update_wave(r32 delta, void *data)
{
    wave_t *wave_setup = (wave_t *) data;

    if (!wave_setup)
        return;

    u32 index = 0;
    wave_setup->total_delta += delta;

    for (s32 row = 0; row < wave_setup->rows; ++row)
    {
        r32 radian_row = wave_setup->total_delta * wave_setup->row_radian_ms + row * wave_setup->row_offset;
        r32 row_z = (sinf(radian_row) + wave_setup->sin_offset) * wave_setup->row_amplitude;

        for (s32 col = 0; col < wave_setup->cols; ++col)
        {
            gc_model_t *current = wave_setup->models + index++;

            r32 radian_col = wave_setup->total_delta * wave_setup->col_radian_ms + col * wave_setup->col_offset;
            r32 col_z = (sinf(radian_col) + wave_setup->sin_offset) * wave_setup->col_amplitude;

            if (wave_setup->animation == WAVE_ANIMATION_POSITION)
                current->object.position.v3.z = (row_z + col_z) * 0.5f;
            else if (wave_setup->animation == WAVE_ANIMATION_SCALING)
                current->object.scaling.v3.z = (row_z + col_z) * 0.5f;
        }
    }

    // Update the shadow maps.
    for (u32 i = 0; i < wave_setup->level->light_count; ++i)
    {
        gc_light_t *light = wave_setup->level->lights + i;

        if (light->type == GC_SUN_LIGHT || light->type == GC_POINT_LIGHT)
            light->updated = true;
    }
}