// ----------------------------------------------------------------------------------
// -- File: gcsr_asset_builder.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-11-21 19:39:41
// -- Modified:
// ----------------------------------------------------------------------------------

#define GC_WIN32 1
#define GC_LINUX 2
#define GC_PLATFORM GC_WIN32
#define GCSR_ASSET_BUILDER 1

#include <stdio.h>
#include <assert.h>

#define USE_SSE2 1
#include "../libs/sse_math/sse_mathfun.h"

#define GCSR_MALLOC(bytes) malloc(bytes)
#define MEM_LABEL(label)
// Overwrite the engine allocation.
#define gc_mem_allocate(bytes) malloc(bytes)
#define gc_mem_reallocate(data, bytes) realloc(data, bytes)
#define gc_mem_free(block) free(block)
#define mem_set_chunk(chunk)
#define mem_restore_chunk()

#include "SDL.h"
#include "../gcsr_macros.h"
#include "../gcsr_types.h"
#include "../gcsr_data.h"
#include "../gcsr_file_interface.h"
#include "../gcsr_asset.h"
#include "../gcsr_math.h"
#include "../gcsr_vecmat.h"
#include "gcsr_asset_builder.h"

#if GC_PLATFORM == GC_WIN32
#include <windows.h>
#include "../platform/win32/gcsr_file_impl.cpp"
#elif GC_PLATFORM == GC_LINUX
#include "../platform/linux/gcsr_file_impl.cpp"
#endif

#include "../gcsr_texture.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb/stb_image.h"
#include "../libs/json-parser/json.c"

#include "builder_messages.h"
#include "loaders/bitmap.h"
#include "loaders/pbr.h"
#include "loaders/gltf.h"
#include "loaders/parser_asset_config.h"

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)

#define BytesToKilo(value) (r32) value / 1024
#define BytesToMega(value) (r32) value / (1024 * 1024)

// ----------------------------------------------------------------------------------
// -- Multithreading.
// ----------------------------------------------------------------------------------

int thread_work(void *data)
{
    u32 thread_id = (u32) data;

    while (true)
    {
        // -- Idle state.

        SDL_LockMutex(threads.worker_lock);

        if (threads.running_count > 0)
            threads.running_count--;

        if (threads.running_count == 0)
            SDL_CondSignal(threads.scheduler_condition);

        SDL_CondWait(threads.worker_condition, threads.worker_lock);
        SDL_UnlockMutex(threads.worker_lock);

        if (threads.state == PBR_IRRADIANCE_STATE) {
            _sse_generate_irradiance(thread_id);
        }

        else if (threads.state == PBR_PREFILTERED_STATE) {
        }

        else if (threads.state == END_STATE)
        {
            if (threads.running_count > 0)
                threads.running_count--;

            if (threads.running_count == 0)
                SDL_CondSignal(threads.scheduler_condition);

            break;
        }
    }

    return 1;
}

void init_threads()
{
    threads.running_count = 0;
    threads.state = IDLE_STATE;
    threads.worker_lock = SDL_CreateMutex();
    threads.worker_condition = SDL_CreateCond();
    threads.scheduler_condition = SDL_CreateCond();

    for (u32 i = 0; i < ASSET_BUILDER_THREAD_COUNT; ++i) {
        threads.thread_pool[i] = SDL_CreateThread(thread_work, 0, (void *) i);
    }
}

void destroy_threads()
{
    s32 status = 0;

    BUILDER_THREAD_START(END_STATE);

    for (u8 i = 0; i < ASSET_BUILDER_THREAD_COUNT   ; ++i) {
        SDL_WaitThread(threads.thread_pool[i], &status);
    }

    SDL_DestroyMutex(threads.worker_lock);
    SDL_DestroyCond(threads.worker_condition);
    SDL_DestroyCond(threads.scheduler_condition);
}

void generate_thread_work(u32 size)
{
    u32 tile_size = size >> THREAD_TILE_DIVISION_SHIFT;
    u32 tile_rows = (1 << THREAD_TILE_DIVISION_SHIFT);
    u32 tile_cols = (1 << THREAD_TILE_DIVISION_SHIFT);

    threads.work.tile_count = tile_rows * tile_cols;
    threads.work.tiles = (thread_tile_t *) calloc(threads.work.tile_count, sizeof(thread_tile_t));
    SDL_AtomicSet(&threads.work.index, 0);

    for (u32 row = 0; row < tile_rows; ++row)
    {
        for (u32 col = 0; col < tile_cols; ++col)
        {
            u32 offset = row * tile_rows + col;
            thread_tile_t *current_tile = threads.work.tiles + offset;

            current_tile->sx = col * tile_size;
            current_tile->sy = row * tile_size;

            current_tile->ex = current_tile->sx + tile_size - 1;
            current_tile->ey = current_tile->sy + tile_size - 1;
        }
    }
}

void destroy_thread_work()
{
    threads.work.tile_count = 0;

    if (threads.work.tiles)
        free(threads.work.tiles);
}

// ----------------------------------------------------------------------------------

void free_loaded_mesh_list(loaded_mesh_list_t *mesh_list)
{
    if (mesh_list)
    {
        for (u32 mi = 0; mi < mesh_list->length; ++mi)
        {
            loaded_asset_mesh_t *mesh = mesh_list->meshes + mi;
            free(mesh->index_buffer);

            mesh->index_buffer = 0;
            mesh->vertex_buffer = 0;
        }

        free(mesh_list);
    }
}

void debug_loaded_texture(char *filepath, loaded_texture_t *texture)
{
    if (texture->meta.type == ASSET_TEXTURE2D)
        _debug_basic_texture(filepath, texture);
    else if (texture->meta.type == ASSET_TEXTURE_CUBEMAP)
        _debug_cubemap_texture(filepath, texture);
    else if (texture->meta.type == ASSET_TEXTURE_PBR_AMBIENT)
        _debug_pbr_ambient_texture(filepath, texture);
}

// ----------------------------------------------------------------------------------
// -- Temporary chunk memory buffer.
// ----------------------------------------------------------------------------------

chunk_buffer_t create_chunk_buffer()
{
    chunk_buffer_t buffer;

    buffer.metadata_buffer = malloc(CHUNK_BUFFER_METADATA_BYTES);
    buffer.data_buffer = malloc(CHUNK_BUFFER_DATA_BYTES);

    buffer.asset_count = 0;
    buffer.metadata_bytes = 0;
    buffer.data_bytes = 0;

    memset(buffer.metadata_buffer, 0, CHUNK_BUFFER_METADATA_BYTES);
    memset(buffer.data_buffer, 0, CHUNK_BUFFER_DATA_BYTES);

    return buffer;
}

void reset_chunk_buffer(chunk_buffer_t *buffer)
{
    if (buffer)
    {
        memset(buffer->metadata_buffer, 0, CHUNK_BUFFER_METADATA_BYTES);
        memset(buffer->data_buffer, 0, CHUNK_BUFFER_DATA_BYTES);

        buffer->asset_count = 0;
        buffer->metadata_bytes = 0;
        buffer->data_bytes = 0;
    }
}

void free_chunk_buffer(chunk_buffer_t *buffer)
{
    if (buffer)
    {
        free(buffer->metadata_buffer);
        free(buffer->data_buffer);
    }
}

asset_metadata_t *register_asset(chunk_buffer_t *buffer)
{
    asset_metadata_t *res = (asset_metadata_t *) ((u8 *) buffer->metadata_buffer + buffer->metadata_bytes);

    buffer->metadata_bytes += sizeof(asset_metadata_t);
    buffer->asset_count++;

    return res;
}

void chunk_buffer_copy(chunk_buffer_t *buffer, void *src_data, size_t bytes)
{
    if (buffer && src_data)
    {
        if (buffer->data_bytes + bytes > CHUNK_BUFFER_DATA_BYTES)
            EXIT_ALERT(B_ERROR(CHUNK_BUFFER_OVERFLOW));

        u8 *dest_ptr = (u8 *) buffer->data_buffer + buffer->data_bytes;
        memcpy(dest_ptr, src_data, bytes);
        buffer->data_bytes += bytes;
    }
}

// void chunk_buffer_flush(asset_file_header_t *asset_header, chunk_buffer_t *buffer, FILE *output_file);

#if CHUNK_LIMIT == CHUNK_LIMIT_ASSET_COUNT
#define CHUNK_BUFFER_CHECK(buffer) if ((buffer)->asset_count >= MAX_CHUNK_ASSETS)
#elif CHUNK_LIMIT == CHUNK_LIMIT_ASSET_BYTES
#define CHUNK_BUFFER_CHECK(buffer) if ((buffer)->metadata_bytes + (buffer)->data_bytes >= MAX_CHUNK_BYTES)
#endif

#define CHUNK_CHECK_FLUSH(asset_header, buffer, output_file) \
    CHUNK_BUFFER_CHECK(buffer) \
    { \
        chunk_buffer_flush((asset_header), (buffer), (output_file)); \
    }

#define CHUNK_FORCE_FLUSH(asset_header, buffer, output_file) chunk_buffer_flush((asset_header), (buffer), (output_file))

void chunk_buffer_flush(asset_file_header_t *asset_header, chunk_buffer_t *buffer, gc_file_t *output_file)
{
    if (asset_header && buffer && output_file)
    {
        asset_chunk_t chunk;

        chunk.asset_count = buffer->asset_count;
        chunk.metadata_bytes = buffer->metadata_bytes;
        chunk.data_bytes = buffer->data_bytes;

        u64 chunk_total_bytes = sizeof(asset_chunk_t) + buffer->metadata_bytes + buffer->data_bytes;
        chunk.next_chunk_offset = buffer->output_file_offset + chunk_total_bytes;

        asset_metadata_t *metadata_ptr = (asset_metadata_t *) buffer->metadata_buffer;
        u64 data_offset = buffer->output_file_offset + sizeof(asset_chunk_t) + buffer->metadata_bytes;

        // Set the data offset for each asset.
        for (u16 i = 0; i < chunk.asset_count; ++i)
        {
            asset_metadata_t *metadata = metadata_ptr + i;
            metadata->data_offset = data_offset;
            data_offset += metadata->data_bytes;
        }

        // Write the chunk header.
        // fseek(output_file, buffer->output_file_offset, SEEK_SET);
        write_file(output_file, sizeof(asset_chunk_t), &chunk);
        buffer->output_file_offset += sizeof(asset_chunk_t);

        // Write the chunk metadata.
        write_file(output_file, buffer->metadata_bytes, buffer->metadata_buffer);
        buffer->output_file_offset += buffer->metadata_bytes;

        // Write the chunk data.
        write_file(output_file, buffer->data_bytes, buffer->data_buffer);
        buffer->output_file_offset += buffer->data_bytes;

        // buffer->output_file_offset += chunk_total_bytes;
        asset_header->chunks++;
        asset_header->total_bytes += chunk_total_bytes;

        reset_chunk_buffer(buffer);
    }
}

void add_mesh_to_chunk_buffer(loaded_asset_mesh_t *mesh,
                              chunk_buffer_t *buffer,
                              asset_file_header_t *asset_header,
                              gc_file_t *asset_name_file,
                              gc_file_t *output_file)
{
    if (!mesh)
        return;

    char tmp_string[255];
    sprintf(tmp_string, "%s / %llu bytes \n", mesh->name, mesh->index_buffer_bytes + mesh->vertex_buffer_bytes);
    write_file(asset_name_file, strlen(tmp_string), tmp_string);

    asset_header->total_assets++;
    asset_header->meshes++;

    asset_metadata_t *metadata = register_asset(buffer);

    metadata->type = ASSET_MESH;
    strncpy(metadata->name, mesh->name, METADATA_NAME_LENGTH);
    metadata->data_bytes = mesh->index_buffer_bytes + mesh->vertex_buffer_bytes;
    metadata->data_offset = 0;

    metadata->mesh.type = mesh->type;
    metadata->mesh.indices = mesh->indices;
    metadata->mesh.vertices = mesh->vertices;

    metadata->mesh.indices_offset = 0;
    metadata->mesh.vertices_offset = mesh->index_buffer_bytes;

    metadata->mesh.indices_bytes = mesh->index_buffer_bytes;
    metadata->mesh.vertices_bytes = mesh->vertex_buffer_bytes;

    // Copy the mesh data to the chunk buffer.
    chunk_buffer_copy(buffer, mesh->index_buffer, mesh->index_buffer_bytes);
    chunk_buffer_copy(buffer, mesh->vertex_buffer, mesh->vertex_buffer_bytes);

    CHUNK_CHECK_FLUSH(asset_header, buffer, output_file);
}

void write_package_header(gc_file_t *package, u32 objects)
{
    asset_group_header_t asset_group_header;
    asset_group_header.magic_value = ('A' << 0) | ('S' << 8) | ('G' << 16) | ('P' << 24);
    strncpy(asset_group_header.version, VERSION, 6);
    asset_group_header.count = objects;
    package->write_offset = 0;
    write_file(package, sizeof(asset_group_header_t), &asset_group_header);
}

void write_texture_to_package(gc_file_t *package, asset_metadata_t *metadata, void *data, size_t *offset)
{
    *offset += sizeof(asset_metadata_t);
    metadata->data_offset = *offset;
    *offset += metadata->data_bytes;
    metadata->next_meta_offset = *offset;
    strncpy(metadata->package_path, package->name, FILE_MAX_PATH);

    write_file(package, sizeof(asset_metadata_t), metadata);
    write_file(package, metadata->data_bytes, data);
}

void write_mesh_to_package(gc_file_t *package, loaded_asset_mesh_t *mesh, size_t *offset)
{
    asset_metadata_t metadata;
    *offset += sizeof(asset_metadata_t);

    metadata.type = ASSET_MESH;
    strncpy(metadata.name, mesh->name, METADATA_NAME_LENGTH);
    strncpy(metadata.package_path, package->name, FILE_MAX_PATH);

    metadata.data_bytes = mesh->index_buffer_bytes + mesh->vertex_buffer_bytes;
    metadata.data_offset = *offset;

    metadata.mesh.type = mesh->type;
    metadata.mesh.indices = mesh->indices;
    metadata.mesh.vertices = mesh->vertices;

    metadata.mesh.indices_offset = *offset;
    *offset += mesh->index_buffer_bytes;
    metadata.mesh.vertices_offset = *offset;
    *offset += mesh->vertex_buffer_bytes;

    metadata.mesh.indices_bytes = mesh->index_buffer_bytes;
    metadata.mesh.vertices_bytes = mesh->vertex_buffer_bytes;

    metadata.next_meta_offset = *offset;

    write_file(package, sizeof(asset_metadata_t), &metadata);
    write_file(package, metadata.mesh.indices_bytes, mesh->index_buffer);
    write_file(package, metadata.mesh.vertices_bytes, mesh->vertex_buffer);
}

s32 get_file_extension(char *filepath)
{
    s32 ext_index = -1;

    if (filepath)
    {
        u32 len = (u32) strlen(filepath);
        ext_index = len - 1;

        while (true)
        {
            if (ext_index < 0)
                break;

            if (filepath[ext_index] == '.')
            {
                ext_index++;
                break;
            }

            ext_index--;
        }
    }

    return ext_index;
}

// ----------------------------------------------------------------------------------
// -- Routines for generating lines and points from a mesh.
// ----------------------------------------------------------------------------------

loaded_asset_mesh_t *generate_line_mesh(loaded_asset_mesh_t *mesh)
{
    loaded_asset_mesh_t *generated = 0;

    if (mesh)
    {
        // Maximum number of possible lines.
        u32 total_lines = (mesh->indices / 3) * 3;
        size_t indices_bytes = MEM_SIZE_ALIGN(2 * total_lines * sizeof(u32));
        size_t vertices_bytes = MEM_SIZE_ALIGN(2 * total_lines * sizeof(asset_vertex_t));
        size_t generated_mesh_bytes = sizeof(loaded_asset_mesh_t) + indices_bytes + vertices_bytes;

        size_t added_vertices_bytes = mesh->indices * sizeof(b8);
        b8 *added_vertices = (b8 *) malloc(added_vertices_bytes);
        memset(added_vertices, 0, added_vertices_bytes);

        generated = (loaded_asset_mesh_t *) malloc(generated_mesh_bytes);
        memset(generated, 0, generated_mesh_bytes);

        generated->type = GL_MESH_LINE;
        strncpy(generated->name, mesh->name, METADATA_NAME_LENGTH);
        strcat(generated->name, "_lines");
        generated->index_buffer = (u32 *) (generated + 1);
        generated->vertex_buffer = ADDR_OFFSET(generated->index_buffer, indices_bytes);

        ds_edge_t *edges = (ds_edge_t *) generated->index_buffer;
        u32 edge_count = 0;

        ds_hashtable_t *ht = hashtable_create(total_lines);
        ht->hash_function = edge_hash_function;
        ht->compare_function = edge_compare_function;

        for (u32 i = 0; i < mesh->indices; i += 3)
        {
            u32 idx1 = mesh->index_buffer[i];
            u32 idx2 = mesh->index_buffer[i + 1];
            u32 idx3 = mesh->index_buffer[i + 2];

            asset_vertex_t *vertex_1 = (asset_vertex_t *) mesh->vertex_buffer + idx1;
            asset_vertex_t *vertex_2 = (asset_vertex_t *) mesh->vertex_buffer + idx2;
            asset_vertex_t *vertex_3 = (asset_vertex_t *) mesh->vertex_buffer + idx3;

            // ----------------------------------------------------------------------------------
            // -- Vertex selection.
            // ----------------------------------------------------------------------------------

            if (!added_vertices[idx1])
            {
                asset_vertex_t *dest_vertex = (asset_vertex_t *) generated->vertex_buffer + generated->vertices++;
                memcpy(dest_vertex, vertex_1, sizeof(asset_vertex_t));

                added_vertices[idx1] = true;
                generated->vertex_buffer_bytes += sizeof(asset_vertex_t);
            }

            if (!added_vertices[idx2])
            {
                asset_vertex_t *dest_vertex = (asset_vertex_t *) generated->vertex_buffer + generated->vertices++;
                memcpy(dest_vertex, vertex_2, sizeof(asset_vertex_t));

                added_vertices[idx2] = true;
                generated->vertex_buffer_bytes += sizeof(asset_vertex_t);
            }

            if (!added_vertices[idx3])
            {
                asset_vertex_t *dest_vertex = (asset_vertex_t *) generated->vertex_buffer + generated->vertices++;
                memcpy(dest_vertex, vertex_3, sizeof(asset_vertex_t));

                added_vertices[idx3] = true;
                generated->vertex_buffer_bytes += sizeof(asset_vertex_t);
            }

            // ----------------------------------------------------------------------------------
            // -- Edge selection.
            // ----------------------------------------------------------------------------------

            ds_edge_t *edge = edges + edge_count;
            EDGE_INIT(edge, idx1, idx2);
            b8 inserted = hashtable_insert(ht, edge, edge);

            if (inserted)
            {
                edge_count++;

                generated->indices += 2;
                generated->index_buffer_bytes += 2 * sizeof(u32);
            }

            edge = edges + edge_count;
            EDGE_INIT(edge, idx2, idx3);
            inserted = hashtable_insert(ht, edge, edge);

            if (inserted)
            {
                edge_count++;

                generated->indices += 2;
                generated->index_buffer_bytes += 2 * sizeof(u32);
            }

            edge = edges + edge_count;
            EDGE_INIT(edge, idx3, idx1);
            inserted = hashtable_insert(ht, edge, edge);

            if (inserted)
            {
                edge_count++;

                generated->indices += 2;
                generated->index_buffer_bytes += 2 * sizeof(u32);
            }
        }

        free(added_vertices);
        free(ht);
    }

    return generated;
}

loaded_asset_mesh_t *generate_point_mesh(loaded_asset_mesh_t *mesh)
{
    loaded_asset_mesh_t *generated = 0;

    if (mesh)
    {
        // Maximum number of possible lines.
        size_t indices_bytes = MEM_SIZE_ALIGN(mesh->indices * sizeof(u32));
        size_t vertices_bytes = MEM_SIZE_ALIGN(mesh->indices * sizeof(asset_vertex_t));
        size_t generated_mesh_bytes = sizeof(generated_asset_mesh_t) + indices_bytes + vertices_bytes;

        size_t added_vertices_bytes = mesh->indices * sizeof(b8);
        b8 *added_vertices = (b8 *) malloc(added_vertices_bytes);
        memset(added_vertices, 0, added_vertices_bytes);

        generated = (loaded_asset_mesh_t *) malloc(generated_mesh_bytes);
        memset(generated, 0, generated_mesh_bytes);

        generated->type = GL_MESH_POINT;
        strncpy(generated->name, mesh->name, METADATA_NAME_LENGTH);
        strcat(generated->name, "_points");
        generated->index_buffer = (u32 *) (generated + 1);
        generated->vertex_buffer = ADDR_OFFSET(generated->index_buffer, indices_bytes);

        for (u32 i = 0; i < mesh->indices; i += 3)
        {
            u32 idx1 = mesh->index_buffer[i];
            u32 idx2 = mesh->index_buffer[i + 1];
            u32 idx3 = mesh->index_buffer[i + 2];

            asset_vertex_t *vertex_1 = (asset_vertex_t *) mesh->vertex_buffer + idx1;
            asset_vertex_t *vertex_2 = (asset_vertex_t *) mesh->vertex_buffer + idx2;
            asset_vertex_t *vertex_3 = (asset_vertex_t *) mesh->vertex_buffer + idx3;

            // ----------------------------------------------------------------------------------
            // -- Vertex selection.
            // ----------------------------------------------------------------------------------

            if (!added_vertices[idx1])
            {
                generated->index_buffer[generated->indices++] = idx1;

                asset_vertex_t *dest_vertex = (asset_vertex_t *) generated->vertex_buffer + generated->vertices++;
                memcpy(dest_vertex, vertex_1, sizeof(asset_vertex_t));

                added_vertices[idx1] = true;
                generated->index_buffer_bytes += sizeof(u32);
                generated->vertex_buffer_bytes += sizeof(asset_vertex_t);
            }

            if (!added_vertices[idx2])
            {
                generated->index_buffer[generated->indices++] = idx2;

                asset_vertex_t *dest_vertex = (asset_vertex_t *) generated->vertex_buffer + generated->vertices++;
                memcpy(dest_vertex, vertex_2, sizeof(asset_vertex_t));

                added_vertices[idx2] = true;
                generated->index_buffer_bytes += sizeof(u32);
                generated->vertex_buffer_bytes += sizeof(asset_vertex_t);
            }

            if (!added_vertices[idx3])
            {
                generated->index_buffer[generated->indices++] = idx3;

                asset_vertex_t *dest_vertex = (asset_vertex_t *) generated->vertex_buffer + generated->vertices++;
                memcpy(dest_vertex, vertex_3, sizeof(asset_vertex_t));

                added_vertices[idx3] = true;
                generated->index_buffer_bytes += sizeof(u32);
                generated->vertex_buffer_bytes += sizeof(asset_vertex_t);
            }
        }

        free(added_vertices);
    }

    return generated;
}

void write_texture_to_file(chunk_buffer_t *buffer,
                           asset_config_source_t *source,
                           loaded_texture_t *texture,
                           asset_file_header_t *asset_header,
                           gc_file_t *asset_name_file,
                           gc_file_t *output_file)
{
    char tmp_string[255];

    sprintf(tmp_string, "%s: %llu bytes\n", source->alias, texture->meta.data_bytes);
    write_file(asset_name_file, strlen(tmp_string), tmp_string);

    if (texture->meta.type == ASSET_TEXTURE2D)
    {
        asset_header->total_assets++;
        asset_header->textures++;

        asset_metadata_t *metadata = register_asset(buffer);

        memcpy(metadata, &texture->meta, sizeof(asset_metadata_t));
        strncpy(metadata->name, source->alias, METADATA_NAME_LENGTH);

        texture2d_t *texture_object = (texture2d_t *) texture->data;

        // Copy the bitmap data to the chunk buffer.
        chunk_buffer_copy(buffer, texture_object->mips->header, texture_object->data_bytes);
        CHUNK_CHECK_FLUSH(asset_header, buffer, output_file);

        gc_destroy_texture2d(texture_object);
    }

    else if (texture->meta.type == ASSET_TEXTURE_CUBEMAP)
    {
        asset_header->total_assets++;
        asset_header->textures++;

        asset_metadata_t *metadata = register_asset(buffer);

        memcpy(metadata, &texture->meta, sizeof(asset_metadata_t));
        strncpy(metadata->name, source->alias, METADATA_NAME_LENGTH);

        cube_texture_t *texture_object = (cube_texture_t *) texture->data;

        // Copy the bitmap data to the chunk buffer.
        chunk_buffer_copy(buffer, texture_object->faces[0]->mips->header, texture_object->data_bytes);
        CHUNK_CHECK_FLUSH(asset_header, buffer, output_file);

        gc_destroy_cube_texture(texture_object);
    }

    else if (texture->meta.type == ASSET_TEXTURE_PBR_AMBIENT)
    {
        asset_header->total_assets += 4;
        asset_header->textures += 4;

        pbr_ambient_texture_t *texture_object = (pbr_ambient_texture_t *) texture->data;

        asset_metadata_t *metadata = register_asset(buffer);

        metadata->type = ASSET_TEXTURE_CUBEMAP;
        metadata->data_offset = 0;
        metadata->data_bytes = texture_object->environment->data_bytes;
        metadata->texture.width = texture_object->environment->faces[0]->mips->header->width;
        metadata->texture.height = texture_object->environment->faces[0]->mips->header->height;
        metadata->texture.mip_count = texture_object->environment->faces[0]->mip_count;
        sprintf(metadata->name, "%s_%s", source->alias, "environment");

        chunk_buffer_copy(buffer, texture_object->environment->faces[0]->mips->header, texture_object->environment->data_bytes);
        CHUNK_CHECK_FLUSH(asset_header, buffer, output_file);

        metadata = register_asset(buffer);

        metadata->type = ASSET_TEXTURE_CUBEMAP;
        metadata->data_offset = 0;
        metadata->data_bytes = texture_object->irradiance->data_bytes;
        metadata->texture.width = texture_object->irradiance->faces[0]->mips->header->width;
        metadata->texture.height = texture_object->irradiance->faces[0]->mips->header->height;
        metadata->texture.mip_count = texture_object->irradiance->faces[0]->mip_count;
        sprintf(metadata->name, "%s_%s", source->alias, "irradiance");

        chunk_buffer_copy(buffer, texture_object->irradiance->faces[0]->mips->header, texture_object->irradiance->data_bytes);
        CHUNK_CHECK_FLUSH(asset_header, buffer, output_file);

        metadata = register_asset(buffer);

        metadata->type = ASSET_TEXTURE_CUBEMAP;
        metadata->data_offset = 0;
        metadata->data_bytes = texture_object->prefiltered->data_bytes;
        metadata->texture.width = texture_object->prefiltered->faces[0]->mips->header->width;
        metadata->texture.height = texture_object->prefiltered->faces[0]->mips->header->height;
        metadata->texture.mip_count = texture_object->prefiltered->faces[0]->mip_count;
        sprintf(metadata->name, "%s_%s", source->alias, "prefiltered");

        chunk_buffer_copy(buffer, texture_object->prefiltered->faces[0]->mips->header, texture_object->prefiltered->data_bytes);
        CHUNK_CHECK_FLUSH(asset_header, buffer, output_file);

        metadata = register_asset(buffer);

        metadata->type = ASSET_TEXTURE2D;
        metadata->data_offset = 0;
        metadata->data_bytes = texture_object->brdf_lut->data_bytes;
        metadata->texture.width = texture_object->brdf_lut->mips->header->width;
        metadata->texture.height = texture_object->brdf_lut->mips->header->height;
        metadata->texture.mip_count = texture_object->brdf_lut->mip_count;
        sprintf(metadata->name, "%s_%s", source->alias, "brdf_lut");

        chunk_buffer_copy(buffer, texture_object->brdf_lut->mips->header, texture_object->brdf_lut->data_bytes);
        CHUNK_CHECK_FLUSH(asset_header, buffer, output_file);

        gc_destroy_pbr_ambient_texture(texture_object);
    }
}

// ----------------------------------------------------------------------------------

void write_asset_file(asset_config_t *asset_list, u32 selected_group, u32 selected_package)
{
    // ----------------------------------------------------------------------------------
    // -- Textures.
    // ----------------------------------------------------------------------------------

    if (selected_group == 0 || selected_group == 1)
    {
        asset_group_t *current_package = asset_list->textures;
        u32 package_index = 0;
        b8 is_selection = selected_package;

        while (current_package)
        {
            b8 write_package = true;

            if (is_selection && (selected_package - 1) != package_index)
                write_package = false;

            if (write_package && current_package->count)
            {
                gc_file_t package_file;
                char group_filepath[255];
                sprintf(group_filepath, "data/assets/textures/%s.pkg", current_package->name);
                open_file(&package_file, group_filepath, GC_FILE_WRITE);

                size_t offset = sizeof(asset_group_header_t);
                package_file.write_offset = offset;
                current_package->objects = 0;

                // Process each group source.
                for (u32 i = 0; i < current_package->count; ++i)
                {
                    asset_config_source_t *source = current_package->sources + i;
                    loaded_texture_t texture;

                    // ----------------------------------------------------------------------------------
                    // -- Cubemaps.
                    // ----------------------------------------------------------------------------------

                    if (source->type == ASSET_TEXTURE_CUBEMAP)
                    {
                        if (source->flags & SOURCE_DISABLED)
                            printf(builder_info_messages[PACKING_DISABLED], current_package->name, source->alias);
                        else
                        {
                            printf(builder_info_messages[PACKING], current_package->name, source->alias);

                            load_cubemap(source->cubepath, &texture);

                            if (source->flags & SOURCE_DEBUG)
                                debug_loaded_texture(source->alias, &texture);

                            if (texture.data)
                            {
                                current_package->objects++;
                                strncpy(texture.meta.name, current_package->prefix, METADATA_NAME_LENGTH);
                                strncat(texture.meta.name, "/", METADATA_NAME_LENGTH);
                                strncat(texture.meta.name, source->alias, METADATA_NAME_LENGTH);

                                cube_texture_t *texture_object = (cube_texture_t *) texture.data;
                                write_texture_to_package(&package_file, &texture.meta, texture_object->faces[0]->mips->header, &offset);

                                gc_destroy_cube_texture((cube_texture_t *) texture.data);
                            }
                        }
                    }

                    // ----------------------------------------------------------------------------------
                    // -- Textures.
                    // ----------------------------------------------------------------------------------

                    else
                    {
                        if (source->flags & SOURCE_DISABLED)
                            printf(builder_info_messages[PACKING_DISABLED], current_package->name, source->alias);
                        else
                        {
                            printf(builder_info_messages[PACKING], current_package->name, source->alias);

                            b8 loaded = load_texture(source->filepath, &texture);

                            if (source->flags & SOURCE_DEBUG)
                                debug_loaded_texture(source->filepath, &texture);

                            if (loaded && texture.data)
                            {
                                current_package->objects++;
                                // strncpy(texture.meta.name, source->alias, METADATA_NAME_LENGTH);
                                strncpy(texture.meta.name, current_package->prefix, METADATA_NAME_LENGTH);
                                strncat(texture.meta.name, "/", METADATA_NAME_LENGTH);
                                strncat(texture.meta.name, source->alias, METADATA_NAME_LENGTH);

                                texture2d_t *texture_object = (texture2d_t *) texture.data;
                                write_texture_to_package(&package_file, &texture.meta, texture_object->mips->header, &offset);

                                gc_destroy_texture2d((texture2d_t *) texture.data);
                            }
                        }
                    }
                }

                write_package_header(&package_file, current_package->objects);
                close_file(&package_file);
            }

            current_package = current_package->next;
            package_index++;
        }
    }

    // ----------------------------------------------------------------------------------
    // -- HDRs.
    // ----------------------------------------------------------------------------------

    if (selected_group == 0 || selected_group == 2)
    {
        asset_group_t *current_package = asset_list->hdrs;
        u32 package_index = 0;
        b8 is_selection = selected_package;

        while (current_package)
        {
            b8 write_package = true;

            if (is_selection && (selected_package - 1) != package_index)
                write_package = false;

            if (write_package && current_package->count)
            {
                gc_file_t package_file;
                char group_filepath[255];
                sprintf(group_filepath, "data/assets/textures/%s.pkg", current_package->name);
                open_file(&package_file, group_filepath, GC_FILE_WRITE);

                size_t offset = sizeof(asset_group_header_t);
                package_file.write_offset = offset;
                current_package->objects = 0;

                // Process each group source.
                for (u32 i = 0; i < current_package->count; ++i)
                {
                    asset_config_source_t *source = current_package->sources + i;
                    loaded_texture_t texture;

                    // ----------------------------------------------------------------------------------
                    // -- Pbr ambient.
                    // ----------------------------------------------------------------------------------

                    if (source->type == ASSET_TEXTURE_PBR_AMBIENT)
                    {
                        if (source->flags & SOURCE_DISABLED)
                            printf(builder_info_messages[PACKING_DISABLED], current_package->name, source->alias);
                        else
                        {
                            printf(builder_info_messages[PACKING], current_package->name, source->alias);

                            load_hdri_map(source->filepath, &texture);

                            if (source->flags & SOURCE_DEBUG)
                                debug_loaded_texture(source->alias, &texture);

                            pbr_ambient_texture_t *texture_object = (pbr_ambient_texture_t *) texture.data;

                            if (texture_object)
                            {
                                asset_metadata_t environment_metadata;
                                asset_metadata_t irradiance_metadata;
                                asset_metadata_t prefiltered_metadata;
                                asset_metadata_t brdf_lut_metadata;

                                // ----------------------------------------------------------------------------------
                                // -- Environment.
                                // ----------------------------------------------------------------------------------

                                current_package->objects++;

                                environment_metadata.type = ASSET_TEXTURE_CUBEMAP;
                                environment_metadata.data_offset = 0;
                                environment_metadata.data_bytes = texture_object->environment->data_bytes;
                                environment_metadata.texture.width = texture_object->environment->faces[0]->mips->header->width;
                                environment_metadata.texture.height = texture_object->environment->faces[0]->mips->header->height;
                                environment_metadata.texture.mip_count = texture_object->environment->faces[0]->mip_count;
                                // sprintf(environment_metadata.name, "%s_%s", source->alias, "environment");
                                strncpy(environment_metadata.name, current_package->prefix, METADATA_NAME_LENGTH);
                                strncat(environment_metadata.name, "/", METADATA_NAME_LENGTH);
                                strncat(environment_metadata.name, source->alias, METADATA_NAME_LENGTH);
                                strncat(environment_metadata.name, "_", METADATA_NAME_LENGTH);
                                strncat(environment_metadata.name, "environment", METADATA_NAME_LENGTH);

                                write_texture_to_package(&package_file, &environment_metadata, texture_object->environment->faces[0]->mips->header, &offset);

                                // ----------------------------------------------------------------------------------
                                // -- Irradiance.
                                // ----------------------------------------------------------------------------------

                                current_package->objects++;

                                irradiance_metadata.type = ASSET_TEXTURE_CUBEMAP;
                                irradiance_metadata.data_offset = 0;
                                irradiance_metadata.data_bytes = texture_object->irradiance->data_bytes;
                                irradiance_metadata.texture.width = texture_object->irradiance->faces[0]->mips->header->width;
                                irradiance_metadata.texture.height = texture_object->irradiance->faces[0]->mips->header->height;
                                irradiance_metadata.texture.mip_count = texture_object->irradiance->faces[0]->mip_count;
                                // sprintf(irradiance_metadata.name, "%s_%s", source->alias, "irradiance");
                                strncpy(irradiance_metadata.name, current_package->prefix, METADATA_NAME_LENGTH);
                                strncat(irradiance_metadata.name, "/", METADATA_NAME_LENGTH);
                                strncat(irradiance_metadata.name, source->alias, METADATA_NAME_LENGTH);
                                strncat(irradiance_metadata.name, "_", METADATA_NAME_LENGTH);
                                strncat(irradiance_metadata.name, "irradiance", METADATA_NAME_LENGTH);

                                write_texture_to_package(&package_file, &irradiance_metadata, texture_object->irradiance->faces[0]->mips->header, &offset);

                                // ----------------------------------------------------------------------------------
                                // -- Prefiltered.
                                // ----------------------------------------------------------------------------------

                                current_package->objects++;

                                prefiltered_metadata.type = ASSET_TEXTURE_CUBEMAP;
                                prefiltered_metadata.data_offset = 0;
                                prefiltered_metadata.data_bytes = texture_object->prefiltered->data_bytes;
                                prefiltered_metadata.texture.width = texture_object->prefiltered->faces[0]->mips->header->width;
                                prefiltered_metadata.texture.height = texture_object->prefiltered->faces[0]->mips->header->height;
                                prefiltered_metadata.texture.mip_count = texture_object->prefiltered->faces[0]->mip_count;
                                // sprintf(prefiltered_metadata.name, "%s_%s", source->alias, "prefiltered");
                                strncpy(prefiltered_metadata.name, current_package->prefix, METADATA_NAME_LENGTH);
                                strncat(prefiltered_metadata.name, "/", METADATA_NAME_LENGTH);
                                strncat(prefiltered_metadata.name, source->alias, METADATA_NAME_LENGTH);
                                strncat(prefiltered_metadata.name, "_", METADATA_NAME_LENGTH);
                                strncat(prefiltered_metadata.name, "prefiltered", METADATA_NAME_LENGTH);

                                write_texture_to_package(&package_file, &prefiltered_metadata, texture_object->prefiltered->faces[0]->mips->header, &offset);

                                // ----------------------------------------------------------------------------------
                                // -- Brdf lut.
                                // ----------------------------------------------------------------------------------

                                current_package->objects++;

                                brdf_lut_metadata.type = ASSET_TEXTURE2D;
                                brdf_lut_metadata.data_offset = 0;
                                brdf_lut_metadata.data_bytes = texture_object->brdf_lut->data_bytes;
                                brdf_lut_metadata.texture.width = texture_object->brdf_lut->mips->header->width;
                                brdf_lut_metadata.texture.height = texture_object->brdf_lut->mips->header->height;
                                brdf_lut_metadata.texture.mip_count = texture_object->brdf_lut->mip_count;
                                // sprintf(brdf_lut_metadata.name, "%s_%s", source->alias, "brdf_lut");
                                strncpy(brdf_lut_metadata.name, current_package->prefix, METADATA_NAME_LENGTH);
                                strncat(brdf_lut_metadata.name, "/", METADATA_NAME_LENGTH);
                                strncat(brdf_lut_metadata.name, source->alias, METADATA_NAME_LENGTH);
                                strncat(brdf_lut_metadata.name, "_", METADATA_NAME_LENGTH);
                                strncat(brdf_lut_metadata.name, "brdf_lut", METADATA_NAME_LENGTH);

                                write_texture_to_package(&package_file, &brdf_lut_metadata, texture_object->brdf_lut->mips->header, &offset);

                                gc_destroy_pbr_ambient_texture(texture_object);
                            }
                        }
                    }
                }

                write_package_header(&package_file, current_package->objects);
                close_file(&package_file);
            }

            current_package = current_package->next;
            package_index++;
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Meshes.
    // ----------------------------------------------------------------------------------

    if (selected_group == 0 || selected_group == 3)
    {
        asset_group_t *current_package = asset_list->meshes;
        u32 package_index = 0;
        b8 is_selection = selected_package;

        while (current_package)
        {
            b8 write_package = true;

            if (is_selection && (selected_package - 1) != package_index)
                write_package = false;

            if (write_package && current_package->count)
            {
                gc_file_t package_file;
                char group_filepath[255];
                sprintf(group_filepath, "data/assets/meshes/%s.pkg", current_package->name);
                open_file(&package_file, group_filepath, GC_FILE_WRITE);

                size_t offset = sizeof(asset_group_header_t);
                package_file.write_offset = offset;
                current_package->objects = 0;

                // Process each group source.
                for (u32 i = 0; i < current_package->count; ++i)
                {
                    asset_config_source_t *source = current_package->sources + i;

                    if (source->flags & SOURCE_DISABLED)
                        printf(builder_info_messages[PACKING_DISABLED], current_package->name, source->filepath);
                    else
                    {
                        printf(builder_info_messages[PACKING], current_package->name, source->filepath);
                        loaded_mesh_list_t *mesh_list = gltf_extract(source->filepath);

                        for (u32 mi = 0; mi < mesh_list->length; ++mi)
                        {
                            loaded_asset_mesh_t *mesh = mesh_list->meshes + mi;
                            char tmp_string[MESH_NAME_LENGTH];

                            if (source->flags & SOURCE_GENERATOR)
                            {
                                loaded_asset_mesh_t *mesh_lines = generate_line_mesh(mesh);
                                loaded_asset_mesh_t *mesh_points = generate_point_mesh(mesh);

                                current_package->objects++;
                                strncpy(tmp_string, current_package->prefix, MESH_NAME_LENGTH);
                                strncat(tmp_string, "/", MESH_NAME_LENGTH);
                                strncat(tmp_string, mesh_lines->name, MESH_NAME_LENGTH);
                                strncpy(mesh_lines->name, tmp_string, MESH_NAME_LENGTH);
                                write_mesh_to_package(&package_file, mesh_lines, &offset);

                                current_package->objects++;
                                strncpy(tmp_string, current_package->prefix, MESH_NAME_LENGTH);
                                strncat(tmp_string, "/", MESH_NAME_LENGTH);
                                strncat(tmp_string, mesh_points->name, MESH_NAME_LENGTH);
                                strncpy(mesh_points->name, tmp_string, MESH_NAME_LENGTH);
                                write_mesh_to_package(&package_file, mesh_points, &offset);

                                free(mesh_lines);
                                free(mesh_points);
                            }
                            else
                            {
                                current_package->objects++;

                                strncpy(tmp_string, current_package->prefix, MESH_NAME_LENGTH);
                                strncat(tmp_string, "/", MESH_NAME_LENGTH);
                                strncat(tmp_string, mesh->name, MESH_NAME_LENGTH);
                                strncpy(mesh->name, tmp_string, MESH_NAME_LENGTH);

                                write_mesh_to_package(&package_file, mesh, &offset);
                            }
                        }

                        free_loaded_mesh_list(mesh_list);
                    }
                }

                write_package_header(&package_file, current_package->objects);
                close_file(&package_file);
            }

            current_package = current_package->next;
            package_index++;
        }
    }
}

// ----------------------------------------------------------------------------------

void _asset_debug_texture_group(char *filepath)
{
    gc_file_t group_file;

    open_file(&group_file, filepath, GC_FILE_READ);
    void *data_buffer = malloc(group_file.bytes);
    read_file(&group_file, 0, group_file.bytes, data_buffer);

    asset_group_header_t *header = (asset_group_header_t *) data_buffer;
    void *base_ptr = header;
    size_t next_metadata_offset = sizeof(asset_group_header_t);

    assert(header->magic_value == 1346851649);

    for (u32 i = 0; i < header->count; ++i)
    {
        asset_metadata_t *metadata = (asset_metadata_t *) ADDR_OFFSET(base_ptr, next_metadata_offset);
        // texture_data_header_t *texture_data = (texture_data_header_t *) ADDR_OFFSET(base_ptr, metadata->data_offset);
        next_metadata_offset = metadata->next_meta_offset;
    }

    free(data_buffer);
    close_file(&group_file);
}

void _asset_debug_mesh_group(char *filepath)
{
    gc_file_t group_file;

    open_file(&group_file, filepath, GC_FILE_READ);
    void *data_buffer = malloc(group_file.bytes);
    read_file(&group_file, 0, group_file.bytes, data_buffer);

    asset_group_header_t *header = (asset_group_header_t *) data_buffer;
    void *base_ptr = header;
    size_t next_metadata_offset = sizeof(asset_group_header_t);

    assert(header->magic_value == 1346851649);

    for (u32 i = 0; i < header->count; ++i)
    {
        asset_metadata_t *metadata = (asset_metadata_t *) ADDR_OFFSET(base_ptr, next_metadata_offset);
        // u32 *index_buffer = (u32 *) ADDR_OFFSET(base_ptr, metadata->mesh.indices_offset);
        // asset_vertex_t *vertex_buffer = (asset_vertex_t *) ADDR_OFFSET(base_ptr, metadata->mesh.vertices_offset);
        next_metadata_offset = metadata->next_meta_offset;
    }

    free(data_buffer);
    close_file(&group_file);
}

void _display_group(asset_group_t *group, const char *name)
{
    u32 tabs = 5;
    u32 tab_count = 0;
    u32 package_index = 1;

    char tmp_string[255];
    printf("\n%s\n", name);

    while (group)
    {
        if (group->count)
        {
            if (tab_count % tabs == 0)
            {
                if (tab_count > 0)
                    printf("\n");

                sprintf(tmp_string, "[%d] %s", package_index++, group->name);
                printf("%-30s", tmp_string);
            }
            else
            {
                sprintf(tmp_string, "[%d] %s", package_index++, group->name);
                printf("%-30s", tmp_string);
            }

            tab_count++;
        }

        group = group->next;
    }

    printf("\n");
}

void _view_package_group(asset_config_t *asset_list, u32 selected_group, u32 selected_package)
{
    asset_group_t *package = 0;
    b8 check = false;

    if (selected_group == 1)
    {
        package = asset_list->textures;
        check = selected_package <= asset_list->texture_groups;
    }
    else if (selected_group == 2)
    {
        package = asset_list->hdrs;
        check = selected_package <= asset_list->hdr_groups;
    }
    else if (selected_group == 3)
    {
        // NOTE(gabic): Meshes get their alias after reading the model file, so nothing is displayed here.
        // package = asset_list->meshes;
        // check = selected_package <= asset_list->mesh_groups;
    }

    if (package && check)
    {
        u32 index = 0;

        while (++index < selected_package)
            package = package->next;

        for (u32 i = 0; i < package->count; ++i)
        {
            asset_config_source_t *source = package->sources + i;
            printf("[%d] %s/%s\n", i + 1, package->prefix, source->alias);
        }
    }
}

void read_char(char *input)
{
    char tmp = '\0';
    scanf("%c", &tmp);
    *input = tmp;

    while (tmp != '\n')
        scanf("%c", &tmp);
}

void read_u32(u32 *input)
{
    char tmp_string[255];
    char c = '\0';
    u32 char_count = 0;

    while ((c = getchar()) != '\n')
        tmp_string[char_count++] = c;

    tmp_string[char_count++] = '\0';
    *input = atoi(tmp_string);
}

int main(int argc, char *argv[])
{
    init_threads();

    asset_config_t asset_list;
    char config_asset_file[] = CONFIG_ASSET_FILE;
    parse_asset_config_json(config_asset_file, &asset_list);

    // ----------------------------------------------------------------------------------
    // -- Menu.
    // ----------------------------------------------------------------------------------

    b8 done = false;
    char selection = '\0';

    _display_group(asset_list.textures, "[1. textures]");
    _display_group(asset_list.hdrs, "[2. hdrs]");
    _display_group(asset_list.meshes, "[3. meshes]");

    int input_char = 0;

    while (!done)
    {
        printf("\n[l] list / [v] view / [a] all / [s] selected / [x] exit\n");

        printf(">> %c", input_char);
        read_char(&selection);

        if (selection == 'v')
        {
            u32 selected_group = 0;
            u32 selected_package = 0;

            printf("group: ");
            read_u32(&selected_group);
            printf("package: ");
            read_u32(&selected_package);

            printf("\n");
            _view_package_group(&asset_list, selected_group, selected_package);
        }
        else if (selection == 'l')
        {
            _display_group(asset_list.textures, "[1. textures]");
            _display_group(asset_list.hdrs, "[2. hdrs]");
            _display_group(asset_list.meshes, "[3. meshes]");
        }
        else if (selection == 'a')
        {
            printf("\n");
            write_asset_file(&asset_list, 0, 0);
        }
        else if (selection == 's')
        {
            u32 selected_group = 0;
            u32 selected_package = 0;

            printf("group: ");
            read_u32(&selected_group);
            printf("package: ");
            read_u32(&selected_package);

            printf("\n");
            write_asset_file(&asset_list, selected_group, selected_package);
        }
        else if (selection == 'x') {
            done = true;
        }
    }

    // _asset_debug_texture_group("data/assets/textures/hdr_waterfall.pkg");
    // _asset_debug_texture_group("data/assets/textures/cubemaps.pkg");
    // _asset_debug_mesh_group("data/assets/meshes/shapes.pkg");

    // debug_asset_config(&asset_list);
    free_asset_config(&asset_list);

    destroy_threads();

    return 0;
}