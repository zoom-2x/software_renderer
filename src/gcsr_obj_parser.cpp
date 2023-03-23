// ----------------------------------------------------------------------------------
// -- File: gcsr_obj_parser.cpp
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description: Simple obj format parser.
// -- Created: 2020-08-22 20:28:04
// -- Modified:
// ----------------------------------------------------------------------------------

#include "gcsr_obj_parser.h"

ObjParser ObjParser_create(FileRead *File, Parse_t *Callback)
{
    ObjParser *Parser = (ObjParser *) malloc(sizeof(ObjParser));
    memset(Parser, 0, sizeof(ObjParser));

    Parser->initialized = true;
    Parser->totalSize = OBJ_PARSER_MEMORY_POOL_SIZE;

    Parser->engine_memory_pool_t = (u8 *) malloc(OBJ_PARSER_MEMORY_POOL_SIZE);
    memset(Parser->engine_memory_pool_t, 0, OBJ_PARSER_MEMORY_POOL_SIZE);

    Parser->bytes = File->size;
    Parser->Input = (u8 *) File->data;
    Parser->Callback = Callback;

    return Parser;
}

void ObjParser_destroy(ObjParser *Parser)
{
    free(Parser->Input);
    free(Parser->engine_memory_pool_t);

    Parser->engine_memory_pool_t = 0;
    Parser->initialized = false;
    Parser->totalSize = 0;
    Parser->allocated = 0;

    free(Parser);
}