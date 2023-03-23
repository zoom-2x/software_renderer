// ---------------------------------------------------------------------------------
// -- File: gcsr_data.h
// ---------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-06-08 22:18:26
// -- Modified: 2022-06-08 22:18:28
// ---------------------------------------------------------------------------------

#ifndef GCSR_DATA_H
#define GCSR_DATA_H

#define SEPARATOR "\n----------------------------------------------------------------------------------"
#define SEPARATOR_TITLE "\n=================================================================================="

#define gcSize(type) sizeof(type)
#define gcGetData(Node) (Node + 1)

// ----------------------------------------------------------------------------------
// -- Old hashtable (remove it).
// ----------------------------------------------------------------------------------

#ifndef ASSET_BUILDER_MODE

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

type_hashtable2_t *tht2_create(u32 buckets)
{
    type_hashtable2_t *res = 0;

    size_t table_bytes = sizeof(type_hashtable2_t) + sizeof(type_hashtable2_bucket_t) * buckets;

    res = (type_hashtable2_t *) GCSR_MALLOC(table_bytes);
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

    assert(Table->slots_used < Table->slots);

    u64 ti = hash_djb22(key) % Table->buckets;
    type_hashtable2_bucket_t *bucket = &Table->table[ti];

    // NOTE(gabic): No overflow bucket for now.
    assert(bucket->cursor < HASHTABLE2_BUCKET_SIZE);

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

#endif

// ----------------------------------------------------------------------------------
// -- Edge structure.
// ----------------------------------------------------------------------------------

#pragma pack(push, 1)
typedef struct
{
    u32 low;
    u32 high;
} ds_edge_t;
#pragma pack(pop)

#define EDGE_INIT(edge, idx1, idx2) \
{ \
    if (idx1 <= idx2) \
    { \
        (edge)->low = idx1; \
        (edge)->high = idx2; \
    } \
    else \
    { \
        (edge)->low = idx2; \
        (edge)->high = idx1; \
    } \
}

u64 string_hash_function(void *key)
{
    char *k = (char *) key;

    u32 c;
    u64 hash = 5381;
    u32 len = (u32) strlen(k);

    for (u32 i = 0; i < len; ++i)
    {
        c = k[i];
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

b8 string_compare_function(void *key1, void *key2)
{
    char *k1 = (char *) key1;
    char *k2 = (char *) key2;

    if (strcmp(k1, k2) == 0)
        return true;

    return false;
}

u64 edge_hash_function(void *key)
{
    ds_edge_t *edge = (ds_edge_t *) key;
    u32 hash = (edge->low << 8) ^ edge->high;
    return hash;
}

b8 edge_compare_function(void *e1, void *e2)
{
    ds_edge_t *edge1 = (ds_edge_t *) e1;
    ds_edge_t *edge2 = (ds_edge_t *) e2;

    b8 res = edge1->low == edge2->low && edge1->high == edge2->high;

    return res;
}

// NOTE: Blender's edge hashing function but the algorithm is simplified.
#define EDGE_HASH(E) ((E.low << 8) ^ E.high)
#define EDGE_EQUAL(E1, E2) ((E1.low == E2.low) && (E1.high == E2.high))

// ----------------------------------------------------------------------------------
// -- Dynamic array (malloc, realloc).
// ----------------------------------------------------------------------------------

typedef struct
{
    u32 capacity;
    u32 length;
} dynamic_array_header_t;

#define da_create(array, type, capacity) (array) = (type*) _da_create(sizeof(*(array)), (capacity))
#define da_header(array) ((dynamic_array_header_t *) (array) - 1)
#define da_length(array) (da_header(array)->length)
#define da_push(array, type) (da_header(array)->length++, da_needs_expansion(array, type), da_header(array)->length - 1)
#define da_needs_expansion(array, type) ((!(array) || da_header(array)->length == da_header(array)->capacity) ? ((array) = (type*) _da_expand(array, sizeof *(array))) : 0)

void *_da_create(size_t elem_size, u32 capacity)
{
    size_t bytes = sizeof(dynamic_array_header_t) + elem_size * capacity;
    // dynamic_array_header_t *array = (dynamic_array_header_t *) malloc(bytes);
    dynamic_array_header_t *array = (dynamic_array_header_t *) gc_mem_allocate(bytes);
    // memset(array, 0, bytes);

    array->capacity = capacity;
    array->length = 0;

    return (array + 1);
}

void *_da_expand(void *array, size_t elem_size)
{
    dynamic_array_header_t *header = da_header(array);

    u32 new_capacity = header->capacity * 2;
    u32 old_length = header->length;

    size_t bytes = sizeof(dynamic_array_header_t) + elem_size * new_capacity;

    // dynamic_array_header_t *new_array = (dynamic_array_header_t *) realloc(header, bytes);
    dynamic_array_header_t *new_array = (dynamic_array_header_t *) gc_mem_reallocate(header, bytes);
    new_array->capacity = new_capacity;
    new_array->length = old_length;

    return (new_array + 1);
}

void da_delete(void *array)
{}

// ----------------------------------------------------------------------------------
// -- Dynamic array (custom memory allocator).
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
// -- General hashtable.
// ----------------------------------------------------------------------------------

// NOTE(gabic): Trebuie sa rezolv problema asta de la hashtable
#define DS_HASHTABLE_LOAD_FACTOR 0.8f
#define DS_HASHTABLE_FILLRATE(ht) ht->fill_rate = (r32) ht->used / ht->slots

typedef u64 (* hashtable_hash_function_t) (void *data);
typedef b8 (* hashtable_compare_function_t) (void *data1, void *data2);

typedef struct
{
    b8 full;
    void *key;
    void *data;
} ds_hashtable_slot_t;

typedef struct
{
    u32 slots;
    u32 used;
    r32 fill_rate;

    hashtable_hash_function_t hash_function;
    hashtable_compare_function_t compare_function;

    ds_hashtable_slot_t *table;
} ds_hashtable_t;


#define hashtable_get_slot(ht, hash_index, offset) (&ht->table[(hash_index + offset * offset) % ht->slots]);

ds_hashtable_t *hashtable_create(u32 slots)
{
    ds_hashtable_t *ht = 0;
    // r32 load_factor = DS_HASHTABLE_LOAD_FACTOR;
    slots = (u32) (slots / DS_HASHTABLE_LOAD_FACTOR);

    if (slots)
    {
        size_t table_bytes = sizeof(ds_hashtable_t) + sizeof(ds_hashtable_slot_t) * slots;
        ht = (ds_hashtable_t *) gc_mem_allocate(table_bytes);

        #ifdef GCSR_ASSET_BUILDER
            memset(ht, 0, table_bytes);
        #endif

        ht->slots = slots;
        ht->used = 0;
        ht->fill_rate = 0;
        ht->hash_function = 0;
        ht->compare_function = 0;
        ht->table = (ds_hashtable_slot_t *) (ht + 1);
    }

    return ht;
}

void *hashtable_search(ds_hashtable_t *ht, void *key)
{
    if (!ht || !key)
        return 0;

    if (!ht->hash_function || !ht->compare_function)
    {
        printf("[ERROR]: Missing hashtable hash functions !\n");
        return 0;
    }

    void *res = 0;
    b8 done = false;
    u32 offset = 0;
    u32 hash_index = ht->hash_function(key) % ht->slots;

    while (!done)
    {
        u32 current_index = (hash_index + offset) % ht->slots;
        ds_hashtable_slot_t *slot = ht->table + current_index;

        if (offset && current_index == hash_index) {
            return 0;
        }
        else if (!slot->full) {
            done = true;
        }
        else if (slot->key && ht->compare_function(slot->key, key))
        {
            done = true;
            res = slot->data;
        }
        else
            offset++;
    }

    return res;
}

// The data should be stored externally.
b8 hashtable_insert(ds_hashtable_t *ht, void *key, void *data)
{
    if (!ht || !key || !data)
        return false;

    if (!ht->hash_function || !ht->compare_function)
    {
        printf("[ERROR]: Missing hashtable functions !\n");
        return false;
    }

    b8 res = false;
    b8 done = false;
    u32 offset = 0;
    u32 hash_index = ht->hash_function(key) % ht->slots;

    while (!done)
    {
        u32 current_index = (hash_index + offset) % ht->slots;
        ds_hashtable_slot_t *slot = ht->table + current_index;

        if (offset && current_index == hash_index) {
            return 0;
        }
        // Found an empty slot.
        else if (!slot->full)
        {
            slot->full = true;
            slot->key = key;
            slot->data = data;
            ht->used++;
            DS_HASHTABLE_FILLRATE(ht);

            done = true;
            res = true;
        }
        // Data already inserted.
        else if (slot->key && ht->compare_function(slot->key, key)) {
            done = true;
        }
        else
            offset++;
    }

    return res;
}

void hashtable_remove(ds_hashtable_t *ht, void *key)
{
    if (!ht || !key)
        return;

    if (!ht->hash_function || !ht->compare_function)
    {
        printf("[ERROR]: Missing hashtable hash functions !\n");
        return;
    }

    b8 done = false;
    u32 offset = 0;
    u32 hash_index = ht->hash_function(key) % ht->slots;

    while (!done)
    {
        u32 current_index = (hash_index + offset) % ht->slots;
        ds_hashtable_slot_t *slot = ht->table + current_index;

        if (offset && current_index == hash_index) {
            done = true;
        }
        else if (!slot->full) {
            done = true;
        }
        else if (slot->key && ht->compare_function(slot->key, key))
        {
            slot->full = false;
            slot->key = 0;
            slot->data = 0;

            ht->used--;
            DS_HASHTABLE_FILLRATE(ht);

            done = true;
        }
        else
            offset++;
    }
}

// ----------------------------------------------------------------------------------
// -- String tokenizer.
// ----------------------------------------------------------------------------------

typedef struct
{
    char *source;
    char tokens[10][64];
    u8 count;
} ds_string_tokenizer_t;

void string_tokenize(ds_string_tokenizer_t *st, char *source, char separator)
{
    if (source && separator)
    {
        st->source = source;
        u32 start_index = 0;
        u32 current_index = 0;

        while (true)
        {
            char c = *source++;
            current_index++;

            if (c == separator || c == '\0')
            {
                strncpy(st->tokens[st->count++], st->source + start_index, current_index - start_index - 1);
                start_index = current_index;
            }

            if (c == '\0')
                break;
        }
    }
}

void string_tokenizer_reset(ds_string_tokenizer_t *st)
{
    st->source = 0;
    st->count = 0;
}

// ----------------------------------------------------------------------------------
// -- String buffer.
// ----------------------------------------------------------------------------------

typedef struct
{
    u32 count;
    char string[10][255];
} string_buffer_t;

char *sb_next_buffer(string_buffer_t *buffer)
{
    assert(buffer->count < 10);
    return buffer->string[buffer->count++];
}

void sb_reset(string_buffer_t *buffer) {
    buffer->count = 0;
}

#endif