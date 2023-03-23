// ----------------------------------------------------------------------------------
// -- File: gcsr_text_config_parser.h
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description: Simple config text file parser.
// -- Created: 2020-08-22 14:42:49
// -- Modified:
// ----------------------------------------------------------------------------------

#ifndef GCSR_CONFIG_PARSER_H
#define GCSR_CONFIG_PARSER_H

#include <stdlib.h>
#include "gcsr_types.h"

#define TEXT_CONFIG_LINE_KEY_SIZE 64
#define TEXT_CONFIG_LINE_VALUE_SIZE 128

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)

#define TEXT_CONFIG_PARSER_MEMORY_POOL_SIZE Megabytes(1)

// struct TextConfigLine
// {
//     size_t valueLen;

//     char key[64];
//     char value[128];
// };

struct TextConfigParser
{
    u32 lines;

    u32 bytes;
    u32 index;

    u8 *Input;

    b32 initialized;

    size_t totalSize;
    size_t allocated;
    u8 *engine_memory_pool_t;

    AssetTextEntry *Lines;
};

#endif
