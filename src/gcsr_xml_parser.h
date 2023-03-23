// ----------------------------------------------------------------------------------
// -- File: gcsr_xml_parser.h
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description: Simple xml parser.
// -- Created: 2020-08-17 15:18:31
// -- Modified:
// ----------------------------------------------------------------------------------

#ifndef GCSR_XML_PARSER_H
#define GCSR_XML_PARSER_H

#include <stdlib.h>
#include "gcsr_types.h"

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)

#define XML_PARSER_MEMORY_POOL_SIZE Megabytes(1)

struct XMLParser
{
    b32 initialized;

    size_t totalSize;
    size_t allocated;
    u8 *engine_memory_pool_t;
};

struct XMLNode
{
    char *tag;
    char *innerText;

    u32 childrenCount;

    XMLNode *Parent;
    XMLNode *Next;
    XMLNode *Children;
};

struct XMLDocument
{
    char *source;
    XMLNode *Root;
};

void XMLParser_init();
void *XMLParser_allocate(size_t size);
b32 XMLDocument_load(XMLDocument *Doc, char *filepath);
XMLNode *XMLNode_new(XMLNode *Parent);
void XMLParser_free();

#endif
