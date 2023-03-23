// ----------------------------------------------------------------------------------
// -- File: gcsr_text_config_parser.cpp
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description: Simple config text file parser.
// -- Created: 2020-08-22 14:49:28
// -- Modified:
// ----------------------------------------------------------------------------------

#include "gcsr_text_config_parser.h"

static u8 get_current(TextConfigParser *Parser)
{
    u8 c = Parser->Input[Parser->index];
    return c;
}

// static void next(TextConfigParser *Parser) {
//     Parser->index++;
// }

static b32 is_whitespace(TextConfigParser *Parser)
{
    u8 c = get_current(Parser);

    if (c == ' ' || c == '\t')
        return true;

    return false;
}

static b32 is_char(TextConfigParser *Parser, char t)
{
    u8 c = get_current(Parser);

    if (c == t)
        return true;

    return false;
}

static b32 newline(TextConfigParser *Parser)
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

// ----------------------------------------------------------------------------------

TextConfigParser *TextConfigParser_create(FileRead *File)
{
    TextConfigParser *Parser = (TextConfigParser *) malloc(sizeof(TextConfigParser));
    memset(Parser, 0, sizeof(TextConfigParser));

    Parser->initialized = true;
    Parser->totalSize = TEXT_CONFIG_PARSER_MEMORY_POOL_SIZE;

    Parser->engine_memory_pool_t = (u8 *) malloc(TEXT_CONFIG_PARSER_MEMORY_POOL_SIZE);
    memset(Parser->engine_memory_pool_t, 0, TEXT_CONFIG_PARSER_MEMORY_POOL_SIZE);

    Parser->bytes = File->size;
    Parser->Input = (u8 *) File->data;

    return Parser;
}

__INLINE__ void TextConfigParser_check(TextConfigParser *Parser)
{
    if (!Parser->initialized)
    {
        printf("\nERROR: The xml parser was not initialized !");
        exit(1);
    }
}

void *TextConfigParser_allocate(TextConfigParser *Parser, size_t size)
{
    TextConfigParser_check(Parser);

    void *Result = 0;

    if (Parser->allocated + size <= Parser->totalSize)
    {
        Result = Parser->engine_memory_pool_t + Parser->allocated;
        Parser->allocated += size;
    }

    return Result;
}

void TextConfigParser_destroy(TextConfigParser *Parser)
{
    free(Parser->Input);
    free(Parser->engine_memory_pool_t);

    Parser->engine_memory_pool_t = 0;
    Parser->initialized = false;
    Parser->totalSize = 0;
    Parser->allocated = 0;

    free(Parser);
}