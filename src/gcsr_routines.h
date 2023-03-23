// ----------------------------------------------------------------------------------
// -- File: gcsr_routines.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-06-08 22:24:53
// -- Modified: 2022-06-08 22:24:55
// ----------------------------------------------------------------------------------

#ifndef GCSR_ROUTINES_H
#define GCSR_ROUTINES_H

// ----------------------------------------------------------------------------------
// -- Memory routines.
// ----------------------------------------------------------------------------------

#define GCSR_MALLOC(bytes) gc_mem_allocate(bytes)

memory_block_t *gc_mem_search_for_empty_block(memory_type_t chunk, size_t size);
void *gc_mem_allocate(size_t size);
void *gc_mem_reallocate(void *data, size_t size);
void gc_mem_merge(memory_type_t type);
void gc_mem_free(void *data);
void gc_mem_clear();

#define mem_current_chunk() (GCSR.memory_manager->alloc_chunk)
#define mem_set_chunk(chunk) \
{ \
    GCSR.memory_manager->old_chunk = GCSR.memory_manager->alloc_chunk; \
    GCSR.memory_manager->alloc_chunk = chunk; \
}
#define mem_restore_chunk() (GCSR.memory_manager->alloc_chunk = GCSR.memory_manager->old_chunk)

#ifndef MEMORY_DEBUG_ENABLED
#define mem_debug_name(name) (GCSR.memory_manager->alloc_name = name)
#else
#define mem_debug_name(name)
#endif

#endif