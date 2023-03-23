// ---------------------------------------------------------------------------------
// -- File: gcsr_engine_core.cpp
// ---------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2020-06-22 14:01:11
// -- Modified: 2022-11-16 19:16:35
// ---------------------------------------------------------------------------------

#include "xmmintrin.h"
#include "emmintrin.h"
#include "immintrin.h"

#include <assert.h>

#include "gcsr_settings.h"
#include "gcsr_macros.h"
#include "gcsr_engine.h"                // main engine header
#include "gcsr_math.h"                  // math library
#include "gcsr_vecmat.h"
#include "gcsr_simd_math.h"

#include "gcsr_memory.h"                // memory management and game state
#include "gcsr_routines.h"
#include "gcsr_data.h"                  // complex data types

#include "gcsr_asset.h"
#include "gcsr_gl.h"                    // graphics library
#include "gcsr_state.h"                 // game state
#include "gcsr_bezier.h"
#include "gcsr_animation.h"

#include "gcsr_bezier.cpp"
#include "gcsr_animation.cpp"

#include "gcsr_common_routines.cpp"
#include "gcsr_memory_manager.cpp"          // memory allocation

#include "gcsr_vertex.cpp"
#include "gcsr_texture.cpp"
#include "gcsr_tilebuffer.cpp"
#include "gcsr_framebuffer.cpp"
#include "gcsr_rasterizer.cpp"          // rasterization routines
#include "gcsr_gl_algorithms.cpp"
#include "gcsr_sprites.cpp"             // various sprites used on screen (eg: 3d point representation)
#include "gcsr_vertex_stream.cpp"       // vertex readers
#include "gcsr_pipeline.cpp"            // pipeline
#include "gcsr_camera.cpp"              // camera processing
#include "gcsr_level.cpp"
#include "gcsr_shadow.cpp"
#include "gcsr_gl.cpp"                  // graphics library
#include "gcsr_asset_loader.cpp"        // asset loading routines

#include "routine_tables/_shader_table.cpp"
#include "routine_tables/_program_table.cpp"

// ----------------------------------------------------------------------------------
// -- Globals.
// ----------------------------------------------------------------------------------

engine_api_t global_export_api;
global_vars_t GCSR;

// ----------------------------------------------------------------------------------

void setup_global_var(engine_state_t *state)
{
    GCSR.state = state;
    GCSR.gl = &state->GL;
    GCSR.pipeline = &state->GL.pipeline;
    GCSR.memory_manager = &state->manager;

    init_shader_table();
    init_program_table();
    // init_setup_table();
    // init_update_table();
}

void load_engine_settings(engine_state_t *state)
{
    gc_file_t settings_file;
    platform_api_t *API = get_platform_api();
    char settings_filepath[] = GCSR_SETTING_FILEPATH;
    API->open_file(&settings_file, settings_filepath, GC_FILE_READ);

    // -- Default settings.

    state->settings.res_width = GCSR_MIN_RESOLUTION_WIDTH;
    state->settings.res_height = GCSR_MIN_RESOLUTION_HEIGHT;
    state->settings.res_scaling = 1;
    state->settings.fps = 0;
    state->settings.selected_level[0] = '\0';

    gc_grid_mesh_t *debug_grid = &state->GL.debug_grid_mesh;

    debug_grid->rows = 10;
    debug_grid->cols = 10;
    debug_grid->z_axis_height = 8;
    debug_grid->step = 1.0f;
    debug_grid->axis_extension = 0.4f;

    debug_grid->base_color.data[0] = 1.0f;
    debug_grid->base_color.data[1] = 1.0f;
    debug_grid->base_color.data[2] = 1.0f;
    debug_grid->base_color.data[3] = 0.02f;

    PRE_MULT_ALPHA(debug_grid->base_color);

    debug_grid->x_color.data[0] = 1.0f;
    debug_grid->x_color.data[1] = 0.0f;
    debug_grid->x_color.data[2] = 0.0f;
    debug_grid->x_color.data[3] = 0.2f;

    PRE_MULT_ALPHA(debug_grid->x_color);

    debug_grid->y_color.data[0] = 0.0f;
    debug_grid->y_color.data[1] = 1.0f;
    debug_grid->y_color.data[2] = 0.0f;
    debug_grid->y_color.data[3] = 0.2f;

    PRE_MULT_ALPHA(debug_grid->y_color);

    debug_grid->z_color.data[0] = 0.0f;
    debug_grid->z_color.data[1] = 0.0f;
    debug_grid->z_color.data[2] = 1.0f;
    debug_grid->z_color.data[3] = 0.2f;

    PRE_MULT_ALPHA(debug_grid->z_color);

    if (settings_file.handle)
    {
        mem_set_chunk(MEMORY_TEMPORARY);
        void *buffer = gc_mem_allocate(settings_file.bytes);
        mem_restore_chunk();
        API->read_file(&settings_file, 0, settings_file.bytes, buffer);

        char error_buf[json_error_max];
        json_value *json = json_parse((json_char *) buffer, settings_file.bytes, error_buf);

        if (json)
        {
            JSON_VALUE_OBJECT_LOOP(json, pi)
            {
                struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(json, pi);

                if (JSON_PROPERTY_COMPARE(prop, "res_width", json_integer))
                    state->settings.res_width = JSON_VALUE_INTEGER(prop->value);
                else if (JSON_PROPERTY_COMPARE(prop, "res_height", json_integer))
                    state->settings.res_height = JSON_VALUE_INTEGER(prop->value);
                else if (JSON_PROPERTY_COMPARE(prop, "res_scaling", json_integer))
                    state->settings.res_scaling = JSON_VALUE_INTEGER(prop->value);
                else if (JSON_PROPERTY_COMPARE(prop, "fps", json_integer))
                    state->settings.fps = JSON_VALUE_INTEGER(prop->value);
                else if (JSON_PROPERTY_COMPARE(prop, "level", json_string))
                    strncpy(state->settings.selected_level, JSON_PROPERTY_VALUE_STRING(prop), 64);
                else if (JSON_PROPERTY_COMPARE(prop, "debug_grid", json_object))
                {
                    JSON_VALUE_OBJECT_LOOP(prop->value, si)
                    {
                        struct _json_object_entry *attr = JSON_OBJECT_PROPERTY(prop->value, si);

                        if (JSON_PROPERTY_COMPARE(attr, "base_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 4)
                        {
                            debug_grid->base_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->base_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->base_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->base_color.data[3] = JSON_ARRAY_VALUE_FLOAT(attr->value, 3);

                            gl_gamma_srgb_to_linear(&debug_grid->base_color);
                            PRE_MULT_ALPHA(debug_grid->base_color);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "base_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 3)
                        {
                            debug_grid->base_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->base_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->base_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->base_color.data[3] = 1.0f;

                            gl_gamma_srgb_to_linear(&debug_grid->base_color);
                            PRE_MULT_ALPHA(debug_grid->base_color);
                        }

                        if (JSON_PROPERTY_COMPARE(attr, "x_axis_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 4)
                        {
                            debug_grid->x_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->x_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->x_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->x_color.data[3] = JSON_ARRAY_VALUE_FLOAT(attr->value, 3);

                            gl_gamma_srgb_to_linear(&debug_grid->x_color);
                            PRE_MULT_ALPHA(debug_grid->x_color);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "x_axis_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 3)
                        {
                            debug_grid->x_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->x_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->x_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->x_color.data[3] = 1.0f;

                            gl_gamma_srgb_to_linear(&debug_grid->x_color);
                            PRE_MULT_ALPHA(debug_grid->x_color);
                        }

                        if (JSON_PROPERTY_COMPARE(attr, "y_axis_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 4)
                        {
                            debug_grid->y_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->y_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->y_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->y_color.data[3] = JSON_ARRAY_VALUE_FLOAT(attr->value, 3);

                            gl_gamma_srgb_to_linear(&debug_grid->y_color);
                            PRE_MULT_ALPHA(debug_grid->y_color);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "y_axis_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 3)
                        {
                            debug_grid->y_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->y_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->y_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->y_color.data[3] = 1.0f;

                            gl_gamma_srgb_to_linear(&debug_grid->y_color);
                            PRE_MULT_ALPHA(debug_grid->y_color);
                        }

                        if (JSON_PROPERTY_COMPARE(attr, "z_axis_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 4)
                        {
                            debug_grid->z_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->z_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->z_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->z_color.data[3] = JSON_ARRAY_VALUE_FLOAT(attr->value, 3);

                            gl_gamma_srgb_to_linear(&debug_grid->z_color);
                            PRE_MULT_ALPHA(debug_grid->z_color);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "z_axis_color", json_array) && JSON_ARRAY_LENGTH(attr->value) == 3)
                        {
                            debug_grid->z_color.data[0] = JSON_ARRAY_VALUE_FLOAT(attr->value, 0);
                            debug_grid->z_color.data[1] = JSON_ARRAY_VALUE_FLOAT(attr->value, 1);
                            debug_grid->z_color.data[2] = JSON_ARRAY_VALUE_FLOAT(attr->value, 2);
                            debug_grid->z_color.data[3] = 1.0f;

                            gl_gamma_srgb_to_linear(&debug_grid->z_color);
                            PRE_MULT_ALPHA(debug_grid->z_color);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "step", json_double)) {
                            debug_grid->step = JSON_PROPERTY_VALUE_DOUBLE(attr);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "z_axis_height", json_double)) {
                            debug_grid->z_axis_height = JSON_PROPERTY_VALUE_DOUBLE(attr);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "rows", json_integer)) {
                            debug_grid->rows = JSON_PROPERTY_VALUE_INTEGER(attr);
                        }

                        else if (JSON_PROPERTY_COMPARE(attr, "cols", json_integer)) {
                            debug_grid->cols = JSON_PROPERTY_VALUE_INTEGER(attr);
                        }
                    }
                }
            }

            // Validate the settings.
            if (state->settings.res_width < GCSR_MIN_RESOLUTION_WIDTH)
                state->settings.res_width = GCSR_MIN_RESOLUTION_WIDTH;

            if (state->settings.res_height < GCSR_MIN_RESOLUTION_HEIGHT)
                state->settings.res_height = GCSR_MIN_RESOLUTION_HEIGHT;

            if (state->settings.res_scaling > GCSR_MAX_RESOLUTION_SCALING)
                state->settings.res_scaling = GCSR_MAX_RESOLUTION_SCALING;

            json_value_free(json);
        }
        else
            printf("[ERROR] {%s} %s\n", GCSR_SETTING_FILEPATH, error_buf);

        gc_mem_free(buffer);
    }

    API->close_file(&settings_file);
}

engine_state_t *initialize_state()
{
    engine_memory_pool_t *memory = get_engine_memory_pool();

    // NOTE(gabic): Sa initializez mai intai memoria si abia pe urma sa aloc state-ul ?

    // The engine_state_t will be the first thing in the permanent memory pool.
    engine_state_t *state = (engine_state_t *) memory->permanent_pool;

    // Initialize once.
    if (!state->is_initialized)
    {
        platform_api_t *API = get_platform_api();
        memory_manager_t *manager = &state->manager;

        setup_global_var(state);

        gc_mem_initialize_memory_manager(memory);
        manager->stack = create_stack(Megabytes(1));

        load_engine_settings(state);
        API->update_engine(&GCSR.state->settings);
        gl_initialize();

        state->is_initialized = true;
    }

    if (GCSR.global_dll_reloaded)
    {
        setup_global_var(state);
        GCSR.global_dll_reloaded = false;
    }

    return state;
}

void _debug_camera()
{
    printf("-----------------------------------------------------------\n");
    printf("-- CAMERA STATE\n");
    printf("-----------------------------------------------------------\n\n");

    printf("CAMERA: eye{%ff, %ff, %ff}\n",
            GCSR.gl->camera.eye.data[0],
            GCSR.gl->camera.eye.data[1],
            GCSR.gl->camera.eye.data[2]);

    printf("CAMERA: target{%ff, %ff, %ff}\n",
            GCSR.gl->camera.target.data[0],
            GCSR.gl->camera.target.data[1],
            GCSR.gl->camera.target.data[2]);

    printf("CAMERA: rotation{%ff, %ff, %ff}\n\n",
            GCSR.gl->camera.rotation.pitch,
            GCSR.gl->camera.rotation.yaw,
            GCSR.gl->camera.rotation.roll);

    // fflush(GCSR.gl->debug_file.f);
}

#define DEBUG_MEM_HEADER(debug_file) fprintf(debug_file, "+----------------------------------------------------------------------------------------------+\n");
#define DEBUG_MEM_STR_PAD(debug_file, prefix, format, val) \
    str1[0] = '\0'; \
    sprintf(str2, format, val); \
    strcat(str1, prefix); \
    strcat(str1, str2); \
    fprintf(debug_file, "%-95s|\n", str1)

void _debug_memory_chunk(FILE *debug_file, memory_chunk_t *chunk, const char *chunk_name)
{
    char str1[255];
    char str2[255];

    DEBUG_MEM_HEADER(debug_file);
    DEBUG_MEM_STR_PAD(debug_file, "| ", "%s", chunk_name);
    DEBUG_MEM_HEADER(debug_file);
    DEBUG_MEM_STR_PAD(debug_file, "| Address: ", "0x%p", chunk);
    DEBUG_MEM_STR_PAD(debug_file, "| Overhead: ", "%llu", chunk->overhead);
    DEBUG_MEM_STR_PAD(debug_file, "| Total bytes: ", "%llu", chunk->total_bytes);
    DEBUG_MEM_STR_PAD(debug_file, "| Allocated bytes: ", "%llu", chunk->allocated_bytes);
    DEBUG_MEM_STR_PAD(debug_file, "| Free bytes: ", "%llu", chunk->total_bytes - chunk->allocated_bytes);
    DEBUG_MEM_STR_PAD(debug_file, "| Fill rate: ", "%.4f%%", (r32) chunk->allocated_bytes / chunk->total_bytes);
    DEBUG_MEM_STR_PAD(debug_file, "| Blocks: ", "%u", chunk->block_count);
    DEBUG_MEM_STR_PAD(debug_file, "| Empty: ", "%u", chunk->empty_blocks);
    DEBUG_MEM_STR_PAD(debug_file, "| Filled: ", "%u", chunk->filled_blocks);
    DEBUG_MEM_HEADER(debug_file);

    memory_block_t *current = chunk->Head;

    while (current)
    {
        DEBUG_MEM_STR_PAD(debug_file, "| Label: ", "%s", current->label);

        if (current->description) {
            DEBUG_MEM_STR_PAD(debug_file, "| Description: ", "%s", current->description);
        }

        DEBUG_MEM_STR_PAD(debug_file, "| Address: ", "0x%p", current);

        if (current->status == 0) {
            DEBUG_MEM_STR_PAD(debug_file, "| Status: ", "%s", "BLOCK_FREE");
        }
        else if (current->status == 1) {
            DEBUG_MEM_STR_PAD(debug_file, "| Status: ", "%s", "BLOCK_STATIC");
        }
        else if (current->status == 2) {
            DEBUG_MEM_STR_PAD(debug_file, "| Status: ", "%s", "BLOCK_TEMP");
        }
        else if (current->status == 3) {
            DEBUG_MEM_STR_PAD(debug_file, "| Status: ", "%s", "BLOCK_RESERVED");
        }
        else if (current->status == 4) {
            DEBUG_MEM_STR_PAD(debug_file, "| Status: ", "%s", "BLOCK_USED");
        }
        else if (current->status == 5) {
            DEBUG_MEM_STR_PAD(debug_file, "| Status: ", "%s", "BLOCK_EXTENSION");
        }

        DEBUG_MEM_STR_PAD(debug_file, "| Bytes: ", "%llu", current->bytes);
        DEBUG_MEM_HEADER(debug_file);

        current = current->Next;
    }

    fprintf(debug_file, "\n");
}

void _debug_memory()
{
    memory_manager_t *manager = &GCSR.state->manager;
    memory_chunk_t *permanent_chunk = manager->chunks + MEMORY_PERMANENT;
    memory_chunk_t *temporary_chunk = manager->chunks + MEMORY_TEMPORARY;
    memory_chunk_t *asset_chunk = manager->chunks + MEMORY_ASSETS;

    FILE *debug_file = fopen("debug_memory.txt", "w");

    if (debug_file)
    {
        _debug_memory_chunk(debug_file, permanent_chunk, "Permanent memory chunk");
        _debug_memory_chunk(debug_file, temporary_chunk, "Temporary memory chunk");
        _debug_memory_chunk(debug_file, asset_chunk, "Asset memory chunk");

        fclose(debug_file);
    }
}

void init_bindings()
{
    OPTICK_EVENT("init_bindings");

    input_bindings_t *bindings = get_input_bindings();

    if (bindings->is_init)
        return;

    if (bindings)
    {
        binding_t *input = 0;

        // -- Regular bindings.

        input = &bindings->actions[MOVE_UP];
        input->primary_keycode = SDLK_w;

        input = &bindings->actions[MOVE_DOWN];
        input->primary_keycode = SDLK_s;

        input = &bindings->actions[MOVE_LEFT];
        input->primary_keycode = SDLK_a;

        input = &bindings->actions[MOVE_RIGHT];
        input->primary_keycode = SDLK_d;

        // -- Directional light rotation.

        input = &bindings->actions[LIGHT_ROTATE_RIGHT];
        input->primary_keycode = SDLK_RIGHT;

        input = &bindings->actions[LIGHT_ROTATE_LEFT];
        input->primary_keycode = SDLK_LEFT;

        input = &bindings->actions[LIGHT_ROTATE_UP];
        input->primary_keycode = SDLK_UP;

        input = &bindings->actions[LIGHT_ROTATE_DOWN];
        input->primary_keycode = SDLK_DOWN;

        // -- Camera bindings.

        input = &bindings->actions[MOVE_CAMERA_FORWARD];
        input->primary_keycode = SDLK_w;

        input = &bindings->actions[MOVE_CAMERA_BACKWARD];
        input->primary_keycode = SDLK_s;

        input = &bindings->actions[MOVE_CAMERA_LEFT];
        input->primary_keycode = SDLK_a;

        input = &bindings->actions[MOVE_CAMERA_RIGHT];
        input->primary_keycode = SDLK_d;

        input = &bindings->actions[MOVE_CAMERA_UP];
        input->primary_keycode = SDLK_z;

        input = &bindings->actions[MOVE_CAMERA_DOWN];
        input->primary_keycode = SDLK_c;

        input = &bindings->actions[ROTATE_CAMERA_LEFT];
        input->primary_keycode = SDLK_q;

        input = &bindings->actions[ROTATE_CAMERA_RIGHT];
        input->primary_keycode = SDLK_e;

        input = &bindings->actions[CAMERA_FOCUS_LIMIT_HORIZONTAL];
        input->primary_keycode = SDLK_x;

        input = &bindings->actions[CAMERA_FOCUS_LIMIT_VERTICAL];
        input->primary_keycode = SDLK_y;

        input = &bindings->actions[CAMERA_CENTER_ORIGIN];
        input->primary_keycode = SDLK_RETURN;

        input = &bindings->actions[CHANGE_RENDER_MODE];
        input->primary_keycode = SDLK_F1;

        input = &bindings->actions[DEBUG_OUTPUT_MODE];
        input->primary_keycode = SDLK_F9;

        bindings->is_init = true;
    }
}

void process_input(r32 delta)
{
    OPTICK_EVENT("process_input");

    input_bindings_t *bindings = get_input_bindings();

    if (GCSR.state->level)
    {
        // 180 degrees per second => degrees per ms
        // per 60 frames => 180 / 60 = 3
        r32 light_rotation_speed = 3 * delta * ONE_OVER_TARGET_FRAME_TIME;

        // First light in the list should be a directional light.
        gc_light_t *light = GCSR.state->level->lights;

        if (light && light->type == GC_SUN_LIGHT)
        {
            if (bindings->actions[LIGHT_ROTATE_RIGHT].is_down)
            {
                light->updated = true;
                light->object.rotation.v3.z += light_rotation_speed;
            }

            if (bindings->actions[LIGHT_ROTATE_LEFT].is_down)
            {
                light->updated = true;
                light->object.rotation.v3.z -= light_rotation_speed;
            }

            if (bindings->actions[LIGHT_ROTATE_UP].is_down)
            {
                light->updated = true;
                light->object.rotation.v3.y += light_rotation_speed;
            }

            if (bindings->actions[LIGHT_ROTATE_DOWN].is_down)
            {
                light->updated = true;
                light->object.rotation.v3.y -= light_rotation_speed;
            }

            // light->directional.rotation.v3.x = clamp(-89, 89, light->directional.rotation.v3.x);
            light->object.rotation.v3.z = wrap(0, 360, light->object.rotation.v3.z);
        }

        if (bindings->actions[CAMERA_CENTER_ORIGIN].is_down)
        {
            camera_reset(&GCSR.gl->camera);

            bindings->actions[CAMERA_CENTER_ORIGIN].is_down = false;
            bindings->actions[CAMERA_CENTER_ORIGIN].is_up = true;
        }

        u32 modes[] = {
            GC_MODE_RENDERED | GC_MODE_NORMAL,
            GC_MODE_RENDERED | GC_MODE_NORMAL | GC_MODE_WIREFRAME,
            GC_MODE_WIREFRAME,
            GC_MODE_NORMAL | GC_MODE_SOLID,
            GC_MODE_NORMAL | GC_MODE_MATERIAL
        };

        const char *modes_name[] = {
            "rendered normal",
            "rendered normal wireframe",
            "wireframe",
            "solid",
            "material"
        };

        if (bindings->actions[CHANGE_RENDER_MODE].is_down)
        {
            if (GCSR.state->level->settings.current_mode_cycle < 0)
                GCSR.state->level->settings.current_mode_cycle = 0;
            else
            {
                GCSR.state->level->settings.current_mode_cycle++;
                GCSR.state->level->settings.current_mode_cycle %= 5;
            }

            printf("[RENDER MODE]: %s\n", modes_name[GCSR.state->level->settings.current_mode_cycle]);

            GCSR.state->level->settings.flags &= ~(GC_MODE_NORMAL | GC_MODE_RENDERED | GC_MODE_WIREFRAME | GC_MODE_SOLID | GC_MODE_MATERIAL);
            GCSR.state->level->settings.flags |= modes[GCSR.state->level->settings.current_mode_cycle];

            bindings->actions[CHANGE_RENDER_MODE].is_down = false;
            bindings->actions[CHANGE_RENDER_MODE].is_up = true;
        }

        if (bindings->actions[DEBUG_OUTPUT_MODE].is_down)
        {
            _debug_camera();
            _debug_memory();

            bindings->actions[DEBUG_OUTPUT_MODE].is_down = false;
            bindings->actions[DEBUG_OUTPUT_MODE].is_up = true;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Main loop routine.
// ----------------------------------------------------------------------------------

void render_and_update(r32 step)
{
    OPTICK_EVENT("render_and_update");

    engine_state_t *state = initialize_state();

    // ----------------------------------------------------------------------------------
    // -- Base framebuffer creation and resolution setup.
    // ----------------------------------------------------------------------------------

    if (!GCSR.gl->framebuffers[0])
        gc_change_resolution(state->settings.res_width, state->settings.res_height, state->settings.res_scaling);

    // ----------------------------------------------------------------------------------
    // -- Load the level and initialize some debug data.
    // ----------------------------------------------------------------------------------

    if (!state->level)
    {
        asset_load_level(state->settings.selected_level);

        if (state->level->settings.debug_grid)
            init_debug_grid();

        if (state->level->settings.debug_lights)
            init_debug_light_mesh();
    }

    // ----------------------------------------------------------------------------------

    GCSR.gl->pipeline.params.delta = step - state->last_step;
    GCSR.gl->pipeline.params.total_time_sec += GCSR.gl->pipeline.params.delta * 0.001f;
    state->last_step = step;

#if MODE_NO_RENDERING || ENGINE_TEST_MODE
        global_core->is_running = false;
        return;
#else
    init_bindings();

    // ----------------------------------------------------------------------------------
    // -- Level rendering.
    // ----------------------------------------------------------------------------------

    if (state->level)
    {
        process_input(GCSR.gl->pipeline.params.delta);
        process_camera_input(&GCSR.gl->camera);

        if (state->level->program_id)
        {
            gc_level_program_t *program = GET_PROGRAM(state->level->program_id);

            if (!GCSR.gl->program_started && program->setup)
            {
                GCSR.gl->program_data = program->setup(state->level);
                GCSR.gl->program_started = true;
            }

            if (program->update)
                program->update(GCSR.gl->pipeline.params.delta, GCSR.gl->program_data);
        }

        gl_render_3d(state->level);

        // The current framebuffer needs to be copied to the main framebuffer.
        if (GCSR.gl->current_framebuffer->flags & FB_FLAG_COPY)
            gc_copy_framebuffer(GCSR.gl->current_framebuffer);
    }
#endif
}

// ----------------------------------------------------------------------------------
// -- Export routine.
// ----------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

engine_api_t *get_engine_api(engine_core_t *output_core)
{
    GCSR.core = output_core;

    // #ifdef GC_DEBUG_MODE
    // __DebugApi__ = &GCSR.core->API.DebugApi;
    // #endif

    global_export_api.render_and_update = render_and_update;
    GCSR.global_dll_reloaded = true;

    return &global_export_api;
}

#ifdef __cplusplus
}
#endif