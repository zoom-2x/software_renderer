// ----------------------------------------------------------------------------------
// -- File: gcsr_engine.h
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-06-18 16:06:04
// -- Modified: 2020-10-08 19:42:04
// ----------------------------------------------------------------------------------

#ifndef GCSR_ENGINE_PLATFORM_H
#define GCSR_ENGINE_PLATFORM_H

#define GC_WIN32 1
#define GC_LINUX 2
#define GC_PLATFORM GC_WIN32

// #include <windows.h>
#include "SDL.h"

#include <stdlib.h>
#include "gcsr_types.h"     // engine type definitions
#include "gcsr_debug_tools.h"
#include "gcsr_shared.h"
#include "gcsr_file_interface.h"
#include "gcsr_input.h"     // input info

#define GCSR_MIN_RESOLUTION_WIDTH 256
#define GCSR_MIN_RESOLUTION_HEIGHT 256
#define GCSR_MAX_RESOLUTION_SCALING 4

#define GCSR_SETTING_FILEPATH "data/settings.json"
#define GCSR_ASSETS_FILEPATH "data/assets.pkg"

#define MEM_THREAD_BUFFER_SIZE Megabytes(32)
#define MEM_PERMANENT_SIZE Megabytes(64)
#define MEM_TEMPORARY_SIZE Megabytes(256)
#define MEM_ASSETS_SIZE Megabytes(1024)
#define MEM_PIPELINE_SIZE GC_PIPE_NUM_THREADS * MEM_THREAD_BUFFER_SIZE

typedef struct
{
    u32 width;
    u32 height;
    u32 pixel_depth;
    u32 bytes_per_pixel;
    u32 pitch;
    r32 aspect;

    u32 *video_memory;
} engine_framebuffer_config_t;

typedef struct
{
    u32 res_width;
    u32 res_height;
    u8 res_scaling;
    u32 fps;
    char selected_level[64];
} engine_config_t;

// ---------------------------------------------------------------------------------
// -- Routines set by the platform (Platform API).
// ---------------------------------------------------------------------------------

typedef struct
{
    void (* yield) ();

    // -- Engine operations.

    u64 (* get_wall_clock) ();
    r32 (* get_seconds_elapsed) (u64 start, u64 end);
    r32 (* get_ms_elapsed) (u64 start, u64 end);
    void (* switch_resolution) (engine_framebuffer_config_t *framebuffer);
    void (* update_engine) (engine_config_t *config);

    // -- Memory operations.

    void *(* mem_allocate) (size_t size);
    void (* mem_copy) (void *dest, void *src, size_t dest_size, size_t bytes_to_copy);
    b32 (* mem_free) (void *address);
    void (* mem_clear) (void *address, size_t length);
    void (* mem_set) (void *address, u8 data, size_t);

    // -- Filesystem operations.

    b32 (* file_exists) (char *filepath);
    u64 (* get_last_write_time) (char *filepath);
    u64 (* file_size) (gc_file_t *file);
    void (* open_file) (gc_file_t *file, char *filepath, file_access_t access);
    void (* read_file) (gc_file_t *file, u64 offset, u64 bytes_to_read, void *dest_buffer);
    void (* write_file) (gc_file_t *file, u64 byte_count, void *source);
    void (* close_file) (gc_file_t *file);
    void * (* find_first_file) (char *path, file_attributes_t *attrs);
    b8 (* find_next_file) (void *handle, file_attributes_t *attrs);

    // void (* get_exe_filename) (char *filepath, u32 path_size, char *filename, u32 filename_size);
    // void (* get_cwd) (char *pathname, size_t size);
    // // b32 (* get_file_attributes) (char *filepath, file_attributes_t *attributes);
    // large_number_t (* get_last_write_time) (char *filepath);
    // void (* get_readable_time) (large_number_t time, user_time_t *readtime);
    // b32 (* file_exists) (char *filepath);
    // file_handle_t (* open_file) (char *filepath, file_access_t access);
    // large_number_t (* file_size) (file_handle_t handle);
    // void (* read_from_file) (file_handle_t handle, u64 offset, u64 byte_count, void *buffer);

    // void (* write_to_file) (file_handle_t handle, u32 offset, u32 byte_count, void *source);
    // void (* close_file) (file_handle_t *handle);
} platform_api_t;

// typedef struct
// {
//     b32 is_main_buffer;

//     u8 bytes_per_pixel;
//     u8 pixel_depth;
//     u32 width;
//     u32 height;
//     u32 size;
//     u32 pitch;
//     u32 operation;

//     size_t buffer_size;
//     void *memory;
// } videobuffer_t;

typedef struct
{
    size_t total_bytes;

    size_t permanent_pool_size;
    size_t temporary_pool_size;
    size_t pipeline_pool_size;
    size_t asset_pool_size;

    void *permanent_pool;
    void *temporary_pool;
    void *pipeline_pool;
    void *asset_pool;
} engine_memory_pool_t;

typedef struct
{
    char *windowTitle;

    u32 target_fps;
    u32 sleep_time;

    SDL_Window *sdl_window;
#ifndef GL_USE_SDL_TEXTURE
    SDL_Surface *sdl_window_surface;
    SDL_Surface *backbuffer_surface;
#else
    SDL_Renderer *sdl_renderer;
    SDL_Texture *window_texture;
    engine_framebuffer_config_t *framebuffer;
#endif

    // videobuffer_t buffer;
} engine_window_t;

typedef struct
{
    size_t permanent;
    size_t temporary;
    // size_t pipeline;
    size_t assets;
} engine_memory_config_t;

typedef struct
{
    b32 is_initialized;
    b32 is_running;

    engine_memory_pool_t memory;
    engine_window_t main_window;
    input_bindings_t bindings;

    platform_api_t API;
} engine_core_t;

typedef struct
{
    void (* render_and_update)(r32 step);
} engine_api_t;

typedef engine_api_t *(* get_engine_api_t)(engine_core_t *core);

// TODO(gabic): Trebuie sa ma mai gandesc la asta si la un alt mod de a organiza datele astea.
// Eventual sa am acces la ele si pe partea de engine_core ?

typedef struct
{
    char exe_directory[MAX_STRING_LENGTH];
    char exe_filename[MAX_STRING_LENGTH];

    char platform_library_name[MAX_STRING_LENGTH];
    char platform_library_tmp_name[MAX_STRING_LENGTH];
    char lock_filename[MAX_STRING_LENGTH];

    char game_code_library_path[MAX_STRING_LENGTH];
    char gamecode_library_tmp_path[MAX_STRING_LENGTH];
    char lock_filepath[MAX_STRING_LENGTH];
} engine_file_system_t;

typedef struct
{
    void *handler;      // struct to hold a specific platform dll handler.

    u64 last_write_time;
    engine_file_system_t file_system;

    engine_api_t *API;     // the engine api.
    b8 is_valid;
} engine_code_t;

#endif
