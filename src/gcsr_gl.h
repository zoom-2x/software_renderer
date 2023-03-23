// ----------------------------------------------------------------------------------
// -- File: gcsr_gl.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2020-10-08 18:32:04
// -- Modified: 2022-11-16 19:15:44
// ----------------------------------------------------------------------------------

#ifndef GCSR_GL_H
#define GCSR_GL_H

typedef struct gc_primitive_s gc_primitive_t;
typedef struct gc_bin_s gc_bin_t;
typedef struct gl_worker gl_worker_t;
typedef struct gc_framebuffer_s gc_framebuffer_t;

typedef enum
{
    GL_RASTER_STATE_IDLE,
    GL_RASTER_STATE_FRONTEND,
    GL_RASTER_STATE_BACKEND,
    GL_RASTER_STATE_SCALING,
    GL_RASTER_STATE_TRANSPARENCY,
    GL_RASTER_STATE_LSB_TO_TEXTURE,
    GL_RASTER_STATE_CLEAR_LSB
} gc_worker_state_t;

// ----------------------------------------------------------------------------------

typedef enum
{
    GC_NONE,

    GC_RENDER_OBJECT_2D,
    GC_RENDER_OBJECT_3D,

    GC_PROJECTION_2D,
    GC_PROJECTION_PERSPECTIVE,
    GC_PROJECTION_ORTHOGRAPHIC,
    GC_PROJECTION_OBLIQUE,

    GC_CAMERA_BASIC,
    GC_CAMERA_FOCUS_POINT,

    // GC_WINDING_CW,
    // GC_WINDING_CCW,
    // GL_WINDING_BOTH,

    GC_MATERIAL_BLINN,
    GC_MATERIAL_PBR,
    GC_MATERIAL_SKYBOX,

    GC_SUN_LIGHT,
    GC_POINT_LIGHT,
    GC_SPOT_LIGHT,

    GC_COVERAGE_NONE,
    GC_COVERAGE_PARTIAL,
    GC_COVERAGE_TOTAL,

    GC_CONSTANT_COUNT
} gc_constant_t;

typedef enum
{
    CLIP_INSIDE = 0,
    CLIP_LEFT = 1,
    CLIP_RIGHT = 2,
    CLIP_BOTTOM = 4,
    CLIP_TOP = 8,
    CLIP_FAR = 16,
    CLIP_NEAR = 32
} gl_clip_codes_t;

typedef enum
{
    GL2D_SHAPE_OUTLINE,
    GL2D_SHAPE_COLOR,
    GL2D_SHAPE_FONT,
    GL2D_SHAPE_BITMAP
} gl_operation_2d_t;

typedef enum
{
    GL_SPRITE_POINT_1x1 = 1,
    GL_SPRITE_POINT_3x3,
    GL_SPRITE_POINT_5x5,
    GL_SPRITE_POINT_CIRCLE,
    GL_SPRITE_POINT_SQUARE,
    GL_SPRITE_POINT_CROSS,
    GL_SPRITE_POINT_TRIANGLE,

    GL_SPRITE_COUNT
} gc_point_sprite_type_t;

typedef enum
{
    M_MODEL_WORLD,
    M_VIEW_WORLD,
    M_VIEW,
    M_MODEL_VIEW,
    M_PROJECTION,
    M_VIEWPORT,
    M_VIEWPORT_INV,
    M_MODEL_VIEW_PROJECTION,
    M_NORMAL,
    M_NORMAL_WORLD,
    M_SUN_LIGHT,

    MATRIX_COUNT
} gl_matrix_t;

// ----------------------------------------------------------------------------------
// -- Global flags.
// ----------------------------------------------------------------------------------

#define GC_MODE_NORMAL 1
#define GC_MODE_POINT 2
#define GC_MODE_WIREFRAME 4
#define GC_MODE_SOLID 8
#define GC_MODE_MATERIAL 16
#define GC_MODE_RENDERED 32
#define GC_DEPTH_TEST 64
#define GC_MIP 128
#define GC_MIP_FILTER 256
#define GC_BACKFACE_CULL 512
#define GC_FORCE_DEPTH_ONE 1024
#define GC_TRANSPARENCY 2048
#define GC_SHADOW 4096
#define GC_POST_PROCESSING 8192
#define GC_SHADOW_PASS 16384
#define GC_WINDING_CCW 32768
#define GC_WINDING_CW 65536

#define GC_MESH_WIREFRAME_COLOR 131072
#define GC_MESH_POINT_COLOR 262144
#define GC_MESH_OPACITY 524288

#define GC_LEVEL_ALLOWED_FLAGS (GC_MODE_NORMAL | GC_MODE_WIREFRAME | GC_MODE_POINT | GC_MODE_SOLID | GC_MODE_MATERIAL | GC_MODE_RENDERED | GC_TRANSPARENCY | GC_SHADOW | GC_POST_PROCESSING | GC_MIP | GC_MIP_FILTER)

#define FB_FLAG_COPY 1
#define FB_FLAG_LSB 2
#define FB_FLAG_TRANSPARENCY 4

// ----------------------------------------------------------------------------------
// -- Shader param flags.
// ----------------------------------------------------------------------------------

#define SHADER_FLAG_SHADOW 1
#define SHADER_FLAG_BLINN_DIFFUSE 2
#define SHADER_FLAG_BLINN_NORMAL 4
#define SHADER_FLAG_BLINN_SPECULAR 8
#define SHADER_FLAG_BLINN_EMISSION 16
#define SHADER_FLAG_BLINN_FRESNEL 32
#define SHADER_FLAG_BLINN_AO 64
#define SHADER_FLAG_BLINN_RR 128
#define SHADER_FLAG_BLINN_REFLECTION 256
#define SHADER_FLAG_BLINN_REFRACTION 512

#define SHADER_FLAG_PBR_ALBEDO 2
#define SHADER_FLAG_PBR_NORMAL 4
#define SHADER_FLAG_PBR_METALNESS 8
#define SHADER_FLAG_PBR_ROUGHNESS 16
#define SHADER_FLAG_PBR_AO 32
#define SHADER_FLAG_PBR_EMISSION 64
#define SHADER_FLAG_PBR_AMBIENT 128
#define SHADER_FLAG_PBR_METALNESS_ROUGHNESS 256
#define SHADER_FLAG_PBR_AO_ROUGHNESS_METALNESS 512
#define SHADER_FLAG_PBR_UNLIT 1024

#define SHADER_TEXTURE_SLOTS 10

// ----------------------------------------------------------------------------------
// -- Model flags.
// ----------------------------------------------------------------------------------

#define MOD_BACKFACE_CULL 1
#define MOD_EXCLUDE_FROM_SHADOW 2

typedef enum
{
    SHADER_NONE = 0,
    // Wireframe.
    SHADER_WIREFRAME = 1,
    SHADER_POINT = 2,

    // Meshes.
    SHADER_FLAT_COLOR = 3,
    SHADER_TEXTURE = 4,
    SHADER_BLINN_PHONG = 5,
    SHADER_PBR = 6,
    SHADER_CARTOON = 7,
    SHADER_DEPTH_BUFFER = 8,

    SHADER_SHADOW = 9,
    SHADER_POINT_SHADOW = 10,

    SHADER_REFLECTION_REFRACTION = 11,
    SHADER_SKYBOX = 12,
    SHADER_SKYBOX_PBR = 13,

    SHADER_DEBUG_LIGHT = 14,
    SHADER_DEBUG_GRID = 15,

    SHADER_COUNT
} shader_id_t;

typedef enum
{
    PRG_NONE = 0,
    PRG_ROTATION_AROUND_Z,
    PRG_WAVE,
    PRG_TEXT_PANEL,
    PRG_TEXT_PANEL_EXTRUDE,
    PRG_BEZIER_CURVE,
    PRG_CAMERA_ROTATION,

    PRG_COUNT
} program_id_t;

typedef enum
{
    SETUP_NONE = 0,
    SETUP_ROTATION_AROUND_Z = 1,
    SETUP_WAVE = 2,
    SETUP_TEXT_PANEL = 3,
    SETUP_TEXT_PANEL_EXTRUDE = 4,
    SETUP_BEZIER = 5,
    SETUP_CAMERA_ROTATION = 6,
    SETUP_COUNT
} setup_id_t;

typedef enum
{
    UPDATE_NONE = 0,
    UPDATE_ROTATION_AROUND_Z = 1,
    UPDATE_WAVE = 2,
    UPDATE_TEXT_PANEL = 3,
    UPDATE_TEXT_PANEL_EXTRUDE = 4,
    UPDATE_BEZIER = 5,
    UPDATE_CAMERA_ROTATION = 6,
    UPDATE_COUNT
} update_id_t;

typedef enum
{
    GL_SET_DISCARDED = 1,
    GL_SET_BACKFACE = 2,
    GL_SET_FROM_GB_CLIP = 4,
} gc_vset_flag_t;

// ----------------------------------------------------------------------------------

typedef __ALIGN__ struct
{
    r32 data[VERTEX_ATTRIBUTE_SIZE];
    r32 pos[4];
} gc_vertex_t;

typedef struct
{
    gc_constant_t type;

    // debugging
    s8 near_dir;
    r32 near_speed;
    r32 near_threshold;
    r32 fov;

    union
    {
        struct
        {
            r32 f_near;
            r32 f_far;
        } perspective;

        struct
        {
            r32 f_top;
            r32 f_bottom;
            r32 f_right;
            r32 f_left;
            r32 f_near;
            r32 f_far;
        } orthographic;
    };
} gc_projection_t;

typedef struct
{
    texture2d_t *diffuse_map;
    texture2d_t *emission_map;
    texture2d_t *specular_map;
    texture2d_t *normal_map;
    texture2d_t *ao_map;
    texture2d_t *rr_map;
    cube_texture_t *cubemap;

    r32 specular;
    r32 shininess;
    r32 min_fresnel;
    r32 max_fresnel;
    r32 tau;

    gc_vec_t diffuse_multiplier;

    // Reflection, refraction.
    r32 rr_diffuse_ratio;
    r32 rr_ratio;
    r32 refr_idx1;
    r32 refr_idx2;
    r32 refr_ratio;

    gc_vec_t diffuse;
    gc_vec_t ambient;
} gc_blinn_material_t;

typedef struct
{
    gc_vec_t albedo;
    r32 metalness;
    r32 roughness;
    r32 ao;
    r32 specular;
} pbr_multipliers_t;

typedef struct
{
    texture2d_t *albedo_map;
    texture2d_t *normal_map;
    texture2d_t *metalness_map;
    texture2d_t *roughness_map;
    texture2d_t *ao_map;
    texture2d_t *arm_map;   // ao + roughness + metallnes
    texture2d_t *emission_map;
    pbr_ambient_texture_t *ambient_map;

    // Used only when a texture map is present.
    pbr_multipliers_t multipliers;

    gc_vec_t albedo;
    gc_vec_t ambient;

    r32 metalness;
    r32 roughness;
    r32 ao;
    gc_vec_t f0;
} gc_pbr_material_t;

typedef struct
{
    cube_texture_t *input;
} gc_skybox_material_t;

typedef struct
{
    gc_constant_t type;

    b8 transparency;
    r32 opacity;
    u32 components;

    union {
        gc_blinn_material_t blinn;
        gc_pbr_material_t pbr;
        gc_skybox_material_t skybox;
    };
} gc_material_t;

// Pipeline settings that can be overridden.
// Order: base < level < model

#define PARAM_BASE 0
#define PARAM_LEVEL 1
#define PARAM_MODEL 2

typedef __ALIGN__ union
{
    gc_vec_t u_vector;
    r32 u_float;
    s32 u_integer;
    u8 u_byte;
    b8 u_bool;
} overwrite_val_t;

#define PIPE_PARAM_VALUE(lvl, name, type) (GCSR.gl->pipeline.params.overwrites.name[lvl]->type)
#define PIPE_PARAM_VALUE_FLOAT(lvl, name) (GCSR.gl->pipeline.params.overwrites.name[lvl]->u_float)
#define PIPE_PARAM_VALUE_INTEGER(lvl, name) (GCSR.gl->pipeline.params.overwrites.name[lvl]->u_integer)
#define PIPE_PARAM_VECTOR(lvl, name, index) (GCSR.gl->pipeline.params.overwrites.name[lvl]->u_vector.data[index])

#define MODEL_OVERWRITE_VECTOR_SET(model, name, x, y, z, w) \
    (model)->overwrites.name.overwrite = true; \
    (model)->overwrites.name.value.u_vector.data[0] = x; \
    (model)->overwrites.name.value.u_vector.data[1] = y; \
    (model)->overwrites.name.value.u_vector.data[2] = z; \
    (model)->overwrites.name.value.u_vector.data[3] = w

#define PIPE_PARAM_COPY(from_lvl, to_lvl) \
    overwrites->tone_mapping[to_lvl] = overwrites->tone_mapping[from_lvl]; \
    overwrites->postprocessing[to_lvl] = overwrites->postprocessing[from_lvl]; \
    overwrites->saturation[to_lvl] = overwrites->saturation[from_lvl]; \
    overwrites->forced_opacity[to_lvl] = overwrites->forced_opacity[from_lvl]; \
    overwrites->tint_color[to_lvl] = overwrites->tint_color[from_lvl]; \
    overwrites->solid_color[to_lvl] = overwrites->solid_color[from_lvl]; \
    overwrites->ambient_color[to_lvl] = overwrites->ambient_color[from_lvl]; \
    overwrites->background_color[to_lvl] = overwrites->background_color[from_lvl]; \
    overwrites->wireframe_color[to_lvl] = overwrites->wireframe_color[from_lvl]; \
    overwrites->point_color[to_lvl] = overwrites->point_color[from_lvl]; \
    overwrites->uv_scaling[to_lvl] = overwrites->uv_scaling[from_lvl]; \
    overwrites->point_radius[to_lvl] = overwrites->point_radius[from_lvl]

typedef struct
{
    overwrite_val_t value;
    u8 overwrite;
} pipe_param_t;

typedef __ALIGN__ struct
{
    pipe_param_t tone_mapping;

    pipe_param_t postprocessing;
    pipe_param_t saturation;
    pipe_param_t tint_color;

    pipe_param_t solid_color;
    pipe_param_t ambient_color;
    pipe_param_t background_color;

    pipe_param_t wireframe_color;
    pipe_param_t point_color;
    pipe_param_t uv_scaling;
    pipe_param_t forced_opacity;

    pipe_param_t point_radius;
} pipe_param_input_table_t;

typedef struct
{
    overwrite_val_t *tone_mapping[3];

    overwrite_val_t *postprocessing[3];
    overwrite_val_t *saturation[3];
    overwrite_val_t *tint_color[3];

    overwrite_val_t *solid_color[3];
    overwrite_val_t *ambient_color[3];
    overwrite_val_t *background_color[3];

    overwrite_val_t *wireframe_color[3];
    overwrite_val_t *point_color[3];
    overwrite_val_t *uv_scaling[3];
    overwrite_val_t *forced_opacity[3];

    overwrite_val_t *point_radius[3];
} pipe_param_merged_table_t;

// ----------------------------------------------------------------------------------
// -- GL state.
// ----------------------------------------------------------------------------------

typedef __ALIGN__ struct
{
    u32 count;
    gc_mat_t *data;
} gc_matrix_stack_t;

#define MAX_TRANSPARENCY_STACK 4

typedef __ALIGN__ struct
{
    r32 stack_r[MAX_TRANSPARENCY_STACK][GC_FRAG_SIZE];
    r32 stack_g[MAX_TRANSPARENCY_STACK][GC_FRAG_SIZE];
    r32 stack_b[MAX_TRANSPARENCY_STACK][GC_FRAG_SIZE];
    r32 stack_a[MAX_TRANSPARENCY_STACK][GC_FRAG_SIZE];

    r32 z[MAX_TRANSPARENCY_STACK][GC_FRAG_SIZE];
    u8 count[GC_FRAG_SIZE];
} gc_transparency_frag_t;

typedef __ALIGN__ struct
{
    gc_transparency_frag_t frags[GL_BIN_FRAGS];

    u16 index;
    u16 x;
    u16 y;

    b8 dirty;
} gc_transparency_bin_t;

typedef enum
{
    SPW_LINE,
    SPW_DOTTED,
    SPW_WAVE,
    SPW_NOFILTER,
} gc_single_pass_wireframe_type_t;

typedef struct
{
    gc_single_pass_wireframe_type_t type;
    b8 no_filter;
    vec4 color;
    // Maximum distance at which the pixels are tested with de edge.
    r32 d_max;
    // Scaling of the attenuation function.
    r32 scaling;
    // Oscilation interval / dotted line.
    r32 n;
    // Oscilation offset.
    r32 offset;

    r32 wave_scaling;
    r32 wave_offset_x;
    r32 wave_offset_y;
} gc_single_pass_wireframe_t;

typedef __ALIGN__ struct
{
    // The relative position of the real point inside the sprite square.
    u8 offx;
    u8 offy;

    // The point sprite is 8x8 pixels.
    u8 mask[8];
} gc_point_sprite_t;

typedef struct
{
    s32 radius;
    gc_screen_rect_t sprite_box;
    gc_screen_rect_t sprite_cropped_box;
} gc_point_setup_t;

typedef __ALIGN__ struct
{
    b8 is_dx;

    r32 la;
    r32 lb;
    r32 lc;

    r32 a;
    r32 b;

    r32 interp_z;

    r32 box_min_x;
    r32 box_min_y;
    r32 box_max_x;
    r32 box_max_y;
} gc_line_setup_t;

typedef __ALIGN__ struct
{
    b8 flags;

    r32 one_over_area;
    s64 fp_area;

    s64 l1a;
    s64 l1b;
    s64 l1c;

    s64 l2a;
    s64 l2b;
    s64 l2c;

    s64 l3a;
    s64 l3b;
    s64 l3c;

    s64 frag_l1dx;
    s64 frag_l1dy;

    s64 frag_l2dx;
    s64 frag_l2dy;

    s64 frag_l3dx;
    s64 frag_l3dy;

    vec2 interp_z;
    vec2 interp_w;
    vec2 interp_varying[VERTEX_ATTRIBUTE_SIZE];

    s64 l1dx;
    s64 l1dy;

    s64 l2dx;
    s64 l2dy;

    s64 l3dx;
    s64 l3dy;
} gc_triangle_setup_t;

__ALIGN__ struct gc_primitive_s
{
    gc_vertex_t base;

    gc_vertex_t v1;
    gc_vertex_t v2;
    gc_vertex_t v3;

    u32 id;
    gc_mesh_type_t type;
    gc_screen_rect_t box;
    b8 is_backface;

    __ALIGN__ union
    {
        gc_point_setup_t point;
        gc_line_setup_t line;
        gc_triangle_setup_t triangle;
    };
};

typedef struct
{
    gc_vertex_t v[3];

    s32 v1_posfp[2];
    s32 v2_posfp[2];
    s32 v3_posfp[2];
    s64 area;

    u8 flags;
} gc_vset_t;

// ----------------------------------------------------------------------------------
// -- Rasterization buffers.
// ----------------------------------------------------------------------------------

typedef __ALIGN__ struct
{
    gc_vertex_t data[CLIPPING_VERTEX_BUFFER_COUNT];
    u32 start;
    u32 count;
    u32 read;
} gc_clipping_vertex_buffer_t;

typedef struct
{
    s32 min;
    s32 max;
} gc_guard_band_t;

// ----------------------------------------------------------------------------------
// -- Workers.
// ----------------------------------------------------------------------------------

typedef struct
{
    // SDL_atomic_t running_count;
    u8 running_count;
    gc_worker_state_t state;

    SDL_mutex *worker_lock;
    SDL_cond *worker_condition;
    SDL_cond *scheduler_condition;

    u8 count;
    SDL_Thread **pool;
} gc_multithreading_t;

// ----------------------------------------------------------------------------------
// -- Main graphics state.
// ----------------------------------------------------------------------------------

typedef enum
{
    PRIMARY_BUFFER,
    SECONDARY_BUFFER,
    SHADOWS_BUFFER,

    BUFFER_COUNT
} gc_framebuffer_slot_t;

typedef struct
{
    u16 bins[8192];
    SDL_atomic_t count;
    SDL_atomic_t r_cursor;
    SDL_atomic_t f_cursor;
} gc_work_bin_queue_t;

typedef __ALIGN__ struct
{
    r32 r[GC_FRAG_SIZE];
    r32 g[GC_FRAG_SIZE];
    r32 b[GC_FRAG_SIZE];
    r32 z[GC_FRAG_SIZE];
} gc_fragment_t;

typedef __ALIGN__ struct
{
    gc_fragment_t fragments[GL_BIN_FRAGS];
    u16 x, y;
} gc_tile_buffer_t;

typedef struct
{
    gc_tile_buffer_t *tiles;
    SDL_atomic_t cursor;
    gc_vec_t clear_color;

    size_t bytes;
} gc_linear_screen_buffer_t;

typedef __ALIGN__ struct
{
    r32 r[GC_FRAG_SIZE];
    r32 g[GC_FRAG_SIZE];
    r32 b[GC_FRAG_SIZE];
    r32 a[GC_FRAG_SIZE];
    r32 z[GC_FRAG_SIZE];
    r32 varyings[VERTEX_ATTRIBUTE_SIZE][GC_FRAG_SIZE];
    r32 shadow[GC_FRAG_SIZE];

    r32 dudx[GC_FRAG_SIZE];
    r32 dvdx[GC_FRAG_SIZE];
    r32 dudy[GC_FRAG_SIZE];
    r32 dvdy[GC_FRAG_SIZE];

    // u8 state;
    b8 discarded;
    u8 mask;
    u16 index;
    u16 x, y;
    gc_primitive_t *primitive;
} gc_processed_fragment_t;

typedef __ALIGN__ struct
{
    void *data;
    u32 count;
} gc_pipe_array_t;

typedef struct gc_bin_prim_link_s gc_bin_prim_link_t;
struct gc_bin_prim_link_s
{
    b8 total_coverage;
    u16 prim_idx;
    // gc_primitive_t *primitive;
    gc_bin_prim_link_t *next;
};

typedef struct
{
    gc_bin_prim_link_t *start;
    gc_bin_prim_link_t *last;
    u16 count;
} gc_bin_prim_list_t;

struct gc_bin_s
{
    u16 x, y;

    gc_bin_prim_list_t list[GC_PIPE_NUM_THREADS];

    // The bin was added to the queue and is waiting to be rasterized.
    SDL_atomic_t dirty;
};

typedef struct
{
    u32 start_index;
    u32 end_index;
} gc_partition_t;

typedef __ALIGN__ struct
{
    void *data;
    u32 count;
    u32 start;
    u32 end;
} gc_fragments_array_t;

typedef struct
{
    gc_fragments_array_t *fragments;
    pipe_memory_t *memory;
} thread_batch_memory_t;

typedef struct
{
    b8 enabled;
    r32 saturation;
    gc_vec_t tint;
} gc_postprocessing_t;

#define gc_set_framebuffer(slot) GCSR.gl->current_framebuffer = GCSR.gl->framebuffers[slot];

typedef void (* load_tile_buffer_t) (gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer);
typedef void (* save_tile_buffer_t) (gc_framebuffer_t *framebuffer, gc_tile_buffer_t *tile_buffer);

__ALIGN__ struct gc_framebuffer_s
{
    u32 width;
    u32 height;
    r32 aspect;
    u32 flags;
    size_t bytes;

    u16 bin_rows;
    u16 bin_cols;
    u16 total_bins;

    u16 frag_rows;
    u16 frag_cols;
    u32 depth_pitch;

    u32 tiled_width;
    u32 tiled_height;

    texture2d_t *color;
    // r32 *depth;

    load_tile_buffer_t load;
    save_tile_buffer_t save;

    // NOTE(gabic): Each framebuffer will have it's own
    // bin queue based on it's size.

    gc_bin_t *bins;
    gc_transparency_bin_t *transparency;
    gc_linear_screen_buffer_t lsb;

    SDL_atomic_t bin_cursor;
    SDL_atomic_t transparency_cursor;
};

// ----------------------------------------------------------------------------------

#define gc_projection_perspective(aspect, fov, f_near, f_far) \
    gc_mat4_perspective(aspect, fov, f_near, f_far, GET_MATRIX(M_PROJECTION))

#define gc_projection_orthografic(f_right, f_left, f_top, f_bottom, f_near, f_far) \
    gc_mat4_orthographic(f_right, f_left, f_top, f_bottom, f_near, f_far, GET_MATRIX(M_PROJECTION))

#define gc_viewport(width, height, sx, sy) \
    gc_mat4_viewport(width, height, sx, sy, 1, GET_MATRIX(M_VIEWPORT)); \
    gc_mat4_inv(GET_MATRIX(M_VIEWPORT), GET_MATRIX(M_VIEWPORT_INV))

#define gc_copy_position(vertex, gl_position) \
    vertex->pos[0] = gl_position.v4.x; \
    vertex->pos[1] = gl_position.v4.y; \
    vertex->pos[2] = gl_position.v4.z; \
    vertex->pos[3] = gl_position.v4.w

void gc_push_matrix(gc_mat_t *m);
void gc_compose_matrix(gc_mat_t *out);
__INLINE__ void sse_matrix_stack_compose(gc_mat_t *out);

#endif