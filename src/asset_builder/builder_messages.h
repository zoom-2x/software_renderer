// ----------------------------------------------------------------------------------
// -- File: builder_messages.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-07-09 12:33:06
// -- Modified: 2022-07-09 12:33:06
// ----------------------------------------------------------------------------------

typedef enum
{
    PACKING,
    PACKING_DISABLED,

    BUILDER_INFOS
} builder_info_message_t;

typedef enum
{
    CHUNK_BUFFER_OVERFLOW,
    INVALID_JSON_FILE,
    INVALID_ASSET_MESH_TYPE,
    MISSING_FILE,

    BUILDER_ERRORS
} builder_error_message_t;

const char *builder_info_messages[] = {
    "[PACKING]: [%s] %s\n",
    "[PACKING]: [%s] %s [DISABLED]\n",
};

const char *builder_error_messages[] = {
    "Chunk data buffer overflow, please increase the buffer size !\n",
    "Invalid json config file {\"%s\"} !\n",
    "[Asset builder] Invalid asset mesh type {\"%s\"} !\n",
    "Missing file {\"%s\"} !\n",
};

#define B_INFO(code) builder_info_messages[code]
#define B_ERROR(code) builder_error_messages[code]
#define PRINT_INFO(code, alias) printf(builder_info_messages[code], alias)
#define PRINT_ERROR(code, alias) printf(builder_error_messages[code], alias)