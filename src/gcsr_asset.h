// ----------------------------------------------------------------------------------
// -- File: gcsr_asset.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-07-09 13:16:50
// -- Modified: 2022-07-09 13:16:51
// ----------------------------------------------------------------------------------

#ifndef GCSR_ASSET_H
#define GCSR_ASSET_H

// ----------------------------------------------------------------------------------
// -- Versiunea noua asset file.
// ----------------------------------------------------------------------------------

#define ASSET_FILE "data/assets.pkg"
#define ASSET_NAME_FILE "data/asset_info.txt"
#define CONFIG_ASSET_FILE "data/package/asset_builder_config.json"
#define VERSION "1.3.0"

#define MAX_ASSET_FILEPATH_SIZE 255
#define METADATA_NAME_LENGTH 64
#define CHUNK_BUFFER_METADATA_BYTES Megabytes(16)
#define CHUNK_BUFFER_DATA_BYTES Megabytes(640)

// 1 = asset count, 2 = asset bytes
#define CHUNK_LIMIT_ASSET_COUNT 1
#define CHUNK_LIMIT_ASSET_BYTES 2
#define CHUNK_LIMIT CHUNK_LIMIT_ASSET_BYTES
#define MAX_CHUNK_ASSETS 10
#define MAX_CHUNK_BYTES Megabytes(128)

#define CHUNK_MESH_INDICES(assets, chunk_metadata) ((u8 *) assets + chunk_metadata->data_offset + chunk_metadata->mesh.indices_offset)
#define CHUNK_MESH_VERTICES(assets, chunk_metadata) ((u8 *) assets + chunk_metadata->data_offset + chunk_metadata->mesh.vertices_offset)

// #define PBR_DEBUG_BASE_HDR 1
#define PBR_CUBEMAP_FILTERING 1

#define DEBUG_CUBEMAP_MIP_LEVEL 0
#define DEBUG_CUBEMAP_ENVIRONMENT_MIP_LEVEL 2
#define DEBUG_CUBEMAP_PREFILTERED_MIP_LEVEL 3

#define PBR_ENVIRONMENT_SIZE 1024
#define PBR_EXPOSURE 2
#define PBR_ENVIRONMENT_MIP_LEVELS 1

#define PBR_IRRADIANCE_SIZE 64
#define PBR_IRRADIANCE_MIP_LEVELS 1
#define PBR_IRRADIANCE_PHI_SAMPLES 1500
// #define PBR_IRRADIANCE_THETA_SAMPLES 1200

#define PBR_PREFILTERED_SIZE 256
#define PBR_PREFILTERED_MIP_LEVELS 5
#define PBR_PREFILTERED_SAMPLE_COUNT 512

#define PBR_BRDF_LUT_SIZE 512
#define PBR_BRDF_LUT_SAMPLE_COUNT 256

#define TEX_BYTES_INT(tex_width, tex_height) (tex_width) * (tex_height) * sizeof(u32)
#define TEX_BYTES_FLOAT(tex_width, tex_height) (tex_width) * (tex_height) * 4 * sizeof(r32)

#define TEX_HEADER_BYTES(tex_count) (sizeof(texture2d_t) + (tex_count) * sizeof(texture_mip_t))
#define TEX_DATA_BYTES_RGBAU8(tex_width, tex_height) (sizeof(texture_data_header_t) + (tex_width) * (tex_height) * sizeof(u32))
#define TEX_DATA_BYTES_RGBAF(tex_width, tex_height) (sizeof(texture_data_header_t) + (tex_width) * (tex_height) * 4 * sizeof(r32))
#define TEX_DATA_BYTES_RGBF(tex_width, tex_height) (sizeof(texture_data_header_t) + (tex_width) * (tex_height) * 3 * sizeof(r32))
#define TEX_DATA_BYTES_RGF(tex_width, tex_height) (sizeof(texture_data_header_t) + (tex_width) * (tex_height) * 2 * sizeof(r32))
#define TEX_MIP_COUNT(size) (u32) log2f(size) + 1

#define TEX_HEADER_INDEX(base, index) (texture_mip_t *) (base) + (index);

#define TEXTURE_FILTER 1
#define TEXTURE_MIPS 2
#define TEXTURE_MIPS_FILTER 4
#define TEXTURE_WRAP_CLAMP 8
#define TEXTURE_WRAP_REPEAT 16
#define TEXTURE_WRAP_MIRROR 32
#define TEXTURE_GAUSSIAN_BLUR 64

#define TEXTURE_WRAP_MASK 0b111000

#define TEXTURE_FORMAT_RGBAU8 1
#define TEXTURE_FORMAT_RGBAF 2
#define TEXTURE_FORMAT_RGBF 3
#define TEXTURE_FORMAT_RGF 4
#define TEXTURE_FORMAT_SHADOW 5

typedef struct texture2d_s texture2d_t;
typedef struct cube_texture_s cube_texture_t;

// ----------------------------------------------------------------------------------
// -- Asset sources (structures used for reading the assets from the config file).
// ----------------------------------------------------------------------------------

typedef enum
{
    CUBE_LEFT,
    CUBE_RIGHT,
    CUBE_FRONT,
    CUBE_BACK,
    CUBE_TOP,
    CUBE_BOTTOM,
    CUBE_TOTAL
} gl_cube_faces_t;

typedef enum
{
    GL_MESH_NONE = 0,
    GL_MESH_POINT = 1,
    GL_MESH_LINE,
    GL_MESH_TRIANGLE,
} gc_mesh_type_t;

typedef enum
{
    ASSET_TEXTURE2D = 1,
    ASSET_TEXTURE_CUBEMAP,
    ASSET_TEXTURE_PBR_AMBIENT,
    ASSET_MESH,
    ASSET_FONT,
    ASSET_TEXT
} asset_type_t;

typedef enum
{
    ASSET_NOT_LOADED = 0,
    ASSET_LOADED = 1,
    ASSET_LOADED_BUT_NOT_USED = 2
} asset_status_t;

typedef struct
{
    s32 width;
    s32 height;
    r32 du;
    r32 dv;
    u32 row_pitch;
    u32 col_pitch;
} hdr_data_t;

typedef __ALIGN__ struct
{
    // 2 + 3 + 4
    r32 data[12];
    r32 pos[4];
} asset_vertex_t;

#define SOURCE_DEBUG 1
#define SOURCE_DISABLED 2
#define SOURCE_GENERATOR 4

typedef struct
{
    asset_type_t type;

    char alias[METADATA_NAME_LENGTH];
    char filepath[MAX_ASSET_FILEPATH_SIZE];
    char cubepath[6][MAX_ASSET_FILEPATH_SIZE];

    u32 flags;
} asset_config_source_t;

typedef struct
{
    // asset_type_t type;
    asset_config_source_t *sources;
    u32 count;
} asset_config_category_t;

typedef struct
{
    u32 total_assets;

    asset_config_category_t textures;
    asset_config_category_t meshes;
    asset_config_category_t fonts;
    asset_config_category_t texts;
} asset_config_data_t;

typedef struct
{
    u16 asset_count;

    void *metadata_buffer;
    void *data_buffer;

    u64 metadata_bytes;
    u64 data_bytes;

    u64 output_file_offset;
} chunk_buffer_t;

// ----------------------------------------------------------------------------------
// -- Asset file structure.
// ----------------------------------------------------------------------------------

// +---------------------+--------+--------+
// | asset_file_header_t | chunk0 | chunk1 |
// +---------------------+--------+--------+
// +-------+   +--------------+-----------+-----+-----------+-------+-----+-------+
// | chunk | = | chunk_header | metadata0 | ... | metadataN | data0 | ... | dataN |
// +-------+   +--------------+-----------+-----+-----------+-------+-----+-------+

typedef __ALIGN__ struct
{
    u32 magic_value;
    char version[6];

    u16 chunks;
    u32 total_assets;
    size_t total_bytes;

    u32 textures;
    u32 meshes;
    u32 fonts;
    u32 texts;
} asset_file_header_t;

typedef struct
{
    u32 asset_count;
    size_t metadata_bytes;
    size_t data_bytes;
    size_t next_chunk_offset;
} asset_chunk_t;

#pragma pack(push, 1)

typedef struct
{
    r32 r;
    r32 g;
    r32 b;
    r32 a;
} texpixel_rgbaf_t;

#define RGBAFSET(pixel, pr, pg, pb, pa) \
    pixel.r = pr; \
    pixel.g = pg; \
    pixel.b = pb; \
    pixel.a = pa

typedef struct
{
    r32 r;
    r32 g;
    r32 b;
} texpixel_rgbf_t;

#define RGBFSET(pixel, pr, pg, pb) \
    pixel.r = pr; \
    pixel.g = pg; \
    pixel.b = pb

typedef struct
{
    r32 r;
    r32 g;
} texpixel_rgf_t;

#define RGFSET(pixel, pr, pg) \
    pixel.r = pr; \
    pixel.g = pg

typedef struct
{
    r32 data[3][3];
} kernel_3x3_t;

typedef struct
{
    r32 data[5][5];
} kernel_5x5_t;

#pragma pack(pop)

typedef __ALIGN__ struct
{
    size_t bytes;

    u32 width;
    u32 height;
    r32 tex_du;
    r32 tex_dv;
} texture_data_header_t;

typedef struct
{
    texture_data_header_t *header;
    void *data;
} texture_mip_t;

typedef __ALIGN__ struct
{
    u32 low[GC_FRAG_SIZE];
    u32 high[GC_FRAG_SIZE];
    r32 interp[GC_FRAG_SIZE];
} lod_t;

#define LOD_CLEAR(lod) \
    lod.low[0] = 0; \
    lod.low[1] = 0; \
    lod.low[2] = 0; \
    lod.low[3] = 0; \
    lod.high[0] = 0; \
    lod.high[1] = 0; \
    lod.high[2] = 0; \
    lod.high[3] = 0; \
    lod.interp[0] = 0; \
    lod.interp[1] = 0; \
    lod.interp[2] = 0; \
    lod.interp[3] = 0

typedef void (* tex_clear_func_t) (texture2d_t *texture, gc_vec_t c);
typedef void (* tex_wrap_func_t) (r32 *u, r32 *v, r32 *out_u, r32 *out_v);
typedef void (* tex_sample_func_t) (texture2d_t *texture, r32 *u, r32 *v, u32 *lod, shader_color_t *output);
typedef void (* tex_cube_sample_func_t) (cube_texture_t *cubemap, r32 *u, r32 *v, u32 *face_index, u32 *lod, shader_color_t *output);

// typedef void (* sse_tex_clear_func_t) (texture2d_t *texture, gc_vec_t c);
// typedef void (* sse_tex_wrap_func_t) (r32 *c, r32 *out);
// typedef void (* sse_tex_sample_func_t) (texture2d_t *texture, sse_v2_t *texcoord, u32 lod, sse_color_t *output);
// typedef void (* sse_tex_cube_sample_func_t) (cube_texture_t *cubemap, sse_v2_t *texcoord, u32 *face_index, u32 lod, sse_color_t *output);

typedef struct
{
    u32 flags;
    u32 format;

    tex_clear_func_t tex_clear;
    tex_wrap_func_t tex_wrap;
    tex_sample_func_t tex_sample;
    tex_cube_sample_func_t tex_cube_sample;
} texture_settings_t;

struct texture2d_s
{
    asset_type_t type;

    texture_settings_t settings;
    size_t data_bytes;
    u32 mip_count;
    texture_mip_t *mips;
};

struct cube_texture_s
{
    asset_type_t type;

    texture_settings_t settings;
    size_t data_bytes;
    u32 mip_count;
    struct texture2d_s *faces[6];
};

typedef struct
{
    r32 a;
    r32 b;
} brdf_lut_pixel_t;

typedef struct
{
    asset_type_t type;

    cube_texture_t *environment;
    cube_texture_t *irradiance;
    cube_texture_t *prefiltered;
    texture2d_t *brdf_lut;
} pbr_ambient_texture_t;

// typedef struct
// {
//     u32 width;
//     u32 height;
//     u32 mip_count;
// } asset_metadata_texture2d_t;

// typedef struct
// {
//     u32 width;
//     u32 height;
//     u32 mip_count;
// } asset_metadata_texture_cubemap_t;

// typedef struct
// {
//     size_t environment_bytes;
//     size_t irradiance_bytes;
//     size_t prefiltered_bytes;
//     size_t brdf_lut_bytes;

//     u32 environment_mip_count;
//     u32 irradiance_mip_count;
//     u32 prefiltered_mip_count;
//     u32 brdf_lut_mip_count;
// } asset_metadata_texture_pbr_ambient_t;

typedef struct
{
    u32 width;
    u32 height;
    u32 mip_count;
    u32 format;
} asset_metadata_texture_t;

typedef struct
{
    gc_mesh_type_t type;

    u32 indices;
    u32 vertices;

    size_t indices_offset;
    size_t vertices_offset;

    size_t indices_bytes;
    size_t vertices_bytes;
} asset_metadata_mesh_t;

typedef struct
{
    u32 count;
    size_t bytes;
    size_t data_offset;
} asset_metadata_text_t;

typedef struct
{
    asset_type_t type;

    char package_path[FILE_MAX_PATH];
    char name[METADATA_NAME_LENGTH];

    u32 chunk;
    size_t next_meta_offset;
    size_t data_offset;
    size_t data_bytes;

    union
    {
        asset_metadata_texture_t texture;
        asset_metadata_mesh_t mesh;
        asset_metadata_text_t text;
    };
} asset_metadata_t;

typedef struct
{
    u32 magic_value; // ASGP
    char version[6];
    u32 count;
} asset_group_header_t;

typedef struct
{
    u32 red_shift;
    u32 green_shift;
    u32 blue_shift;
    u32 alpha_shift;

    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
    u32 alpha_mask;
} pixel_format_t;

typedef struct
{
    u32 width;
    u32 height;
    u32 bytes_per_pixel;
    u32 pitch;
    r32 width_over_height;
    u32 components;
    u32 mip_count;

    size_t bytes;
    void *data;
} loaded_bitmap_t;

typedef struct
{
    asset_metadata_t meta;
    void *data;
} loaded_texture_t;

#pragma pack(push, 1)

// TODO(gabic): Sa adaug header-ele separate bitmap header + dib header

typedef struct
{
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 bitmap_offset;

    u32 size;
    s32 width;
    s32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 size_of_bitmap;
    s32 horz_resolution;
    s32 vert_resolution;
    u32 colors_used;
    u32 colors_important;

    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
    u32 alpha_mask;
    u32 cstype;
    s32 redx;
    s32 redy;
    s32 redz;
    s32 greenx;
    s32 greeny;
    s32 greenz;
    s32 bluex;
    s32 bluey;
    s32 bluez;
    u32 gamma_red;
    u32 gamma_green;
    u32 gamma_blue;
} bitmap_header_t;

typedef struct
{
    u8 b;
    u8 g;
    u8 r;
} bitmap_color_24_t;

#pragma pack(pop)

typedef struct
{
    r32 r;
    r32 g;
    r32 b;
    r32 a;
} texcolor_t;

typedef struct
{
    u32 width;
    u32 height;
    r32 tex_du;
    r32 tex_dv;
    u32 pitch;

    void *data;
} mipmap_t;

typedef __ALIGN__ struct
{
    gc_mesh_type_t type;
    u32 indices_count;
    u32 *indices;
    asset_vertex_t *vertices;
} mesh_t;

typedef struct
{
    u32 id;
    asset_status_t status;
    asset_metadata_t *meta;
    void *data;
} asset_t;

typedef struct
{
    gc_file_t file;
    asset_file_header_t header;

    u32 chunk_count;
    u32 metadata_count;

    asset_chunk_t *chunk_table;
    asset_metadata_t *metadata_table;
} asset_file_t;

typedef struct
{
    u32 access_count;
    gc_file_t file;
} cache_item_t;

typedef struct
{
    cache_item_t package_cache[5];

    asset_metadata_t *metadata_table;
    asset_t *asset_table;
    ds_hashtable_t *asset_access_table;
} gc_asset_manager_t;

#endif