// ----------------------------------------------------------------------------------
// -- File: gcsr_builder_memory_manager.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-09-17 13:19:47
// -- Modified: 2022-09-17 13:19:48
// ----------------------------------------------------------------------------------

#ifndef GCSR_BUILDER_MEMORY_MANAGER_H
#define GCSR_BUILDER_MEMORY_MANAGER_H

typedef struct
{
    size_t bytes;
    size_t used_bytes;
    size_t chunk_bytes;
} memory_block_t;

memory_block_t *b_alloc(size_t chunk_bytes, u32 chunk_count)
{
    size_t bytes = sizeof(memory_block_t) + chunk_count * chunk_bytes;
    memory_block_t *memory = (memory_block_t *) malloc(bytes);
    memset(memory, 0, bytes);

    memory->bytes = chunk_bytes;
    memory->used_bytes = 0;
    memory->chunk_bytes = chunk_bytes;

    return memory;
}

memory_block_t *b_realloc(memory_block_t *block, size_t new_bytes)
{
    size_t used_bytes = block->used_bytes;
    memory_block_t *memory = (memory_block_t *) realloc(block, sizeof(memory_block_t) + new_bytes);
    memory->bytes = new_bytes;

    void *clear_memory = ADDR_OFFSET(memory, sizeof(memory_block_t) + used_bytes);
    memset(clear_memory, 0, new_bytes - used_bytes);

    return memory;
}

void b_free(memory_block_t *block) {
    free(block);
}

void b_free_data(void *data) {
    b_free((memory_block_t *) data - 1);
}

size_t b_push(memory_block_t **block, size_t bytes)
{
    memory_block_t *base = *block;

    // Within the memory block boundary.
    if (base->used_bytes + bytes <= base->bytes)
    {
        memory = ADDR_OFFSET(base, sizeof(memory_block_t) + base->used_bytes);
        base->used_bytes += bytes;
    }
    // Reallocation is needed.
    else
    {
        // Find the right chunk bytes extensions needed to hold the pushed size.

        u32 chunk_count = 0;

        while ((chunk_count * base->chunk_bytes) <= bytes) {
            chunk_count++;
        }

        // Double the new size.
        size_t new_size = 2 * chunk_count * base->chunk_bytes;

        *block = b_realloc(base, new_size);
        base = *block;

        memory = ADDR_OFFSET(base, sizeof(memory_block_t) + base->used_bytes);
        base->used_bytes += bytes;
    }

    return memory;
}

#endif