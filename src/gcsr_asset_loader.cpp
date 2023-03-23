// ----------------------------------------------------------------------------------
// -- File: gcsr_asset_loader.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description: Loads various assets from the asset file.
// -- Created: 2021-03-08 21:14:31
// -- Modified:
// ----------------------------------------------------------------------------------

// #include "asset_builder/parser/gcsr_parser.h"
#include "libs/json-parser/json.c"

extern global_vars_t GCSR;

mesh_t *asset_load_mesh(char *mesh_name);

// #define JSON_LOOP(json, index) for (u16 index = 0; index < json->u.object.length; ++index)
#define JSON_VALUE_OBJECT_LOOP(val, index) for (u16 index = 0; index < val->u.object.length; ++index)
#define JSON_VALUE_ARRAY_LOOP(val, index) for (u16 index = 0; index < val->u.array.length; ++index)

#define JSON_OBJECT_PROPERTY(obj, index) (obj->u.object.values + index)

#define JSON_PROPERTY_NAME_EQUALS(prop, required_name) (strcmp(prop->name, required_name) == 0)
#define JSON_PROPERTY_TYPE_EQUALS(prop, required_type) (prop->value->type == required_type)
#define JSON_PROPERTY_COMPARE(prop, required_name, required_type) JSON_PROPERTY_NAME_EQUALS(prop, required_name) && JSON_PROPERTY_TYPE_EQUALS(prop, required_type)

#define JSON_VALUE_INTEGER(val) (val->u.integer)
#define JSON_VALUE_BOOL(val) (val->u.integer)
#define JSON_VALUE_STRING(val) (val->u.string.ptr)

#define JSON_PROPERTY_VALUE_INTEGER(prop) (prop->value->u.integer)
#define JSON_PROPERTY_VALUE_BOOL(prop) (prop->value->u.integer)
#define JSON_PROPERTY_VALUE_STRING(prop) (prop->value->u.string.ptr)
#define JSON_PROPERTY_VALUE_DOUBLE(prop) (r32) (prop->value->u.dbl)

#define JSON_ARRAY_LENGTH(arr) arr->u.array.length
#define JSON_ARRAY_VALUE(arr, pi) (arr->u.array.values[pi])
#define JSON_ARRAY_VALUE_STRING(arr, pi) (arr->u.array.values[pi]->u.string.ptr)
#define JSON_ARRAY_VALUE_FLOAT(arr, pi) (r32) (arr->u.array.values[pi]->u.dbl)
#define JSON_ARRAY_VALUE_INTEGER(arr, pi) (arr->u.array.values[pi]->u.integer)

void _json_prop_extract_integer(u32 *dest, struct _json_object_entry *prop) {
    *dest = prop->value->u.integer;
}

void _json_prop_extract_float(r32 *dest, struct _json_object_entry *prop) {
    *dest = (r32) prop->value->u.dbl;
}

void _json_prop_extract_v2(gc_vec_t *dest, struct _json_object_entry *prop)
{
    dest->v2.x = prop->value->u.array.values[0]->u.dbl;
    dest->v2.y = prop->value->u.array.values[1]->u.dbl;
}

void _json_prop_extract_v3(gc_vec_t *dest, struct _json_object_entry *prop)
{
    dest->v4.x = prop->value->u.array.values[0]->u.dbl;
    dest->v4.y = prop->value->u.array.values[1]->u.dbl;
    dest->v4.z = prop->value->u.array.values[2]->u.dbl;
    dest->v4.w = 1.0f;
}

void _json_prop_extract_v4(gc_vec_t *dest, struct _json_object_entry *prop)
{
    dest->c.r = prop->value->u.array.values[0]->u.dbl;
    dest->c.g = prop->value->u.array.values[1]->u.dbl;
    dest->c.b = prop->value->u.array.values[2]->u.dbl;
    dest->c.a = prop->value->u.array.values[3]->u.dbl;
}

void _json_read_color(struct _json_object_entry *prop, gc_vec_t *dest)
{
    if (JSON_ARRAY_LENGTH(prop->value) == 3)
    {
        dest->c.r = prop->value->u.array.values[0]->u.dbl;
        dest->c.g = prop->value->u.array.values[1]->u.dbl;
        dest->c.b = prop->value->u.array.values[2]->u.dbl;
        dest->c.a = 1.0f;
    }
    else if (JSON_ARRAY_LENGTH(prop->value) == 4)
    {
        dest->c.r = prop->value->u.array.values[0]->u.dbl;
        dest->c.g = prop->value->u.array.values[1]->u.dbl;
        dest->c.b = prop->value->u.array.values[2]->u.dbl;
        dest->c.a = prop->value->u.array.values[3]->u.dbl;
    }
}

void _json_read_transform(struct _json_value *transform_data, transform_t *transform)
{
    if (transform_data->type == json_object)
    {
        JSON_VALUE_OBJECT_LOOP(transform_data, i)
        {
            struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(transform_data, i);

            if (JSON_PROPERTY_COMPARE(prop, "translation", json_array) && JSON_ARRAY_LENGTH(prop->value) == 3)
            {
                transform->translation.data[0] = JSON_ARRAY_VALUE_FLOAT(prop->value, 0);
                transform->translation.data[1] = JSON_ARRAY_VALUE_FLOAT(prop->value, 1);
                transform->translation.data[2] = JSON_ARRAY_VALUE_FLOAT(prop->value, 2);
                transform->translation.data[3] = 0;
            }

            else if (JSON_PROPERTY_COMPARE(prop, "rotation", json_array) && JSON_ARRAY_LENGTH(prop->value) == 3)
            {
                transform->rotation.data[0] = JSON_ARRAY_VALUE_FLOAT(prop->value, 0);
                transform->rotation.data[1] = JSON_ARRAY_VALUE_FLOAT(prop->value, 1);
                transform->rotation.data[2] = JSON_ARRAY_VALUE_FLOAT(prop->value, 2);
                transform->rotation.data[3] = 0;
            }

            else if (JSON_PROPERTY_COMPARE(prop, "scaling", json_array) && JSON_ARRAY_LENGTH(prop->value) == 3)
            {
                transform->scaling.data[0] = JSON_ARRAY_VALUE_FLOAT(prop->value, 0);
                transform->scaling.data[1] = JSON_ARRAY_VALUE_FLOAT(prop->value, 1);
                transform->scaling.data[2] = JSON_ARRAY_VALUE_FLOAT(prop->value, 2);
                transform->scaling.data[3] = 0;
            }
        }
    }
}

void asset_read_package_metadata(char *package_filepath)
{
    gc_file_t file;
    platform_api_t *API = get_platform_api();

    API->open_file(&file, package_filepath, GC_FILE_READ);

    if (file.handle && file.bytes)
    {
        asset_group_header_t *package_header = (asset_group_header_t *) malloc(file.bytes);
        size_t meta_offset = sizeof(asset_group_header_t);
        API->read_file(&file, 0, file.bytes, package_header);
        SDL_assert(package_header->magic_value == 1346851649);

        for (u32 i = 0; i < package_header->count; ++i)
        {
            asset_metadata_t *metadata = (asset_metadata_t *) ADDR_OFFSET(package_header, meta_offset);

            dynamic_array_header_t *header = da_header(GCSR.state->asset_manager->metadata_table);
            asset_metadata_t *current_metadata = GCSR.state->asset_manager->metadata_table + header->length;
            API->read_file(&file, meta_offset, sizeof(asset_metadata_t), current_metadata);
            da_push(GCSR.state->asset_manager->metadata_table, asset_metadata_t);

            meta_offset = metadata->next_meta_offset;
        }

        API->close_file(&file);
        free(package_header);
    }
    else
        printf("[WARNING] Package not found [%s] !\n", package_filepath);
}

void asset_read_packages(char *dirpath)
{
    platform_api_t *API = get_platform_api();
    file_attributes_t attrs;

    char tmp_string[255];
    char directory[255];

    void *handle = API->find_first_file(dirpath, &attrs);

    char *sep = strrchr(dirpath, '/');
    u32 len = sep - dirpath;
    strncpy(directory, dirpath, len);

    if (handle)
    {
        sprintf(tmp_string, "%s/%s", directory, attrs.filename);
        printf("[LOADING] %s\n", tmp_string);
        asset_read_package_metadata(tmp_string);

        while (API->find_next_file(handle, &attrs))
        {
            sprintf(tmp_string, "%s/%s", directory, attrs.filename);
            printf("[LOADING] %s\n", tmp_string);
            asset_read_package_metadata(tmp_string);
        }
    }
}

void load_package(char *filepath)
{
    if (filepath)
    {
        printf("[LOADING] %s\n", filepath);
        asset_read_package_metadata(filepath);
    }
}

void generate_asset_access_tables()
{
    gc_asset_manager_t *asset_manager = GCSR.state->asset_manager;

    // Asset metadata table location.
    dynamic_array_header_t *header = da_header(GCSR.state->asset_manager->metadata_table);
    asset_metadata_t *metadata_table = GCSR.state->asset_manager->metadata_table;

    if (header->length)
    {
        MEM_LABEL("asset_table");
        asset_manager->asset_table = (asset_t *) gc_mem_allocate(sizeof(asset_t) * header->length);
        MEM_LABEL("asset_access_table");
        asset_manager->asset_access_table = hashtable_create(header->length);
        asset_manager->asset_access_table->hash_function = string_hash_function;
        asset_manager->asset_access_table->compare_function = string_compare_function;

        // Add all the assets to the asset hashtable for easy access.
        for (u32 i = 0; i < header->length; ++i)
        {
            asset_metadata_t *current_metadata = metadata_table + i;
            asset_t *current_asset = asset_manager->asset_table + i;

            current_asset->id = i + 1;
            current_asset->status = ASSET_NOT_LOADED;
            current_asset->meta = current_metadata;
            current_asset->data = 0;

            hashtable_insert(asset_manager->asset_access_table, current_asset->meta->name, current_asset);
        }
    }
}

void asset_init_asset_manager()
{
    if (GCSR.state && !GCSR.state->asset_manager)
    {
        mem_set_chunk(MEMORY_TEMPORARY);
        mem_debug_name("asset manager");
        MEM_LABEL("asset_manager");
        GCSR.state->asset_manager = (gc_asset_manager_t *) gc_mem_allocate(sizeof(gc_asset_manager_t));

        mem_debug_name("asset metadata");
        MEM_LABEL("asset_metadata");
        da_create(GCSR.state->asset_manager->metadata_table, asset_metadata_t, 10);
        mem_restore_chunk();
    }
}

gc_file_t *get_package_from_cache(char *package_path)
{
    gc_file_t *cache_file = 0;
    platform_api_t *API = get_platform_api();
    gc_asset_manager_t *asset_manager = GCSR.state->asset_manager;
    u32 lowest_access = 0xffffffff;
    u8 lowest_access_index = 0;

    for (u8 i = 0; i < 5; ++i)
    {
        cache_item_t *cache = asset_manager->package_cache + i;
        gc_file_t *tmp_file = &cache->file;

        if (cache->access_count < lowest_access)
        {
            lowest_access = cache->access_count;
            lowest_access_index = i;
        }

        // The package was already opened and is returned from the cache.
        if (cache && tmp_file->handle && strcmp(tmp_file->name, package_path) == 0)
        {
            cache_file = tmp_file;
            break;
        }
    }

    // The package was not opened yet.
    if (!cache_file)
    {
        cache_item_t *cache = asset_manager->package_cache + lowest_access_index;
        gc_file_t *tmp_file = &cache->file;

        // Close the existing file in this cache slot.
        if (tmp_file)
            API->close_file(tmp_file);

        API->open_file(tmp_file, package_path, GC_FILE_READ);
        cache->access_count = 1;
        cache_file = tmp_file;
    }

    return cache_file;
}

texture2d_t *asset_load_texture2d(char *texture_name, u32 flags, gc_level_t *level)
{
    gc_asset_manager_t *asset_manager = GCSR.state->asset_manager;
    texture2d_t *texture = 0;
    asset_t *asset = (asset_t *) hashtable_search(asset_manager->asset_access_table, texture_name);

    if (!asset)
    {
        printf("[Asset loader] Asset not found {%s} !\n", texture_name);
        return 0;
    }

    if (asset->meta->type != ASSET_TEXTURE2D)
    {
        printf("[Asset loader] Specified asset is not a {ASSET_TEXTURE2D} !\n");
        return 0;
    }

    if (level)
    {
        u32 index = level->texture_count++;
        SDL_assert(level->texture_count <= LEVEL_MAX_TEXTURES);
        level->textures[index] = 0;
    }

    if (asset->status == ASSET_NOT_LOADED)
    {
        platform_api_t *API = get_platform_api();
        asset_metadata_texture_t *texture_meta = &asset->meta->texture;

        if (asset->meta->type == ASSET_TEXTURE2D)
        {
            MEM_DESCRIPTION(texture_name);
            texture = gc_create_texture2d(texture_meta->width,
                                          texture_meta->height,
                                          texture_meta->mip_count,
                                          texture_meta->format,
                                          flags);
            asset->status = ASSET_LOADED;
            asset->data = texture;

            u8 *data_pointer = (u8 *) texture->mips->header;

            gc_file_t *package_file = get_package_from_cache(asset->meta->package_path);
            API->read_file(package_file, asset->meta->data_offset, asset->meta->data_bytes, data_pointer);

            if (level)
            {
                level->assets[level->asset_count++] = asset;
                level->textures[level->texture_count - 1] = texture;
                SDL_assert(level->asset_count < LEVEL_MAX_ASSETS);
            }
        }
    }

    return texture;
}

cube_texture_t *asset_load_cube_texture(char *texture_name, u32 flags, gc_level_t *level)
{
    gc_asset_manager_t *asset_manager = GCSR.state->asset_manager;
    cube_texture_t *texture = 0;
    asset_t *asset = (asset_t *) hashtable_search(asset_manager->asset_access_table, texture_name);

    if (!asset)
    {
        printf("[Asset loader] Asset not found {%s} !\n", texture_name);
        return 0;
    }

    if (asset->meta->type != ASSET_TEXTURE_CUBEMAP)
    {
        printf("[Asset loader] Specified asset is not a {ASSET_TEXTURE_CUBEMAP} !\n");
        return 0;
    }

    if (asset->status == ASSET_NOT_LOADED)
    {
        platform_api_t *API = get_platform_api();
        asset_metadata_texture_t *texture_meta = &asset->meta->texture;

        if (asset->meta->type == ASSET_TEXTURE_CUBEMAP)
        {
            MEM_DESCRIPTION(texture_name);
            texture = gc_create_cubemap_texture(texture_meta->width,
                                                texture_meta->height,
                                                texture_meta->mip_count,
                                                texture_meta->format,
                                                flags);
            asset->status = ASSET_LOADED;
            asset->data = texture;

            u8 *data_pointer = (u8 *) texture->faces[0]->mips->header;

            gc_file_t *package_file = get_package_from_cache(asset->meta->package_path);
            API->read_file(package_file, asset->meta->data_offset, asset->meta->data_bytes, data_pointer);

            if (level)
            {
                level->assets[level->asset_count++] = asset;
                level->textures[level->texture_count++] = texture;
                SDL_assert(level->asset_count <= LEVEL_MAX_ASSETS);
                SDL_assert(level->texture_count <= LEVEL_MAX_TEXTURES);
            }
        }
    }

    return texture;
}

pbr_ambient_texture_t *asset_load_pbr_ambient_texture(char *texture_name, u32 flags, gc_level_t *level)
{
    gc_asset_manager_t *asset_manager = GCSR.state->asset_manager;

    char environment_texture_name[METADATA_NAME_LENGTH];
    char irradiance_texture_name[METADATA_NAME_LENGTH];
    char prefiltered_texture_name[METADATA_NAME_LENGTH];
    char brdf_lut_texture_name[METADATA_NAME_LENGTH];

    sprintf(environment_texture_name, "%s_%s", texture_name, "environment");
    sprintf(irradiance_texture_name, "%s_%s", texture_name, "irradiance");
    sprintf(prefiltered_texture_name, "%s_%s", texture_name, "prefiltered");
    sprintf(brdf_lut_texture_name, "%s_%s", texture_name, "brdf_lut");

    asset_t *asset_environment = (asset_t *) hashtable_search(asset_manager->asset_access_table, environment_texture_name);
    asset_t *asset_irradiance = (asset_t *) hashtable_search(asset_manager->asset_access_table, irradiance_texture_name);
    asset_t *asset_prefiltered = (asset_t *) hashtable_search(asset_manager->asset_access_table, prefiltered_texture_name);
    asset_t *asset_brdf_lut = (asset_t *) hashtable_search(asset_manager->asset_access_table, brdf_lut_texture_name);

    if (!asset_environment || !asset_irradiance || !asset_prefiltered || !asset_brdf_lut)
    {
        printf("[Asset loader] Pbr ambient asset not found or incomplete {%s} !\n", texture_name);
        return 0;
    }

    if (asset_environment->meta->type != ASSET_TEXTURE_CUBEMAP ||
        asset_irradiance->meta->type != ASSET_TEXTURE_CUBEMAP ||
        asset_prefiltered->meta->type != ASSET_TEXTURE_CUBEMAP ||
        asset_brdf_lut->meta->type != ASSET_TEXTURE2D)
    {
        printf("[Asset loader] Specified asset is not a {ASSET_TEXTURE_PBR_AMBIENT} !\n");
        return 0;
    }

    platform_api_t *API = get_platform_api();

    // Already loaded.
    if (asset_environment->status == ASSET_LOADED && asset_irradiance->status == ASSET_LOADED &&
        asset_prefiltered->status == ASSET_LOADED && asset_brdf_lut->status == ASSET_LOADED) {
        return (pbr_ambient_texture_t *) asset_environment->data;
    }

    // NOTE(gabic): If the texture will be loaded multiple times then the pbr_ambient_texture_t
    // is recreated but it will use the initial loaded assets. Maybe I will change this in the
    // future.

    MEM_DESCRIPTION(texture_name);
    pbr_ambient_texture_t *texture = gc_create_pbr_ambient_texture(flags);

    if (asset_environment->status == ASSET_NOT_LOADED)
    {
        u8 *environment_data_pointer = (u8 *) texture->environment->faces[0]->mips->header;

        gc_file_t *package_file = get_package_from_cache(asset_environment->meta->package_path);
        API->read_file(package_file, asset_environment->meta->data_offset, asset_environment->meta->data_bytes, environment_data_pointer);

        if (level)
        {
            level->assets[level->asset_count++] = asset_environment;
            SDL_assert(level->asset_count < LEVEL_MAX_ASSETS);
        }

        asset_environment->status = ASSET_LOADED;
        asset_environment->data = texture;
    }

    if (asset_irradiance->status == ASSET_NOT_LOADED)
    {
        u8 *irradiance_data_pointer = (u8 *) texture->irradiance->faces[0]->mips->header;

        gc_file_t *package_file = get_package_from_cache(asset_irradiance->meta->package_path);
        API->read_file(package_file, asset_irradiance->meta->data_offset, asset_irradiance->meta->data_bytes, irradiance_data_pointer);

        if (level)
        {
            level->assets[level->asset_count++] = asset_irradiance;
            SDL_assert(level->asset_count < LEVEL_MAX_ASSETS);
        }

        asset_irradiance->status = ASSET_LOADED;
        asset_irradiance->data = texture;
    }

    if (asset_prefiltered->status == ASSET_NOT_LOADED)
    {
        u8 *prefiltered_data_pointer = (u8 *) texture->prefiltered->faces[0]->mips->header;

        gc_file_t *package_file = get_package_from_cache(asset_prefiltered->meta->package_path);
        API->read_file(package_file, asset_prefiltered->meta->data_offset, asset_prefiltered->meta->data_bytes, prefiltered_data_pointer);

        if (level)
        {
            level->assets[level->asset_count++] = asset_prefiltered;
            SDL_assert(level->asset_count < LEVEL_MAX_ASSETS);
        }

        asset_prefiltered->status = ASSET_LOADED;
        asset_prefiltered->data = texture;
    }

    if (asset_brdf_lut->status == ASSET_NOT_LOADED)
    {
        u8 *brdf_lut_data_pointer = (u8 *) texture->brdf_lut->mips->header;

        gc_file_t *package_file = get_package_from_cache(asset_brdf_lut->meta->package_path);
        API->read_file(package_file, asset_brdf_lut->meta->data_offset, asset_brdf_lut->meta->data_bytes, brdf_lut_data_pointer);

        if (level)
        {
            level->assets[level->asset_count++] = asset_brdf_lut;
            SDL_assert(level->asset_count < LEVEL_MAX_ASSETS);
        }

        asset_brdf_lut->status = ASSET_LOADED;
        asset_brdf_lut->data = texture;
    }

    if (level)
    {
        level->textures[level->texture_count++] = texture;
        SDL_assert(level->texture_count <= LEVEL_MAX_TEXTURES);
    }

    return texture;
}

// ----------------------------------------------------------------------------------
// -- Load the specified mesh's data from the asset file.
// ----------------------------------------------------------------------------------

mesh_t *asset_load_mesh(char *mesh_name)
{
    mesh_t *mesh = 0;
    gc_asset_manager_t *asset_manager = GCSR.state->asset_manager;
    asset_t *asset = (asset_t *) hashtable_search(asset_manager->asset_access_table, mesh_name);

    if (!asset)
    {
        printf("[Asset loader] Mesh not found {%s} !\n", mesh_name);
        return 0;
    }

    if (asset->meta->type != ASSET_MESH)
    {
        printf("[Asset loader] Specified asset is not a {ASSET_MESH} !\n");
        return 0;
    }

    if (asset->status == ASSET_NOT_LOADED)
    {
        // memory_type_t current_chunk = mem_current_chunk();
        platform_api_t *API = get_platform_api();
        asset_metadata_t *asset_meta = asset->meta;

        size_t mesh_indices_bytes = MEM_SIZE_ALIGN(asset_meta->mesh.indices_bytes);
        size_t mesh_vertices_bytes = asset_meta->mesh.vertices_bytes;
        size_t mesh_bytes = sizeof(mesh_t) + mesh_indices_bytes + mesh_vertices_bytes;

        mem_set_chunk(MEMORY_ASSETS);
        MEM_LABEL("mesh_data");
        mesh = (mesh_t *) gc_mem_allocate(mesh_bytes);
        mem_restore_chunk();

        if (mesh)
        {
            asset->data = mesh;
            mesh->type = asset_meta->mesh.type;
            mesh->indices_count = asset_meta->mesh.indices;
            mesh->indices = (u32 *) (mesh + 1);
            mesh->vertices = (asset_vertex_t *) ADDR_OFFSET(mesh->indices, mesh_indices_bytes);

            gc_file_t *package_file = get_package_from_cache(asset->meta->package_path);

            API->read_file(package_file,
                           asset_meta->mesh.indices_offset,
                           asset_meta->mesh.indices_bytes,
                           mesh->indices);

            API->read_file(package_file,
                           asset_meta->mesh.vertices_offset,
                           asset_meta->mesh.vertices_bytes,
                           mesh->vertices);

            asset->status = ASSET_LOADED;
        }
    }

    return mesh;
}

// ----------------------------------------------------------------------------------
// NOTE(gabic): Kept for the normal tangent-bitangent generation (small bugs).
// ----------------------------------------------------------------------------------

// void asset_load_mesh_data(gc_asset_manager_t *asset_manager, mesh_t *mesh)
// {
//     if (!mesh || (mesh->status & ASSET_LOADED))
//         return;

//     platform_api_t *API = get_platform_api();
//     gl_asset_mesh_metadata_t *mesh_meta = mesh->meta;
//     size_t data_block_size = mesh_meta->bytes;

//     b8 generate_tangents = false;

//     if (mesh_meta->type == GL_MESH_TRIANGLE &&
//         mesh_meta->buffer_count >= 3 &&
//         mesh_meta->buffers[0].type == GL_ATTR_POS &&
//         mesh_meta->buffers[1].type == GL_ATTR_UV &&
//         mesh_meta->buffers[2].type == GL_ATTR_NORM)
//     {
//         u32 vertex_count = mesh_meta->buffers[0].count;
//         u32 tangent_buffer_size = sizeof(vec4) * vertex_count;
//         data_block_size += tangent_buffer_size;
//         generate_tangents = true;
//     }

//     u8 *data_block = (u8 *) mem_allocate_name(MEMORY_ASSETS, data_block_size, "mesh data");

//     if (data_block)
//     {
//         mesh->type = mesh_meta->type;
//         mesh->data = data_block;
//         mesh->primitives = data_block;

//         // ----------------------------------------------------------------------------------
//         // -- Read the mesh data from the asset file.
//         // ----------------------------------------------------------------------------------

//         API->read_file(asset_manager->asset_file,
//                             mesh_meta->primitives.data_offset,
//                             mesh_meta->primitives.bytes,
//                             mesh->primitives);

//         data_block += mesh_meta->primitives.bytes;

//         size_t last_bytes = 0;
//         u32 last_buffer = 0;

//         for (u32 i = 0; i < mesh_meta->buffer_count; ++i)
//         {
//             gl_asset_mesh_metadata_attributes_t *buffer = &mesh_meta->buffers[i];

//             mesh->attributes[i] = data_block + last_bytes;
//             mesh->attribute_structure[i].components = buffer->components;
//             mesh->attribute_structure[i].size = buffer->line_size;
//             mesh->attribute_structure[i].use_position_index = false;

//             API->read_file(asset_manager->asset_file,
//                                 buffer->data_offset,
//                                 buffer->bytes,
//                                 mesh->attributes[i]);

//             last_bytes += buffer->bytes;
//             last_buffer++;
//         }

//         // ----------------------------------------------------------------------------------
//         // -- Tangent buffer generation.
//         // ----------------------------------------------------------------------------------

//         if (generate_tangents)
//         {
//             mesh->attributes[last_buffer] = data_block + last_bytes;
//             mesh->attribute_structure[last_buffer].components = 4;
//             mesh->attribute_structure[last_buffer].size = sizeof(vec4);
//             mesh->attribute_structure[last_buffer].use_position_index = true;

//             vec4 *vertex_tangent = (vec4 *) mesh->attributes[last_buffer];

//             u32 primitive_count = mesh_meta->primitives.count;
//             u32 attr_count = mesh_meta->buffer_count;
//             u32 primitive_stride = 3 * attr_count;

//             u32 *primitives = (u32 *) mesh->primitives;
//             u32 vertex_count = mesh_meta->buffers[0].count;
//             vec3 *vertices = (vec3 *) mesh->attributes[0];
//             vec2 *uv = (vec2 *) mesh->attributes[1];
//             vec3 *norm = (vec3 *) mesh->attributes[2];

//             size_t tmp_buffers_size = 2 * sizeof(vec3) * vertex_count;
//             vec3 *tangent_buffer = (vec3 *) gc_mem_allocate(MEMORY_TEMPORARY, tmp_buffers_size);
//             u32 *vertex_normal_index_buffer = (u32 *) gc_mem_allocate(MEMORY_TEMPORARY, vertex_count * sizeof(u32));
//             vec3 *bitangent_buffer = tangent_buffer + vertex_count;

//             for (u32 i = 0; i < primitive_count; ++i)
//             {
//                 u32 *indices_1 = primitives;
//                 u32 *indices_2 = indices_1 + attr_count;
//                 u32 *indices_3 = indices_2 + attr_count;

//                 u32 pos_idx1 = indices_1[0] - 1;
//                 u32 pos_idx2 = indices_2[0] - 1;
//                 u32 pos_idx3 = indices_3[0] - 1;

//                 u32 uv_idx1 = indices_1[1] - 1;
//                 u32 uv_idx2 = indices_2[1] - 1;
//                 u32 uv_idx3 = indices_3[1] - 1;

//                 u32 norm_idx1 = indices_1[2] - 1;
//                 u32 norm_idx2 = indices_2[2] - 1;
//                 u32 norm_idx3 = indices_3[2] - 1;

//                 vertex_normal_index_buffer[pos_idx1] = norm_idx1;
//                 vertex_normal_index_buffer[pos_idx2] = norm_idx2;
//                 vertex_normal_index_buffer[pos_idx3] = norm_idx3;

//                 vec3 *v1 = vertices + pos_idx1;
//                 vec3 *v2 = vertices + pos_idx2;
//                 vec3 *v3 = vertices + pos_idx3;

//                 vec2 *uv1 = uv + uv_idx1;
//                 vec2 *uv2 = uv + uv_idx2;
//                 vec2 *uv3 = uv + uv_idx3;

//                 primitives += 3 * attr_count;

//                 vec3 e1 = vec3_sub(*v2, *v1);
//                 vec3 e2 = vec3_sub(*v3, *v1);

//                 r32 du1 = uv2->u - uv1->u;
//                 r32 dv1 = uv2->v - uv1->v;
//                 r32 du2 = uv3->u - uv1->u;
//                 r32 dv2 = uv3->v - uv1->v;

//                 if (du1 == du2)
//                     du2 += 0.0001f;

//                 if (dv1 == dv2)
//                     dv2 += 0.0001f;

//                 // if (!du1 && !dv1)
//                 //     du1 += 0.0001f;

//                 // if (!du2 && !dv2)
//                 //     du2 += 0.0001f;

//                 r32 r = 1.0f / (du1 * dv2 - dv1 * du2);

//                 vec3 t = {
//                     (dv2 * e1.x - dv1 * e2.x) * r,
//                     (dv2 * e1.y - dv1 * e2.y) * r,
//                     (dv2 * e1.z - dv1 * e2.z) * r,
//                 };

//                 vec3 b = {
//                     (-du2 * e1.x + du1 * e2.x) * r,
//                     (-du2 * e1.y + du1 * e2.y) * r,
//                     (-du2 * e1.z + du1 * e2.z) * r,
//                 };

//                 vec3 pre = {
//                     tangent_buffer[pos_idx1].x,
//                     tangent_buffer[pos_idx1].y,
//                     tangent_buffer[pos_idx1].z
//                 };

//                 tangent_buffer[pos_idx1] = vec3_add(tangent_buffer[pos_idx1], t);
//                 tangent_buffer[pos_idx2] = vec3_add(tangent_buffer[pos_idx2], t);
//                 tangent_buffer[pos_idx3] = vec3_add(tangent_buffer[pos_idx3], t);
//                 bitangent_buffer[pos_idx1] = vec3_add(bitangent_buffer[pos_idx1], b);
//                 bitangent_buffer[pos_idx2] = vec3_add(bitangent_buffer[pos_idx2], b);
//                 bitangent_buffer[pos_idx3] = vec3_add(bitangent_buffer[pos_idx3], b);

//                 SDL_assert(tangent_buffer[pos_idx1].x || tangent_buffer[pos_idx1].y || tangent_buffer[pos_idx1].z);
//                 SDL_assert(tangent_buffer[pos_idx2].x || tangent_buffer[pos_idx2].y || tangent_buffer[pos_idx2].z);
//                 SDL_assert(tangent_buffer[pos_idx3].x || tangent_buffer[pos_idx3].y || tangent_buffer[pos_idx3].z);
//             }

//             for (u32 i = 0; i < vertex_count; ++i, ++vertex_tangent)
//             {
//                 vec3 t = tangent_buffer[i];
//                 vec3 b = bitangent_buffer[i];
//                 u32 nidx = vertex_normal_index_buffer[i];
//                 vec3 n = norm[nidx];

//                 SDL_assert(t.x || t.y || t.z);

//                 vec3 tmpv = force_perp(t, n);
//                 t = vec3_normalize(tmpv);

//                 vec3 tmp = vec3_cross(t, b);
//                 vec3 ntmp = vec3_normalize(tmp);
//                 r32 check = vec3_dot(tmp, n);

//                 vertex_tangent->x = t.x;
//                 vertex_tangent->y = t.y;
//                 vertex_tangent->z = t.z;
//                 vertex_tangent->w = check < 0 ? -1 : 1;
//             }

//             gc_mem_free(tangent_buffer);
//             gc_mem_free(vertex_normal_index_buffer);
//         }

//         mesh->status = ASSET_LOADED;
//     }

//     // -- Generate extra meshes.

//     if (mesh->type == GL_MESH_TRIANGLE)
//     {
//         asset_generate_line_mesh(mesh);
//         asset_generate_point_mesh(mesh);
//     }
//     else if (mesh->type == GL_MESH_LINE) {
//         asset_generate_point_mesh(mesh);
//     }
// }

// ----------------------------------------------------------------------------------
// -- Load a specified level file.
// ----------------------------------------------------------------------------------

void *parse_level(gc_asset_manager_t *asset_manager, gc_file_t *File);
gc_level_t *parse_level_json(char *filepath);

void asset_load_level(char *name)
{
    char filepath[100] = "data/levels/";
    strcat(filepath, name);

    gc_level_t *level = parse_level_json(filepath);
    GCSR.state->level = level;
    init_camera(&GCSR.gl->camera);

    if (level)
    {
        // Push the models into the queue.
        for (u32 i = 0; i < level->model_count; ++i) {
            PUSH_TRIANGLE(level->models + i);
        }

        init_camera(&GCSR.gl->camera);
        b8 shadow_enabled = FLAG(level->settings.flags, GC_SHADOW);

        // Shadow framebuffer.
        if (shadow_enabled)
            gc_create_framebuffer(level->settings.shadow_map_size, level->settings.shadow_map_size, FB_FLAG_LSB, 2);

        // Create a shadow texture for each light.
        for (u8 i = 0; i < level->light_count; ++i)
        {
            gc_light_t *current_light = level->lights + i;

            if (shadow_enabled)
            {
                if (current_light->type == GC_POINT_LIGHT)
                {
                    current_light->shadow.f_len_inv = 1.0f / (current_light->shadow.f_far - current_light->shadow.f_near);
                    MEM_DESCRIPTION("point_light_shadow_map");
                    current_light->shadow_texture = gc_create_cubemap_texture(level->settings.shadow_map_size,
                                                                              level->settings.shadow_map_size,
                                                                              0, TEXTURE_FORMAT_RGF, current_light->shadow.flags);
                }
                else if (current_light->type == GC_SUN_LIGHT)
                {
                    GCSR.gl->pipeline.params.sun_light = current_light;
                    current_light->shadow.f_len_inv = 1.0f / (current_light->shadow.f_far - current_light->shadow.f_near);
                    MEM_DESCRIPTION("sun_light_shadow_map");
                    current_light->shadow_texture = gc_create_texture2d(level->settings.shadow_map_size,
                                                                        level->settings.shadow_map_size,
                                                                        0, TEXTURE_FORMAT_RGF, current_light->shadow.flags);
                }
            }
        }

        GCSR.state->update_params = level;
    }
}

void asset_unload(asset_t *asset)
{
    if (asset && (asset->status & (ASSET_LOADED | ASSET_LOADED_BUT_NOT_USED)))
    {
        asset->status = ASSET_NOT_LOADED;
        gc_mem_free(asset->data);
    }
}

// void asset_unload_mesh_data(mesh_t *mesh)
// {
//     if (mesh && (mesh->status & (ASSET_LOADED | ASSET_LOADED_BUT_NOT_USED)))
//     {
//         mesh->status = ASSET_NOT_LOADED;
//         mem_start_mark_free(MEMORY_ASSETS);
//         gc_mem_free(mesh->data);
//         mem_end_mark_free();
//     }
// }

void asset_unload_level()
{
    gc_level_t *level = GCSR.state->level;

    if (!level)
        return;

    for (u32 i = 0; i < level->texture_count; ++i)
    {
        void *texture = level->textures[i];
        gc_mem_free(texture);
    }

    for (u32 i = 0; i < level->mesh_count; ++i)
    {
        void *mesh = level->meshes[i];
        gc_mem_free(mesh);
    }

    for (u32 i = 0; i < level->asset_count; ++i)
    {
        asset_t *asset = level->assets[i];
        asset->status = ASSET_NOT_LOADED;
        asset->data = 0;
    }

    gc_mem_free(level);
    GCSR.state->level = 0;
}

// ----------------------------------------------------------------------------------
// -- LEVEL parser.
// ----------------------------------------------------------------------------------

typedef enum
{
    CATEGORY_NONE = 1,
    CATEGORY_MATERIAL,
    CATEGORY_MODEL,
    CATEGORY_LIGHT,
} DataCategory;

void _init_default_level(gc_level_t *level)
{}

gc_level_t *parse_level_json(char *filepath)
{
    gc_level_t *level = 0;
    gc_file_t level_file;
    void *file_contents = 0;

    mem_set_chunk(MEMORY_TEMPORARY);
    platform_api_t *API = get_platform_api();
    API->open_file(&level_file, filepath, GC_FILE_READ);

    if (level_file.handle)
    {
        MEM_LABEL("level_file");
        file_contents = gc_mem_allocate(level_file.bytes);
        API->read_file(&level_file, 0, level_file.bytes, file_contents);
    }

    struct _json_object_entry *settings = 0;
    struct _json_object_entry *packages = 0;
    struct _json_object_entry *textures = 0;
    struct _json_object_entry *meshes = 0;
    struct _json_object_entry *materials = 0;
    struct _json_object_entry *models = 0;
    struct _json_object_entry *lights = 0;

    if (file_contents)
    {
        char error_buf[json_error_max];
        json_value *json = json_parse((json_char *) file_contents, level_file.bytes, error_buf);

        if (json)
        {
            u32 material_count = 0;
            u32 model_count = 0;
            u32 light_count = 0;

            JSON_VALUE_OBJECT_LOOP(json, pi)
            {
                struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(json, pi);

                if (JSON_PROPERTY_NAME_EQUALS(prop, "settings"))
                    settings = prop;
                else if (JSON_PROPERTY_NAME_EQUALS(prop, "packages"))
                    packages = prop;
                else if (JSON_PROPERTY_NAME_EQUALS(prop, "textures"))
                    textures = prop;
                else if (JSON_PROPERTY_NAME_EQUALS(prop, "meshes"))
                    meshes = prop;
                else if (JSON_PROPERTY_NAME_EQUALS(prop, "materials"))
                {
                    materials = prop;
                    material_count = JSON_ARRAY_LENGTH(materials->value);
                }
                else if (JSON_PROPERTY_NAME_EQUALS(prop, "models"))
                {
                    models = prop;
                    model_count = JSON_ARRAY_LENGTH(models->value);
                }
                else if (JSON_PROPERTY_NAME_EQUALS(prop, "lights"))
                {
                    lights = prop;
                    light_count = JSON_ARRAY_LENGTH(lights->value);
                }
            }

            if (!settings)
            {
                printf("[Asset loader] The level is missing the settings information {%s} !\n", filepath);
                goto end_level;
            }

            size_t materials_bytes = material_count * sizeof(gc_material_t);
            size_t models_bytes = model_count * sizeof(gc_model_t);
            size_t lights_bytes = light_count * sizeof(gc_light_t);
            size_t level_bytes = sizeof(gc_level_t) + materials_bytes + lights_bytes;

            MEM_LABEL("level");
            level = (gc_level_t *) gc_mem_allocate(level_bytes);

            level->shader_id = SHADER_NONE;
            level->program_id = PRG_NONE;

            level->missing_assets = false;
            level->texture_count = 0;
            level->mesh_count = 0;

            level->material_count = material_count;
            level->light_count = light_count;
            level->model_count = model_count;

            level->materials = (gc_material_t *) (level + 1);
            level->lights = (gc_light_t *) ADDR_OFFSET(level->materials, materials_bytes);

            MEM_LABEL("models");
            level->models = (gc_model_t *) gc_mem_allocate(models_bytes);

            // ----------------------------------------------------------------------------------
            // -- Default values.
            // ----------------------------------------------------------------------------------

            level->settings.current_mode_cycle = -1;
            level->settings.flags = GC_MODE_NORMAL | GC_MODE_RENDERED | GC_BACKFACE_CULL;
            level->settings.debug_lights = false;
            level->settings.debug_grid = false;
            level->settings.shadow_map_size = 512;
            // level->settings.tone_mapping = TONE_MAPPING_CLAMP;
            // VSET4(level->settings.background_color, 0.1f, 0.1f, 0.1f, 1.0f);
            // VSET4(level->settings.solid_color, 0.5f, 0.5f, 0.5f, 1.0f);
            // VSET4(level->settings.ambient_color, 0.05f, 0.05f, 0.05f, 1.0f);
            // level->settings.postprocessing.enabled = false;
            // VSET3(level->settings.postprocessing.tint, 1.0f, 1.0f, 1.0f);
            // level->settings.postprocessing.saturation = 1.0f;
            level->settings.default_material.components = SHADER_FLAG_BLINN_DIFFUSE;

            // gl_gamma_srgb_to_linear(&level->settings.background_color);
            // gl_gamma_srgb_to_linear(&level->settings.wireframe_color);
            // gl_gamma_srgb_to_linear(&level->settings.solid_color);
            // gl_gamma_srgb_to_linear(&level->settings.ambient_color);
            // gl_gamma_srgb_to_linear(&level->settings.postprocessing.tint);

            default_camera(&GCSR.gl->camera);

            // ----------------------------------------------------------------------------------
            // -- Settings.
            // ----------------------------------------------------------------------------------

            JSON_VALUE_OBJECT_LOOP(settings->value, mi)
            {
                struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(settings->value, mi);

                if (JSON_PROPERTY_NAME_EQUALS(prop, "name")) {
                    strncpy(level->settings.name, JSON_PROPERTY_VALUE_STRING(prop), 64);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "debug_lights") && JSON_PROPERTY_TYPE_EQUALS(prop, json_boolean)) {
                    level->settings.debug_lights = JSON_PROPERTY_VALUE_BOOL(prop);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "debug_grid") && JSON_PROPERTY_TYPE_EQUALS(prop, json_boolean)) {
                    level->settings.debug_grid = JSON_PROPERTY_VALUE_BOOL(prop);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "shadow_map_size") && JSON_PROPERTY_TYPE_EQUALS(prop, json_integer)) {
                    level->settings.shadow_map_size = JSON_PROPERTY_VALUE_INTEGER(prop);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "tone_mapping") && JSON_PROPERTY_TYPE_EQUALS(prop, json_string))
                {
                    if (strcmp(JSON_VALUE_STRING(prop->value), "clamp") == 0)
                    {
                        level->settings.overwrites.tone_mapping.overwrite = true;
                        level->settings.overwrites.tone_mapping.value.u_integer = TONE_MAPPING_CLAMP;
                    }

                    else if (strcmp(JSON_VALUE_STRING(prop->value), "reinhard") == 0)
                    {
                        level->settings.overwrites.tone_mapping.overwrite = true;
                        level->settings.overwrites.tone_mapping.value.u_integer = TONE_MAPPING_REINHARD;
                    }

                    else if (strcmp(JSON_VALUE_STRING(prop->value), "filmic") == 0)
                    {
                        level->settings.overwrites.tone_mapping.overwrite = true;
                        level->settings.overwrites.tone_mapping.value.u_integer = TONE_MAPPING_FILMIC;
                    }

                    else if (strcmp(JSON_VALUE_STRING(prop->value), "aces") == 0)
                    {
                        level->settings.overwrites.tone_mapping.overwrite = true;
                        level->settings.overwrites.tone_mapping.value.u_integer = TONE_MAPPING_ACES;
                    }

                    else if (strcmp(JSON_VALUE_STRING(prop->value), "aces_aprox") == 0)
                    {
                        level->settings.overwrites.tone_mapping.overwrite = true;
                        level->settings.overwrites.tone_mapping.value.u_integer = TONE_MAPPING_ACES_APROX;
                    }
                }

                else if (JSON_PROPERTY_COMPARE(prop, "shader", json_integer))
                {
                    u32 shader_id = JSON_PROPERTY_VALUE_INTEGER(prop);

                    if (shader_id > SHADER_NONE && shader_id < SHADER_COUNT)
                        level->shader_id = (shader_id_t) shader_id;
                }

                else if (JSON_PROPERTY_COMPARE(prop, "program", json_integer))
                {
                    u32 program_id = JSON_PROPERTY_VALUE_INTEGER(prop);

                    if (program_id > PRG_NONE && program_id < PRG_COUNT)
                        level->program_id = (program_id_t) program_id;
                }

                else if (JSON_PROPERTY_COMPARE(prop, "program_settings", json_object)) {
                    level->program_settings = prop;
                }

                // ----------------------------------------------------------------------------------
                // -- Flags.
                // ----------------------------------------------------------------------------------

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "flags") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_object))
                {
                    u32 mode_flags = GC_MODE_SOLID | GC_MODE_MATERIAL | GC_MODE_RENDERED;

                    JSON_VALUE_OBJECT_LOOP(prop->value, fi)
                    {
                        struct _json_object_entry *flag_prop = JSON_OBJECT_PROPERTY(prop->value, fi);

                        if (JSON_PROPERTY_COMPARE(flag_prop, "mode_normal", json_boolean))
                        {
                            if (JSON_PROPERTY_VALUE_BOOL(flag_prop) == true)
                            {
                                level->settings.flags |= GC_MODE_NORMAL;

                                if (!(level->settings.flags & GC_MODE_SOLID) && !(level->settings.flags & GC_MODE_MATERIAL))
                                    level->settings.flags |= GC_MODE_RENDERED;
                            }
                            else
                                level->settings.flags &= ~(GC_MODE_NORMAL | GC_MODE_RENDERED);
                        }

                        else if (JSON_PROPERTY_COMPARE(flag_prop, "mode_wireframe", json_boolean))
                        {
                            if (JSON_PROPERTY_VALUE_BOOL(flag_prop) == true)
                                level->settings.flags |= GC_MODE_WIREFRAME;
                            else
                                level->settings.flags &= ~GC_MODE_WIREFRAME;
                        }

                        else if (JSON_PROPERTY_COMPARE(flag_prop, "mode_point", json_boolean))
                        {
                            if (JSON_PROPERTY_VALUE_BOOL(flag_prop) == true)
                                level->settings.flags |= GC_MODE_POINT;
                            else
                                level->settings.flags &= ~GC_MODE_POINT;
                        }

                        else if (JSON_PROPERTY_COMPARE(flag_prop, "mode_solid", json_boolean))
                        {
                            if (JSON_PROPERTY_VALUE_BOOL(flag_prop) == true)
                            {
                                level->settings.flags &= ~mode_flags;
                                level->settings.flags |= GC_MODE_NORMAL | GC_MODE_SOLID;
                            }
                            else
                                level->settings.flags &= ~GC_MODE_SOLID;
                        }

                        else if (JSON_PROPERTY_COMPARE(flag_prop, "mode_material", json_boolean))
                        {
                            if (JSON_PROPERTY_VALUE_BOOL(flag_prop) == true)
                            {
                                level->settings.flags &= ~mode_flags;
                                level->settings.flags |= GC_MODE_NORMAL | GC_MODE_MATERIAL;
                            }
                            else
                                level->settings.flags &= ~GC_MODE_MATERIAL;
                        }

                        // NOTE(gabic): Nu mai retin care e motivul pentru "mode_rendered"
                        else if (JSON_PROPERTY_COMPARE(flag_prop, "mode_rendered", json_boolean))
                        {
                            if (JSON_PROPERTY_VALUE_BOOL(flag_prop) == true)
                            {
                                level->settings.flags &= ~mode_flags;
                                level->settings.flags |= GC_MODE_NORMAL | GC_MODE_RENDERED;
                            }
                            else
                                level->settings.flags &= ~GC_MODE_RENDERED;
                        }

                        else if (JSON_PROPERTY_COMPARE(flag_prop, "shadow", json_boolean) && JSON_PROPERTY_VALUE_BOOL(flag_prop)) {
                            level->settings.flags |= GC_SHADOW;
                        }
                    }
                }

                // ----------------------------------------------------------------------------------
                // -- Post processing.
                // ----------------------------------------------------------------------------------

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "postprocessing") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_object))
                {
                    JSON_VALUE_OBJECT_LOOP(prop->value, ppi)
                    {
                        struct _json_object_entry *post_prop = JSON_OBJECT_PROPERTY(prop->value, ppi);

                        if (JSON_PROPERTY_COMPARE(post_prop, "enabled", json_boolean) && JSON_PROPERTY_VALUE_BOOL(post_prop))
                        {
                            level->settings.overwrites.postprocessing.overwrite = true;
                            level->settings.overwrites.postprocessing.value.u_bool = true;

                            level->settings.flags |= GC_POST_PROCESSING;
                        }

                        else if (JSON_PROPERTY_COMPARE(post_prop, "tint", json_array) && JSON_ARRAY_LENGTH(post_prop->value) == 3)
                        {
                            level->settings.overwrites.tint_color.overwrite = true;
                            level->settings.overwrites.tint_color.value.u_vector.c.r = JSON_ARRAY_VALUE_FLOAT(post_prop->value, 0);
                            level->settings.overwrites.tint_color.value.u_vector.c.g = JSON_ARRAY_VALUE_FLOAT(post_prop->value, 1);
                            level->settings.overwrites.tint_color.value.u_vector.c.b = JSON_ARRAY_VALUE_FLOAT(post_prop->value, 2);

                            gl_gamma_srgb_to_linear(&level->settings.overwrites.tint_color.value.u_vector);
                        }

                        else if (JSON_PROPERTY_COMPARE(post_prop, "saturation", json_double))
                        {
                            level->settings.overwrites.saturation.overwrite = true;
                            level->settings.overwrites.saturation.value.u_float = JSON_PROPERTY_VALUE_DOUBLE(post_prop);
                        }
                    }
                }

                // ----------------------------------------------------------------------------------
                // -- Camera.
                // ----------------------------------------------------------------------------------

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "camera") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_object))
                {
                    JSON_VALUE_OBJECT_LOOP(prop->value, pi)
                    {
                        struct _json_object_entry *camera_prop = JSON_OBJECT_PROPERTY(prop->value, pi);

                        if (JSON_PROPERTY_COMPARE(camera_prop, "type", json_string))
                        {
                            if (strcmp(JSON_VALUE_STRING(camera_prop->value), "focus") == 0)
                                GCSR.gl->camera.type = GC_CAMERA_FOCUS_POINT;

                            else if (strcmp(JSON_VALUE_STRING(camera_prop->value), "basic") == 0)
                                GCSR.gl->camera.type = GC_CAMERA_BASIC;
                        }

                        else if (JSON_PROPERTY_COMPARE(camera_prop, "eye", json_array) && JSON_ARRAY_LENGTH(camera_prop->value) == 3)
                        {
                            GCSR.gl->camera.origin_eye.v3.x = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 0);
                            GCSR.gl->camera.origin_eye.v3.y = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 1);
                            GCSR.gl->camera.origin_eye.v3.z = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 2);
                        }

                        else if (JSON_PROPERTY_COMPARE(camera_prop, "target", json_array) && JSON_ARRAY_LENGTH(camera_prop->value) == 3)
                        {
                            GCSR.gl->camera.origin_target.v3.x = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 0);
                            GCSR.gl->camera.origin_target.v3.y = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 1);
                            GCSR.gl->camera.origin_target.v3.z = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 2);
                        }

                        // NOTE(gabic): Cred ca merge scoasa asta.
                        else if (JSON_PROPERTY_COMPARE(camera_prop, "up", json_array) && JSON_ARRAY_LENGTH(camera_prop->value) == 3)
                        {
                            GCSR.gl->camera.up.v3.x = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 0);
                            GCSR.gl->camera.up.v3.y = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 1);
                            GCSR.gl->camera.up.v3.z = JSON_ARRAY_VALUE_FLOAT(camera_prop->value, 2);
                        }

                        else if (JSON_PROPERTY_COMPARE(camera_prop, "dampening", json_double)) {
                            GCSR.gl->camera.dampening = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (JSON_PROPERTY_COMPARE(camera_prop, "h_sens", json_double)) {
                            GCSR.gl->camera.h_sens = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (JSON_PROPERTY_COMPARE(camera_prop, "v_sens", json_double)) {
                            GCSR.gl->camera.v_sens = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (GCSR.gl->camera.type == GC_CAMERA_BASIC && JSON_PROPERTY_COMPARE(camera_prop, "global_sens", json_double)) {
                            GCSR.gl->camera.global_sens = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (GCSR.gl->camera.type == GC_CAMERA_FOCUS_POINT && JSON_PROPERTY_COMPARE(camera_prop, "wheel_sens", json_double)) {
                            GCSR.gl->camera.wheel_sens = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (GCSR.gl->camera.type == GC_CAMERA_FOCUS_POINT && JSON_PROPERTY_COMPARE(camera_prop, "orbit_h_sens", json_double)) {
                            GCSR.gl->camera.orbit_h_sens = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (GCSR.gl->camera.type == GC_CAMERA_FOCUS_POINT && JSON_PROPERTY_COMPARE(camera_prop, "orbit_v_sens", json_double)) {
                            GCSR.gl->camera.orbit_v_sens = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (JSON_PROPERTY_COMPARE(camera_prop, "projection", json_string))
                        {
                            if (strcmp(JSON_VALUE_STRING(camera_prop->value), "perspective") == 0)
                                GCSR.gl->camera.projection.type = GC_PROJECTION_PERSPECTIVE;
                            else if (strcmp(JSON_VALUE_STRING(camera_prop->value), "orthographic") == 0)
                                GCSR.gl->camera.projection.type = GC_PROJECTION_ORTHOGRAPHIC;
                        }

                        if (JSON_PROPERTY_COMPARE(camera_prop, "fov", json_integer))
                            GCSR.gl->camera.projection.fov = JSON_PROPERTY_VALUE_INTEGER(camera_prop);

                        if (GCSR.gl->camera.projection.type == GC_PROJECTION_PERSPECTIVE)
                        {
                            if (JSON_PROPERTY_COMPARE(camera_prop, "near", json_double))
                                GCSR.gl->camera.projection.perspective.f_near = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                            else if (JSON_PROPERTY_COMPARE(camera_prop, "far", json_double))
                                GCSR.gl->camera.projection.perspective.f_far = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }

                        else if (GCSR.gl->camera.projection.type == GC_PROJECTION_ORTHOGRAPHIC)
                        {
                            if (JSON_PROPERTY_COMPARE(camera_prop, "top", json_double))
                                GCSR.gl->camera.projection.orthographic.f_top = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                            else if (JSON_PROPERTY_COMPARE(camera_prop, "bottom", json_double))
                                GCSR.gl->camera.projection.orthographic.f_bottom = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                            else if (JSON_PROPERTY_COMPARE(camera_prop, "left", json_double))
                                GCSR.gl->camera.projection.orthographic.f_left = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                            else if (JSON_PROPERTY_COMPARE(camera_prop, "right", json_double))
                                GCSR.gl->camera.projection.orthographic.f_right = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                            else if (JSON_PROPERTY_COMPARE(camera_prop, "near", json_double))
                                GCSR.gl->camera.projection.orthographic.f_near = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                            else if (JSON_PROPERTY_COMPARE(camera_prop, "far", json_double))
                                GCSR.gl->camera.projection.orthographic.f_far = JSON_PROPERTY_VALUE_DOUBLE(camera_prop);
                        }
                    }
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "background_color") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_array) &&
                         JSON_ARRAY_LENGTH(prop->value) == 4)
                {
                    level->settings.overwrites.background_color.overwrite = true;

                    _json_prop_extract_v4(&level->settings.overwrites.background_color.value.u_vector, prop);
                    gl_gamma_srgb_to_linear(&level->settings.overwrites.background_color.value.u_vector);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "wireframe_color") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_array) &&
                         JSON_ARRAY_LENGTH(prop->value) == 4)
                {
                    level->settings.flags |= GC_MESH_WIREFRAME_COLOR;
                    level->settings.overwrites.wireframe_color.overwrite = true;

                    _json_prop_extract_v4(&level->settings.overwrites.wireframe_color.value.u_vector, prop);
                    gl_gamma_srgb_to_linear(&level->settings.overwrites.wireframe_color.value.u_vector);
                    PRE_MULT_ALPHA(level->settings.overwrites.wireframe_color.value.u_vector);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "point_color") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_array) &&
                         JSON_ARRAY_LENGTH(prop->value) == 4)
                {
                    level->settings.flags |= GC_MESH_POINT_COLOR;
                    level->settings.overwrites.point_color.overwrite = true;

                    _json_prop_extract_v4(&level->settings.overwrites.point_color.value.u_vector, prop);
                    gl_gamma_srgb_to_linear(&level->settings.overwrites.point_color.value.u_vector);
                    PRE_MULT_ALPHA(level->settings.overwrites.point_color.value.u_vector);
                }

                if (JSON_PROPERTY_COMPARE(prop, "point_radius", json_integer))
                {
                    level->settings.overwrites.point_radius.overwrite = true;
                    level->settings.overwrites.point_radius.value.u_integer = JSON_PROPERTY_VALUE_INTEGER(prop);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "solid_color") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_array) &&
                         JSON_ARRAY_LENGTH(prop->value) == 4)
                {
                    level->settings.overwrites.solid_color.overwrite = true;

                    _json_prop_extract_v4(&level->settings.overwrites.solid_color.value.u_vector, prop);
                    gl_gamma_srgb_to_linear(&level->settings.overwrites.solid_color.value.u_vector);
                }

                else if (JSON_PROPERTY_NAME_EQUALS(prop, "ambient_color") &&
                         JSON_PROPERTY_TYPE_EQUALS(prop, json_array) &&
                         JSON_ARRAY_LENGTH(prop->value) == 4)
                {
                    level->settings.overwrites.ambient_color.overwrite = true;

                    _json_prop_extract_v4(&level->settings.overwrites.ambient_color.value.u_vector, prop);
                    gl_gamma_srgb_to_linear(&level->settings.overwrites.ambient_color.value.u_vector);
                }
            }
        }
        else
        {
            printf("[ERROR] {%s} %s\n", filepath, error_buf);
            goto end_level;
        }
    }
    else
    {
        printf("[Asset loader] File not found {%s} !\n", filepath);
        goto end_level;
    }

    // ----------------------------------------------------------------------------------
    // -- Packages.
    // ----------------------------------------------------------------------------------

    asset_init_asset_manager();

    if (packages)
    {
        u32 package_count = JSON_ARRAY_LENGTH(packages->value);

        for (u32 pi = 0; pi < package_count; ++pi)
        {
            struct _json_value *package = JSON_ARRAY_VALUE(packages->value, pi);

            if (package->type == json_string)
                load_package(package->u.string.ptr);
            else
                printf("[Asset loader] Invalid package \n");
        }
    }

    generate_asset_access_tables();

    // ----------------------------------------------------------------------------------
    // -- Textures.
    // ----------------------------------------------------------------------------------

    if (textures)
    {
        u32 texture_count = JSON_ARRAY_LENGTH(textures->value);

        for (u32 ti = 0; ti < texture_count; ++ti)
        {
            struct _json_value *texture = JSON_ARRAY_VALUE(textures->value, ti);
            u32 type = ASSET_TEXTURE2D;
            char *texture_name = 0;
            u32 texture_flags = 0;
            u8 max_mip = 0;

            JSON_VALUE_OBJECT_LOOP(texture, tmi)
            {
                struct _json_object_entry *texture_prop = JSON_OBJECT_PROPERTY(texture, tmi);

                if (JSON_PROPERTY_NAME_EQUALS(texture_prop, "type") && JSON_PROPERTY_TYPE_EQUALS(texture_prop, json_string))
                {
                    if (strcmp(JSON_PROPERTY_VALUE_STRING(texture_prop), "cubemap") == 0)
                        type = ASSET_TEXTURE_CUBEMAP;
                    else if (strcmp(JSON_PROPERTY_VALUE_STRING(texture_prop), "pbr_ambient") == 0)
                        type = ASSET_TEXTURE_PBR_AMBIENT;
                    else
                        type = ASSET_TEXTURE2D;
                }

                else if (JSON_PROPERTY_NAME_EQUALS(texture_prop, "name") && JSON_PROPERTY_TYPE_EQUALS(texture_prop, json_string))
                    texture_name = JSON_PROPERTY_VALUE_STRING(texture_prop);

                else if (JSON_PROPERTY_NAME_EQUALS(texture_prop, "wrap") && JSON_PROPERTY_TYPE_EQUALS(texture_prop, json_string))
                {
                    if (strcmp(JSON_PROPERTY_VALUE_STRING(texture_prop), "clamp") == 0)
                        texture_flags |= TEXTURE_WRAP_CLAMP;
                    else if (strcmp(JSON_PROPERTY_VALUE_STRING(texture_prop), "repeat") == 0)
                        texture_flags |= TEXTURE_WRAP_REPEAT;
                    else if (strcmp(JSON_PROPERTY_VALUE_STRING(texture_prop), "mirror") == 0)
                        texture_flags |= TEXTURE_WRAP_MIRROR;
                }

                else if (JSON_PROPERTY_NAME_EQUALS(texture_prop, "filter") && JSON_PROPERTY_TYPE_EQUALS(texture_prop, json_boolean) && JSON_PROPERTY_VALUE_BOOL(texture_prop))
                    texture_flags |= TEXTURE_FILTER;
                else if (JSON_PROPERTY_NAME_EQUALS(texture_prop, "mip") && JSON_PROPERTY_TYPE_EQUALS(texture_prop, json_boolean) && JSON_PROPERTY_VALUE_BOOL(texture_prop))
                    texture_flags |= TEXTURE_MIPS;
                else if (JSON_PROPERTY_NAME_EQUALS(texture_prop, "mip_filter") && JSON_PROPERTY_TYPE_EQUALS(texture_prop, json_boolean) && JSON_PROPERTY_VALUE_BOOL(texture_prop))
                    texture_flags |= TEXTURE_MIPS_FILTER;
                else if (JSON_PROPERTY_NAME_EQUALS(texture_prop, "max_mip") && JSON_PROPERTY_TYPE_EQUALS(texture_prop, json_integer))
                    max_mip = (u8) JSON_PROPERTY_VALUE_INTEGER(texture_prop);
            }

            // Load the texture.
            if (texture_name)
            {
                void *tex_pointer = 0;

                if (!texture_flags)
                    texture_flags |= TEXTURE_WRAP_CLAMP;

                if (type == ASSET_TEXTURE2D)
                {
                    tex_pointer = asset_load_texture2d(texture_name, texture_flags, level);

                    if (max_mip)
                        ((texture2d_t *) tex_pointer)->mip_count = max_mip;
                }
                else if (type == ASSET_TEXTURE_CUBEMAP)
                {
                    tex_pointer = asset_load_cube_texture(texture_name, texture_flags, level);

                    if (max_mip)
                        ((cube_texture_t *) tex_pointer)->mip_count = max_mip;
                }
                else if (type == ASSET_TEXTURE_PBR_AMBIENT)
                    tex_pointer = asset_load_pbr_ambient_texture(texture_name, texture_flags, level);

                if (!tex_pointer)
                    level->missing_assets = true;
            }
            else
                printf("Texture has no \"name\" property specified !\n");
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Meshes.
    // ----------------------------------------------------------------------------------

    if (meshes)
    {
        u32 mesh_count = JSON_ARRAY_LENGTH(meshes->value);

        for (u32 mi = 0; mi < mesh_count; ++mi)
        {
            struct _json_value *mesh = JSON_ARRAY_VALUE(meshes->value, mi);
            char *mesh_name = 0;

            JSON_VALUE_OBJECT_LOOP(mesh, mmi)
            {
                struct _json_object_entry *mesh_prop = JSON_OBJECT_PROPERTY(mesh, mmi);

                if (JSON_PROPERTY_NAME_EQUALS(mesh_prop, "name") && JSON_PROPERTY_TYPE_EQUALS(mesh_prop, json_string))
                    mesh_name = JSON_PROPERTY_VALUE_STRING(mesh_prop);
            }

            if (mesh_name)
            {
                mesh_t *loaded_mesh = asset_load_mesh(mesh_name);

                if (loaded_mesh)
                {
                    level->meshes[level->mesh_count++] = loaded_mesh;
                    SDL_assert(level->mesh_count <= LEVEL_MAX_MESHES);
                }
                else
                    level->missing_assets = true;
            }
            else
                printf("Mesh has no \"name\" property specified !\n");
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Materials.
    // ----------------------------------------------------------------------------------

    if (level->material_count)
    {
        for (u32 mi = 0; mi < level->material_count; ++mi)
        {
            struct _json_value *material = JSON_ARRAY_VALUE(materials->value, mi);
            gc_material_t *level_material = level->materials + mi;

            // Default settings.
            level_material->type = GC_MATERIAL_BLINN;
            level_material->components = 0;
            level_material->transparency = false;
            level_material->opacity = 1.0f;

            JSON_VALUE_OBJECT_LOOP(material, mpi)
            {
                struct _json_object_entry *material_prop = JSON_OBJECT_PROPERTY(material, mpi);

                if (JSON_PROPERTY_NAME_EQUALS(material_prop, "type") &&
                    JSON_PROPERTY_TYPE_EQUALS(material_prop, json_string) &&
                    strcmp(JSON_PROPERTY_VALUE_STRING(material_prop), "pbr") == 0)
                {
                    level_material->type = GC_MATERIAL_PBR;
                }

                else if (JSON_PROPERTY_NAME_EQUALS(material_prop, "type") &&
                    JSON_PROPERTY_TYPE_EQUALS(material_prop, json_string) &&
                    strcmp(JSON_PROPERTY_VALUE_STRING(material_prop), "skybox") == 0)
                {
                    level_material->type = GC_MATERIAL_SKYBOX;
                }

                // Common settings.
                else if (JSON_PROPERTY_NAME_EQUALS(material_prop, "settings"))
                {
                    JSON_VALUE_OBJECT_LOOP(material_prop->value, si)
                    {
                        struct _json_object_entry *setting_prop = JSON_OBJECT_PROPERTY(material_prop->value, si);

                        if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "transparency") &&
                                    JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_boolean) &&
                                    JSON_PROPERTY_VALUE_BOOL(setting_prop) == true)
                        {
                            level_material->transparency = true;
                        }

                        else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "opacity") &&
                            JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                        {
                            level_material->opacity = JSON_PROPERTY_VALUE_DOUBLE(setting_prop);

                            if (level_material->opacity < 0)
                                level_material->opacity = 0;

                            if (level_material->opacity > 1.0f)
                                level_material->opacity = 1.0f;
                        }
                    }
                }

                // TODO(gabic): Texturile "not_found" sa fie trecute undeva separat de lista de texturi din nivel.

                if (level_material->type == GC_MATERIAL_SKYBOX)
                {
                    if (JSON_PROPERTY_NAME_EQUALS(material_prop, "input"))
                    {
                        u32 texture_index = JSON_PROPERTY_VALUE_INTEGER(material_prop);

                        if (texture_index >= level->texture_count) {
                            EXIT_ALERT("[ERROR] [skybox material] Out of bounds texture index !\n");
                        }

                        level_material->skybox.input = (cube_texture_t *) level->textures[texture_index];

                        if (level_material->skybox.input->type == ASSET_TEXTURE_PBR_AMBIENT)
                            level_material->skybox.input = (cube_texture_t *) ((pbr_ambient_texture_t *) level->textures[texture_index])->environment;

                        if (!level_material->skybox.input) {
                            EXIT_ALERT("[ERROR] Invalid skybox input specified !\n");
                        }

                        if (level_material->skybox.input->type != ASSET_TEXTURE_CUBEMAP &&
                            level_material->skybox.input->type != ASSET_TEXTURE_PBR_AMBIENT) {
                            EXIT_ALERT("[ERROR] Invalid skybox input specified !\n");
                        }
                    }
                }

                else if (level_material->type == GC_MATERIAL_BLINN)
                {
                    // Default values.
                    level_material->blinn.specular = 0.3f;
                    level_material->blinn.shininess = 50.0f;
                    level_material->blinn.min_fresnel = 0.75f;
                    level_material->blinn.max_fresnel = 20.0f;

                    level_material->blinn.diffuse_multiplier.v3.x = 1.0f;
                    level_material->blinn.diffuse_multiplier.v3.y = 1.0f;
                    level_material->blinn.diffuse_multiplier.v3.z = 1.0f;

                    VSET3(level_material->blinn.diffuse, 1.0f, 0.0f, 1.0f);
                    VSET3(level_material->blinn.ambient, 0.05f, 0.05f, 0.05f);
                    level_material->blinn.rr_diffuse_ratio = 0;
                    level_material->blinn.rr_ratio = 0.5f;
                    level_material->blinn.refr_idx1 = 1.0f;
                    level_material->blinn.refr_idx2 = 1.33f;
                    level_material->blinn.refr_ratio = level_material->blinn.refr_idx1 / level_material->blinn.refr_idx2;

                    gl_gamma_srgb_to_linear(&level_material->blinn.diffuse);
                    gl_gamma_srgb_to_linear(&level_material->blinn.ambient);

                    if (JSON_PROPERTY_NAME_EQUALS(material_prop, "maps"))
                    {
                        JSON_VALUE_OBJECT_LOOP(material_prop->value, mapi)
                        {
                            struct _json_object_entry *map = JSON_OBJECT_PROPERTY(material_prop->value, mapi);

                            if (map->value->type != json_integer)
                                continue;

                            u32 texture_index = JSON_PROPERTY_VALUE_INTEGER(map);

                            if (JSON_PROPERTY_NAME_EQUALS(map, "diffuse"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [blinn-phong:diffuse] Out of bounds texture index !\n");
                                }

                                level_material->blinn.diffuse_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "normal"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [blinn-phong:normal] Out of bounds texture index !\n");
                                }

                                level_material->blinn.normal_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "specular"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [blinn-phong:specular] Out of bounds texture index !\n");
                                }

                                level_material->blinn.specular_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "emission"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [blinn-phong:emission] Out of bounds texture index !\n");
                                }

                                level_material->blinn.emission_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "ao"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [blinn-phong:ao] Out of bounds texture index !\n");
                                }

                                level_material->blinn.ao_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "rr"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [blinn-phong:rr] Out of bounds texture index !\n");
                                }

                                level_material->blinn.rr_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "cubemap"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [blinn-phong:cubemap] Out of bounds texture index !\n");
                                }

                                level_material->blinn.cubemap = (cube_texture_t *) level->textures[texture_index];

                                if (!level_material->blinn.cubemap) {
                                    EXIT_ALERT("[ERROR] Invalid cubemap specified !\n");
                                }

                                if (level_material->blinn.cubemap->type != ASSET_TEXTURE_CUBEMAP) {
                                    EXIT_ALERT("[ERROR] Invalid cubemap specified !\n");
                                }
                            }
                        }
                    }

                    else if (JSON_PROPERTY_NAME_EQUALS(material_prop, "components"))
                    {
                        JSON_VALUE_ARRAY_LOOP(material_prop->value, ci)
                        {
                            struct _json_value *component = JSON_ARRAY_VALUE(material_prop->value, ci);

                            if (component->type == json_string)
                            {
                                char *cstr = JSON_VALUE_STRING(component);

                                if (strcmp(cstr, "diffuse") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_DIFFUSE;
                                else if (strcmp(cstr, "specular") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_SPECULAR;
                                else if (strcmp(cstr, "normal") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_NORMAL;
                                else if (strcmp(cstr, "emission") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_EMISSION;
                                else if (strcmp(cstr, "fresnel") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_FRESNEL;
                                else if (strcmp(cstr, "ao") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_AO;
                                else if (strcmp(cstr, "reflection") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_REFLECTION;
                                else if (strcmp(cstr, "refraction") == 0)
                                    level_material->components |= SHADER_FLAG_BLINN_REFRACTION;
                            }
                        }
                    }

                    else if (JSON_PROPERTY_NAME_EQUALS(material_prop, "settings"))
                    {
                        JSON_VALUE_OBJECT_LOOP(material_prop->value, si)
                        {
                            struct _json_object_entry *setting_prop = JSON_OBJECT_PROPERTY(material_prop->value, si);

                            if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "specular") &&
                                JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.specular, setting_prop);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "shininess") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.shininess, setting_prop);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "min_fresnel") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.min_fresnel, setting_prop);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "max_fresnel") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.max_fresnel, setting_prop);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "diffuse_multiplier") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                        JSON_ARRAY_LENGTH(setting_prop->value) == 3)
                            {
                                _json_prop_extract_v3(&level_material->blinn.diffuse_multiplier, setting_prop);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "diffuse_color") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                        JSON_ARRAY_LENGTH(setting_prop->value) == 3)
                            {
                                _json_prop_extract_v3(&level_material->blinn.diffuse, setting_prop);
                                level_material->blinn.diffuse.c.a = 1.0f;
                                gl_gamma_srgb_to_linear(&level_material->blinn.diffuse);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "diffuse_color") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                        JSON_ARRAY_LENGTH(setting_prop->value) == 4)
                            {
                                _json_prop_extract_v4(&level_material->blinn.diffuse, setting_prop);
                                gl_gamma_srgb_to_linear(&level_material->blinn.diffuse);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "ambient_color") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                        JSON_ARRAY_LENGTH(setting_prop->value) == 3)
                            {
                                _json_prop_extract_v3(&level_material->blinn.ambient, setting_prop);
                                level_material->blinn.ambient.c.a = 1.0f;

                                gl_gamma_srgb_to_linear(&level_material->blinn.ambient);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "ambient_color") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                        JSON_ARRAY_LENGTH(setting_prop->value) == 4)
                            {
                                _json_prop_extract_v4(&level_material->blinn.ambient, setting_prop);
                                gl_gamma_srgb_to_linear(&level_material->blinn.ambient);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "rr_diffuse_ratio") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.rr_diffuse_ratio, setting_prop);

                                if (level_material->blinn.rr_diffuse_ratio < 0)
                                    level_material->blinn.rr_diffuse_ratio = 0;

                                if (level_material->blinn.rr_diffuse_ratio > 1)
                                    level_material->blinn.rr_diffuse_ratio = 1;
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "rr_ratio") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.rr_ratio, setting_prop);

                                if (level_material->blinn.rr_ratio < 0)
                                    level_material->blinn.rr_ratio = 0;

                                if (level_material->blinn.rr_ratio > 1)
                                    level_material->blinn.rr_ratio = 1;
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "refr_idx1") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.refr_idx1, setting_prop);

                                if (level_material->blinn.refr_idx1 <= 0)
                                    level_material->blinn.refr_idx1 = 1.0f;
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "refr_idx2") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                            {
                                _json_prop_extract_float(&level_material->blinn.refr_idx2, setting_prop);

                                if (level_material->blinn.refr_idx2 <= 0)
                                    level_material->blinn.refr_idx2 = 1.33f;

                                level_material->blinn.refr_ratio = level_material->blinn.refr_idx1 / level_material->blinn.refr_idx2;
                            }
                        }
                    }
                }

                else if (level_material->type == GC_MATERIAL_PBR)
                {
                    // Default values.
                    VSET4(level_material->pbr.ambient, 0.2f, 0.2f, 0.2f, 1.0f);
                    VSET4(level_material->pbr.albedo, 0.2f, 0.2f, 0.2f, 1.0f);
                    level_material->pbr.metalness = 0.1f;
                    level_material->pbr.roughness = 0.3f;
                    level_material->pbr.ao = 1.0f;

                    level_material->pbr.multipliers.albedo.v3.x = 1.0f;
                    level_material->pbr.multipliers.albedo.v3.y = 1.0f;
                    level_material->pbr.multipliers.albedo.v3.z = 1.0f;

                    level_material->pbr.multipliers.metalness = 1.0f;
                    level_material->pbr.multipliers.roughness = 1.0f;
                    level_material->pbr.multipliers.ao = 1.0f;
                    level_material->pbr.multipliers.specular = 1.0f;
                    VSET3(level_material->pbr.f0, 0.2f, 0.2f, 0.2f);

                    gl_gamma_srgb_to_linear(&level_material->pbr.ambient);
                    gl_gamma_srgb_to_linear(&level_material->pbr.albedo);

                    if (JSON_PROPERTY_NAME_EQUALS(material_prop, "maps"))
                    {
                        JSON_VALUE_OBJECT_LOOP(material_prop->value, mapi)
                        {
                            struct _json_object_entry *map = JSON_OBJECT_PROPERTY(material_prop->value, mapi);

                            if (map->value->type != json_integer)
                                continue;

                            u32 texture_index = JSON_PROPERTY_VALUE_INTEGER(map);

                            if (JSON_PROPERTY_NAME_EQUALS(map, "albedo"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:albedo] Out of bounds texture index !\n");
                                }

                                level_material->pbr.albedo_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "normal"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:normal] Out of bounds texture index !\n");
                                }

                                level_material->pbr.normal_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "arm"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:mra] Out of bounds texture index !\n");
                                }

                                level_material->pbr.arm_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "roughness"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:roughness] Out of bounds texture index !\n");
                                }

                                level_material->pbr.roughness_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "metalness"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:metalness] Out of bounds texture index !\n");
                                }

                                level_material->pbr.metalness_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "ao"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:ao] Out of bounds texture index !\n");
                                }

                                level_material->pbr.ao_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "emission"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:emission] Out of bounds texture index !\n");
                                }

                                level_material->pbr.emission_map = (texture2d_t *) level->textures[texture_index];
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(map, "ambient"))
                            {
                                if (texture_index >= level->texture_count) {
                                    EXIT_ALERT("[ERROR] [pbr:ambient] Out of bounds texture index !\n");
                                }

                                level_material->pbr.ambient_map = (pbr_ambient_texture_t *) level->textures[texture_index];
                            }
                        }
                    }

                    else if (JSON_PROPERTY_NAME_EQUALS(material_prop, "components"))
                    {
                        b8 is_arm = false;

                        JSON_VALUE_ARRAY_LOOP(material_prop->value, ci)
                        {
                            struct _json_value *component = JSON_ARRAY_VALUE(material_prop->value, ci);

                            if (component->type == json_string)
                            {
                                char *cstr = JSON_VALUE_STRING(component);

                                if (strcmp(cstr, "albedo") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_ALBEDO;
                                else if (strcmp(cstr, "normal") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_NORMAL;
                                else if (strcmp(cstr, "arm") == 0)
                                {
                                    level_material->components |= SHADER_FLAG_PBR_AO_ROUGHNESS_METALNESS;
                                    is_arm = true;
                                }
                                else if (!is_arm && strcmp(cstr, "metalness") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_METALNESS;
                                else if (!is_arm && strcmp(cstr, "roughness") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_ROUGHNESS;
                                else if (!is_arm && strcmp(cstr, "ao") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_AO;
                                else if (strcmp(cstr, "emission") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_EMISSION;
                                else if (strcmp(cstr, "ambient") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_AMBIENT;
                                else if (strcmp(cstr, "unlit") == 0)
                                    level_material->components |= SHADER_FLAG_PBR_UNLIT;
                            }
                        }
                    }

                    else if (JSON_PROPERTY_NAME_EQUALS(material_prop, "settings"))
                    {
                        JSON_VALUE_OBJECT_LOOP(material_prop->value, si)
                        {
                            struct _json_object_entry *setting_prop = JSON_OBJECT_PROPERTY(material_prop->value, si);

                            if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "ambient") &&
                                JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                JSON_ARRAY_LENGTH(setting_prop->value) == 3)
                            {
                                _json_prop_extract_v3(&level_material->pbr.ambient, setting_prop);
                                gl_gamma_srgb_to_linear(&level_material->pbr.ambient);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "albedo") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                        JSON_ARRAY_LENGTH(setting_prop->value) == 3)
                            {
                                _json_prop_extract_v3(&level_material->pbr.albedo, setting_prop);
                                gl_gamma_srgb_to_linear(&level_material->pbr.albedo);
                            }

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "metalness") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                                _json_prop_extract_float(&level_material->pbr.metalness, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "roughness") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                                _json_prop_extract_float(&level_material->pbr.roughness, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "ao") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                                _json_prop_extract_float(&level_material->pbr.ao, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "m_albedo") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array) &&
                                        JSON_ARRAY_LENGTH(setting_prop->value) == 3)
                                _json_prop_extract_v3(&level_material->pbr.multipliers.albedo, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "m_metalness") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                                _json_prop_extract_float(&level_material->pbr.multipliers.metalness, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "m_roughness") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                                _json_prop_extract_float(&level_material->pbr.multipliers.roughness, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "m_ao") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                                _json_prop_extract_float(&level_material->pbr.multipliers.ao, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "m_specular") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_double))
                                _json_prop_extract_float(&level_material->pbr.multipliers.specular, setting_prop);

                            else if (JSON_PROPERTY_NAME_EQUALS(setting_prop, "f0") &&
                                        JSON_PROPERTY_TYPE_EQUALS(setting_prop, json_array))
                                _json_prop_extract_v3(&level_material->pbr.f0, setting_prop);
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Models.
    // ----------------------------------------------------------------------------------

    if (level->model_count)
    {
        for (u32 mi = 0; mi < level->model_count; ++mi)
        {
            struct _json_value *model = JSON_ARRAY_VALUE(models->value, mi);
            gc_model_t *level_model = level->models + mi;

            // -- Default values.

            model_init(level_model,
                       SHADER_NONE,
                       0, 0, 0,
                       0, 0, 0,
                       1, 1, 1,
                       0, 0, 0);

            level_model->material = &level->settings.default_material;

            JSON_VALUE_OBJECT_LOOP(model, mpi)
            {
                struct _json_object_entry *model_prop = JSON_OBJECT_PROPERTY(model, mpi);

                if (JSON_PROPERTY_COMPARE(model_prop, "mesh", json_integer))
                {
                    u32 mesh_index = JSON_PROPERTY_VALUE_INTEGER(model_prop);

                    if (mesh_index >= level->mesh_count) {
                        EXIT_ALERT("[ERROR] Out of bounds mesh index !\n");
                    }

                    level_model->meshes[0] = (mesh_t *) level->meshes[mesh_index];
                }

                else if (JSON_PROPERTY_COMPARE(model_prop, "mesh_lines", json_integer))
                {
                    u32 mesh_index = JSON_PROPERTY_VALUE_INTEGER(model_prop);

                    if (mesh_index >= level->mesh_count) {
                        EXIT_ALERT("[ERROR] Out of bounds mesh index !\n");
                    }

                    level_model->meshes[1] = (mesh_t *) level->meshes[mesh_index];
                }

                else if (JSON_PROPERTY_COMPARE(model_prop, "mesh_points", json_integer))
                {
                    u32 mesh_index = JSON_PROPERTY_VALUE_INTEGER(model_prop);

                    if (mesh_index >= level->mesh_count) {
                        EXIT_ALERT("[ERROR] Out of bounds mesh index !\n");
                    }

                    level_model->meshes[2] = (mesh_t *) level->meshes[mesh_index];
                }

                else if (JSON_PROPERTY_COMPARE(model_prop, "material", json_integer))
                {
                    u32 material_id = JSON_PROPERTY_VALUE_INTEGER(model_prop);

                    // TODO(gabic): cazul pentru un material id invalid
                    if (material_id < level->material_count)
                    {
                        level_model->material = level->materials + material_id;

                        if (level_model->material->opacity < 1)
                        {
                            level_model->overwrites.forced_opacity.overwrite = true;
                            level_model->overwrites.forced_opacity.value.u_float = level_model->material->opacity;
                        }
                    }
                }

                else if (JSON_PROPERTY_COMPARE(model_prop, "position", json_array) && JSON_ARRAY_LENGTH(model_prop->value) == 3)
                    _json_prop_extract_v3(&level_model->object.position, model_prop);

                else if (JSON_PROPERTY_COMPARE(model_prop, "rotation", json_array) && JSON_ARRAY_LENGTH(model_prop->value) == 3)
                    _json_prop_extract_v3(&level_model->object.rotation, model_prop);

                else if (JSON_PROPERTY_COMPARE(model_prop, "scaling", json_array) && JSON_ARRAY_LENGTH(model_prop->value) == 3)
                    _json_prop_extract_v3(&level_model->object.scaling, model_prop);

                else if (JSON_PROPERTY_COMPARE(model_prop, "uv_scaling", json_array) && JSON_ARRAY_LENGTH(model_prop->value) == 2)
                {
                    level_model->overwrites.uv_scaling.overwrite = true;
                    _json_prop_extract_v2(&level_model->overwrites.uv_scaling.value.u_vector, model_prop);
                }

                else if (JSON_PROPERTY_COMPARE(model_prop, "backface_cull", json_boolean))
                {
                    b8 backface_cull = JSON_PROPERTY_VALUE_BOOL(model_prop);

                    if (backface_cull)
                        FLAG_ENABLE(level_model->flags, MOD_BACKFACE_CULL);
                    else
                        FLAG_DISABLE(level_model->flags, MOD_BACKFACE_CULL);
                }

                else if (JSON_PROPERTY_COMPARE(model_prop, "exclude_from_shadow", json_boolean))
                {
                    b8 exclude_from_shadow = JSON_PROPERTY_VALUE_BOOL(model_prop);

                    if (exclude_from_shadow)
                        FLAG_ENABLE(level_model->flags, MOD_EXCLUDE_FROM_SHADOW);
                    else
                        FLAG_DISABLE(level_model->flags, MOD_EXCLUDE_FROM_SHADOW);
                }

                else if (JSON_PROPERTY_COMPARE(model_prop, "shader", json_integer))
                {
                    level_model->shader_id = (shader_id_t) JSON_PROPERTY_VALUE_INTEGER(model_prop);

                    if (level_model->shader_id >= SHADER_COUNT)
                        level_model->shader_id = SHADER_NONE;
                    // else
                    //     level_model->shader = GET_SHADER(level_model->shader_id);
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Lights.
    // ----------------------------------------------------------------------------------

    if (level->light_count)
    {
        for (u32 li = 0; li < level->light_count; ++li)
        {
            struct _json_value *light = JSON_ARRAY_VALUE(lights->value, li);
            gc_light_t *level_light = level->lights + li;

            // Default shadow values.
            level_light->shadow.flags = TEXTURE_WRAP_CLAMP;
            level_light->shadow.camera_distance = 5.0f;
            level_light->shadow.camera_fov = 10.0f;
            level_light->shadow.f_near = 1.0f;
            level_light->shadow.f_far = 100.0f;
            level_light->shadow.f_len_inv = 1.0f / (level_light->shadow.f_far - level_light->shadow.f_near);

            level_light->shadow.radius = 0.001f;
            level_light->shadow.depth_bias = 0.001f;
            level_light->shadow.normal_offset = 0.005f;
            level_light->shadow.vsm_bias = 0.001f;
            level_light->shadow.light_bleed_reduction = 0.0f;

            level_light->color.c.r = 1.0f;
            level_light->color.c.g = 1.0f;
            level_light->color.c.b = 1.0f;
            level_light->color.c.a = 1.0f;

            #if defined(GC_PIPE_AVX)
            level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample;
            level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample;
            #elif defined(GC_PIPE_SSE)
            level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample;
            level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample;
            #else
            level_light->shadow.sun_shadow_visibility_r = shadow_map_sun_sample;
            level_light->shadow.point_shadow_visibility_r = shadow_map_point_sample;
            #endif

            JSON_VALUE_OBJECT_LOOP(light, lpi)
            {
                struct _json_object_entry *light_prop = JSON_OBJECT_PROPERTY(light, lpi);

                if (JSON_PROPERTY_COMPARE(light_prop, "type", json_string))
                {
                    if (strcmp(JSON_VALUE_STRING(light_prop->value), "sun") == 0)
                    {
                        level_light->type = GC_SUN_LIGHT;
                        level_light->updated = true;

                        VSET3(level_light->object.axis.forward, 0, 1, 0);   // world y
                        VSET3(level_light->object.axis.side, -1, 0, 0);     // world -x
                        VSET3(level_light->object.axis.up, 0, 0, 1);        // world z
                    }
                    else if (strcmp(JSON_VALUE_STRING(light_prop->value), "point") == 0)
                    {
                        level_light->type = GC_POINT_LIGHT;
                        level_light->updated = true;

                        level_light->point.kc = 1;
                        level_light->point.kl = 1;
                        level_light->point.kq = 1;
                    }
                }

                else if (JSON_PROPERTY_COMPARE(light_prop, "kc", json_double) && level_light->type == GC_POINT_LIGHT)
                    _json_prop_extract_float(&level_light->point.kc, light_prop);

                else if (JSON_PROPERTY_COMPARE(light_prop, "kl", json_double) && level_light->type == GC_POINT_LIGHT)
                        _json_prop_extract_float(&level_light->point.kl, light_prop);

                else if (JSON_PROPERTY_COMPARE(light_prop, "kq", json_double) && level_light->type == GC_POINT_LIGHT)
                        _json_prop_extract_float(&level_light->point.kq, light_prop);

                else if (JSON_PROPERTY_COMPARE(light_prop, "position", json_array) && JSON_ARRAY_LENGTH(light_prop->value) == 3)
                    _json_prop_extract_v3(&level_light->object.position, light_prop);

                else if (JSON_PROPERTY_COMPARE(light_prop, "rotation", json_array) && JSON_ARRAY_LENGTH(light_prop->value) == 3)
                    _json_prop_extract_v3(&level_light->object.rotation, light_prop);

                else if (JSON_PROPERTY_COMPARE(light_prop, "color", json_array) && JSON_ARRAY_LENGTH(light_prop->value) == 4)
                {
                    _json_prop_extract_v4(&level_light->color, light_prop);
                    gl_gamma_srgb_to_linear(&level_light->color);
                }

                else if (JSON_PROPERTY_COMPARE(light_prop, "il", json_double))
                {
                    _json_prop_extract_float(&level_light->il, light_prop);

                    level_light->color.c.r *= level_light->il;
                    level_light->color.c.g *= level_light->il;
                    level_light->color.c.b *= level_light->il;
                }

                else if (JSON_PROPERTY_COMPARE(light_prop, "shadow", json_object))
                {
                    JSON_VALUE_OBJECT_LOOP(light_prop->value, spi)
                    {
                        struct _json_object_entry *shadow_prop = JSON_OBJECT_PROPERTY(light_prop->value, spi);

                        if (JSON_PROPERTY_COMPARE(shadow_prop, "type", json_string))
                        {
                            if (level_light->type == GC_SUN_LIGHT)
                            {
                                #if defined(GC_PIPE_AVX)
                                if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "basic") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_3x3") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_3x3;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_3x3_linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_3x3_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_5x5") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_5x5;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_5x5_linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_5x5_linear;
                                #elif defined(GC_PIPE_SSE)
                                if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "basic") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_3x3") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_3x3;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_3x3_linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_3x3_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_5x5") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_5x5;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_5x5_linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = sse_shadow_map_sun_sample_pcf_5x5_linear;
                                #else
                                if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "basic") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = shadow_map_sun_sample;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = shadow_map_sun_sample_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_3x3") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = shadow_map_sun_sample_pcf_3x3;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_3x3_linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = shadow_map_sun_sample_pcf_3x3_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_5x5") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = shadow_map_sun_sample_pcf_5x5;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_5x5_linear") == 0)
                                    level_light->shadow.sun_shadow_visibility_r = shadow_map_sun_sample_pcf_5x5_linear;
                                #endif
                            }
                            else if (level_light->type == GC_POINT_LIGHT)
                            {
                                #if defined(GC_PIPE_AVX)
                                if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "basic") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "linear") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample_pcf;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_linear") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample_pcf_linear;
                                #elif defined(GC_PIPE_SSE)
                                if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "basic") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "linear") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample_pcf;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_linear") == 0)
                                    level_light->shadow.point_shadow_visibility_r = sse_shadow_map_point_sample_pcf_linear;
                                #else
                                if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "basic") == 0)
                                    level_light->shadow.point_shadow_visibility_r = shadow_map_point_sample;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "linear") == 0)
                                    level_light->shadow.point_shadow_visibility_r = shadow_map_point_sample_linear;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf") == 0)
                                    level_light->shadow.point_shadow_visibility_r = shadow_map_point_sample_pcf;
                                else if (strcmp(JSON_VALUE_STRING(shadow_prop->value), "pcf_linear") == 0)
                                    level_light->shadow.point_shadow_visibility_r = shadow_map_point_sample_pcf_linear;
                                #endif
                            }
                        }

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "camera_distance", json_double))
                            level_light->shadow.camera_distance = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "camera_fov", json_double))
                            level_light->shadow.camera_fov = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "f_near", json_double))
                            level_light->shadow.f_near = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "f_far", json_double))
                            level_light->shadow.f_far = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "depth_bias", json_double))
                            level_light->shadow.depth_bias = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "normal_offset", json_double))
                            level_light->shadow.normal_offset = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "radius", json_double))
                            level_light->shadow.radius = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "vsm_bias", json_double))
                            level_light->shadow.vsm_bias = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);

                        else if (JSON_PROPERTY_COMPARE(shadow_prop, "light_bleed_reduction", json_double))
                            level_light->shadow.light_bleed_reduction = JSON_PROPERTY_VALUE_DOUBLE(shadow_prop);
                    }
                }
            }
        }
    }

    end_level:

    API->close_file(&level_file);
    gc_mem_free(file_contents);
    mem_restore_chunk();

    return level;
}