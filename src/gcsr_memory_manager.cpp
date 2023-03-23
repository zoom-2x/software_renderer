// ---------------------------------------------------------------------------------
// -- File: gcsr_memory_manager.cpp
// ---------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2020-06-27 12:29:28
// -- Modified: 2022-11-16 19:15:29
// ---------------------------------------------------------------------------------

/*
 * Initializez the memory manager and sets the first default empty block of memory.
 */
void gc_mem_initialize_memory_manager(engine_memory_pool_t *memory)
{
    memory_manager_t *manager = &GCSR.state->manager;

    manager->memory = memory;
    manager->total_allocated_bytes = 0;
    manager->alloc_debug_label = DEFAULT_MEM_LABEL;
    // size_t engine_state_size = MEM_SIZE_ALIGN(sizeof(engine_state_t));
    // manager->total_allocated_bytes = engine_state_size;

    // The memory addresses are MEMORY_ALIGN_SIZE aligned.
    memory_chunk_t *permanent_chunk = manager->chunks + MEMORY_PERMANENT;
    memory_chunk_t *temporary_chunk = manager->chunks + MEMORY_TEMPORARY;
    memory_chunk_t *asset_chunk = manager->chunks + MEMORY_ASSETS;

    permanent_chunk->base_address = manager->memory->permanent_pool;
    temporary_chunk->base_address = manager->memory->temporary_pool;
    asset_chunk->base_address = manager->memory->asset_pool;

    // size_t mem_markers_size = MEM_SIZE_ALIGN(sizeof(memory_markers_t));
    size_t state_bytes = MEM_SIZE_ALIGN(sizeof(engine_state_t));

    memory_block_t *permanent_start_block = (memory_block_t *) ADDR_OFFSET(permanent_chunk->base_address, state_bytes);
    memory_block_t *temporary_start_block = (memory_block_t *) temporary_chunk->base_address;
    memory_block_t *asset_start_block = (memory_block_t *) asset_chunk->base_address;

    // ----------------------------------------------------------------------------------
    // -- Permanent memory pool.
    // ----------------------------------------------------------------------------------

    permanent_start_block->bytes = memory->permanent_pool_size - state_bytes - MEMORY_BLOCK_OVERHEAD;
    permanent_start_block->location = MEMORY_PERMANENT;
    permanent_start_block->status = BLOCK_FREE;
    permanent_start_block->bcc = BLOCK_CORRUPTION_CHECK;
    permanent_start_block->Next = 0;
    permanent_start_block->Prev = 0;
    permanent_start_block->label = DEFAULT_MEM_LABEL;
    permanent_start_block->description = 0;

    permanent_chunk->base_address = manager->memory->permanent_pool;
    permanent_chunk->overhead = MEMORY_BLOCK_OVERHEAD;
    permanent_chunk->total_bytes = memory->permanent_pool_size;
    permanent_chunk->allocated_bytes = state_bytes;
    permanent_chunk->filled_blocks = 0;
    permanent_chunk->block_count = 1;
    permanent_chunk->empty_blocks = 1;
    permanent_chunk->Head = permanent_start_block;

    // ----------------------------------------------------------------------------------
    // -- Temporary memory pool.
    // ----------------------------------------------------------------------------------

    temporary_start_block->bytes = memory->temporary_pool_size - MEMORY_BLOCK_OVERHEAD;
    temporary_start_block->location = MEMORY_TEMPORARY;
    temporary_start_block->status = BLOCK_FREE;
    temporary_start_block->bcc = BLOCK_CORRUPTION_CHECK;
    temporary_start_block->Next = 0;
    temporary_start_block->Prev = 0;
    temporary_start_block->label = DEFAULT_MEM_LABEL;
    temporary_start_block->description = 0;

    temporary_chunk->base_address = manager->memory->temporary_pool;
    temporary_chunk->overhead = MEMORY_BLOCK_OVERHEAD;
    temporary_chunk->total_bytes = memory->temporary_pool_size;
    temporary_chunk->allocated_bytes = 0;
    temporary_chunk->filled_blocks = 0;
    temporary_chunk->block_count = 1;
    temporary_chunk->empty_blocks = 1;
    temporary_chunk->Head = temporary_start_block;

    // ----------------------------------------------------------------------------------
    // -- Asset memory pool.
    // ----------------------------------------------------------------------------------

    asset_start_block->bytes = memory->asset_pool_size - MEMORY_BLOCK_OVERHEAD;
    asset_start_block->location = MEMORY_ASSETS;
    asset_start_block->status = BLOCK_FREE;
    asset_start_block->bcc = BLOCK_CORRUPTION_CHECK;
    asset_start_block->Next = 0;
    asset_start_block->Prev = 0;
    asset_start_block->label = DEFAULT_MEM_LABEL;
    asset_start_block->description = 0;

    asset_chunk->base_address = manager->memory->asset_pool;
    asset_chunk->overhead = MEMORY_BLOCK_OVERHEAD;
    asset_chunk->total_bytes = memory->asset_pool_size;
    asset_chunk->allocated_bytes = 0;
    asset_chunk->filled_blocks = 0;
    asset_chunk->block_count = 1;
    asset_chunk->empty_blocks = 1;
    asset_chunk->Head = asset_start_block;

    // ----------------------------------------------------------------------------------
    // -- Pipeline memory.
    // ----------------------------------------------------------------------------------

    // size_t bytes_per_thread = memory->pipeline_pool_size / GC_PIPE_NUM_THREADS;
    u8 *pipeline_memory_pointer = (u8 *) memory->pipeline_pool;

    for (u8 i = 0; i < GC_PIPE_NUM_THREADS; ++i)
    {
        pipe_memory_t *pmemory = &manager->pipeline[i];

        pmemory->data = pipeline_memory_pointer + i * MEM_THREAD_BUFFER_SIZE;
        pmemory->cursor = 0;
        pmemory->bytes = MEM_THREAD_BUFFER_SIZE;
    }
}

/*
 * Searches for an empty block of memory that can hold the specified data size.
 * When searching for memory space the size of the memory_block_t overhead is taken
 * into account.
 */
memory_block_t *gc_mem_search_for_empty_block(memory_type_t chunk, size_t memsize)
{
    memory_manager_t *manager = get_memory_manager();
    memory_chunk_t *base_chunk = &manager->chunks[chunk];

    size_t size = MEM_SIZE_ALIGN(memsize);

    memory_block_t *gc_mem_free_block = 0;
    memory_block_t *next_block = base_chunk->Head;

    // size_t clear_bytes = size;
    // size_t total_block_bytes = sizeof(memory_block_t) + size;
    u32 block_index = 0;

    while (next_block && block_index < base_chunk->block_count)
    {
        // -- A free block big enough to hold the data was found.

        SDL_assert(next_block->bcc == BLOCK_CORRUPTION_CHECK);

        if (next_block->status == BLOCK_FREE && next_block->bytes >= size)
        {
            gc_mem_free_block = next_block;
            size_t diff_bytes = next_block->bytes - size;

            // -- Split the remaining block space as a new empty block.

            if (diff_bytes >= MIN_BLOCK_BYTES)
            {
                memory_block_t *remaining_block = (memory_block_t *) ADDR_OFFSET(gc_mem_free_block, MEMORY_BLOCK_OVERHEAD + size);

                // SET_BLOCK_DEBUG_MARKER(remaining_block);
                remaining_block->bcc = BLOCK_CORRUPTION_CHECK;
                remaining_block->location = chunk;
                remaining_block->status = BLOCK_FREE;
                remaining_block->bytes = diff_bytes - MEMORY_BLOCK_OVERHEAD;
                remaining_block->label = DEFAULT_MEM_LABEL;
                remaining_block->description = 0;
                // remaining_block->data_bytes = remaining_block->total_bytes - sizeof(memory_block_t);
                // DBG_SetDataType(remaining_block, DATA_NONE);

                base_chunk->block_count++;
                base_chunk->empty_blocks++;
                base_chunk->overhead += MEMORY_BLOCK_OVERHEAD;

                // -- The allocation was done from the end of the empty chunk.

                if (!gc_mem_free_block->Next)
                    remaining_block->Next = 0;
                else
                    remaining_block->Next = gc_mem_free_block->Next;

                gc_mem_free_block->Next = remaining_block;
                remaining_block->Prev = gc_mem_free_block;
            }

            break;
        }
        else
            next_block = next_block->Next;

        block_index++;
    }

    // -- Out of memory, maybe do something specific when this happens.
    SDL_assert(gc_mem_free_block);

    if (gc_mem_free_block)
    {
        // SET_BLOCK_DEBUG_MARKER(gc_mem_free_block);

        gc_mem_free_block->bytes = size;
        gc_mem_free_block->location = chunk;
        gc_mem_free_block->status = BLOCK_USED;

        base_chunk->allocated_bytes += size;
        base_chunk->filled_blocks++;
        base_chunk->empty_blocks--;
    }

    return gc_mem_free_block;
}

void *gc_mem_allocate(size_t size)
{
    memory_manager_t *manager = GCSR.memory_manager;
    memory_type_t chunk = manager->alloc_chunk;

    void *Result = 0;

    memory_block_t *gc_mem_free_block = gc_mem_search_for_empty_block(chunk, size);

    // The debug name is used only for one allocation block.
    if (manager->alloc_debug_label)
    {
        gc_mem_free_block->label = manager->alloc_debug_label;
        manager->alloc_debug_label = DEFAULT_MEM_LABEL;
    }

    if (manager->alloc_debug_description)
    {
        gc_mem_free_block->description = manager->alloc_debug_description;
        manager->alloc_debug_description = 0;
    }

    platform_api_t *API = get_platform_api();
    API->mem_clear(gc_mem_free_block + 1, gc_mem_free_block->bytes);
    manager->total_allocated_bytes += gc_mem_free_block->bytes;

    Result = gc_mem_free_block + 1;

    return Result;
}

void *gc_mem_reallocate(void *data, size_t size)
{
    // memory_manager_t *manager = GCSR.memory_manager;
    // memory_type_t chunk = manager->alloc_chunk;
    platform_api_t *API = get_platform_api();

    void *res = 0;

    if (data)
    {
        memory_block_t *old_block = GET_BLOCK(data);

        mem_set_chunk(old_block->location);
        res = gc_mem_allocate(size);
        mem_restore_chunk();

        memory_block_t *new_block = GET_BLOCK(res);

        if (old_block->label)
            new_block->label = old_block->label;

        if (old_block->description)
            new_block->description = old_block->description;

        API->mem_copy(res, data, size, old_block->bytes);
        gc_mem_free(data);
    }

    return res;
}

// NOTE(gabic): ar merge facut astfel incat merge-ul sa se faca o singura
// data pe frame, pentru a limita numarul de apelari. Trebuie verificat
// mai tarziu cat de eficient ar fi.

void gc_mem_merge(memory_type_t type)
{
    memory_chunk_t *base_chunk = GET_CHUNK(type);
    memory_block_t *Current = base_chunk->Head;

    while (Current)
    {
        if (Current->status == BLOCK_FREE)
        {
            size_t merge_total_bytes = Current->bytes;

            // -- Check for forward adjacent blocks.

            memory_block_t *CurrentMerge = Current->Next;

            while (CurrentMerge && CurrentMerge->status == BLOCK_FREE)
            {
                // Add the block header cells also.
                merge_total_bytes += CurrentMerge->bytes + MEMORY_BLOCK_OVERHEAD;

                Current->Next = CurrentMerge->Next;
                base_chunk->empty_blocks--;
                base_chunk->block_count--;
                base_chunk->overhead -= MEMORY_BLOCK_OVERHEAD;

                CurrentMerge = CurrentMerge->Next;
            }

            Current->bytes = merge_total_bytes;
            // Current->data_bytes = Current->total_bytes - sizeof(memory_block_t);

            Current = CurrentMerge;
        }
        else
            Current = Current->Next;
    }
}

// TODO(gabic): Functia trebuie scoasa.
__INLINE__ void gc_mem_free_block(memory_block_t *block)
{
    memory_manager_t *manager = GCSR.memory_manager;
    memory_chunk_t *base_chunk = GET_BLOCK_CHUNK(block);

    block->status = BLOCK_FREE;
    block->bcc = BLOCK_CORRUPTION_CHECK;
    block->label = DEFAULT_MEM_LABEL;
    block->description = 0;

    base_chunk->empty_blocks++;
    base_chunk->filled_blocks--;
    base_chunk->allocated_bytes -= block->bytes;
    manager->total_allocated_bytes -= block->bytes;
}

// ----------------------------------------------------------------------------------
// -- For use when multiple blocks of memory need to be freed at once. The merge
// -- routine will be called once at the end (not tested).
// ----------------------------------------------------------------------------------

static b32 __markForFree = false;
static memory_type_t __markLocation = MEMORY_TEMPORARY;

#define mem_start_mark_free(location) \
{ \
    __markForFree = true; \
    __markLocation = location; \
}

#define mem_end_mark_free() \
{ \
    __markForFree = false; \
 \
    gc_mem_merge(__markLocation); \
}

/*
 * Clears the memory block for the specified variable pointer.
 *
 * TODO(gabic): un mod prin care sa verific daca pointerul primit
 * face parte dintr-un block de memorie.
 */
void gc_mem_free(void *data)
{
    if (!data)
        return;

    memory_manager_t *manager = GCSR.memory_manager;
    memory_block_t *block_to_free = GET_BLOCK(data);
    memory_chunk_t *base_chunk = GET_BLOCK_CHUNK(block_to_free);

    if (base_chunk->empty_blocks == 1 && base_chunk->filled_blocks == 0)
        return;

    block_to_free->status = BLOCK_FREE;
    block_to_free->bcc = BLOCK_CORRUPTION_CHECK;
    block_to_free->label = DEFAULT_MEM_LABEL;
    block_to_free->description = 0;

    base_chunk->empty_blocks++;
    base_chunk->filled_blocks--;
    base_chunk->allocated_bytes -= block_to_free->bytes;
    manager->total_allocated_bytes -= block_to_free->bytes;

    if (!__markForFree)
        gc_mem_merge(__markLocation);
}

// Stack allocations will be aligned.
memory_stack_t *create_stack(size_t size)
{
    memory_stack_t *stack = 0;

    size_t stack_bytes = sizeof(memory_stack_t) + MEM_SIZE_ALIGN(size);
    mem_set_chunk(MEMORY_TEMPORARY);
    MEM_LABEL("temporary_stack");
    stack = (memory_stack_t *) gc_mem_allocate(stack_bytes);
    mem_restore_chunk();

    stack->base = stack + 1;
    stack->pointer = 0;
    stack->size = stack_bytes;

    return stack;
}

#define stack_mark(stack) stack->pointer
#define stack_return(stack, mark) stack->pointer = mark

void *stack_push(memory_stack_t *stack, size_t size)
{
    void *address = 0;
    size_t push_bytes = MEM_SIZE_ALIGN(size);

    address = ADDR_OFFSET(stack->base, stack->pointer);
    stack->pointer += push_bytes;
    SDL_assert(stack->pointer < stack->size);

    return address;
}

#define delete_stack(stack) gc_mem_free(stack)

/*
 * Clears the entire temporary memory chunk.
 */
// void gc_mem_clear()
// {
//     memory_manager_t *manager = get_memory_manager();

//     memory_chunk_t *base_chunk = &manager->chunks[MEMORY_TEMPORARY];
//     memory_block_t *Block = (memory_block_t *) base_chunk->base_address;

//     base_chunk->block_count = 1;
//     base_chunk->empty_blocks = 1;
//     base_chunk->filled_blocks = 0;
//     base_chunk->allocated_bytes = 0;
//     base_chunk->Head = Block;

//     Block->total_bytes = base_chunk->total_bytes;
//     Block->data_bytes = Block->total_bytes - sizeof(memory_block_t);
//     Block->location = MEMORY_TEMPORARY;
//     Block->status = BLOCK_FREE;
//     Block->bcc = BLOCK_CORRUPTION_CHECK;
//     Block->Next = 0;
//     Block->Prev = 0;
// }

// NOTE(gabic): o alta idee ar fi ca marker-ele sa nu fie sterse la mem_markStop
// ci la apelarea unei alte metode, in felul asta pot sa fiu ceva mai selectiv
// la ce alocari se face stergerea.

/*
 * Start to record the temporary memory allocations. All the blocks
 * allocated following the call mem_markStart() will be deallocated
 * when mem_markStop() is called.
 */
// void mem_markStart()
// {
//     memory_chunk_t *base_chunk = GET_CHUNK(MEMORY_TEMPORARY);
//     memory_markers_t *Markers = (memory_markers_t *) base_chunk->base_address;

//     Markers->isOn = true;
// }

// #define mem_markStoreVar(var) mem_markStore(GET_BLOCK(var))
// void mem_markStore(memory_block_t *Block)
// {
//     memory_chunk_t *base_chunk = GET_CHUNK(MEMORY_TEMPORARY);
//     memory_markers_t *Markers = (memory_markers_t *) base_chunk->base_address;

//     if (!Markers->isOn)
//         return;

//     if (Markers->count < MAX_MEMORY_BLOCKS_TO_REMEMBER)
//         Markers->Blocks[Markers->count++] = Block;
// }

/*
 * Resets all the recorder temporary memory allocations.
 */
// void mem_markStop()
// {
//     memory_chunk_t *base_chunk = GET_CHUNK(MEMORY_TEMPORARY);
//     memory_markers_t *Markers = (memory_markers_t *) base_chunk->base_address;

//     if (base_chunk->empty_blocks == 1 && base_chunk->filled_blocks == 0)
//         return;

//     // -- Clear the markers.

//     for (u32 i = 0; i < Markers->count; ++i)
//     {
//         memory_block_t *Block = Markers->Blocks[i];
//         // size_t freedBytes = Block->total_bytes;
//         gc_mem_free_block(Block);
//     }

//     gc_mem_merge(MEMORY_TEMPORARY);

//     Markers->isOn = false;
//     Markers->count = 0;
// }