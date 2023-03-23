// ----------------------------------------------------------------------------------
// -- File: gcsr_memory_interface.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-08-24 11:15:11
// -- Modified:
// ----------------------------------------------------------------------------------

void *api_mem_allocate(size_t size);
void api_mem_copy(void *dest, void *src, size_t dest_size, size_t bytes_to_copy);
void api_mem_free(void *addr);
void api_mem_clear(void *addr, size_t len);
void api_mem_set(void *addr, u8 val, size_t count);