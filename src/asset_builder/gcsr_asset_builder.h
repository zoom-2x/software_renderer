// ----------------------------------------------------------------------------------
// -- File: gcsr_asset_builder.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-11-21 15:06:43
// -- Modified: 2022-04-20 21:59:37
// ----------------------------------------------------------------------------------

#ifndef GCSR_ASSET_BUILDER_H
#define GCSR_ASSET_BUILDER_H

// typedef struct gl_asset_bitmap_metadata_t gl_asset_bitmap_metadata_t;
// typedef struct gl_asset_text_metadata_t gl_asset_text_metadata_t;
// typedef struct gl_asset_info_t gl_asset_info_t;
// typedef struct gl_asset_file_header_t gl_asset_file_header_t;
// typedef struct asset_config_source_t asset_config_source_t;
// typedef struct asset_config_category_t asset_config_category_t;
// typedef struct asset_config_data_t asset_config_data_t;

#define GL_MAX_ATTRIBUTE_COUNT 10
#define GL_PIXEL_FORMAT SDL_PIXELFORMAT_RGBA8888

#define GL_PIXEL_FORMAT_RED_SHIFT 24
#define GL_PIXEL_FORMAT_GREEN_SHIFT 16
#define GL_PIXEL_FORMAT_BLUE_SHIFT 8
#define GL_PIXEL_FORMAT_ALPHA_SHIFT 0

#define DEBUG_FOLDER "data/debug/"
#define MESH_NAME_LENGTH 64

#define THREAD_TILE_DIVISION_SHIFT 3
#define ASSET_BUILDER_THREAD_COUNT 4
#define PROGRESS(format, step, total) printf(format, (u32) ((100.0f * (step)) / (total)));

typedef enum
{
    IDLE_STATE,
    END_STATE,
    PBR_IRRADIANCE_STATE,
    PBR_PREFILTERED_STATE
} worker_state_t;

typedef struct
{
    u32 sx;
    u32 sy;
    u32 ex;
    u32 ey;
} thread_tile_t;

typedef struct
{
    cube_texture_t *irradiance;
    cube_texture_t *environment;
    gl_cube_faces_t face;
} thread_irradiance_work_t;

typedef struct
{
    u32 tile_count;
    SDL_atomic_t index;
    thread_tile_t *tiles;

    thread_irradiance_work_t irradiance;
} thread_work_t;

typedef struct
{
    u8 running_count;
    worker_state_t state;

    SDL_mutex *worker_lock;
    SDL_cond *worker_condition;
    SDL_cond *scheduler_condition;

    SDL_Thread *thread_pool[ASSET_BUILDER_THREAD_COUNT];

    thread_work_t work;
} thread_box_t;

#define BUILDER_THREAD_START(worker_state) \
{ \
    SDL_LockMutex(threads.worker_lock); \
    threads.state = worker_state; \
    threads.running_count = ASSET_BUILDER_THREAD_COUNT; \
    SDL_CondBroadcast(threads.worker_condition); \
    SDL_CondWait(threads.scheduler_condition, threads.worker_lock); \
    SDL_UnlockMutex(threads.worker_lock); \
}

thread_box_t threads;
string_buffer_t _string_buffer;

// NOTE(gabic): No output size checking.
char *extract_file_name(char *filepath, char *out)
{
    size_t len = strlen(filepath) - 1;

    char *res = sb_next_buffer(&_string_buffer);
    char *output = res;

    if (out)
    {
        res = out;
        output = out;
    }

    s32 pos = (s32) len;
    s32 start = -1;
    s32 end = -1;

    while (pos > 0)
    {
        if (filepath[pos] == '/')
        {
            start = pos + 1;
            break;
        }

        if (filepath[pos] == '.' && end == -1)
            end = pos;

        pos--;
    }

    if (start >= 0)
    {
        end = end >= 0 ? end : (s32) len;

        for (s32 i = start; i < end; ++i) {
            *output++ = filepath[i];
        }

        *output = '\0';
    }

    return res;
}

// ----------------------------------------------------------------------------------
// -- Bitmaps.
// ----------------------------------------------------------------------------------

// typedef __ALIGN__ struct
// {
//     char name[MESH_NAME_LENGTH];
//     u64 bytes;
//     u64 data_offset;

//     u32 bytes_per_pixel;
//     u32 pitch;
//     u32 block_pitch;
//     u32 width;
//     u32 height;
// } gl_asset_bitmap_metadata_t;

// // ----------------------------------------------------------------------------------
// // -- Models.
// // ----------------------------------------------------------------------------------

// #define USE_MESH_V2 1

// typedef enum
// {
//     GL_ATTR_POS = 1,
//     GL_ATTR_UV = 2,
//     GL_ATTR_NORM = 4,
//     GL_ATTR_COLOR = 8,
//     GL_ATTR_TANGENT = 16,
// } gl_mesh_attribute_type_t;

// typedef struct
// {
//     u64 bytes;
//     u32 count;
//     u32 vertices;

//     u64 data_offset;
// } gl_asset_mesh_metadata_primitives_t;

// typedef struct
// {
//     gl_mesh_attribute_type_t type;

//     u64 bytes;
//     u32 count;
//     u8 components;
//     u32 line_size;

//     u64 data_offset;
// } gl_asset_mesh_metadata_attributes_t;

// typedef __ALIGN__ struct
// {
//     char name[MESH_NAME_LENGTH];
//     u64 bytes;

//     gc_mesh_type_t type;
//     u8 buffer_count;
//     u8 total_components;
//     u8 attr_mask;

//     gl_asset_mesh_metadata_primitives_t primitives;
//     gl_asset_mesh_metadata_attributes_t buffers[GL_MAX_ATTRIBUTE_COUNT];
// } gl_asset_mesh_metadata_t;

// typedef struct
// {
//     char name[64];

//     gc_mesh_type_t type;

//     u32 indices;
//     u32 vertices;

//     u32 indices_offset;
//     u32 vertices_offset;

//     size_t indices_bytes;
//     size_t vertices_bytes;
// } gl_asset_mesh_metadata_v2_t;

// ----------------------------------------------------------------------------------
// -- Text.
// ----------------------------------------------------------------------------------

typedef struct
{
    u32 count;
    u64 bytes;
    u64 data_offset;
} gl_asset_text_metadata_t;

// ----------------------------------------------------------------------------------

// typedef struct
// {
//     u32 count;
//     u64 bytes;
//     u64 metadata_offset;
// } gl_asset_info_t;

// struct gl_asset_file_header_t
// {
//     u32 magic_value;
//     r32 version;

//     u32 total_assets;

//     u64 metadata_offset;
//     u64 data_offset;

//     gl_asset_info_t bitmaps;
//     gl_asset_info_t meshes;
//     gl_asset_info_t fonts;
//     gl_asset_info_t texts;
// };

typedef __ALIGN__ struct
{
    gc_mesh_type_t type;
    char name[MESH_NAME_LENGTH];

    u32 indices;
    u32 vertices;

    size_t index_buffer_bytes;
    size_t vertex_buffer_bytes;

    u32 *index_buffer;
    void *vertex_buffer;
} loaded_asset_mesh_t;

typedef struct
{
    loaded_asset_mesh_t *ptr;
} loaded_mesh_ptr_t;

typedef __ALIGN__ struct
{
    loaded_asset_mesh_t *meshes;
    u16 length;
} loaded_mesh_list_t;

typedef struct
{
    u32 indices;
    u64 bytes;
    u32 *index_buffer;
} generated_asset_mesh_t;

void generate_thread_work(u32 size);
void destroy_thread_work();

#endif