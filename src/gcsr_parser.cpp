// ----------------------------------------------------------------------------------
// -- File: gcsr_parser.cpp
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-08-24 20:16:28
// -- Modified:
// ----------------------------------------------------------------------------------

#include "gcsr_parser.h"

GC_Parser *GC_Parser_create(GC_ParserType type, size_t memorySize)
{
    GC_Parser *Parser = 0;
    Parse_t parse = 0;
    size_t size = 0;

    switch (type)
    {
        case PARSER_BITMAP:
        {}
        break;

        case PARSER_TEXT_CONFIG:
        {
            size = sizeof(GC_Parser_TextConfig);
            parse = TextConfigParser_parse;
        }
        break;

        case PARSER_OBJ:
        {
            size = sizeof(GC_Parser_Obj);
            parse = ObjParser_parse;
        }
        break;

        case PARSER_TEXTURE:
        {}
        break;

        case PARSER_SOUND:
        {}
        break;

        case PARSER_FONT:
        {}
        break;
    }

    Parser = (GC_Parser *) malloc(size);
    memset(Parser, 0, size);

    Parser->parse = parse;

    if (memorySize == 0)
        memorySize = PARSER_MEMORY_POOL_SIZE;

    Parser->initialized = true;
    Parser->totalSize = memorySize;

    Parser->engine_memory_pool_t = (u8 *) malloc(memorySize);
    memset(Parser->engine_memory_pool_t, 0, memorySize);

    // Parser->bytes = Abstract->File->size;
    // Parser->Input = (u8 *) Abstract->File->data;

    return Parser;
}

void GC_Parser_setFile(GC_Parser *Parser, FileRead File)
{
    Parser->bytes = File.size;
    Parser->Input = (u8 *) File.data;
}

__INLINE__ void GC_Parser_check(GC_Parser *Parser)
{
    if (!Parser->initialized)
    {
        printf("\nERROR: The parser was not initialized !");
        exit(1);
    }
}

void *GC_Parser_allocate(GC_Parser *Parser, size_t size)
{
    GC_Parser_check(Parser);

    void *Result = 0;

    if (Parser->allocated + size <= Parser->totalSize)
    {
        Result = Parser->engine_memory_pool_t + Parser->allocated;
        Parser->allocated += size;
    }
    else
    {
        printf("ERROR: Out of memory, increase the memory pool !");
        exit(EXIT_FAILURE);
    }

    return Result;
}

__INLINE__ b32 GC_Parser_parse(GC_Parser *Parser)
{
    b32 result = Parser->parse(Parser);
    return result;
}

void GC_Parser_destroy(GC_Parser *Parser)
{
    // Parser->Abstract.destroy();

    free(Parser->Input);
    free(Parser->engine_memory_pool_t);

    Parser->engine_memory_pool_t = 0;
    Parser->initialized = false;
    Parser->totalSize = 0;
    Parser->allocated = 0;

    free(Parser);
}

// ----------------------------------------------------------------------------------
// -- Common routines.
// ----------------------------------------------------------------------------------

static __INLINE__ u8 get_current(GC_Parser *Parser)
{
    u8 c = Parser->Input[Parser->index];
    return c;
}

// static void next(TextConfigParser *Parser) {
//     Parser->index++;
// }

static b32 is_whitespace(GC_Parser *Parser)
{
    u8 c = get_current(Parser);

    if (c == ' ' || c == '\t')
        return true;

    return false;
}

static b32 is_char(GC_Parser *Parser, char t)
{
    u8 c = get_current(Parser);

    if (c == t)
        return true;

    return false;
}

static b32 newline(GC_Parser *Parser)
{
    b32 result = false;
    u8 c = get_current(Parser);

    if (c == '\n')
    {
        Parser->index++;
        result = true;
    }
    else if (c == '\r')
    {
        Parser->index++;

        if (Parser->index < Parser->bytes - 1)
        {
            u8 c2 = get_current(Parser);

            if (c2 == '\n')
            {
                Parser->index++;
                result = true;
            }
        }
    }

    return result;
}

void readToken(GC_Parser *Parser, char *buffer)
{
    u32 bufferIndex = 0;
    char c = Parser->Input[Parser->index];

    while (c != ' ' || c != '\r' || c != '\n' || c != '\t')
    {
        buffer[bufferIndex++] = c;

        Parser->index++;
        c = Parser->Input[Parser->index];
    }

    buffer[bufferIndex] = '\0';
}

void readTokenUntil(GC_Parser *Parser, char *buffer, char delim)
{}

// ----------------------------------------------------------------------------------
// -- Various implementations.
// ----------------------------------------------------------------------------------

b32 TextConfigParser_parse(GC_Parser *Parser)
{
    b32 result = false;

    if (Parser && Parser->bytes > 0)
    {
        GC_Parser_check(Parser);

        u8 *data = Parser->Input;

        GC_Parser_TextConfig *TextParser = (GC_Parser_TextConfig *) Parser;

        AssetTextEntry *Line = (AssetTextEntry *) GC_Parser_allocate(Parser, sizeof(AssetTextEntry));
        TextParser->Lines = Line;

        // Line->isKey = true;
        b32 isKey = true;

        u8 *key = (u8 *) Line->Key.key;
        u8 *value = (u8 *) Line->value;

        u32 key_size = 0;
        u32 value_size = 0;

        while (Parser->index < Parser->bytes)
        {
            b32 is_newline = newline(Parser);

            if (is_newline)
            {
                if (key_size > 0 && value_size > 0)
                {
                    TextParser->count++;

                    *key = '\0';
                    *value = '\0';

                    Line->valueBytes = strlen(Line->value);

                    Line = (AssetTextEntry *) GC_Parser_allocate(Parser, sizeof(AssetTextEntry));

                    // Line->isKey = true;
                    isKey = true;
                    key = (u8 *) Line->Key.key;
                    value = (u8 *) Line->value;

                    key_size = 0;
                    value_size = 0;
                }
                // -- Invalid line, reuse the same Line, don't allocate a new one.
                else
                {
                    // Line->isKey = true;
                    isKey = true;
                    key = (u8 *) Line->Key.key;
                    value = (u8 *) Line->value;

                    *key = '\0';
                    *value = '\0';

                    key_size = 0;
                    value_size = 0;
                }
            }

            if ((isKey && is_whitespace(Parser)) ||
                (!isKey && value_size == 0 && is_whitespace(Parser)) ||
                is_char(Parser, '=')) {
                isKey = false;
            }
            else
            {
                u8 c = get_current(Parser);

                if (isKey)
                {
                    *key++ = c;
                    key_size++;
                }
                else
                {
                    *value++ = c;
                    value_size++;
                }
            }

            Parser->index++;

            if (Parser->index == Parser->bytes)
            {
                if (key_size > 0 && value_size > 0)
                {
                    TextParser->count++;

                    *key = '\0';
                    *value = '\0';
                    Line->valueBytes = strlen(Line->value);
                }
            }
        }

        result = true;
    }

    return result;
}

b32 ObjParser_parse(GC_Parser *Parser)
{
    b32 result = false;

    if (Parser && Parser->bytes > 0)
    {
        GC_Parser_check(Parser);

        u8 *data = Parser->Input;

        GC_Parser_Obj *ObjParser = (GC_Parser_Obj *) Parser;
        b32 is_newline = false;

        u32 tokenIndex = 0;
        u32 charIndex = 0;
        GC_Parser_ObjLineToken *Token = &ObjParser->tokens[tokenIndex++];

        // ----------------------------------------------------------------------------------
        // -- Allocate data lists.
        // ----------------------------------------------------------------------------------

        // NOTE(gabic): verificare pentru numarul de elemente in buffer.
        if (ObjParser->bufferCount == 0)
            ObjParser->bufferCount = 4096 * 4;

        size_t vertexBytes = sizeof(r32) * 3;
        size_t faceBytes = sizeof(u32) * 3;
        size_t uvBytes = sizeof(r32) * 2;
        size_t normalBytes = sizeof(r32) * 3;

        ObjParser->vertices = (ObjectVertex_ptr) GC_Parser_allocate(Parser, vertexBytes * ObjParser->bufferCount);
        ObjParser->uvs = (ObjectUV_ptr) GC_Parser_allocate(Parser, uvBytes * ObjParser->bufferCount);
        ObjParser->normals = (ObjectNormal_ptr) GC_Parser_allocate(Parser, normalBytes * ObjParser->bufferCount);
        ObjParser->facesVertices = (ObjectFaceVertex_ptr) GC_Parser_allocate(Parser, faceBytes * ObjParser->bufferCount);
        ObjParser->facesUVs = (ObjectFaceUV_ptr) GC_Parser_allocate(Parser, faceBytes * ObjParser->bufferCount);
        ObjParser->facesNormals = (ObjectFaceNormal_ptr) GC_Parser_allocate(Parser, faceBytes * ObjParser->bufferCount);

        ObjectVertex_ptr vertex = ObjParser->vertices;
        ObjectUV_ptr uvs = ObjParser->uvs;
        ObjectNormal_ptr normals = ObjParser->normals;
        ObjectFaceVertex_ptr facesVertices = ObjParser->facesVertices;
        ObjectFaceUV_ptr facesUVs = ObjParser->facesUVs;
        ObjectFaceNormal_ptr facesNormals = ObjParser->facesNormals;

        ObjParser->vertices = vertex;
        u32 lines = 0;

        while (Parser->index < Parser->bytes)
        {
            if (ObjParser->vertexCount > ObjParser->bufferCount ||
                ObjParser->faceCount > ObjParser->bufferCount ||
                ObjParser->uvCount > ObjParser->bufferCount ||
                ObjParser->normalCount > ObjParser->bufferCount)
            {
                printf("ERROR: Element buffer is too small !");
                exit(EXIT_FAILURE);
            }

            char c = get_current(Parser);

            // -- End of token value, switch to the next token.
            if (c == ' ' || c == '\t' || c == '/')
            {
                Token->value[charIndex] = '\0';
                Token->delim = c;

                charIndex = 0;
                ObjParser->lastUsedTokens++;
                Token = &ObjParser->tokens[tokenIndex++];
            }
            else
            {
                // NOTE(gabic): No token value length check !
                Token->value[charIndex++] = c;
            }

            Parser->index++;
            b32 process = false;

            // -- End of characters.
            if (Parser->index == Parser->bytes)
                process = true;

            is_newline = newline(Parser);

            if (process || is_newline)
            {
                lines++;
				Token->value[charIndex] = '\0';

                // -- Tokens processing.
                ObjParser->lastUsedTokens++;

                // NOTE(gabic): sa fac o verificare daca este adaugat un caracter intr-un token
                if (ObjParser->lastUsedTokens > 0)
                {
                    GC_Parser_ObjLineToken *CurrentToken = &ObjParser->tokens[0];

                    if (strcmp(CurrentToken->value, "v") == 0 &&
                        ObjParser->lastUsedTokens == 4)
                    {
                        CurrentToken++;

                        (*vertex)[0] = (r32) atof(CurrentToken->value);
                        CurrentToken++;
                        (*vertex)[1] = (r32) atof(CurrentToken->value);
                        CurrentToken++;
                        (*vertex)[2] = (r32) atof(CurrentToken->value);

                        ObjParser->vertexCount++;
                        vertex++;
                    }
                    else if (strcmp(CurrentToken->value, "vt") == 0 &&
                             ObjParser->lastUsedTokens == 3)
                    {
                        CurrentToken++;

                        (*uvs)[0] = (r32) atof(CurrentToken->value);
                        CurrentToken++;
                        (*uvs)[1] = (r32) atof(CurrentToken->value);

                        ObjParser->uvCount++;
                        uvs++;
                    }
                    else if (strcmp(CurrentToken->value, "vn") == 0 &&
                             ObjParser->lastUsedTokens == 4)
                    {
                        CurrentToken++;

                        (*normals)[0] = (r32) atof(CurrentToken->value);
                        CurrentToken++;
                        (*normals)[1] = (r32) atof(CurrentToken->value);
                        CurrentToken++;
                        (*normals)[2] = (r32) atof(CurrentToken->value);

                        ObjParser->normalCount++;
                        normals++;
                    }
                    else if (strcmp(CurrentToken->value, "f") == 0 &&
                             ObjParser->lastUsedTokens == 10)
                    {
                        CurrentToken++;

                        // -- First vertex.
                        (*facesVertices)[0] = atoi(CurrentToken->value);
                        CurrentToken++;
                        (*facesUVs)[0] = atoi(CurrentToken->value);
                        CurrentToken++;
                        (*facesNormals)[0] = atoi(CurrentToken->value);
                        CurrentToken++;

                        if (!ObjParser->faceComponents)
                        {
                            ObjParser->faceComponents = FACE_VERTEX;

                            if ((*facesUVs)[0] != 0)
                                ObjParser->faceComponents |= FACE_UV;

                            if ((*facesNormals)[0] != 0)
                                ObjParser->faceComponents |= FACE_NORMAL;
                        }

                        // -- Second vertex.
                        (*facesVertices)[1] = atoi(CurrentToken->value);
                        CurrentToken++;
                        (*facesUVs)[1] = atoi(CurrentToken->value);
                        CurrentToken++;
                        (*facesNormals)[1] = atoi(CurrentToken->value);
                        CurrentToken++;

                        // -- Third vertex.
                        (*facesVertices)[2] = atoi(CurrentToken->value);
                        CurrentToken++;
                        (*facesUVs)[2] = atoi(CurrentToken->value);
                        CurrentToken++;
                        (*facesNormals)[2] = atoi(CurrentToken->value);

                        ObjParser->faceCount++;

                        facesVertices++;
                        facesUVs++;
                        facesNormals++;
                    }
                }

                tokenIndex = 0;
                charIndex = 0;
                ObjParser->lastUsedTokens = 0;

                Token = &ObjParser->tokens[tokenIndex++];
                Token->value[0] = '\0';
            }
        }

        if (ObjParser->vertexCount > 0 || ObjParser->faceCount > 0 || ObjParser->uvCount > 0 || ObjParser->normalCount > 0)
            result = true;
    }

    return result;
}