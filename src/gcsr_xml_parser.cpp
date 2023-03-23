// ----------------------------------------------------------------------------------
// -- File: gcsr_xml_parser
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description: Simple xml parser.
// -- Created: 2020-08-17 15:19:24
// -- Modified:
// ----------------------------------------------------------------------------------

#include "gcsr_xml_parser.h"

XMLParser _Parser;

void XMLParser_init()
{
    _Parser.initialized = true;

    _Parser.totalSize = XML_PARSER_MEMORY_POOL_SIZE;
    _Parser.allocated = 0;
    _Parser.engine_memory_pool_t = (u8 *) malloc(XML_PARSER_MEMORY_POOL_SIZE);
    memset(_Parser.engine_memory_pool_t, 0, XML_PARSER_MEMORY_POOL_SIZE);
}

__INLINE__ void XMLParser_check()
{
    if (!_Parser.initialized)
    {
        printf("\nERROR: The xml parser was not initialized !");
        exit(1);
    }
}

void *XMLParser_allocate(size_t size)
{
    XMLParser_check();

    void *Result;

    if (_Parser.allocated + size <= _Parser.totalSize)
    {
        Result = _Parser.engine_memory_pool_t + _Parser.allocated;
        _Parser.allocated += size;
    }
    else
    {
        printf("\nERROR: _Parser memory buffer is too small !");
        exit(1);
    }

    return Result;
}

b32 XMLDocument_load(XMLDocument *Doc, char *filepath)
{
    XMLParser_check();

    b32 result = false;

    if (Doc && filepath)
    {
        FILE *f = fopen(filepath, "rb");

        if (f)
        {
            fseek(f, 0, SEEK_END);
            u64 size = ftell(f);
            fseek(f, 0, SEEK_SET);

            Doc->source = (char *) XMLParser_allocate(size + 1);
            fread(Doc->source, size, 1, f);
            Doc->source[size] = '\0';

            fclose(f);

            // -- Parse the xml.

            char *buf = Doc->source;
            char c;
            char text[512];

            u32 textIdx = 0;
            u32 i = 0;
            b32 tagStart = false;
            b32 tagClose = false;

            XMLNode *CurrentNode = 0;
            XMLNode *PrevNode = 0;

            while ((c = buf[i++]) != '\0')
            {
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                    continue;

                // -- Closing tag.
                if (c == '<' && buf[i] == '/')
                {
                    tagClose = true;

                    // -- Check if closing tag matches the currentNode node tag.

                    char *tmp = CurrentNode->tag;
                    size_t len = strlen(CurrentNode->tag);
                    size_t check_len = 0;

                    i++;

                    while ((c = buf[i++]) != '>' && c != '\0')
                    {
                        if (c == *tmp++)
                            check_len++;
                    }

                    if (len != check_len)
                    {
                        printf("\nERROR: Invalid xml file !");
                        exit(1);
                    }

                    // -- If the node has some inner text.
                    if (textIdx > 0)
                    {
                        text[textIdx] = '\0';

                        CurrentNode->innerText = (char *) XMLParser_allocate(textIdx + 1);
                        strcpy(CurrentNode->innerText, text);

                        text[0] = '\0';
                        textIdx = 0;
                    }

                    CurrentNode = CurrentNode->Parent;
                }

                // -- End of start tag.
                else if (c == '>')
                {
                    tagStart = false;
                    text[textIdx] = '\0';

                    CurrentNode->tag = (char *) XMLParser_allocate(textIdx + 1);
                    strcpy(CurrentNode->tag, text);

                    text[0] = '\0';
                    textIdx = 0;
                }

                // -- Start of tag.
                else if (c == '<')
                {
                    // -- Adding a new node.
                    if (!CurrentNode)
                    {
                        CurrentNode = XMLNode_new(0);
                        Doc->Root = CurrentNode;
                    }
                    else
                        CurrentNode = XMLNode_new(CurrentNode);

                    if (CurrentNode->Parent)
                    {
                        CurrentNode->Parent->childrenCount++;

                        if (!CurrentNode->Parent->Children)
                            CurrentNode->Parent->Children = CurrentNode;
                        else
                        {
                            PrevNode = CurrentNode->Parent->Children;

                            while (PrevNode->Next) {
                                PrevNode = PrevNode->Next;
                            }

                            PrevNode->Next = CurrentNode;
                        }
                    }

                    tagStart = true;
                }
                else
                    text[textIdx++] = c;
            }

            result = true;
        }
        else
        {
            printf("\nERROR: Cannot open the configuration xml file: %s", filepath);
            exit(1);
        }
    }

    return result;
}

XMLNode *XMLNode_new(XMLNode *Parent)
{
    XMLParser_check();

    XMLNode *Result = (XMLNode *) XMLParser_allocate(sizeof(XMLNode));

    Result->tag = 0;
    Result->innerText = 0;
    Result->childrenCount = 0;
    Result->Parent = Parent;
    Result->Next = 0;
    Result->Children = 0;

    return Result;
}

void XMLParser_free()
{
    free(_Parser.engine_memory_pool_t);

    _Parser.engine_memory_pool_t = 0;
    _Parser.initialized = false;
    _Parser.totalSize = 0;
    _Parser.allocated = 0;
}