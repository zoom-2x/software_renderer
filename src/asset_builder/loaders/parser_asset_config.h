// ----------------------------------------------------------------------------------
// -- File: parser_asset_config.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-04-29 21:28:05
// -- Modified: 2022-04-29 21:28:07
// ----------------------------------------------------------------------------------

// #define JSON_LOOP(json, index) for (u16 index = 0; index < json->u.object.length; ++index)
#define JSON_OBJECT_PROP_COUNT(prop) prop->value->u.object.length
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

typedef struct asset_group_s asset_group_t;

struct asset_group_s
{
    char name[64];
    char prefix[64];

    // Source count.
    u32 count;
    // Actual object count, meshes can generate line and point meshes..
    u32 objects;

    asset_group_t *next;
    asset_config_source_t *sources;
};

typedef struct
{
    u32 texture_groups;
    u32 hdr_groups;
    u32 mesh_groups;
    u32 font_groups;
    u32 text_groups;

    u32 total_textures;
    u32 total_hdrs;
    u32 total_meshes;

    asset_group_t *textures;
    asset_group_t *hdrs;
    asset_group_t *meshes;
    asset_group_t *fonts;
    asset_group_t *texts;
} asset_config_t;

b8 _read_category_texture(struct _json_object_entry *prop, asset_config_category_t *asset_category)
{
    b8 valid = true;
    u16 ai = 0;

    // for (u16 bi = 0; bi < prop->value->u.array.length; ++bi)
    JSON_VALUE_ARRAY_LOOP (prop->value, bi)
    {
        json_value *entry = JSON_ARRAY_VALUE(prop->value, bi);
        asset_config_source_t *source = asset_category->sources + ai;

        memset(source, 0, sizeof(asset_config_source_t));
        b8 disabled = false;

        // for (u8 bpi = 0; bpi < entry->u.object.length; ++bpi)
        JSON_VALUE_OBJECT_LOOP (entry, bpi)
        {
            struct _json_object_entry *entry_prop = JSON_OBJECT_PROPERTY(entry, bpi);

            if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "type"))
            {
                char *type = JSON_PROPERTY_VALUE_STRING(entry_prop);

                if (strcmp(type, "texture") == 0) {
                    source->type = ASSET_TEXTURE2D;
                }
                else if (strcmp(type, "cubemap") == 0) {
                    source->type = ASSET_TEXTURE_CUBEMAP;
                }
                else if (strcmp(type, "pbr_ambient") == 0) {
                    source->type = ASSET_TEXTURE_PBR_AMBIENT;
                }
            }

            else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "alias")) {
                strncpy(source->alias, JSON_PROPERTY_VALUE_STRING(entry_prop), METADATA_NAME_LENGTH);
            }

            else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "path") && JSON_PROPERTY_TYPE_EQUALS(entry_prop, json_string)) {
                strncpy(source->filepath, JSON_PROPERTY_VALUE_STRING(entry_prop), MAX_ASSET_FILEPATH_SIZE);
            }

            else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "path") && JSON_PROPERTY_TYPE_EQUALS(entry_prop, json_object))
            {
                JSON_VALUE_OBJECT_LOOP (entry_prop->value, pi)
                {
                    struct _json_object_entry *face = JSON_OBJECT_PROPERTY(entry_prop->value, pi);

                    if (JSON_PROPERTY_NAME_EQUALS(face, "front"))
                        strncpy(source->cubepath[CUBE_FRONT], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                    else if (JSON_PROPERTY_NAME_EQUALS(face, "back"))
                        strncpy(source->cubepath[CUBE_BACK], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                    else if (JSON_PROPERTY_NAME_EQUALS(face, "left"))
                        strncpy(source->cubepath[CUBE_LEFT], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                    else if (JSON_PROPERTY_NAME_EQUALS(face, "right"))
                        strncpy(source->cubepath[CUBE_RIGHT], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                    else if (JSON_PROPERTY_NAME_EQUALS(face, "top"))
                        strncpy(source->cubepath[CUBE_TOP], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                    else if (JSON_PROPERTY_NAME_EQUALS(face, "bottom"))
                        strncpy(source->cubepath[CUBE_BOTTOM], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                }
            }

            else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "disabled") && JSON_PROPERTY_VALUE_BOOL(entry_prop))
            {
                disabled = true;
                source->flags |= SOURCE_DISABLED;
            }

            else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "debug") && JSON_PROPERTY_VALUE_BOOL(entry_prop))
                source->flags |= SOURCE_DEBUG;
        }

        // if (disabled)
        //     asset_category->count--;
        // else
        ai++;
    }

    return valid;
}

b8 _read_category_mesh(struct _json_object_entry *prop, asset_config_category_t *asset_category)
{
    b8 valid = true;
    u16 ai = 0;

    // for (u16 bi = 0; bi < prop->value->u.array.length; ++bi)
    JSON_VALUE_ARRAY_LOOP(prop->value, bi)
    {
        json_value *entry = JSON_ARRAY_VALUE(prop->value, bi);
        asset_config_source_t *source = asset_category->sources + ai;

        memset(source, 0, sizeof(asset_config_source_t));
        b8 disabled = false;

        // for (u8 bpi = 0; bpi < entry->u.object.length; ++bpi)
        JSON_VALUE_OBJECT_LOOP(entry, bpi)
        {
            struct _json_object_entry *entry_prop = JSON_OBJECT_PROPERTY(entry, bpi);

            if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "path") && JSON_PROPERTY_TYPE_EQUALS(entry_prop, json_string)) {
                strncpy(source->filepath, JSON_PROPERTY_VALUE_STRING(entry_prop), MAX_ASSET_FILEPATH_SIZE);
            }

            else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "disabled") && JSON_PROPERTY_VALUE_BOOL(entry_prop))
            {
                disabled = true;
                source->flags |= SOURCE_DISABLED;
            }

            else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "generator") && JSON_PROPERTY_VALUE_BOOL(entry_prop))
                source->flags |= SOURCE_GENERATOR;
        }

        if (disabled)
            asset_category->count--;
        else
            ai++;
    }

    return valid;
}

b8 _read_category_font(struct _json_object_entry *prop, asset_config_category_t *asset_category)
{
    b8 valid = true;
    return valid;
}

b8 _read_category_text(struct _json_object_entry *prop, asset_config_category_t *asset_category)
{
    b8 valid = true;
    return valid;
}

void debug_asset_config(asset_config_t *assets)
{
    asset_group_t *current_texture_group = assets->textures;
    asset_group_t *current_mesh_group = assets->meshes;

    printf("-----------------------------------------------------------------------\n");
    printf("-- Texture groups.\n");
    printf("-----------------------------------------------------------------------\n");

    while (current_texture_group)
    {
        printf("[%s]\n", current_texture_group->name);

        if (current_texture_group->count)
        {
            for (u32 i = 0; i < current_texture_group->count; ++i)
            {
                asset_config_source_t *source = current_texture_group->sources + i;
                const char *type = 0;

                if (source->type == ASSET_TEXTURE2D)
                    type = "texture2d";
                else if (source->type == ASSET_TEXTURE_CUBEMAP)
                    type = "cubemap";
                else if (source->type == ASSET_TEXTURE_PBR_AMBIENT)
                    type = "pbr_ambient";

                printf("  %u. %s (%s)\n", i + 1, source->alias, type);
            }
        }

        current_texture_group = current_texture_group->next;
    }

    printf("-----------------------------------------------------------------------\n");
    printf("-- Mesh groups.\n");
    printf("-----------------------------------------------------------------------\n");

    while (current_mesh_group)
    {
        printf("[%s]\n", current_mesh_group->name);

        if (current_mesh_group->count)
        {
            for (u32 i = 0; i < current_mesh_group->count; ++i)
            {
                asset_config_source_t *source = current_mesh_group->sources + i;
                const char *disabled = "";

                if (source->flags & SOURCE_DISABLED)
                    disabled = "(disabled)";

                printf("  %u. %s %s\n", i + 1, source->filepath, disabled);
            }
        }

        current_mesh_group = current_mesh_group->next;
    }
}

void free_asset_config(asset_config_t *assets)
{
    asset_group_t *current_group = assets->textures;

    while (current_group)
    {
        asset_group_t *group_to_free = current_group;
        current_group = current_group->next;

        free(group_to_free);
    }

    current_group = assets->meshes;

    while (current_group)
    {
        asset_group_t *group_to_free = current_group;
        current_group = current_group->next;

        free(group_to_free);
    }

    assets->textures = 0;
    assets->meshes = 0;
}

void parse_asset_config_json(char *filepath, asset_config_t *assets)
{
    char error_buf[json_error_max];
    gc_file_t config_file;
    open_file(&config_file, filepath, GC_FILE_READ);

    assets->texture_groups = 0;
    assets->hdr_groups = 0;
    assets->mesh_groups = 0;
    assets->font_groups = 0;
    assets->text_groups = 0;

    assets->total_textures = 0;
    assets->total_hdrs = 0;
    assets->total_meshes = 0;

    assets->textures = 0;
    assets->hdrs = 0;
    assets->meshes = 0;
    assets->fonts = 0;
    assets->texts = 0;

    if (config_file.handle)
    {
        void *config_file_data = malloc(config_file.bytes);
        read_file(&config_file, 0, config_file.bytes, config_file_data);

        json_value *json = json_parse((json_char *) config_file_data, config_file.bytes, error_buf);

        if (!json)
        {
            printf("%s\n", error_buf);
            EXIT_ALERT("Cannot load the asset config file !\n");
        }

        JSON_VALUE_OBJECT_LOOP(json, pi)
        {
            struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(json, pi);

            // ----------------------------------------------------------------------------------
            // -- Textures.
            // ----------------------------------------------------------------------------------

            if (JSON_PROPERTY_COMPARE(prop, "textures", json_object))
            {
                asset_group_t *last_group = 0;

                JSON_VALUE_OBJECT_LOOP(prop->value, ti)
                {
                    struct _json_object_entry *group = JSON_OBJECT_PROPERTY(prop->value, ti);
                    u32 group_texture_count = JSON_OBJECT_PROP_COUNT(group);

                    if (JSON_PROPERTY_TYPE_EQUALS(group, json_array))
                    {
                        size_t bytes = sizeof(asset_group_t) + sizeof(asset_config_source_t) * group_texture_count;
                        void *memory = malloc(bytes);
                        memset(memory, 0, bytes);

                        asset_group_t *current_group = (asset_group_t *) memory;
                        asset_config_source_t *group_sources = (asset_config_source_t *) (current_group + 1);
                        current_group->sources = group_sources;
                        strncpy(current_group->name, group->name, 64);
                        strncpy(current_group->prefix, "textures/", 64);
                        strncat(current_group->prefix, group->name, 64);
                        assets->texture_groups++;

                        if (!last_group)
                        {
                            assets->textures = current_group;
                            last_group = current_group;
                        }
                        else
                        {
                            last_group->next = current_group;
                            last_group = current_group;
                        }

                        // Cycle through the group's sources.
                        JSON_VALUE_ARRAY_LOOP(group->value, gi)
                        {
                            json_value *entry = JSON_ARRAY_VALUE(group->value, gi);

                            if (entry->type == json_object)
                            {
                                asset_config_source_t *current_source = group_sources + current_group->count++;

                                b8 has_alias = false;
                                b8 has_path = false;

                                current_source->flags = 0;

                                JSON_VALUE_OBJECT_LOOP(entry, si)
                                {
                                    struct _json_object_entry *entry_prop = JSON_OBJECT_PROPERTY(entry, si);

                                    if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "type"))
                                    {
                                        char *type = JSON_PROPERTY_VALUE_STRING(entry_prop);

                                        if (strcmp(type, "texture") == 0) {
                                            current_source->type = ASSET_TEXTURE2D;
                                        }
                                        else if (strcmp(type, "cubemap") == 0) {
                                            current_source->type = ASSET_TEXTURE_CUBEMAP;
                                        }
                                        else if (strcmp(type, "pbr_ambient") == 0) {
                                            current_source->type = ASSET_TEXTURE_PBR_AMBIENT;
                                        }
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "alias"))
                                    {
                                        strncpy(current_source->alias, JSON_PROPERTY_VALUE_STRING(entry_prop), METADATA_NAME_LENGTH);
                                        has_alias = true;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "path") && JSON_PROPERTY_TYPE_EQUALS(entry_prop, json_string))
                                    {
                                        strncpy(current_source->filepath, JSON_PROPERTY_VALUE_STRING(entry_prop), MAX_ASSET_FILEPATH_SIZE);
                                        has_path = true;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "path") && JSON_PROPERTY_TYPE_EQUALS(entry_prop, json_object))
                                    {
                                        u32 face_count = 0;

                                        JSON_VALUE_OBJECT_LOOP (entry_prop->value, pathi)
                                        {
                                            struct _json_object_entry *face = JSON_OBJECT_PROPERTY(entry_prop->value, pathi);

                                            if (JSON_PROPERTY_NAME_EQUALS(face, "front"))
                                            {
                                                strncpy(current_source->cubepath[CUBE_FRONT], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                                                face_count++;
                                            }
                                            else if (JSON_PROPERTY_NAME_EQUALS(face, "back"))
                                            {
                                                strncpy(current_source->cubepath[CUBE_BACK], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                                                face_count++;
                                            }
                                            else if (JSON_PROPERTY_NAME_EQUALS(face, "left"))
                                            {
                                                strncpy(current_source->cubepath[CUBE_LEFT], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                                                face_count++;
                                            }
                                            else if (JSON_PROPERTY_NAME_EQUALS(face, "right"))
                                            {
                                                strncpy(current_source->cubepath[CUBE_RIGHT], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                                                face_count++;
                                            }
                                            else if (JSON_PROPERTY_NAME_EQUALS(face, "top"))
                                            {
                                                strncpy(current_source->cubepath[CUBE_TOP], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                                                face_count++;
                                            }
                                            else if (JSON_PROPERTY_NAME_EQUALS(face, "bottom"))
                                            {
                                                strncpy(current_source->cubepath[CUBE_BOTTOM], JSON_PROPERTY_VALUE_STRING(face), MAX_ASSET_FILEPATH_SIZE);
                                                face_count++;
                                            }
                                        }

                                        if (face_count == 6)
                                            has_path = true;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "disabled") &&
                                             JSON_PROPERTY_VALUE_BOOL(entry_prop))
                                    {
                                        current_source->flags |= SOURCE_DISABLED;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "debug") &&
                                             JSON_PROPERTY_VALUE_BOOL(entry_prop))
                                    {
                                        current_source->flags |= SOURCE_DEBUG;
                                    }
                                }

                                if (!has_alias || !has_path)
                                    current_source->flags |= SOURCE_DISABLED;

                                if (!(current_source->flags & SOURCE_DISABLED))
                                {
                                    if (current_source->type == ASSET_TEXTURE_PBR_AMBIENT)
                                        current_group->objects += 4;
                                    else
                                        current_group->objects += 1;

                                    assets->total_textures++;
                                }
                                else
                                    current_group->count--;
                            }
                        }
                    }
                }
            }

            else if (JSON_PROPERTY_COMPARE(prop, "hdrs", json_object))
            {
                asset_group_t *last_group = 0;

                JSON_VALUE_OBJECT_LOOP(prop->value, ti)
                {
                    struct _json_object_entry *group = JSON_OBJECT_PROPERTY(prop->value, ti);
                    u32 group_texture_count = JSON_OBJECT_PROP_COUNT(group);

                    if (JSON_PROPERTY_TYPE_EQUALS(group, json_array))
                    {
                        size_t bytes = sizeof(asset_group_t) + sizeof(asset_config_source_t) * group_texture_count;
                        void *memory = malloc(bytes);
                        memset(memory, 0, bytes);

                        asset_group_t *current_group = (asset_group_t *) memory;
                        asset_config_source_t *group_sources = (asset_config_source_t *) (current_group + 1);
                        current_group->sources = group_sources;
                        strncpy(current_group->name, group->name, 64);
                        strncpy(current_group->prefix, "textures/", 64);
                        strncat(current_group->prefix, group->name, 64);
                        assets->hdr_groups++;

                        if (!last_group)
                        {
                            assets->hdrs = current_group;
                            last_group = current_group;
                        }
                        else
                        {
                            last_group->next = current_group;
                            last_group = current_group;
                        }

                        // Cycle through the group's sources.
                        JSON_VALUE_ARRAY_LOOP(group->value, gi)
                        {
                            json_value *entry = JSON_ARRAY_VALUE(group->value, gi);

                            if (entry->type == json_object)
                            {
                                asset_config_source_t *current_source = group_sources + current_group->count++;

                                b8 has_alias = false;
                                b8 has_path = false;

                                current_source->flags = 0;
                                current_source->type = ASSET_TEXTURE_PBR_AMBIENT;

                                JSON_VALUE_OBJECT_LOOP(entry, si)
                                {
                                    struct _json_object_entry *entry_prop = JSON_OBJECT_PROPERTY(entry, si);

                                    // if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "type"))
                                    // {
                                    //     char *type = JSON_PROPERTY_VALUE_STRING(entry_prop);

                                    //     if (strcmp(type, "pbr_ambient") == 0) {
                                    //         current_source->type = ASSET_TEXTURE_PBR_AMBIENT;
                                    //     }
                                    // }

                                    if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "alias"))
                                    {
                                        strncpy(current_source->alias, JSON_PROPERTY_VALUE_STRING(entry_prop), METADATA_NAME_LENGTH);
                                        has_alias = true;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "path") && JSON_PROPERTY_TYPE_EQUALS(entry_prop, json_string))
                                    {
                                        strncpy(current_source->filepath, JSON_PROPERTY_VALUE_STRING(entry_prop), MAX_ASSET_FILEPATH_SIZE);
                                        has_path = true;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "disabled") &&
                                             JSON_PROPERTY_VALUE_BOOL(entry_prop))
                                    {
                                        current_source->flags |= SOURCE_DISABLED;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "debug") &&
                                             JSON_PROPERTY_VALUE_BOOL(entry_prop))
                                    {
                                        current_source->flags |= SOURCE_DEBUG;
                                    }
                                }

                                if (!has_alias || !has_path)
                                    current_source->flags |= SOURCE_DISABLED;

                                if (!(current_source->flags & SOURCE_DISABLED))
                                {
                                    current_group->objects += 4;
                                    assets->total_hdrs++;
                                }
                                else
                                    current_group->count--;
                            }
                        }
                    }
                }
            }

            // ----------------------------------------------------------------------------------
            // -- Meshes.
            // ----------------------------------------------------------------------------------

            else if (JSON_PROPERTY_NAME_EQUALS(prop, "meshes"))
            {
                asset_group_t *last_group = 0;

                JSON_VALUE_OBJECT_LOOP(prop->value, ti)
                {
                    struct _json_object_entry *group = JSON_OBJECT_PROPERTY(prop->value, ti);
                    u32 group_mesh_count = JSON_OBJECT_PROP_COUNT(group);

                    if (JSON_PROPERTY_TYPE_EQUALS(group, json_array))
                    {
                        size_t bytes = sizeof(asset_group_t) + sizeof(asset_config_source_t) * group_mesh_count;
                        void *memory = malloc(bytes);
                        memset(memory, 0, bytes);

                        asset_group_t *current_group = (asset_group_t *) memory;
                        asset_config_source_t *group_sources = (asset_config_source_t *) (current_group + 1);
                        current_group->sources = group_sources;
                        strncpy(current_group->name, group->name, 64);
                        strncpy(current_group->prefix, "meshes/", 64);
                        strncat(current_group->prefix, group->name, 64);
                        assets->mesh_groups++;

                        if (!last_group)
                        {
                            assets->meshes = current_group;
                            last_group = current_group;
                        }
                        else
                        {
                            last_group->next = current_group;
                            last_group = current_group;
                        }

                        // Cycle through the group's sources.
                        JSON_VALUE_ARRAY_LOOP(group->value, gi)
                        {
                            json_value *entry = JSON_ARRAY_VALUE(group->value, gi);

                            if (entry->type == json_object)
                            {
                                asset_config_source_t *current_source = group_sources + current_group->count++;

                                b8 has_path = false;

                                current_source->flags = 0;

                                JSON_VALUE_OBJECT_LOOP(entry, si)
                                {
                                    struct _json_object_entry *entry_prop = JSON_OBJECT_PROPERTY(entry, si);

                                    if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "path") && JSON_PROPERTY_TYPE_EQUALS(entry_prop, json_string))
                                    {
                                        strncpy(current_source->filepath, JSON_PROPERTY_VALUE_STRING(entry_prop), MAX_ASSET_FILEPATH_SIZE);
                                        has_path = true;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "disabled") &&
                                             JSON_PROPERTY_VALUE_BOOL(entry_prop))
                                    {
                                        current_source->flags |= SOURCE_DISABLED;
                                    }

                                    else if (JSON_PROPERTY_NAME_EQUALS(entry_prop, "generator") &&
                                             JSON_PROPERTY_VALUE_BOOL(entry_prop))
                                    {
                                        current_source->flags |= SOURCE_GENERATOR;
                                    }
                                }

                                if (!has_path)
                                    current_source->flags |= SOURCE_DISABLED;

                                if (!(current_source->flags & SOURCE_DISABLED))
                                {
                                    if (current_source->flags & SOURCE_GENERATOR)
                                        current_group->objects += 2;
                                    else
                                        current_group->objects += 1;

                                    assets->total_meshes++;
                                }
                                else
                                    current_group->count--;
                            }
                        }
                    }
                }
            }

            else if (JSON_PROPERTY_NAME_EQUALS(prop, "fonts"))
            {}

            else if (JSON_PROPERTY_NAME_EQUALS(prop, "texts"))
            {}
        }

        json_value_free(json);
        free(config_file_data);
        close_file(&config_file);
    }
    else
        printf("[ERROR] {%s} %s\n", filepath, error_buf);
}