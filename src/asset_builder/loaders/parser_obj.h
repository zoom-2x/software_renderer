// ----------------------------------------------------------------------------------
// -- File: parser_obj.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-11-21 18:59:04
// -- Modified: 2021-11-27 22:29:08
// ----------------------------------------------------------------------------------

#ifndef GCSR_PARSER_OBJ_H
#define GCSR_PARSER_OBJ_H

#define INDICES_MINUS_ONE 1

// typedef __ALIGN__ struct
// {
//     r32 data[VERTEX_ATTRIBUTE_SIZE];
//     r32 pos[4];
// } tmp_vertex_t;

typedef struct
{
    gl_mesh_attribute_type_t type;

    u32 bytes;
    u32 count;
    u8 components;

    void *memory;
} parser_model_attribute_buffer_t;

typedef struct
{
    u32 bytes;
    u32 count;
    u32 vertices;

    u32 *memory;
} parser_model_primitive_buffer_t;

typedef struct
{
    gc_mesh_type_t type;
    u8 buffer_count;
    u8 attr_mask;

    parser_model_primitive_buffer_t primitives;
    parser_model_attribute_buffer_t buffers[GL_MAX_ATTRIBUTE_COUNT];
} parser_model_data_t;

// ----------------------------------------------------------------------------------

vec4 sRGB_linear1(vec4 Color)
{
    vec4 Result;

    Result.r = Color.r * ONE_OVER_255;
    Result.g = Color.g * ONE_OVER_255;
    Result.b = Color.b * ONE_OVER_255;
    Result.a = Color.a * ONE_OVER_255;

    return Result;
}

__INLINE__ vec4 linear1_sRGB255(vec4 Color)
{
    vec4 Result;
#if 0
    Result.r = square_root(Color.r) * 255;
    Result.g = square_root(Color.g) * 255;
    Result.b = square_root(Color.b) * 255;
#else
    Result.r = Color.r * 255;
    Result.g = Color.g * 255;
    Result.b = Color.b * 255;
#endif
    Result.a = Color.a * 255;

    return Result;
}

#define vec3_inner(v1, v2) (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z)
#define vec3_dot(v1, v2) (vec3_inner(v1, v2))
#define vec3_add(v1, v2) {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}
#define vec3_sub(v1, v2) {(v1).x - (v2).x, (v1).y - (v2).y, (v1).z - (v2).z}
#define vec3_subp(v1, v2) {v1->x - v2->x, v1->y - v2->y, v1->z - v2->z}
#define vec3_muls(v1, s) {v1.x * s, v1.y * s, v1.z * s}
#define vec3_len(v) (sqrtf(vec3_inner(v, v)))
#define vec3_cross(v1, v2) {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x}

vec3 force_perp(vec3 v1, vec3 v2)
{
    r32 s = vec3_dot(v1, v2) / vec3_dot(v2, v2);
    vec3 tmp = vec3_muls(v2, s);
    return vec3_sub(v1, tmp);
}

__INLINE__ vec3 vec3_normalize(vec3 v)
{
    r32 len = vec3_len(v);
    assert(len);
    r32 one_over_len = 1.0f / len;

    return {v.x * one_over_len, v.y * one_over_len, v.z * one_over_len};
}

u32 parser_hash(u32 *vertex, u32 mod)
{
#if 1
    #define HASH_BASE 31
    u32 res = (((vertex[0] * HASH_BASE + vertex[1]) % mod) * HASH_BASE + vertex[2]) % mod;
#else
    u64 hash = 5381;
    hash = ((hash << 5) + hash) + vertex[0];
    hash = ((hash << 5) + hash) + vertex[1];
    hash = ((hash << 5) + hash) + vertex[2];

    u32 res = hash % mod;
#endif

    return res;
}

typedef struct
{
    u32 vertex[3];
    u32 index;
} vertex_hashtable_item_t;

typedef struct
{
    u32 slots;
    u32 used;
    r32 fill_rate;

    vertex_hashtable_item_t *data;
} vertex_hashtable_t;

vertex_hashtable_t *parser_hashtable_create(u32 slots)
{
    size_t bytes = sizeof(vertex_hashtable_t) + sizeof(vertex_hashtable_item_t) * slots;
    vertex_hashtable_t *res = (vertex_hashtable_t *) malloc(bytes);
    memset(res, 0, bytes);

    res->slots = slots;
    res->used = 0;
    res->fill_rate = 0;
    res->data = (vertex_hashtable_item_t *) (res + 1);

    return res;
}

#define hashtable_get_item(table, hash_index, offset) (&table->data[(hash_index + offset * offset) % table->slots]);

vertex_hashtable_item_t *parser_hashtable_get(vertex_hashtable_t *table, u32 *vertex)
{
    vertex_hashtable_item_t *res = 0;
    b8 done = false;

    u32 offset = 0;
    u32 hash_index = parser_hash(vertex, table->slots);

    while (!done)
    {
        vertex_hashtable_item_t *current = hashtable_get_item(table, hash_index, offset);

        if (current->vertex[0] == vertex[0] &&
            current->vertex[1] == vertex[1] &&
            current->vertex[2] == vertex[2])
        {
            done = true;
            res = current;
        }
        else if (!current->vertex[0] && !current->vertex[1] && !current->vertex[2]) {
            done = true;
        }
        else
            offset++;
    }

    return res;
}

b8 parser_hashtable_insert(vertex_hashtable_t *table, u32 *vertex, u32 index)
{
    b8 inserted = false;
    u32 offset = 0;
    u32 hash_index = parser_hash(vertex, table->slots);
    b8 done = false;
    b8 slot_found = false;

    assert(table->fill_rate < 0.95f);

    while (!done)
    {
        vertex_hashtable_item_t *current = hashtable_get_item(table, hash_index, offset);

        if (current->vertex[0] == vertex[0] &&
            current->vertex[1] == vertex[1] &&
            current->vertex[2] == vertex[2])
        {
            done = true;
        }
        else if (!current->vertex[0] && !current->vertex[1] && !current->vertex[2])
        {
            done = true;
            slot_found = true;
        }
        else
            offset++;
    }

    if (slot_found)
    {
        vertex_hashtable_item_t *slot = hashtable_get_item(table, hash_index, offset);

        table->used++;
        table->fill_rate = (r32) table->used / table->slots;

        slot->vertex[0] = vertex[0];
        slot->vertex[1] = vertex[1];
        slot->vertex[2] = vertex[2];
        slot->index = index;

        inserted = true;
    }

    return inserted;
}

// ----------------------------------------------------------------------------------
// -- OBJ parser.
// ----------------------------------------------------------------------------------

loaded_asset_mesh_t *parse_obj2(gc_file_t *file)
{
    size_t buffer_bytes = Megabytes(64);
    size_t used_bytes = 0;

    u8 *work_buffer = (u8 *) malloc(buffer_bytes);
    memset(work_buffer, 0, buffer_bytes);

    u32 buffer_count = 0;
    gc_mesh_type_t mesh_type = GL_MESH_NONE;

    vec3 *position_buffer = 0;
    u32 position_count = 0;

    vec2 *uv_buffer = 0;
    u32 uv_count = 0;

    vec3 *normal_buffer = 0;
    u32 normal_count = 0;

    vec4 *tangent_buffer = 0;

    // NOTE(gabic): obj buffers -> position + uv + normal
    #define OBJ_BUFFER_COUNT 3

    u32 (*vertices)[OBJ_BUFFER_COUNT] = 0;
    u32 vertex_count = 0;
    u32 primitive_count = 0;

    string_array_t tmp_buffer;
    string_array_t *buffer = &tmp_buffer;

    while (file->cursor < file->bytes)
    {
        STRARR_RESET_MARKERS(buffer);
        b8 res = parser_read_line(file, buffer);

        if (res)
        {
            string_buffer_t *LineBuffer = STRARR_GET_LAST_STRING(buffer);

            // Comment.
            if (LineBuffer->buffer[0] == '#')
                continue;

            // ----------------------------------------------------------------------------------
            // -- Position buffer.
            // ----------------------------------------------------------------------------------

            parser_tokenize_line(buffer, LineBuffer, ' ');
            string_marker_t *Marker = STRARR_GET_LAST_MARKER(buffer);
            string_buffer_t *Token = &buffer->array[Marker->start];

            if (strcmp(Token->buffer, "v") == 0 && Marker->len == 4)
            {
                if (!position_buffer)
                {
                    position_buffer = (vec3 *) (work_buffer + used_bytes);
                    buffer_count++;
                }

                if ((used_bytes + sizeof(vec3)) > buffer_bytes)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                vec3 *vertex = position_buffer + position_count++;

                Token++;
                vertex->x = (r32) atof(Token->buffer);
                Token++;
                vertex->y = (r32) atof(Token->buffer);
                Token++;
                vertex->z = (r32) atof(Token->buffer);

                used_bytes += sizeof(vec3);
            }

            // ----------------------------------------------------------------------------------
            // -- UV buffer.
            // ----------------------------------------------------------------------------------

            else if (strcmp(Token->buffer, "vt") == 0 && Marker->len == 3)
            {
                if (!uv_buffer)
                {
                    uv_buffer = (vec2 *) (work_buffer + used_bytes);
                    buffer_count++;
                }

                if ((used_bytes + sizeof(vec2)) > buffer_bytes)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                vec2 *uv = uv_buffer + uv_count++;

                Token++;
                uv->x = (r32) atof(Token->buffer);
                Token++;
                uv->y = (r32) atof(Token->buffer);

                used_bytes += sizeof(vec2);
            }

            // ----------------------------------------------------------------------------------
            // -- Normal buffer.
            // ----------------------------------------------------------------------------------

            else if (strcmp(Token->buffer, "vn") == 0 && Marker->len == 4)
            {
                if (!normal_buffer)
                {
                    normal_buffer = (vec3 *) (work_buffer + used_bytes);
                    buffer_count++;
                }

                if ((used_bytes + sizeof(vec3)) > buffer_bytes)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                vec3 *normal = normal_buffer + normal_count++;

                Token++;
                normal->x = (r32) atof(Token->buffer);
                Token++;
                normal->y = (r32) atof(Token->buffer);
                Token++;
                normal->z = (r32) atof(Token->buffer);

                used_bytes += sizeof(vec3);
            }

            // ----------------------------------------------------------------------------------
            // -- Primitives (faces).
            // ----------------------------------------------------------------------------------

            // Triangle.
            else if (strcmp(Token->buffer, "f") == 0 && Marker->len == 4)
            {
                if (!vertices)
                {
                    mesh_type = GL_MESH_TRIANGLE;
                    vertices = (u32 (*)[OBJ_BUFFER_COUNT]) (work_buffer + used_bytes);
                }

                if ((used_bytes + 3 * (buffer_count * sizeof(u32))) > buffer_bytes)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                primitive_count++;

                // Parse each vertex.
                for (u32 k = 0; k < 3; ++k)
                {
                    u32 *vertex = (u32 *) (vertices + vertex_count++);

                    Token++;

                    parser_tokenize_line(buffer, Token, '/');
                    string_marker_t *PrimitiveMarker = STRARR_GET_LAST_MARKER(buffer);
                    assert(PrimitiveMarker->len == buffer_count);

                    u32 pstart = PrimitiveMarker->start;
                    string_buffer_t *PrimitiveToken = &buffer->array[pstart];

                    for (u32 l = 0; l < PrimitiveMarker->len; ++l)
                    {
                        vertex[l] = atoi(PrimitiveToken->buffer);
                        PrimitiveToken++;
                        used_bytes += sizeof(u32);
                    }

                    STRARR_REWIND_MARKERS(buffer);
                }
            }

            // Line.
            else if (strcmp(Token->buffer, "f") == 0 && Marker->len == 3)
            {
                if (!vertices)
                {
                    mesh_type = GL_MESH_LINE;
                    vertices = (u32 (*)[OBJ_BUFFER_COUNT]) (work_buffer + used_bytes);
                }

                if ((used_bytes + 2 * (buffer_count * sizeof(u32))) > buffer_bytes)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                primitive_count++;

                // Parse each vertex.
                for (u32 k = 0; k < 2; ++k)
                {
                    u32 *vertex = (u32 *) (vertices + vertex_count++);

                    Token++;

                    parser_tokenize_line(buffer, Token, '/');
                    string_marker_t *PrimitiveMarker = STRARR_GET_LAST_MARKER(buffer);
                    assert(PrimitiveMarker->len == buffer_count);

                    u32 pstart = PrimitiveMarker->start;
                    string_buffer_t *PrimitiveToken = &buffer->array[pstart];

                    for (u32 l = 0; l < PrimitiveMarker->len; ++l)
                    {
                        vertex[l] = atoi(PrimitiveToken->buffer);
                        PrimitiveToken++;
                        used_bytes += sizeof(u32);
                    }

                    STRARR_REWIND_MARKERS(buffer);
                }
            }

            // Point.
            else if (strcmp(Token->buffer, "f") == 0 && Marker->len == 2)
            {
                if (!vertices)
                {
                    mesh_type = GL_MESH_POINT;
                    vertices = (u32 (*)[OBJ_BUFFER_COUNT]) (work_buffer + used_bytes);
                }

                if ((used_bytes + 1 * (buffer_count * sizeof(u32))) > buffer_bytes)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                primitive_count++;
                u32 *vertex = (u32 *) (vertices + vertex_count++);

                Token++;

                parser_tokenize_line(buffer, Token, '/');
                string_marker_t *PrimitiveMarker = STRARR_GET_LAST_MARKER(buffer);
                assert(PrimitiveMarker->len == buffer_count);

                u32 pstart = PrimitiveMarker->start;
                string_buffer_t *PrimitiveToken = &buffer->array[pstart];

                vertex[0] = atoi(PrimitiveToken->buffer);
                PrimitiveToken++;
                used_bytes += sizeof(u32);

                STRARR_REWIND_MARKERS(buffer);
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Hashtable vertices.
    // ----------------------------------------------------------------------------------

    // Hashtable allocation.
    u32 hashtable_slots = (u32) (1.3f * vertex_count);
    vertex_hashtable_t *table = parser_hashtable_create(hashtable_slots);
    u32 vertex_index = 1;

    // Determining the number of unique vertices.
    for (u32 i = 0; i < vertex_count; ++i)
    {
        u32 *vertex = (u32 *) (vertices + i);
        b8 inserted = parser_hashtable_insert(table, vertex, vertex_index);

        if (inserted)
            vertex_index++;
    }

    // ----------------------------------------------------------------------------------
    // -- Tangent buffer generation.
    // ----------------------------------------------------------------------------------
#if 0
    if (mesh_type == GL_MESH_TRIANGLE && uv_buffer && normal_buffer)
    {
        tangent_buffer = (vec4 *) (work_buffer + used_bytes);
        vec4 *tangent_buffer_pointer = tangent_buffer;

        size_t tangent_buffer_size = sizeof(vec4) * vertex_count;
        size_t tmp_buffer_size = 2 * sizeof(vec3) * position_count;

        vec3 *tmp_tangent_buffer = (vec3 *) malloc(tmp_buffer_size);
        vec3 *tmp_bitangent_buffer = tmp_tangent_buffer + position_count;
        memset(tmp_tangent_buffer, 0, tmp_buffer_size);

        u32 *vertex_normal_index_buffer = (u32 *) malloc(position_count * sizeof(u32));
        u32 (*primitive_pointer)[3] = vertices;

        u32 primitive_id = 1;

        for (u32 i = 0; i < vertex_count; i += 3)
        {
            u32 *vertex_0 = (u32 *) (primitive_pointer + 0);
            u32 *vertex_1 = (u32 *) (primitive_pointer + 1);
            u32 *vertex_2 = (u32 *) (primitive_pointer + 2);

            primitive_pointer += 3;

            u32 pos_idx1 = vertex_0[0] - 1;
            u32 pos_idx2 = vertex_1[0] - 1;
            u32 pos_idx3 = vertex_2[0] - 1;

            u32 uv_idx1 = vertex_0[1] - 1;
            u32 uv_idx2 = vertex_1[1] - 1;
            u32 uv_idx3 = vertex_2[1] - 1;

            u32 norm_idx1 = vertex_0[2] - 1;
            u32 norm_idx2 = vertex_1[2] - 1;
            u32 norm_idx3 = vertex_2[2] - 1;

            vertex_normal_index_buffer[pos_idx1] = norm_idx1;
            vertex_normal_index_buffer[pos_idx2] = norm_idx2;
            vertex_normal_index_buffer[pos_idx3] = norm_idx3;

            vec3 *v1 = position_buffer + pos_idx1;
            vec3 *v2 = position_buffer + pos_idx2;
            vec3 *v3 = position_buffer + pos_idx3;

            vec2 uv1 = *(uv_buffer + uv_idx1);
            vec2 uv2 = *(uv_buffer + uv_idx2);
            vec2 uv3 = *(uv_buffer + uv_idx3);

            vec3 e1 = vec3_subp(v2, v1);
            vec3 e2 = vec3_subp(v3, v1);

            // NOTE(gabic): Small corrections in case the coordinates are equal.

            // if (uv3.u == uv1.u && uv3.v == uv1.v)
            // {
            //     uv3.u -= 0.00011f;
            //     uv3.v -= 0.00012f;
            // }

            // if (uv2.u == uv1.u && uv2.v == uv1.v)
            // {
            //     uv2.u += 0.00013f;
            //     uv2.v += 0.00014f;
            // }

            // if (uv3.u == uv2.u && uv3.v == uv2.v)
            // {
            //     uv3.u += 0.00015f;
            //     uv3.v += 0.00016f;
            // }

            // if (uv1.u == uv2.u && uv2.u == uv3.u) {
            //     uv3.u += 0.00021f;
            // }

            // if (uv1.v == uv2.v && uv2.v == uv3.v) {
            //     uv3.v += 0.00022f;
            // }

            r32 du21 = uv2.u - uv1.u;
            r32 dv21 = uv2.v - uv1.v;
            r32 du31 = uv3.u - uv1.u;
            r32 dv31 = uv3.v - uv1.v;

            r32 denom = du21 * dv31 - dv21 * du31;
            // if (denom == 0) denom = 0.00000001f;
            r32 r = 1.0f / denom;

            vec3 t = {
                (dv31 * e1.x - dv21 * e2.x) * r,
                (dv31 * e1.y - dv21 * e2.y) * r,
                (dv31 * e1.z - dv21 * e2.z) * r,
            };

            vec3 b = {
                (-du31 * e1.x + du21 * e2.x) * r,
                (-du31 * e1.y + du21 * e2.y) * r,
                (-du31 * e1.z + du21 * e2.z) * r,
            };

            tmp_tangent_buffer[pos_idx1] = vec3_add(tmp_tangent_buffer[pos_idx1], t);
            tmp_tangent_buffer[pos_idx2] = vec3_add(tmp_tangent_buffer[pos_idx2], t);
            tmp_tangent_buffer[pos_idx3] = vec3_add(tmp_tangent_buffer[pos_idx3], t);
            tmp_bitangent_buffer[pos_idx1] = vec3_add(tmp_bitangent_buffer[pos_idx1], b);
            tmp_bitangent_buffer[pos_idx2] = vec3_add(tmp_bitangent_buffer[pos_idx2], b);
            tmp_bitangent_buffer[pos_idx3] = vec3_add(tmp_bitangent_buffer[pos_idx3], b);

            if ((!tmp_tangent_buffer[pos_idx1].x && !tmp_tangent_buffer[pos_idx1].y && !tmp_tangent_buffer[pos_idx1].z) ||
                (tmp_tangent_buffer[pos_idx1].x == INFINITY && tmp_tangent_buffer[pos_idx1].y == INFINITY && tmp_tangent_buffer[pos_idx1].z == INFINITY))
            {
                u8 a = 0;
                a++;
            }

            if ((!tmp_tangent_buffer[pos_idx2].x && !tmp_tangent_buffer[pos_idx2].y && !tmp_tangent_buffer[pos_idx2].z) ||
                (tmp_tangent_buffer[pos_idx2].x == INFINITY && tmp_tangent_buffer[pos_idx2].y == INFINITY && tmp_tangent_buffer[pos_idx2].z == INFINITY))
            {
                u8 a = 0;
                a++;
            }

            if ((!tmp_tangent_buffer[pos_idx3].x && !tmp_tangent_buffer[pos_idx3].y && !tmp_tangent_buffer[pos_idx3].z) ||
                (tmp_tangent_buffer[pos_idx3].x == INFINITY && tmp_tangent_buffer[pos_idx3].y == INFINITY && tmp_tangent_buffer[pos_idx3].z == INFINITY))
            {
                u8 a = 0;
                a++;
            }

            if (primitive_id == 8)
            {
                u8 a = 0;
                a++;
            }

            // assert(tmp_tangent_buffer[pos_idx1].x || tmp_tangent_buffer[pos_idx1].y || tmp_tangent_buffer[pos_idx1].z);
            // assert(tmp_tangent_buffer[pos_idx2].x || tmp_tangent_buffer[pos_idx2].y || tmp_tangent_buffer[pos_idx2].z);
            // assert(tmp_tangent_buffer[pos_idx3].x || tmp_tangent_buffer[pos_idx3].y || tmp_tangent_buffer[pos_idx3].z);

            primitive_id++;
        }

        for (u32 i = 0; i < position_count; ++i, ++tangent_buffer_pointer)
        {
            vec3 t = tmp_tangent_buffer[i];
            vec3 b = tmp_bitangent_buffer[i];
            u32 nidx = vertex_normal_index_buffer[i];
            vec3 n = normal_buffer[nidx];

            if (i == 6)
            {
                u8 a = 0;
                a++;
            }

            assert(t.x || t.y || t.z);
            vec3 tmpv = force_perp(t, n);
            if (!tmpv.x && !tmpv.y && !tmpv.z)
            {
                u8 a = 0;
                a++;
            }
            t = vec3_normalize(tmpv);

            vec3 check_n = vec3_cross(t, b);
            check_n = vec3_normalize(check_n);
            r32 check = vec3_dot(check_n, n);

            tangent_buffer_pointer->x = t.x;
            tangent_buffer_pointer->y = t.y;
            tangent_buffer_pointer->z = t.z;
            tangent_buffer_pointer->w = check < 0 ? -1 : 1;

            used_bytes += sizeof(vec4);
        }

        free(vertex_normal_index_buffer);
        free(tmp_tangent_buffer);
    }
#endif
    // ----------------------------------------------------------------------------------
    // -- Generating the loaded mesh buffer.
    // ----------------------------------------------------------------------------------

    loaded_asset_mesh_t *mesh = 0;

    if (table->used)
    {
        u32 vertex_flag_buffer_bytes = table->used * sizeof(u8);
        u32 final_index_buffer_bytes = vertex_count * sizeof(u32);
        u32 final_vertex_buffer_bytes = table->used * sizeof(asset_vertex_t);
        u32 loaded_mesh_data_bytes = sizeof(loaded_asset_mesh_t) + final_index_buffer_bytes + final_vertex_buffer_bytes;

        mesh = (loaded_asset_mesh_t *) malloc(loaded_mesh_data_bytes);
        memset(mesh, 0, loaded_mesh_data_bytes);

        mesh->type = GL_MESH_TRIANGLE;

        mesh->indices = vertex_count;
        mesh->vertices = table->used;
        mesh->index_buffer_bytes = final_index_buffer_bytes;
        mesh->vertex_buffer_bytes = final_vertex_buffer_bytes;

        mesh->index_buffer = (u32 *) (mesh + 1);
        mesh->vertex_buffer = (void *) ((u8 *) mesh->index_buffer + mesh->index_buffer_bytes);

        u8 *vertex_flag_buffer = (u8 *) (work_buffer + used_bytes);
        used_bytes += vertex_flag_buffer_bytes;

        u32 *index_buffer_pointer = mesh->index_buffer;
        asset_vertex_t *vertex_buffer_pointer = (asset_vertex_t *) mesh->vertex_buffer;

        for (u32 i = 0; i < vertex_count; ++i)
        {
            u32 *vertex = (u32 *) (vertices + i);
            vertex_hashtable_item_t *found = parser_hashtable_get(table, vertex);

            *index_buffer_pointer++ = found->index - 1;

            // Check if the vertex was copied to the final buffer.
            if (!vertex_flag_buffer[found->index - 1])
            {
                vertex_flag_buffer[found->index - 1] = true;
                asset_vertex_t *dest_vertex = &vertex_buffer_pointer[found->index - 1];

                // Copy the vertex attributes.
                u32 pos_index = vertex[0] - 1;
                vec3 src_pos = position_buffer[pos_index];

                dest_vertex->pos[0] = src_pos.x;
                dest_vertex->pos[1] = src_pos.y;
                dest_vertex->pos[2] = src_pos.z;

                if (mesh_type == GL_MESH_TRIANGLE)
                {
                    u32 uv_index = vertex[1] - 1;
                    u32 normal_index = vertex[2] - 1;

                    vec2 src_uv = uv_buffer[uv_index];
                    vec3 src_normal = normal_buffer[normal_index];
                    // vec4 src_tangent = tangent_buffer[pos_index];

                    dest_vertex->data[0] = src_uv.x;
                    dest_vertex->data[1] = 1.0f - src_uv.y;

                    dest_vertex->data[2] = src_normal.x;
                    dest_vertex->data[3] = src_normal.y;
                    dest_vertex->data[4] = src_normal.z;

                    // dest_vertex->data[5] = src_tangent.x;
                    // dest_vertex->data[6] = src_tangent.y;
                    // dest_vertex->data[7] = src_tangent.z;
                    // dest_vertex->data[8] = src_tangent.w;

                    u8 a = 0;
                }
            }
        }
    }

    free(table);
    free(work_buffer);

    return mesh;
}

void *parse_obj(gc_file_t *File)
{
    // -- Mesh setup.

    size_t buffer_size = Megabytes(64);
    size_t total = sizeof(parser_model_data_t) +
                    (GL_MAX_ATTRIBUTE_COUNT + 1) * buffer_size;

    parser_model_data_t *Mesh = (parser_model_data_t *) malloc(total);
    memset(Mesh, 0, total);

    // -- Reset the model.

    Mesh->buffer_count = 0;
    Mesh->primitives.count = 0;
    Mesh->primitives.bytes = 0;
    Mesh->primitives.memory = (u32 *) ((u8 *) Mesh + sizeof(parser_model_data_t));

    for (u32 i = 0; i < GL_MAX_ATTRIBUTE_COUNT; ++i)
    {
        parser_model_attribute_buffer_t *buffer = &Mesh->buffers[i];

        buffer->bytes = 0;
        buffer->count = 0;
        buffer->memory = ((u8 *) Mesh->primitives.memory + (i + 1) * buffer_size);
    }

    parser_model_primitive_buffer_t *primitive_buffer = &Mesh->primitives;
    parser_model_attribute_buffer_t *vertex_buffer = 0;
    parser_model_attribute_buffer_t *uv_buffer = 0;
    parser_model_attribute_buffer_t *normal_buffer = 0;
    parser_model_attribute_buffer_t *color_buffer = 0;

    u32 *primitive_vertices = primitive_buffer->memory;

    b8 buffer_check[GL_MAX_ATTRIBUTE_COUNT];
    u32 index_count = 0;

    string_array_t tmp_buffer;
    string_array_t *buffer = &tmp_buffer;

    // -- Read the file.

    while (File->cursor < File->bytes)
    {
        STRARR_RESET_MARKERS(buffer);
        b8 res = parser_read_line(File, buffer);

        if (res)
        {
            string_buffer_t *LineBuffer = STRARR_GET_LAST_STRING(buffer);

            // Comment.
            if (LineBuffer->buffer[0] == '#')
                continue;

            // ----------------------------------------------------------------------------------
            // -- Position buffer.
            // ----------------------------------------------------------------------------------

            parser_tokenize_line(buffer, LineBuffer, ' ');
            string_marker_t *Marker = STRARR_GET_LAST_MARKER(buffer);
            string_buffer_t *Token = &buffer->array[Marker->start];

            if (strcmp(Token->buffer, "v") == 0 && Marker->len == 4)
            {
                if (!buffer_check[0])
                {
                    buffer_check[0] = true;

                    vertex_buffer = (parser_model_attribute_buffer_t *) &Mesh->buffers[Mesh->buffer_count++];
                    vertex_buffer->type = GL_ATTR_POS;
                    vertex_buffer->components = 3;
                    Mesh->attr_mask |= GL_ATTR_POS;
                }

                if ((vertex_buffer->bytes + sizeof(vec3)) > buffer_size)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                vec3 *Vertices = (vec3 *) vertex_buffer->memory;
                vec3 *Vertex = Vertices + vertex_buffer->count;

                Token++;
                Vertex->x = (r32) atof(Token->buffer);
                Token++;
                Vertex->y = (r32) atof(Token->buffer);
                Token++;
                Vertex->z = (r32) atof(Token->buffer);

                vertex_buffer->count++;
                vertex_buffer->bytes += sizeof(vec3);
            }

            // ----------------------------------------------------------------------------------
            // -- UV buffer.
            // ----------------------------------------------------------------------------------

            else if (strcmp(Token->buffer, "vt") == 0 && Marker->len == 3)
            {
                if (!buffer_check[1])
                {
                    buffer_check[1] = true;

                    uv_buffer = (parser_model_attribute_buffer_t *) &Mesh->buffers[Mesh->buffer_count++];
                    uv_buffer->type = GL_ATTR_UV;
                    uv_buffer->components = 2;
                    Mesh->attr_mask |= GL_ATTR_UV;
                }

                if ((uv_buffer->bytes + sizeof(vec2)) > buffer_size)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                vec2 *UVs = (vec2 *) uv_buffer->memory;
                vec2 *UV = UVs + uv_buffer->count;

                Token++;
                UV->u = (r32) atof(Token->buffer);
                Token++;
                UV->v = (r32) atof(Token->buffer);

                uv_buffer->count++;
                uv_buffer->bytes += sizeof(vec2);
            }

            // ----------------------------------------------------------------------------------
            // -- Normal buffer.
            // ----------------------------------------------------------------------------------

            else if (strcmp(Token->buffer, "vn") == 0 && Marker->len == 4)
            {
                if (!buffer_check[2])
                {
                    buffer_check[2] = true;

                    normal_buffer = (parser_model_attribute_buffer_t *) &Mesh->buffers[Mesh->buffer_count++];
                    normal_buffer->type = GL_ATTR_NORM;
                    normal_buffer->components = 3;
                    Mesh->attr_mask |= GL_ATTR_NORM;
                }

                if ((normal_buffer->bytes + sizeof(vec3)) > buffer_size)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                vec3 *normals = (vec3 *) normal_buffer->memory;
                vec3 *normal = normals + normal_buffer->count;

                Token++;
                normal->x = (r32) atof(Token->buffer);
                Token++;
                normal->y = (r32) atof(Token->buffer);
                Token++;
                normal->z = (r32) atof(Token->buffer);

                normal_buffer->count++;
                normal_buffer->bytes += sizeof(vec3);
            }

            // ----------------------------------------------------------------------------------
            // -- Primitives (faces).
            // ----------------------------------------------------------------------------------

            // Triangle.
            else if (strcmp(Token->buffer, "f") == 0 && Marker->len == 4)
            {
                u32 vertex_bytes = sizeof(u32) * Mesh->buffer_count;
                u32 primitive_line_bytes = 3 * vertex_bytes;

                if ((primitive_buffer->bytes + primitive_line_bytes) > buffer_size)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                Mesh->type = GL_MESH_TRIANGLE;
                primitive_buffer->count++;

                for (u32 k = 0; k < 3; ++k)
                {
                    Token++;

                    parser_tokenize_line(buffer, Token, '/');
                    string_marker_t *PrimitiveMarker = STRARR_GET_LAST_MARKER(buffer);
                    assert(PrimitiveMarker->len == Mesh->buffer_count);

                    u32 pstart = PrimitiveMarker->start;
                    string_buffer_t *PrimitiveToken = &buffer->array[pstart];

                    u32 *CurrentVertexIndices = primitive_vertices + Mesh->buffer_count * primitive_buffer->vertices++;
                    primitive_buffer->bytes += vertex_bytes;

                    for (u32 l = 0; l < PrimitiveMarker->len; ++l)
                    {
                        *CurrentVertexIndices++ = atoi(PrimitiveToken->buffer);
                        PrimitiveToken++;
                    }

                    STRARR_REWIND_MARKERS(buffer);
                }
            }

            // Line.
            else if (strcmp(Token->buffer, "f") == 0 && Marker->len == 3)
            {
                u32 vertex_bytes = sizeof(u32) * Mesh->buffer_count;
                u32 primitive_line_bytes = 2 * vertex_bytes;

                if ((primitive_buffer->bytes + primitive_line_bytes) > buffer_size)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                Mesh->type = GL_MESH_TRIANGLE;
                primitive_buffer->count++;

                for (u32 k = 0; k < 2; ++k)
                {
                    Token++;

                    parser_tokenize_line(buffer, Token, '/');
                    string_marker_t *PrimitiveMarker = STRARR_GET_LAST_MARKER(buffer);
                    assert(PrimitiveMarker->len == Mesh->buffer_count);

                    u32 pstart = PrimitiveMarker->start;
                    string_buffer_t *PrimitiveToken = &buffer->array[pstart];

                    u32 *CurrentVertexIndices = primitive_vertices + Mesh->buffer_count * primitive_buffer->vertices++;
                    primitive_buffer->bytes += vertex_bytes;

                    for (u32 l = 0; l < PrimitiveMarker->len; ++l)
                    {
                        *CurrentVertexIndices++ = atoi(PrimitiveToken->buffer);
                        PrimitiveToken++;
                    }

                    STRARR_REWIND_MARKERS(buffer);
                }
            }

            // point.
            else if (strcmp(Token->buffer, "f") == 0 && Marker->len == 2)
            {
                u32 vertex_bytes = sizeof(u32) * Mesh->buffer_count;
                u32 primitive_line_bytes = vertex_bytes;

                if ((primitive_buffer->bytes + primitive_line_bytes) > buffer_size)
                {
                    printf("ERROR: [OBJParser] Element buffer is too small !");
                    exit(EXIT_FAILURE);
                }

                Mesh->type = GL_MESH_TRIANGLE;
                primitive_buffer->count++;

                Token++;

                parser_tokenize_line(buffer, Token, '/');
                string_marker_t *PrimitiveMarker = STRARR_GET_LAST_MARKER(buffer);
                assert(PrimitiveMarker->len == Mesh->buffer_count);

                u32 pstart = PrimitiveMarker->start;
                string_buffer_t *PrimitiveToken = &buffer->array[pstart];

                u32 *CurrentVertexIndices = primitive_vertices + Mesh->buffer_count * primitive_buffer->vertices++;
                primitive_buffer->bytes += vertex_bytes;

                for (u32 l = 0; l < PrimitiveMarker->len; ++l)
                {
                    *CurrentVertexIndices++ = atoi(PrimitiveToken->buffer);
                    PrimitiveToken++;
                }

                STRARR_REWIND_MARKERS(buffer);
            }
        }
    }

    Mesh->primitives.bytes = MEM_SIZE_ALIGN(Mesh->primitives.bytes);

    if (primitive_buffer->bytes > buffer_size)
    {
        printf("ERROR: [OBJParser] Element buffer is too small !");
        exit(EXIT_FAILURE);
    }

    for (u32 i = 0; i < GL_MAX_ATTRIBUTE_COUNT; ++i)
    {
        parser_model_attribute_buffer_t *attr_buffer = &Mesh->buffers[i];
        attr_buffer->bytes = MEM_SIZE_ALIGN(attr_buffer->bytes);

        if (attr_buffer->bytes > buffer_size)
        {
            printf("ERROR: [OBJParser] Element buffer is too small !");
            exit(EXIT_FAILURE);
        }
    }

    return Mesh;
}

#endif