// ----------------------------------------------------------------------------------
// -- File: gcsr_obj_parser.h
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-08-24 19:50:58
// -- Modified:
// ----------------------------------------------------------------------------------

#ifndef GCSR_OBJ_PARSER_H
#define GCSR_OBJ_PARSER_H

#include <stdlib.h>
#include "gcsr_types.h"

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)

#define OBJ_PARSER_MEMORY_POOL_SIZE Megabytes(1)

struct ObjParser
{
    u32 vertexCount;
    u32 faceCount;
    u32 uvCount;
    u32 normalCount;

    b32 initialized;
    u32 bytes;

    u8 *Input;

    size_t totalSize;
    size_t allocated;
    u8 *engine_memory_pool_t;
};

#endif