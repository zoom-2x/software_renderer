// ---------------------------------------------------------------------------------
// -- File: gcsr_memory.h
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description:
// -- Created: 2020-06-27 12:15:45
// -- Modified:
// ---------------------------------------------------------------------------------

#ifndef GCSR_MEMORY_H
#define GCSR_MEMORY_H

#define MEMORY_DEBUG_ENABLED
#define DEFAULT_MEM_LABEL "MEMORY_BLOCK"

typedef enum
{
    MEMORY_PERMANENT,
    MEMORY_TEMPORARY,
    MEMORY_ASSETS,

    MEMORY_COUNT,
} memory_type_t;

typedef enum
{
    BLOCK_FREE,         // the block can be allocated.
    BLOCK_STATIC,       // not used for now.
    BLOCK_TEMP,         // not used for now.
    BLOCK_RESERVED,     // the block is reserved and cannot be allocated.
    BLOCK_USED,         // the block is in use and cannot be allocated.
    BLOCK_EXTENSION,    // the block is an extension for a datatype, it will be
                        // deallocated when the respective datatype is released.

    BLOCK_COUNT,
} memory_block_status_t;

#define MEM_LABEL(label) GCSR.memory_manager->alloc_debug_label = label
#define MEM_DESCRIPTION(description) GCSR.memory_manager->alloc_debug_description = description

// ---------------------------------------------------------------------------------
// -- Memory block structures.
// ---------------------------------------------------------------------------------

// -- General block info to keep track of the main memory pool allocation.

typedef struct memory_block_s memory_block_t;

// #define BYTES_TO_CELLS(size) (MEM_SIZE_ALIGN(size) >> MEMORY_CELL_SHIFT)
// #define CELLS_TO_BYTES(cells) (cells << MEMORY_CELL_SHIFT)
// #define MEMORY_BLOCK_OVERHEAD BYTES_TO_CELLS(sizeof(memory_block_t))

__ALIGN__ struct memory_block_s
{
    u32 bcc;

    #ifdef GC_DEBUG_MODE
    char cod[7];
    #endif

    const char *label;
    const char *description;
    size_t bytes;

    memory_type_t location;
    memory_block_status_t status;

    memory_block_t *Next;
    memory_block_t *Prev;
};

typedef struct
{
    size_t overhead;
    size_t total_bytes;
    size_t allocated_bytes;

    u32 block_count;
    u32 empty_blocks;
    u32 filled_blocks;

    memory_block_t *Head;

    void *base_address;
} memory_chunk_t;

typedef struct
{
    void *data;
    size_t cursor;
    size_t bytes;
} pipe_memory_t;

typedef __ALIGN__ struct
{
    void *base;
    size_t pointer;
    size_t size;
} memory_stack_t;

typedef struct
{
    size_t total_allocated_bytes;

    memory_type_t alloc_chunk;
    memory_type_t old_chunk;

    const char *alloc_debug_label;
    const char *alloc_debug_description;

    // TODO(gabic): poate ar trebui sa unific asta cu engine_memory_pool_t + memory_chunk_t (contin aceleasi date)
    engine_memory_pool_t *memory;
    memory_chunk_t chunks[3];
    pipe_memory_t pipeline[GC_PIPE_NUM_THREADS];
    memory_stack_t *stack;
} memory_manager_t;

typedef struct
{
    b32 isOn;
    u32 count;
    memory_block_t *Blocks[MAX_MEMORY_BLOCKS_TO_REMEMBER];
} memory_markers_t;

#if GC_DEBUG_MODE
#define SET_BLOCK_DEBUG_MARKER(Block) _setBlockMarker(Block)
__INLINE__ void _setBlockMarker(memory_block_t *Block)
{
    Block->cod[0] = '<';
    Block->cod[1] = 'b';
    Block->cod[2] = 'l';
    Block->cod[3] = 'o';
    Block->cod[4] = 'c';
    Block->cod[5] = 'k';
    Block->cod[6] = '>';
}
#else
#define SET_BLOCK_DEBUG_MARKER(Block)
#endif

void mem_markStart();
void mem_markStore(memory_block_t *Block);
void mem_markStop();

#endif