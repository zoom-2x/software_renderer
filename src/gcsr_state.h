// ----------------------------------------------------------------------------------
// -- File: gcsr_state.h
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-10-11 16:33:23
// -- Modified: 2020-10-11 16:33:36
// ----------------------------------------------------------------------------------

#ifndef GCSR_STATE_H
#define GCSR_STATE_H

#define SHADER_PROGRAMS 20
#define UPDATE_ROUTINE_TABLE 20
#define MAX_SCENE_LIGHTS 3

typedef struct gc_level_s gc_level_t;
typedef struct gc_level_program_s gc_level_program_t;
typedef struct gl_pipeline_state_s gl_pipeline_state_t;
typedef struct gc_shader_params_s gc_shader_params_t;
typedef struct gc_shader_s gc_shader_t;

typedef struct
{
    gc_vec_t forward;
    gc_vec_t side;
    gc_vec_t up;
} gc_axis_t;

typedef enum
{
    EULER_XYZ,
    EULER_ZYX,
} rotation_type_t;

typedef struct
{
    rotation_type_t rtype;

    gc_vec_t translation;
    gc_vec_t rotation;
    gc_vec_t scaling;
} transform_t;

typedef struct
{
    u32 count;
    transform_t data[3];
} transform_stack_t;

typedef struct
{
    gc_axis_t axis;

    transform_stack_t transforms;

    // Base object transformation.
    rotation_type_t rtype;
    gc_vec_t position;
    gc_vec_t rotation;
    gc_vec_t scaling;

    r32 t;
} gc_object_t;

typedef struct
{
    b8 disabled;

    mesh_t *meshes[3];
    gc_material_t *material;

    shader_id_t shader_id;
    gc_object_t object;
    pipe_param_input_table_t overwrites;

    u32 flags;
} gc_model_t;

typedef struct
{
    gc_rotation_t rotation;

    r32 h_translation;
    r32 v_translation;

    r32 zoom;
} gc_camera_movement_t;

typedef struct
{
    gc_constant_t type;
    gc_projection_t projection;

    gc_model_t *track_target;

    b8 orbiting_mode;
    b8 manual_orientation;
    vec2i start_point;

    gc_vec_t origin_eye;
    gc_vec_t origin_target;

    gc_vec_t eye;
    gc_vec_t target;

    gc_vec_t forward;
    gc_vec_t side;
    gc_vec_t up;

    gc_vec_t axis_x;
    gc_vec_t axis_y;
    gc_vec_t axis_z;

    gc_camera_movement_t movement;

    gc_rotation_t rotation;
    gc_vec_t translation;
    gc_vec_t eye_zoom;

    gc_vec_t pitch_ref_vector;
    gc_vec_t yaw_ref_vector;

    gc_mat_t pitch_rotation_matrix;
    gc_mat_t yaw_rotation_matrix;
    gc_mat_t roll_rotation_matrix;

    r32 zoom;
    r32 orthographic_zoom;

    r32 dampening;
    r32 h_sens;
    r32 v_sens;
    r32 orbit_h_sens;
    r32 orbit_v_sens;
    r32 wheel_sens;
    r32 speed;
    r32 global_sens;

    r32 t_mult_x;
    r32 t_mult_y;
} gc_camera_t;

// ----------------------------------------------------------------------------------
// -- Lights.
// ----------------------------------------------------------------------------------

typedef struct
{
    // gc_vec_t rotation;
    gc_vec_t direction;
} gc_directional_light_t;

typedef struct
{
    r32 kc;                         // distance attenuation constants.
    r32 kl;
    r32 kq;
} gc_point_light_t;

typedef struct
{
    r32 se;                         // exponent scalar.
    r32 kc;                         // distance attenuation constants.
    r32 kl;
    r32 kq;
    r32 angle;                      // light cone angle.
    r32 cos_theta;
} gc_spot_light_t;

typedef void (* sun_shadow_visibility_t) (texture2d_t *shadow_map, r32 *in_u, r32 *in_v, r32 *compare, r32 *output);
typedef void (* point_shadow_visibility_t) (cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output);

typedef struct
{
    u32 flags;
    // light position multiplier.
    u16 camera_distance;
    // projection frustum limits (left, right, top, bottom)
    u16 camera_fov;
    // projection near plane
    r32 f_near;
    // projection far plane
    r32 f_far;
    r32 f_len_inv;

    r32 radius;
    r32 depth_bias;
    r32 normal_offset;
    r32 vsm_bias;
    r32 light_bleed_reduction;

    sun_shadow_visibility_t sun_shadow_visibility_r;
    point_shadow_visibility_t point_shadow_visibility_r;
} gc_shadow_t;

typedef struct
{
    gc_constant_t type;

    gc_object_t object;
    gc_vec_t color;
    gc_vec_t ambient;

    r32 il;
    b8 updated;

    gc_shadow_t shadow;
    void *shadow_texture;

    union
    {
        gc_directional_light_t directional;
        gc_point_light_t point;
        gc_spot_light_t spot;
    };
} gc_light_t;

struct gc_shader_params_s
{
    gc_constant_t tex_filtering;

    r32 delta;
    r32 total_time_sec;

    r32 f_near;
    r32 f_far;

    gc_vec_t world_camera_position;
    gc_vec_t world_camera_direction;

    // gc_point_sprite_type_t point_type;
    // r32 opacity;

    // gc_vec_t background_color;
    // gc_vec_t solid_color;
    // gc_vec_t ambient_color;

    pipe_param_merged_table_t overwrites;

    u32 light_count;
    gc_light_t *lights;
    gc_light_t *current_light;
    gc_light_t *sun_light;

    u32 shader_flags;
    gc_material_t *material;
    void *textures[SHADER_TEXTURE_SLOTS];
};

// Structure used to hold various common data used in the pipeline.
struct gl_pipeline_state_s
{
    u32 flags;

    gc_shader_t *stack_shader;
    u32 stack_varying_count;

    gc_level_program_t *program;
    gc_shader_t *shader;
    u32 varying_count;

    // r32 forced_opacity;
    // u8 tone_mapping;

    // gc_postprocessing_t *postprocessing;
    pipe_param_input_table_t base;
    gc_shader_params_t params;
};

// typedef struct
// {
//     gc_level_t *level;
// } update_params_t;

typedef void (* vstream_reader_callback_t) (gc_vset_t *buffer, u32 *indices, asset_vertex_t *vertices, u32 offset);
typedef void (* shader_setup_callback_t) (gc_material_t *material);
typedef void (* vertex_shader_callback_t) (gc_vertex_t *raw_vertex, gc_shader_params_t *uniforms);
typedef void (* fragment_shader_callback_t) (gc_fragments_array_t *fragments_array, gc_shader_params_t *uniforms);

struct gc_shader_s
{
    shader_id_t id;
    const char *name;
    u32 varying_count;

    vstream_reader_callback_t read;
    shader_setup_callback_t setup;
    vertex_shader_callback_t vs;
    fragment_shader_callback_t fs;
};

// Mesh data buffers.
typedef struct
{
    u32 *indices;
    asset_vertex_t *vertices;
    u32 count;
    // r32 **attributes;
} gc_mesh_arrays_t;

typedef struct
{
    mesh_t *mesh;

    u8 rows;
    u8 cols;
    r32 step;
    r32 z_axis_height;
    r32 axis_extension;

    gc_vec_t base_color;
    gc_vec_t x_color;
    gc_vec_t y_color;
    gc_vec_t z_color;
} gc_grid_mesh_t;

// NOTE(gabic): Fixed size for now, dynamic array for later.
#define LEVEL_MAX_TEXTURES 256
#define LEVEL_MAX_MESHES 256
#define LEVEL_MAX_ASSETS LEVEL_MAX_TEXTURES + LEVEL_MAX_MESHES

typedef struct
{
    char name[64];

    s8 current_mode_cycle;
    u32 flags;
    u32 shadow_map_size;
    u32 tone_mapping;

    b8 debug_lights;
    b8 debug_grid;

    // gc_vec_t background_color;
    // gc_vec_t solid_color;
    // gc_vec_t ambient_color;

    pipe_param_input_table_t overwrites;
    // gc_postprocessing_t postprocessing;
    gc_material_t default_material;
} gc_level_settings_t;

// typedef struct
// {
//     gc_level_t *level;
//     void *data;
// } gc_program_params_t;

typedef void *(* setup_callback_t) (gc_level_t *level);
typedef void (* update_callback_t) (r32 delta, void *data);
typedef void (* clear_callback_t) (void *data);

struct gc_level_program_s
{
    const char *name;

    setup_callback_t setup;
    update_callback_t update;
    clear_callback_t clear;
};

struct gc_level_s
{
    struct _json_object_entry *program_settings;

    b8 missing_assets;

    shader_id_t shader_id;
    program_id_t program_id;

    gc_level_settings_t settings;

    asset_t *assets[LEVEL_MAX_ASSETS + LEVEL_MAX_ASSETS];
    void *textures[LEVEL_MAX_TEXTURES];
    void *meshes[LEVEL_MAX_MESHES];

    gc_material_t *materials;
    gc_model_t *models;
    gc_light_t *lights;

    u16 asset_count;
    u16 texture_count;
    u16 mesh_count;
    u16 material_count;
    u16 model_count;
    u16 light_count;

    texture2d_t *texture_not_found;
    cube_texture_t *cube_texture_not_found;
};

typedef void (* primitive_frontend_func_t) (u32 thread_id);
typedef gc_constant_t (* primitive_coverage_func_t) (gc_primitive_t *primitive, gc_screen_rect_t *block);
typedef void (* primitive_varyings_setup_func_t) (gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer);
typedef void (* primitive_rasterization_func_t) (thread_batch_memory_t *batch_memory, gc_screen_rect_t *box, u16 fragment_index_start, gc_primitive_t *primitive);
// typedef r32 (* tex_wrap_func_t) (r32 c);
// typedef __m128 (* sse_tex_wrap_func_t) (__m128 c);

typedef struct
{
    u32 vertices;
    primitive_frontend_func_t frontend;
    primitive_coverage_func_t coverage;
    primitive_varyings_setup_func_t varyings_setup;
    primitive_rasterization_func_t rasterization;
    // tex_wrap_func_t tex_wrap;
    // sse_tex_wrap_func_t tex_wrap;
} gl_primitive_interface_t;

typedef __ALIGN__ struct
{
    gc_file_t debug_file;
    gc_file_t memory_map_file;
    gc_camera_t camera;

    u32 primitive_id;
    u32 curr_idx;

    gc_model_t **triangle_queue;
    gc_model_t **line_queue;
    gc_model_t **point_queue;

    b8 program_started;
    void *program_data;

    gc_partition_t partitions[GC_PIPE_NUM_THREADS];
    gc_pipe_array_t *vertices[GC_PIPE_NUM_THREADS];
    gc_pipe_array_t *primitives[GC_PIPE_NUM_THREADS];
    gc_multithreading_t *threads;
    gc_work_bin_queue_t bin_queue;
    gc_mesh_arrays_t current_arrays;
    gl_primitive_interface_t pinterface;

    gl_pipeline_state_t pipeline;

    u32 framebuffer_count;
    gc_framebuffer_t *current_framebuffer;
    gc_framebuffer_t *framebuffers[10];

    gc_mat_t matrices[MATRIX_COUNT];
    gc_matrix_stack_t *matrix_buffer;                   // matrix buffer stack

    mesh_t *debug_light_mesh;
    gc_grid_mesh_t debug_grid_mesh;
    gc_model_t debug_grid;

} gl_state_t;

typedef struct
{
    r32 last_step;
    b32 is_initialized;

    engine_config_t settings;

    memory_manager_t manager;
    gl_state_t GL;
    // gl_program_t programs[SHADER_PROGRAMS];
    // level_update_t level_update_table[UPDATE_ROUTINE_TABLE];

    // setup_callback_t setup_table[SETUP_COUNT];
    // update_callback_t update_table[UPDATE_COUNT];
    gc_level_program_t program_table[PRG_COUNT];
    gc_shader_t shader_table[SHADER_COUNT];

    void *update_params;

    file_handle_t asset_file_handle;
    asset_file_header_t *assets;
    gc_asset_manager_t *asset_manager;

    gc_level_t *level;
} engine_state_t;

typedef struct
{
    engine_state_t *state;
    b8 global_dll_reloaded;
    engine_core_t *core;
    engine_api_t export_api;
    memory_manager_t *memory_manager;
    // fixed_point_setup_t global_fp;
    gc_guard_band_t gb;
    gl_state_t *gl;
    gl_pipeline_state_t *pipeline;
    memory_block_t *debug_block;
} global_vars_t;

__INLINE__ transform_t *model_transform_push(gc_model_t *model, gc_vec_t *position, gc_vec_t *rotation, gc_vec_t *scaling, rotation_type_t rtype);
__INLINE__ transform_t *model_transform_push_exp(gc_model_t *model, r32 x, r32 y, r32 z, r32 rx, r32 ry, r32 rz, r32 sx, r32 sy, r32 sz, rotation_type_t rtype);
void apply_transform(transform_t *transform, gc_vec_t *point);
__INLINE__ void apply_transforms(transform_stack_t *transforms, gc_vec_t *point);

#endif