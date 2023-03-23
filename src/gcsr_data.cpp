// ---------------------------------------------------------------------------------
// -- File: gcsr_data.cpp
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description:
// -- Created:
// -- Modified:
// ---------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------
// -- Type_Array operations.
// ---------------------------------------------------------------------------------

Type_Array *ta_create(memory_type_t location, u32 count, size_t size)
{
    Type_Array *Array = (Type_Array *) gc_mem_allocate(location, gcSize(Type_Array) + count * size);

    // DBG_SetType(Array, DATA_ARRAY);
    DBG_SetDataType(GET_BLOCK(Array), DATA_ARRAY);

    Array->length = count;
    Array->data_bytes = size;

    return Array;
}

__INLINE__ void *ta_index(Type_Array *Array, u32 index)
{
    void *Result = 0;

    if (index >= 0 && index < Array->length)
        Result = ((u8 *) (Array + 1) + index * Array->data_bytes);

    return Result;
}

__INLINE__ void ta_delete(Type_Array *Array, u32 index)
{
    if (Array)
    {
        void *data = ta_index(Array, index);

        platform_api_t *API = get_platform_api();
        API->gc_mem_clear(data, Array->data_bytes);
    }
}

__INLINE__ void ta_clear(Type_Array *Array)
{
    if (Array)
    {
        platform_api_t *API = get_platform_api();
        API->gc_mem_clear(Array + 1, Array->length * Array->data_bytes);
    }
}

__INLINE__ void ta_free(Type_Array *Array)
{
    if (Array)
        gc_mem_free(Array);
}

Type_Array2 *ta2_create(memory_type_t location, u32 count, size_t size)
{
    Type_Array2 *Array = (Type_Array2 *) gc_mem_allocate(location, gcSize(Type_Array2) +
                          count * (gcSize(Type_Array2Element) * size));

    DBG_SetDataType(GET_BLOCK(Array), DATA_ARRAY2);

    Array->length = count;
    Array->data_bytes = size;

    return Array;
}

__INLINE__ void *ta2_index(Type_Array2 *Array, u32 index)
{
    void *Result = 0;

    if (index >= 0 && index < Array->length)
    {
        Type_Array2Element *Element = (Type_Array2Element *) ((u8 *) (Array + 1) + index * (Array->data_bytes + gcSize(Type_Array2Element)));
        Result = Element + 1;
    }

    return Result;
}

__INLINE__ void ta2_setIndexUsed(Type_Array2 *Array, u32 index)
{
    Type_Array2Element *Element = (Type_Array2Element *) ta2_index(Array, index) - 1;
    Element->used = true;
}

__INLINE__ b32 ta2_isIndexFree(Type_Array2 *Array, u32 index)
{
    Type_Array2Element *Element = (Type_Array2Element *) ta2_index(Array, index) - 1;

    return Element->used;
}

__INLINE__ void ta2_delete(Type_Array2 *Array, u32 index)
{
    if (Array)
    {
        void *data = ta2_index(Array, index);
        Type_Array2Element *Element = (Type_Array2Element *) data - 1;
        Element->used = false;

        platform_api_t *API = get_platform_api();
        API->gc_mem_clear(data, Array->data_bytes);
    }
}

// ---------------------------------------------------------------------------------
// -- Type_LinkedList operations.
// ---------------------------------------------------------------------------------

// NOTE(gabic): va trebui sa adaug posibilitatea de a adauga elemente de acelasi tip
// dar de dimensiuni diferite, ex: Type_Array, va trebui sa pot specifica atunci
// cand adaug un element ce dimensiune are.

/**
 * @param memory_type_t location The memory location where the list will be allocated.
 * @param size_t size The element size.
 */
Type_LinkedList *tll_create(memory_type_t location, size_t size)
{
    Type_LinkedList *List = (Type_LinkedList *) gc_mem_allocate(location, gcSize(Type_LinkedList));

    // DBG_SetType(List, DATA_LIST);
    DBG_SetDataType(GET_BLOCK(List), DATA_LIST);

    List->length = 0;
    List->total_bytes = gcSize(Type_LinkedList);
    List->data_bytes = size;
    List->Head = 0;
    List->Tail = 0;

    return List;
}

void *tll_get(Type_LinkedList *List, u32 index)
{
    void *Result = 0;

    if (List && index < List->length)
    {
        Type_LinkedListNode *Node = List->Head;
        u32 count = 0;

        while (Node && count++ <= index) {
            Node = Node->Next;
        }

        if (Node)
            Result = Node + 1;
    }

    return Result;
}

// void *tll_insert(Type_LinkedList *List, size_t size)
void *tll_insert(Type_LinkedList *List)
{
    void *Result = 0;

    if (List)
    {
        size_t total_bytes = gcSize(Type_LinkedListNode) + List->data_bytes;
        memory_block_t *Block = GET_BLOCK(List);
        Type_LinkedListNode *Node = (Type_LinkedListNode *) gc_mem_allocate(Block->location, total_bytes);

        // DBG_SetType(Node, DATA_LIST_NODE);
        DBG_SetDataType(GET_BLOCK(Node), DATA_LIST_NODE);
        // DBG_SetDataType(Node, List->DataType);

        Node->Parent = List;
        Result = Node + 1;

        if (!List->Tail)
        {
            List->Head = Node;
            List->Tail = Node;
        }
        else
        {
            List->Tail->Next = Node;
            Node->Prev = List->Tail;
            List->Tail = Node;
        }

        List->total_bytes += gcSize(Type_LinkedListNode) + List->data_bytes;
        List->length++;
    }

    return Result;
}

void *tll_insertPre(Type_LinkedList *List, void *data)
{
    void *Result = 0;
    Type_LinkedListNode *Node = TLL_GetNode(data);

    if (List && Node && Node->Parent == List)
    {
        size_t total_bytes = gcSize(Type_LinkedListNode) + List->data_bytes;

        memory_block_t *Block = GET_BLOCK(List);
        Type_LinkedListNode *FreeNode = (Type_LinkedListNode *) gc_mem_allocate(Block->location, total_bytes);

        // DBG_SetType(Node, DATA_LIST_NODE);
        DBG_SetDataType(GET_BLOCK(Node), DATA_LIST_NODE);
        // DBG_SetDataType(Node, List->DataType);

        FreeNode->Parent = List;
        Result = FreeNode + 1;

        if (Node->Prev)
        {
            Node->Prev->Next = FreeNode;
            FreeNode->Prev = Node->Prev;
        }
        else
            List->Head = FreeNode;

        FreeNode->Next = Node;
        Node->Prev = FreeNode;

        List->total_bytes += gcSize(Type_LinkedListNode) + List->data_bytes;
        List->length++;
    }

    return Result;
}

void *tll_insertPost(Type_LinkedList *List, void *data)
{
    void *Result = 0;
    Type_LinkedListNode *Node = TLL_GetNode(data);

    if (List && Node && Node->Parent == List)
    {
        size_t total_bytes = gcSize(Type_LinkedListNode) + List->data_bytes;

        memory_block_t *Block = GET_BLOCK(List);
        Type_LinkedListNode *FreeNode = (Type_LinkedListNode *) gc_mem_allocate(Block->location, total_bytes);

        // DBG_SetType(Node, DATA_LIST_NODE);
        DBG_SetDataType(GET_BLOCK(Node), DATA_LIST_NODE);
        // DBG_SetDataType(Node, List->DataType);

        FreeNode->Parent = List;
        Result = FreeNode + 1;

        if (Node->Next)
        {
            Node->Next->Prev = FreeNode;
            FreeNode->Next = Node->Next;
        }
        else
            List->Tail = FreeNode;

        Node->Next = FreeNode;
        FreeNode->Prev = Node;

        List->total_bytes += gcSize(Type_LinkedListNode) + List->data_bytes;
        List->length++;
    }

    return Result;
}

void tll_switchNodes(Type_LinkedList *List, void *Data1, void *Data2)
{
    Type_LinkedListNode *Node1 = TLL_GetNode(Data1);
    Type_LinkedListNode *Node2 = TLL_GetNode(Data2);

    if (List && Node1 && Node2 && Node1->Parent == List && Node2->Parent == List)
    {
        Type_LinkedListNode *TmpPrev = Node1->Prev;
        Type_LinkedListNode *TmpNext = Node1->Next;

        if (Node1->Prev == Node2)
        {
            if (Node2->Prev)
                Node2->Prev->Next = Node1;

            Node1->Prev = Node2->Prev;
            Node1->Next = Node2;

            Node2->Prev = Node1;
            Node2->Next = TmpNext;
        }
        else if (Node2->Prev == Node1)
        {
            if (TmpPrev)
                TmpPrev->Next = Node2;

            Node1->Prev = Node2;
            Node1->Next = Node2->Next;

            Node2->Prev = TmpPrev;
            Node2->Next = Node1;
        }
        else
        {
            if (Node1->Prev)
                Node1->Prev->Next = Node2;

            if (Node2->Prev)
                Node2->Prev->Next = Node1;

            Node1->Prev = Node2->Prev;
            Node1->Next = Node2->Next;

            Node2->Prev = TmpPrev;
            Node2->Next = TmpNext;
        }

        if (List->Head == Node1)
            List->Head = Node2;
        else if (List->Head == Node2)
            List->Head = Node1;
        else if (List->Tail == Node1)
            List->Tail = Node2;
        else if (List->Tail == Node2)
            List->Tail = Node1;
    }
}

void *tll_search(Type_LinkedList *List, tll_searchCallback Callback, void *Search)
{
    void *Result = 0;

    if (List && List->length > 0)
    {
        Type_LinkedListNode *Node = List->Head;

        while (Node)
        {
            void *data = Node + 1;
            b32 found = Callback(data, Search);

            if (found)
            {
                Result = data;
                break;
            }

            Node = Node->Next;
        }
    }

    return Result;
}

void tll_delete(Type_LinkedList *List, void *data)
{
    Type_LinkedListNode *Node = TLL_GetNode(data);

    if (List && Node->Parent == List)
    {
        Type_LinkedListNode *PrevNode = Node->Prev;
        Type_LinkedListNode *NextNode = Node->Next;

        // The node is the head.
        if (PrevNode == 0)
        {
            if (NextNode)
            {
                List->Head = NextNode;
                NextNode->Prev = 0;
            }
            else
                List->Head = 0;
        }

        // The node is the tail.
        else if (NextNode == 0)
        {
            if (PrevNode)
            {
                List->Tail = PrevNode;
                PrevNode->Next = 0;
            }
            else
                List->Tail = 0;
        }
        else
        {
            PrevNode->Next = NextNode;
            NextNode->Prev = PrevNode;
        }

        List->total_bytes -= gcSize(Type_LinkedListNode) + List->data_bytes;
        List->length--;

        // DBG_ClearType(Node);
        DBG_ClearType(GET_BLOCK(Node));

        gc_mem_free(Node);
    }
}

void tll_clear(Type_LinkedList *List)
{
    if (List && List->length > 0)
    {
        memory_block_t *ListBlock = GET_BLOCK(List);

        Type_LinkedListNode *Node = List->Head;
        memory_block_t *Block = 0;

        // -- Mark all the list blocks as free.
        while (Node)
        {
            Block = (memory_block_t *) Node - 1;
            gc_mem_free_block(Block);

            Node = Node->Next;
        }

        gc_mem_merge(ListBlock->location);
        List->length = 0;
    }
}

__INLINE__ void tll_free(Type_LinkedList *List)
{
    tll_clear(List);
    gc_mem_free(List);
}

__INLINE__ void *tll_getNext(Type_LinkedList *List, void *data)
{
    void *Result = 0;

    if (List && data)
    {
        Type_LinkedListNode *Node = TLL_GetNode(data);
        Result = Node->Next;
    }

    return Result;
}

__INLINE__ void *tll_getPrev(Type_LinkedList *List, void *data)
{
    void *Result = 0;

    if (List && data)
    {
        Type_LinkedListNode *Node = TLL_GetNode(data);
        Result = Node->Prev;
    }

    return Result;
}

// ---------------------------------------------------------------------------------
// -- Type_StackArray operations.
// ---------------------------------------------------------------------------------

Type_StackArray *tsa_create(memory_type_t location, u32 count, size_t size)
{
    size_t total_bytes = gcSize(Type_StackArray) + count * size;
    Type_StackArray *Stack = (Type_StackArray *) gc_mem_allocate(location, total_bytes);

    // DBG_SetType(Stack, DATA_STACK_ARRAY);
    DBG_SetDataType(GET_BLOCK(Stack), DATA_STACK_ARRAY);

    Stack->size = count;
    Stack->top = -1;
    Stack->data_bytes = size;

    return Stack;
}

__INLINE__ void tsa_free(Type_StackArray *Stack)
{
    if (Stack)
        gc_mem_free(Stack);
}

void *tsa_push(Type_StackArray *Stack)
{
    void *Result = 0;

    if (Stack)
    {
        if (Stack->top + 1 < (s32) Stack->size)
        {
            Stack->top++;
            Result = (u8 *) (Stack + 1) + Stack->top * Stack->data_bytes;
        }
    }

    return Result;
}

void *tsa_pop(Type_StackArray *Stack)
{
    void *Result = 0;

    if (Stack)
    {
        if (Stack->top - 1 >= -1)
        {
            Result = (u8 *) (Stack + 1) + Stack->top * Stack->data_bytes;

            // platform_api_t *API = get_platform_api();
            // API->gc_mem_clear(data, Stack->data_bytes);

            Stack->top--;
        }
    }

    return Result;
}

__INLINE__ void *tsa_peek(Type_StackArray *Stack)
{
    void *Result = 0;

    if (Stack && Stack->top >= 0 && Stack->top < (s32) Stack->size)
        Result = (u8 *) (Stack + 1) + Stack->top * Stack->data_bytes;

    return Result;
}

__INLINE__ void tsa_clear(Type_StackArray *Stack)
{
    if (Stack)
    {
        Stack->top = -1;
        platform_api_t *API = get_platform_api();
        API->gc_mem_clear(Stack + 1, Stack->data_bytes * Stack->size);
    }
}

// ---------------------------------------------------------------------------------
// -- Type_Queue operations.
// ---------------------------------------------------------------------------------

// TODO(gabic): de reimplementat !

Type_Queue *tq_create(memory_type_t location, size_t size)
{
    Type_Queue *Queue = (Type_Queue *) gc_mem_allocate(location, gcSize(Type_Queue));

    // DBG_SetType(Queue, DATA_QUEUE);
    DBG_SetDataType(GET_BLOCK(Queue), DATA_QUEUE);

    Queue->length = 0;
    Queue->data_bytes = size;
    Queue->total_bytes = gcSize(Queue);
    Queue->Front = 0;
    Queue->Rear = 0;

    return Queue;
}

void *tq_enqueue(Type_Queue *Queue)
{
    void *Result = 0;

    if (Queue)
    {
        memory_type_t location = GET_DATA_LOCATION(Queue);
        size_t total = gcSize(Type_QueueNode) + Queue->data_bytes;
        Type_QueueNode *FreeNode = (Type_QueueNode *) gc_mem_allocate(location, total);

        // DBG_SetType(FreeNode, DATA_QUEUE_NODE);
        // DBG_SetDataType(FreeNode, Queue->DataType);
        DBG_SetDataType(GET_BLOCK(FreeNode), DATA_QUEUE_NODE);

        FreeNode->Next = 0;

        if (Queue->Rear)
            Queue->Rear->Next = FreeNode;
        else
            Queue->Front = FreeNode;

        Queue->Rear = FreeNode;
        Queue->length++;
        Queue->total_bytes += total;

        Result = FreeNode + 1;
    }

    return Result;
}

void tq_dequeue(Type_Queue *Queue)
{
    if (Queue->length > 0)
    {
        if (Queue->Front)
        {
            Type_QueueNode *Node = Queue->Front;
            // void *Src = Node + 1;

            // platform_api_t *API = get_platform_api();
            // API->mem_copy(Out, Src, Queue->data_bytes, Queue->data_bytes);

            Queue->Front = Queue->Front->Next;
            Queue->length--;
            Queue->total_bytes -= gcSize(Type_QueueNode) + Queue->data_bytes;
            gc_mem_free(Node);
        }
    }
}

__INLINE__ void *tq_front(Type_Queue *Queue)
{
    void *Result = 0;

    if (Queue)
        Result = Queue->Front + 1;

    return Result;
}

__INLINE__ void *tq_rear(Type_Queue *Queue)
{
    void *Result = 0;

    if (Queue)
        Result = Queue->Rear + 1;

    return Result;
}

void tq_clear(Type_Queue *Queue)
{
    if (Queue)
    {
        if (Queue->length > 0)
        {
            Type_QueueNode *Node = Queue->Front;

            while (Node)
            {
                memory_block_t *NodeBlock = GET_BLOCK(Node);

                // DBG_ClearType(Node);
                DBG_ClearType(NodeBlock);

                // NodeBlock->status = BLOCK_FREE;
                gc_mem_free_block(NodeBlock);
                Node = Node->Next;
            }

            gc_mem_merge(GET_DATA_LOCATION(Queue));
        }

        Queue->total_bytes = gcSize(Type_Queue);
        Queue->length = 0;
        Queue->Front = 0;
        Queue->Rear = 0;
    }
}

void tq_delete(Type_Queue *Queue)
{
    tq_clear(Queue);
    DBG_ClearType(Queue);
    gc_mem_free(Queue);
}

// ---------------------------------------------------------------------------------
// -- Type_Hashtable operations.
// ---------------------------------------------------------------------------------

u32 hash_djb2(char *str, u32 size)
{
    u32 c;
    u64 hash = 5381;
    u32 len = (u32) strlen(str);

    for (u32 i = 0; i < len; ++i)
    {
        c = str[i];
        hash = ((hash << 5) + hash) + c;
    }

    hash = hash % size;

    return (u32) hash;
}

Type_Hashtable *tht_create(memory_type_t location, u32 count)
{
    size_t total_bytes = gcSize(Type_Hashtable) + gcSize(Type_HashtableBucket) * count;
    Type_Hashtable *Table = (Type_Hashtable *) gc_mem_allocate(location, total_bytes);

    DBG_SetDataType(GET_BLOCK(Table), DATA_HASHTABLE);

    Table->count = count;
    Table->free = count;
    Table->used = 0;
    Table->total_bytes = gcSize(Type_Hashtable);

    return Table;
}

// NOTE(gabic): For now there is no check on the key length, it's assumed that
// the key will be 64 bytes long, for now.

void *tht_insert(Type_Hashtable *Table, char *key, size_t size)
{
    void *Result = 0;

    if (Table)
    {
        if (Table->used + 1 > Table->count)
            printf("\nWARNING: Hashtable is full !");
        else
        {
            // -- First the key is searched to see if it exists, if not
            // -- then it will be added.

            u32 index = hash_djb2(key, Table->count);
            Type_HashtableBucket *Bucket = (Type_HashtableBucket *) (Table + 1) + index;

            Type_HashtableElement *Search = Bucket->Head;
            Type_HashtableElement *PrevElement = 0;
            Type_HashtableElement *NextElement = 0;

            b32 keyExists = false;

            if (Bucket->count > 0)
            {
                while (Search && !keyExists)
                {
                    if (strncmp(Search->key, key, HASHTABLE_MAX_KEY_SIZE) == 0)
                    {
                        keyExists = true;
                        NextElement = Search->Next;
                        Table->total_bytes -= gcSize(Type_HashtableElement) + Search->valueBytes;

                        gc_mem_free(Search);

                        break;
                    }

                    if (!keyExists)
                    {
                        PrevElement = Search;
                        Search = Search->Next;
                    }
                }
            }

            memory_type_t location = GET_DATA_LOCATION(Table);
            size_t total_bytes = gcSize(Type_HashtableElement) + size;
            Type_HashtableElement *Element = (Type_HashtableElement *) gc_mem_allocate(location, total_bytes);

            DBG_SetDataType(GET_BLOCK(Element), DATA_HASHTABLE_ELEMENT);
            Table->total_bytes += total_bytes;
            Element->valueBytes = size;

            strncpy(Element->key, key, HASHTABLE_MAX_KEY_SIZE);
            Result = Element + 1;

            if (!keyExists)
            {
                Table->used++;
                Table->free--;

                if (Bucket->count == 0)
                    Bucket->Head = Element;
                else
                {
                    Type_HashtableElement *LastElement = Bucket->Head;

                    while (LastElement->Next) {
                        LastElement = LastElement->Next;
                    }

                    LastElement->Next = Element;
                }

                Bucket->count++;
            }
            else
            {
                if (!PrevElement)
                    Bucket->Head = Element;
                else
                {
                    PrevElement->Next = Element;
                    Element->Next = NextElement;
                }
            }
        }
    }

    return Result;
}

Type_HashtableElement *tht_search(Type_Hashtable *Table, char *key)
{
    Type_HashtableElement *Result = 0;

    if (Table && key)
    {
        u32 index = hash_djb2(key, Table->count);
        Type_HashtableBucket *Bucket = (Type_HashtableBucket *) (Table + 1) + index;
        Type_HashtableElement *Element = Bucket->Head;
        b32 found = false;

        if (Bucket->count > 0)
        {
            while (Element && !found)
            {
                if (strncmp(Element->key, key, HASHTABLE_MAX_KEY_SIZE) == 0)
                {
                    Result = Element;
                    found = true;
                }

                Element = Element->Next;
            }
        }
    }

    return Result;
}

void tht_delete(Type_Hashtable *Table, char *key)
{
    if (Table && key)
    {
        u32 index = hash_djb2(key, Table->count);
        Type_HashtableBucket *Bucket = (Type_HashtableBucket *) (Table + 1) + index;
        Type_HashtableElement *Element = 0;
        Type_HashtableElement *PrevElement = 0;

        b32 found = false;

        if (Bucket->count == 0)
            return;

        if (Bucket->count == 1)
        {
            found = true;
            Element = Bucket->Head;
            Bucket->Head = 0;
            Bucket->count--;
        }
        else
        {
            Element = Bucket->Head;

            while (!found && Element)
            {
                if (strncmp(Element->key, key, HASHTABLE_MAX_KEY_SIZE) == 0)
                    found = true;

                if (!found)
                {
                    PrevElement = Element;
                    Element = Element->Next;
                }
            }

            if (found)
            {
                if (!PrevElement)
                    Bucket->Head = Element->Next;
                else
                    PrevElement->Next = Element->Next;

                Bucket->count--;
                Table->used--;
                Table->free++;
                Table->total_bytes -= gcSize(Type_HashtableElement) + Element->valueBytes;

                memory_block_t *Block = GET_BLOCK(Element);

                if (Block->status != BLOCK_FREE)
                    gc_mem_free(Element);
            }
        }
    }
}

void tht_hashtable_free(Type_Hashtable *Table)
{
    if (Table)
    {
        Type_HashtableBucket *BucketBase = (Type_HashtableBucket *)(Table + 1);

        for (u32 i = 0; i < Table->count; ++i)
        {
            Type_HashtableBucket *Bucket = BucketBase + i;
            Type_HashtableElement *Element = (Type_HashtableElement *) Bucket->Head;

            while (Element)
            {
                memory_block_t *BlockToFree = GET_BLOCK(Element);
                gc_mem_free_block(BlockToFree);

                Element = Element->Next;
            }
        }

        memory_block_t *TableBlock = GET_BLOCK(Table);
        gc_mem_free_block(TableBlock);

        gc_mem_merge(TableBlock->location);
    }
}

// ----------------------------------------------------------------------------------
// -- type_hashtable2_t.
// ----------------------------------------------------------------------------------

u64 hash_djb22(char *str)
{
    u32 c;
    u64 hash = 5381;
    u32 len = (u32) strlen(str);

    for (u32 i = 0; i < len; ++i)
    {
        c = str[i];
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

type_hashtable2_t *tht2_create(u32 buckets, memory_type_t location, char *name)
{
    type_hashtable2_t *res = 0;

    size_t table_bytes = sizeof(type_hashtable2_t) + sizeof(type_hashtable2_bucket_t) * buckets;

    res = (type_hashtable2_t *) mem_allocate_name(MEMORY_ASSETS, table_bytes, name);
    res->table = (type_hashtable2_bucket_t *) (res + 1);
    res->buckets = buckets;
    res->slots = buckets * HASHTABLE2_BUCKET_SIZE;
    res->slots_used = 0;

    for (u32 k = 0; k < buckets; ++k)
    {
        type_hashtable2_bucket_t *bucket = &res->table[k];
        bucket->cursor = 0;
    }

    return res;
}

// Insert into the table a pointer to the wanted data structure.
void tht2_insert(type_hashtable2_t *Table, char *key, void *value)
{
    if (!Table)
        return;

    SDL_assert(Table->slots_used < Table->slots);

    u64 ti = hash_djb22(key) % Table->buckets;
    type_hashtable2_bucket_t *bucket = &Table->table[ti];

    // NOTE(gabic): No overflow bucket for now.
    SDL_assert(bucket->cursor < HASHTABLE2_BUCKET_SIZE);

    u8 bi = bucket->cursor++;
    bucket->key[bi] = key;
    bucket->value[bi] = value;

    Table->slots_used++;
}

void *tht2_get(type_hashtable2_t *table, char *key)
{
    void *res = 0;

    if (table && table->buckets)
    {
        u64 ti = hash_djb22(key) % table->buckets;
        type_hashtable2_bucket_t *bucket = &table->table[ti];

        if (bucket->cursor == 0)
        {
            if (bucket->key[0] && strcmp(key, bucket->key[0]) == 0)
                res = bucket->value[0];
        }
        // Search for the key.
        else
        {
            for (u32 i = 0; i < bucket->cursor; ++i)
            {
                if (bucket->key[i] && strcmp(key, bucket->key[i]) == 0)
                {
                    res = bucket->value[i];
                    break;
                }
            }
        }
    }

    return res;
}

// ----------------------------------------------------------------------------------
// -- Type_String operations.
// ----------------------------------------------------------------------------------

size_t stringLength(char *input)
{
    size_t len = 0;

    while (*input++) {
        len++;
    }

    return len;
}

__INLINE__ size_t ts_length(Type_String *String) {
    return stringLength(String->S);
}

Type_String *ts_createEmpty(memory_type_t location, size_t size)
{
    Type_String *Result = mem_allocateString(location, size + 1);
    DBG_SetDataType(GET_BLOCK(Result), DATA_STRING);

    Result->length = size;
    Result->Next = 0;
    Result->S = (char *) (Result + 1);

    return Result;
}

Type_String *ts_create(memory_type_t location, char *input)
{
    size_t size = stringLength(input);
    Type_String *Result = mem_allocateString(location, size + 1);
    DBG_SetDataType(GET_BLOCK(Result), DATA_STRING);

    Result->length = size;
    Result->Next = 0;

    u32 index = 0;
    char *Src = input;
    char *Dest = (char *) (Result + 1);

    while (index++ < size) {
        *Dest++ = *Src++;
    }

    *Dest = '\0';

    Result->S = (char *) (Result + 1);

    return Result;
}

void ts_copy(Type_String *String, char *Src)
{
    char *Dest = (char *) (String + 1);
    u32 index = 0;

    while (index++ < String->length && *Src != '\0') {
        *Dest++ = *Src++;
    }

    *Dest = '\0';
}

__INLINE__ void ts_append(Type_String *String1, Type_String *String2) {
    String1->Next = String2;
}

Type_String *ts_getAppended(Type_String *String)
{
    size_t length = 0;
    Type_String *Tmp = String;

    while (Tmp)
    {
        length += stringLength(Tmp->S);
        Tmp = Tmp->Next;
    }

    Type_String *Result = mem_allocateString(GET_DATA_LOCATION(String), length + 1);
    Result->length = length;
    Result->Next = 0;

    Tmp = String;
    char *Dest = Result->S;
    u32 index = 0;

    while (Tmp)
    {
        char *Src = Tmp->S;

        while (*Src != '\0' && index++ < Result->length) {
            *Dest++ = *Src++;
        }

        Tmp = Tmp->Next;
    }

    return Result;
}