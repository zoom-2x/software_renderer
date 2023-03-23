// ----------------------------------------------------------------------------------
// -- File: gltf_reader.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-04-20 22:01:45
// -- Modified: 2022-04-20 22:01:47
// ----------------------------------------------------------------------------------

typedef enum
{
    GLTF_POSITION,
    GLTF_NORMAL,
    GLTF_TANGENT,
    GLTF_UV,
    GLTF_INDICES,
} gltf_buffer_type_t;

typedef enum
{
    GLTF_BYTE = 5120,
    GLTF_UNSIGNED_BYTE = 5121,
    GLTF_SHORT = 5122,
    GLTF_UNSIGNED_SHORT = 5123,
    GLTF_UNSIGNED_INT = 5125,
    GLTF_FLOAT = 5126,
} gltf_component_type_t;

typedef enum
{
    GLTF_SCALAR = 1,
    GLTF_VEC2,
    GLTF_VEC3,
    GLTF_VEC4,
    GLTF_MAT2,
    GLTF_MAT3,
    GLTF_MAT4,
} gltf_type_t;

typedef enum
{
    BUFFER_POSITION,
    BUFFER_UV,
    BUFFER_NORMAL,
    BUFFER_TANGENT,
    BUFFER_INDICES,
} buffer_type_t;

typedef struct
{
    buffer_type_t buffer_type;
    u32 buffer_index;
    u32 byte_length;
    u32 byte_offset;
    gltf_type_t type;
    gltf_component_type_t component_type;
    u32 count;
    void *data;
} gltf_bufferview_t;

typedef struct
{
    u32 chunk_length;
    u32 chunk_type;
    u8 data;
} gltf_chunk_t;

typedef struct
{
    u32 magic;
    u32 version;
    u32 length;
} gltf_header_t;

typedef struct
{
    gltf_bufferview_t indices;
    // position, uv, normal, tangent
    gltf_bufferview_t attributes[4];
    u8 attr_count;
} gltf_primitive_set_t;

typedef struct
{
    char *name;
    gltf_primitive_set_t *sets;
    u32 set_count;
    // u8 buffer_count;
    // u32 indices;
    // u32 vertices;
} gltf_mesh_t;

#define GLTF_MAGIC 0x46546C67
#define GLTF_CHUNK_JSON 0x4E4F534A
#define GLTF_CHUNK_BIN 0x004E4942
#define GLTF_CHUNK_HEADER_SIZE (2 * sizeof(u32))

// ----------------------------------------------------------------------------------
// -- Function used to read a single gltf json + binary file.
// ----------------------------------------------------------------------------------

u32 gltf_get_index(gltf_bufferview_t *buffer_indices, u32 offset)
{
    u32 res = 0;

    if (buffer_indices->component_type == GLTF_UNSIGNED_SHORT)
        res = *((u16 *) buffer_indices->data + offset);
    else
        res = *((u32 *) buffer_indices->data + offset);

    return res;
}

void _gltf_extract_buffer_data(struct _json_object_entry *accessors,
                               struct _json_object_entry *attr,
                               struct _json_object_entry *bufferviews,
                               gltf_bufferview_t *mesh_buffer)
{
    u32 current_accessor_index = attr->value->u.integer;
    json_value *current_accessor = accessors->value->u.array.values[current_accessor_index];

    for (u32 k1 = 0; k1 < current_accessor->u.object.length; ++k1)
    {
        struct _json_object_entry *current_prop = current_accessor->u.object.values + k1;

        if (strcmp(current_prop->name, "bufferView") == 0)
        {
            u32 bufferview_index = current_prop->value->u.integer;
            json_value *current_bufferview = bufferviews->value->u.array.values[bufferview_index];

            struct _json_object_entry *buffer_prop = current_bufferview->u.object.values;
            struct _json_object_entry *byte_length_prop = current_bufferview->u.object.values + 1;
            struct _json_object_entry *byte_offset_prop = current_bufferview->u.object.values + 2;

            mesh_buffer->buffer_index = buffer_prop->value->u.integer;
            mesh_buffer->byte_length = byte_length_prop->value->u.integer;
            mesh_buffer->byte_offset = byte_offset_prop->value->u.integer;
        }
        else if (strcmp(current_prop->name, "componentType") == 0) {
            mesh_buffer->component_type = (gltf_component_type_t) current_prop->value->u.integer;
        }
        else if (strcmp(current_prop->name, "count") == 0) {
            mesh_buffer->count = current_prop->value->u.integer;
        }
        else if (strcmp(current_prop->name, "type") == 0)
        {
            if (strcmp(current_prop->value->u.string.ptr, "VEC2") == 0)
                mesh_buffer->type = GLTF_VEC2;
            else if (strcmp(current_prop->value->u.string.ptr, "VEC3") == 0)
                mesh_buffer->type = GLTF_VEC3;
            else if (strcmp(current_prop->value->u.string.ptr, "VEC4") == 0)
                mesh_buffer->type = GLTF_VEC4;
            else if (strcmp(current_prop->value->u.string.ptr, "MAT2") == 0)
                mesh_buffer->type = GLTF_MAT2;
            else if (strcmp(current_prop->value->u.string.ptr, "MAT3") == 0)
                mesh_buffer->type = GLTF_MAT3;
            else if (strcmp(current_prop->value->u.string.ptr, "MAT4") == 0)
                mesh_buffer->type = GLTF_MAT4;
            else if (strcmp(current_prop->value->u.string.ptr, "SCALAR") == 0)
                mesh_buffer->type = GLTF_SCALAR;
        }
    }
}

loaded_mesh_list_t *gltf_extract(char *filepath)
{
    loaded_mesh_list_t *mesh_list = 0;
    gltf_mesh_t *gltf_mesh = 0;

    gc_file_t gltf_file;
    open_file(&gltf_file, filepath, GC_FILE_READ);

    if (gltf_file.handle)
    {
        void *data_buffer = malloc(gltf_file.bytes);
        read_file(&gltf_file, 0, gltf_file.bytes, data_buffer);
        gltf_header_t *gltf = (gltf_header_t *) data_buffer;

        if (gltf->magic != GLTF_MAGIC)
            EXIT_ALERT_ARGS("[GLTF] Invalid format {\"%s\"} !\n", filepath);

        u32 header_size = 12;
        u8 *gltf_chunk_pointer = (u8 *) gltf + header_size;

        gltf_chunk_t *json_chunk = (gltf_chunk_t *) gltf_chunk_pointer;
        gltf_chunk_t *bin_chunk = (gltf_chunk_t *) (gltf_chunk_pointer +
                                                    GLTF_CHUNK_HEADER_SIZE +
                                                    json_chunk->chunk_length);

        if (json_chunk->chunk_type != GLTF_CHUNK_JSON || bin_chunk->chunk_type != GLTF_CHUNK_BIN)
            EXIT_ALERT_ARGS("[GLTF] Invalid format {\"%s\"} !\n", filepath);

        char *json_data = (char *) &json_chunk->data;
        u8 *bin_data = (u8 *) &bin_chunk->data;

        // -- JSON parsing.

        json_value *json = json_parse((json_char *) json_data, json_chunk->chunk_length, 0);

        if (!json)
            EXIT_ALERT_ARGS("[GLTF] Corrupted gltf file {\"%s\"} !\n", filepath);

        struct _json_object_entry *meshes = 0;
        struct _json_object_entry *accessors = 0;
        struct _json_object_entry *bufferviews = 0;
        struct _json_object_entry *buffers = 0;

        // Get the relevant buffers.
        for (u16 i = 0; i < json->u.object.length; ++i)
        {
            struct _json_object_entry *entry = json->u.object.values + i;

            if (strcmp(entry->name, "meshes") == 0)
                meshes = entry;

            if (strcmp(entry->name, "accessors") == 0)
                accessors = entry;

            if (strcmp(entry->name, "bufferViews") == 0)
                bufferviews = entry;

            if (strcmp(entry->name, "buffers") == 0)
                buffers = entry;
        }

        if (!meshes || !accessors || !bufferviews || !buffers)
            EXIT_ALERT("[GLTF] Missing important gltf entries !\n");

        size_t mesh_list_bytes = sizeof(loaded_mesh_list_t) +
                                 sizeof(loaded_asset_mesh_t) * meshes->value->u.array.length;
        mesh_list = (loaded_mesh_list_t *) malloc(mesh_list_bytes);
        memset(mesh_list, 0, mesh_list_bytes);

        mesh_list->meshes = (loaded_asset_mesh_t *) (mesh_list + 1);
        mesh_list->length = meshes->value->u.array.length;

        // ----------------------------------------------------------------------------------
        // -- Process each mesh.
        // ----------------------------------------------------------------------------------

        for (u16 mi = 0; mi < meshes->value->u.array.length; ++mi)
        {
            json_value *current_mesh = meshes->value->u.array.values[mi];
            struct _json_object_entry *mesh_name = current_mesh->u.object.values;
            struct _json_object_entry *mesh_sets = current_mesh->u.object.values + 1;

            size_t gltf_mesh_byte_length = sizeof(gltf_mesh_t) + sizeof(gltf_primitive_set_t) * mesh_sets->value->u.array.length;
            gltf_mesh = (gltf_mesh_t *) malloc(gltf_mesh_byte_length);
            memset(gltf_mesh, 0, gltf_mesh_byte_length);
            gltf_mesh->sets = (gltf_primitive_set_t *) (gltf_mesh + 1);
            gltf_mesh->name = mesh_name->value->u.string.ptr;

            // Process each primitive set.
            for (u32 j = 0; j < mesh_sets->value->u.array.length; ++j)
            {
                json_value *mesh_buffers = mesh_sets->value->u.array.values[j];
                struct _json_object_entry *attributes = mesh_buffers->u.object.values + 0;
                struct _json_object_entry *indices = mesh_buffers->u.object.values + 1;

                gltf_primitive_set_t *current_set = gltf_mesh->sets + gltf_mesh->set_count++;

                // Extract the attribute buffer data.
                for (u16 k = 0; k < attributes->value->u.object.length; ++k)
                {
                    struct _json_object_entry *attr = attributes->value->u.object.values + k;

                    if (strcmp(attr->name, "POSITION") == 0)
                    {
                        gltf_bufferview_t *gltf_buffer = current_set->attributes + current_set->attr_count++;
                        _gltf_extract_buffer_data(accessors, attr, bufferviews, gltf_buffer);
                        gltf_buffer->buffer_type = BUFFER_POSITION;
                    }
                    else if (strcmp(attr->name, "NORMAL") == 0)
                    {
                        gltf_bufferview_t *gltf_buffer = current_set->attributes + current_set->attr_count++;
                        _gltf_extract_buffer_data(accessors, attr, bufferviews, gltf_buffer);
                        gltf_buffer->buffer_type = BUFFER_NORMAL;
                    }
                    else if (strcmp(attr->name, "TANGENT") == 0)
                    {
                        gltf_bufferview_t *gltf_buffer = current_set->attributes + current_set->attr_count++;
                        _gltf_extract_buffer_data(accessors, attr, bufferviews, gltf_buffer);
                        gltf_buffer->buffer_type = BUFFER_TANGENT;
                    }
                    else if (strcmp(attr->name, "TEXCOORD_0") == 0)
                    {
                        gltf_bufferview_t *gltf_buffer = current_set->attributes + current_set->attr_count++;
                        _gltf_extract_buffer_data(accessors, attr, bufferviews, gltf_buffer);
                        gltf_buffer->buffer_type = BUFFER_UV;
                    }
                }

                gltf_bufferview_t *gltf_buffer = &current_set->indices;
                _gltf_extract_buffer_data(accessors, indices, bufferviews, gltf_buffer);
                gltf_buffer->buffer_type = BUFFER_INDICES;
            }

            // -- Determine the total mesh indices and vertices.

            u32 total_mesh_indices = 0;
            u32 total_mesh_vertices = 0;

            for (u32 k = 0; k < gltf_mesh->set_count; ++k)
            {
                gltf_primitive_set_t *set = gltf_mesh->sets + k;

                for (u32 k1 = 0; k1 < set->attr_count; ++k1)
                {
                    if (set->attributes[k1].buffer_type == BUFFER_POSITION)
                        total_mesh_vertices += set->attributes[k1].count;
                }

                total_mesh_indices += set->indices.count;
            }

            // ----------------------------------------------------------------------------------
            // -- Generate the mesh data.
            // ----------------------------------------------------------------------------------

            u32 mesh_indices_bytes = MULTIPLE_OF(sizeof(u32) * total_mesh_indices, 16);
            u32 mesh_vertices_bytes = sizeof(asset_vertex_t) * total_mesh_vertices;
            size_t mesh_data_bytes = mesh_indices_bytes + mesh_vertices_bytes;

            void *mesh_data = malloc(mesh_data_bytes);
            memset(mesh_data, 0, mesh_data_bytes);

            // Add mesh to list.
            loaded_asset_mesh_t *loaded_mesh = mesh_list->meshes + mi;

            loaded_mesh->index_buffer = (u32 *) mesh_data;
            loaded_mesh->vertex_buffer = (u8 *) loaded_mesh->index_buffer + mesh_indices_bytes;

            strncpy(loaded_mesh->name, gltf_mesh->name, 50);
            loaded_mesh->type = GL_MESH_TRIANGLE;
            loaded_mesh->indices = total_mesh_indices;
            loaded_mesh->vertices = total_mesh_vertices;
            loaded_mesh->index_buffer_bytes = mesh_indices_bytes;
            loaded_mesh->vertex_buffer_bytes = mesh_vertices_bytes;

            // Start copying.
            u32 *dest_index_buffer = loaded_mesh->index_buffer;
            asset_vertex_t *dest_vertex_buffer = (asset_vertex_t *) loaded_mesh->vertex_buffer;
            u32 global_vertex_index = 0;

            for (u32 si = 0; si < gltf_mesh->set_count; ++si)
            {
                gltf_primitive_set_t *current_set = gltf_mesh->sets + si;
                gltf_bufferview_t *indices = &current_set->indices;

                // Read every index and collect the attribute data.
                u8 *indices_pointer = bin_data + indices->byte_offset;

                // size_t extra_buffer_bytes = Kilobytes(512);
                size_t extra_buffer_bytes = total_mesh_vertices * sizeof(u32);
                u32 *added_vertices = (u32 *) malloc(extra_buffer_bytes);
                memset(added_vertices, 0, extra_buffer_bytes);

                for (u32 vi = 0; vi < indices->count; ++vi)
                {
                    u32 vertex_index = 0;

                    if (indices->component_type == GLTF_UNSIGNED_SHORT)
                        vertex_index = *((u16 *) indices_pointer + vi);
                    else
                        vertex_index = *((u32 *) indices_pointer + vi);

                    if (vertex_index > extra_buffer_bytes)
                        EXIT_ALERT("[GLTF] Index verification buffer overflow !\n");

                    if (added_vertices[vertex_index])
                        *dest_index_buffer++ = added_vertices[vertex_index] - 1;
                    else
                    {
                        // Zero means that no vertex index was added into that slot, so each index is incremented.
                        added_vertices[vertex_index] = global_vertex_index + 1;

                        *dest_index_buffer++ = global_vertex_index;
                        asset_vertex_t *dest_vertex = dest_vertex_buffer + global_vertex_index;
                        global_vertex_index++;

                        // NOTE(gabic): For now assume all attributes contain floats.
                        for (u32 ai = 0; ai < current_set->attr_count; ++ai)
                        {
                            gltf_bufferview_t *current_attribute = current_set->attributes + ai;
                            u8 *attribute_pointer = bin_data + current_attribute->byte_offset;

                            // Fixed attribute position.
                            if (current_attribute->buffer_type == BUFFER_POSITION)
                            {
                                vec3 *src_position = (vec3 *) attribute_pointer + vertex_index;

                                dest_vertex->pos[0] = src_position->x;
                                dest_vertex->pos[1] = src_position->y;
                                dest_vertex->pos[2] = src_position->z;
                            }
                            else if (current_attribute->buffer_type == BUFFER_UV)
                            {
                                vec2 *src_uv = (vec2 *) attribute_pointer + vertex_index;

                                dest_vertex->data[0] = src_uv->x;
                                dest_vertex->data[1] = src_uv->y;
                            }
                            else if (current_attribute->buffer_type == BUFFER_NORMAL)
                            {
                                vec3 *src_normal = (vec3 *) attribute_pointer + vertex_index;

                                dest_vertex->data[2] = src_normal->x;
                                dest_vertex->data[3] = src_normal->y;
                                dest_vertex->data[4] = src_normal->z;
                            }
                            else if (current_attribute->buffer_type == BUFFER_TANGENT)
                            {
                                vec4 *src_tangent = (vec4 *) attribute_pointer + vertex_index;

                                dest_vertex->data[5] = src_tangent->x;
                                dest_vertex->data[6] = src_tangent->y;
                                dest_vertex->data[7] = src_tangent->z;
                                dest_vertex->data[8] = src_tangent->w;
                            }
                        }
                    }
                }

                free(added_vertices);
            }

            free(gltf_mesh);
        }

        json_value_free(json);
        free(data_buffer);
        close_file(&gltf_file);
    }

    return mesh_list;
}

void gltf_free(gltf_mesh_t *asset) {
    free(asset);
}