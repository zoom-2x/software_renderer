// ---------------------------------------------------------------------------------
// -- File: gcsr_data.h
// ---------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created:
// -- Modified:
// ---------------------------------------------------------------------------------

#ifndef GCSR_DATA_H
#define GCSR_DATA_H

#define MIN_LIST_NODE_COUNT 10
#define MIN_EXTENSION_COUNT 10

#define SEPARATOR "\n----------------------------------------------------------------------------------"
#define SEPARATOR_TITLE "\n=================================================================================="

#define gcSize(type) sizeof(type)
#define gcGetData(Node) (Node + 1)

// ---------------------------------------------------------------------------------
// -- Various data structures.
// ---------------------------------------------------------------------------------

// NOTE(gabic): sa ma mai gandesc daca mai e nevoie de informatiile astea.

enum DataType
{
    #if GC_DEBUG_MODE
    DDATA_CAR,
    DDATA_PERSON,
    #endif

    DDATA_B32,

    DDATA_U8,
    DDATA_S8,

    DDATA_U16,
    DDATA_S16,

    DDATA_U32,
    DDATA_S32,

    DDATA_U64,
    DDATA_S64,

    DDATA_CHAR,
    DDATA_STRING,

    DDATA_TESTSTRUCT
};

struct TestStruct
{
    u32 a;
    u64 b;
};

#define __getB32(data) (b32 *) data
#define __getR32(data) (r32 *) data
#define __getU32(data) (u32 *) data
#define __getS32(data) (s32 *) data

TestStruct *__getTestStruct(void *data) {
    return (TestStruct *) data;
}

// ----------------------------------------------------------------------------------

#define GetString(TypeString) (char *) (TypeString->S)

struct Type_String
{
    DBG_AddStructTypeFields;

    size_t length;
    Type_String *Next;
    char *S;
};

// ----------------------------------------------------------------------------------
// -- A fixed block of memory used to store an array of structs.
// ----------------------------------------------------------------------------------

#define TA_Data(Array) (Array + 1)
#define TA_Index(Array, index, type) (type *) ta_index(Array, index);

// NOTE(gabic): vreau ca sa am posibilitatea de a sterge din array elemente ?
// Daca da atunci va trebui sa stiu care au fost sterse.

struct Type_Array
{
    DBG_AddStructTypeFields;

    u32 length;
    size_t data_bytes;
};

struct Type_Array2Element
{
    b32 used;
};

struct Type_Array2
{
    DBG_AddStructTypeFields;

    u32 length;
    size_t data_bytes;
};

// ---------------------------------------------------------------------------------

#define TLL_GetNode(var) ((Type_LinkedListNode *) var - 1)
#define TLL_GetData(var, type) (type *) (var + 1)
#define TLL_GetFirst(List, type) (type *) (List->Head ? List->Head + 1 : 0)
#define TLL_GetLast(List, type) (type *) (List->Tail ? List->Tail + 1 : 0)
#define TLL_GetPrev(data, type) (type *) (data && TLL_GetNode(data)->Prev ? (TLL_GetNode(data)->Prev + 1) : 0)
#define TLL_GetNext(data, type) (type *) (data && TLL_GetNode(data)->Next ? (TLL_GetNode(data)->Next + 1) : 0)

typedef b32 (* tll_searchCallback)(void *data, void *search);

struct Type_LinkedList;

struct Type_LinkedListNode
{
    Type_LinkedList *Parent;
    Type_LinkedListNode *NextFree;

    Type_LinkedListNode *Next;
    Type_LinkedListNode *Prev;
};

struct Type_LinkedList
{
    DBG_AddStructTypeFields;

    u32 length;
    size_t total_bytes;
    size_t data_bytes;

    Type_LinkedListNode *Head;
    Type_LinkedListNode *Tail;
};

// ---------------------------------------------------------------------------------

struct Type_StackArray
{
    DBG_AddStructTypeFields;

    u32 size;
    s32 top;

    size_t data_bytes;
};

// ----------------------------------------------------------------------------------

struct Type_StackList;

struct Type_StackListNode
{
    Type_StackListNode *Next;
};

struct Type_StackList
{
    DBG_AddStructTypeFields;

    u32 maxNodes;
    u32 length;

    Type_StackListNode *Top;
};

// ---------------------------------------------------------------------------------

#define TQ_GetNode(var) ((Type_QueueNode *) var - 1)
#define TQ_GetData(var, type) (type *) (var + 1)

typedef struct Type_QueueNode Type_QueueNode;

struct Type_QueueNode
{
    Type_QueueNode *Next;
};

typedef struct
{
    DBG_AddStructTypeFields;

    u32 length;

    size_t data_bytes;
    size_t total_bytes;

    Type_QueueNode *Front;
    Type_QueueNode *Rear;
} Type_Queue;

// ----------------------------------------------------------------------------------

#define HASHTABLE_MAX_KEY_SIZE 64
#define tht_data(Element) (void *) ((Type_HashtableElement *) Element + 1)

struct Type_HashtableElement
{
    size_t valueBytes;
    char key[HASHTABLE_MAX_KEY_SIZE];
    Type_HashtableElement *Next;
};

struct Type_HashtableBucket
{
    u32 count;
    Type_HashtableElement *Head;
};

struct Type_Hashtable
{
    DBG_AddStructTypeFields;

    u32 count;
    u32 free;
    u32 used;

    size_t total_bytes;
};

// ----------------------------------------------------------------------------------

#define HASHTABLE2_BUCKET_SIZE 8

typedef struct
{
    u8 cursor;

    char *key[HASHTABLE2_BUCKET_SIZE];
    void *value[HASHTABLE2_BUCKET_SIZE];
} type_hashtable2_bucket_t;

typedef struct
{
    u32 buckets;
    u32 slots;
    u32 slots_used;

    type_hashtable2_bucket_t *table;
} type_hashtable2_t;

// ----------------------------------------------------------------------------------

#endif