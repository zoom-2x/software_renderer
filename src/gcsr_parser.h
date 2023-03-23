// ----------------------------------------------------------------------------------
// -- File: gcsr_parser.h
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-08-24 19:50:58
// -- Modified:
// ----------------------------------------------------------------------------------

#ifndef GCSR_PARSER_H
#define GCSR_PARSER_H

#include <stdlib.h>
#include "gcsr_types.h"

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)

#define PARSER_MEMORY_POOL_SIZE Megabytes(1)
#define PARSER_TMP_BUFFER_SIZE 512
#define BaseParser(Parser) (GC_Parser *) Parser

struct GC_Parser;
typedef b32 (* Parse_t)(GC_Parser *Parser);
typedef void (* Destroy_t)(GC_Parser *Parser);

static u8 get_current(GC_Parser *Parser);
static b32 is_whitespace(GC_Parser *Parser);
static b32 is_char(GC_Parser *Parser, char t);
static b32 newline(GC_Parser *Parser);

enum GC_ParserType
{
    PARSER_BITMAP,
    PARSER_TEXT_CONFIG,
    PARSER_OBJ,
    PARSER_TEXTURE,
    PARSER_SOUND,
    PARSER_FONT,

    PARSER_COUNT
};

struct GC_Parser_Abstract
{
    FileRead *File;
    Parse_t parse;
    // Destroy_t destroy;
};

struct GC_Parser
{
    b32 initialized;
    u32 bytes;

    u32 index;
    u8 *Input;

    size_t totalSize;
    size_t allocated;
    u8 *engine_memory_pool_t;

    FileRead *File;
    Parse_t parse;
    Destroy_t destroy;
};

struct AssetTextEntry
{
    AssetTextKey Key;

    size_t valueBytes;
    char value[2048];
};

struct GC_Parser_TextConfig
{
    GC_Parser Base;

    u32 count;
    AssetTextEntry *Lines;
};

struct GC_Parser_ObjLineToken
{
    char delim;
    char value[64];
};

struct GC_Parser_Obj
{
    GC_Parser Base;

    u32 vertexCount;
    u32 faceCount;
    u32 uvCount;
    u32 normalCount;

    u32 bufferCount;
    u32 faceComponents;

    ObjectVertex_ptr vertices;
    ObjectFaceVertex_ptr facesVertices;
    ObjectFaceUV_ptr facesUVs;
    ObjectFaceNormal_ptr facesNormals;
    ObjectUV_ptr uvs;
    ObjectNormal_ptr normals;

    u32 lastUsedTokens;
    GC_Parser_ObjLineToken tokens[20];
};

b32 TextConfigParser_parse(GC_Parser *Parser);
b32 ObjParser_parse(GC_Parser *Parser);
// void TextConfigParser_destroy(GC_Parser *Parser);

#endif