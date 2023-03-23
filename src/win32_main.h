// ---------------------------------------------------------------------------------
// -- File: win32_main.h
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description:
// -- Created:
// -- Modified:
// ---------------------------------------------------------------------------------

#ifndef WIN32_MAIN_H
#define WIN32_MAIN_H

// -- Win32 HMODULE wrapper.
typedef struct
{
    HMODULE handler;
} win32_engine_code_t;

void win32_initialize_file_system(engine_file_system_t *file_system);
void win32_load_engine_code(engine_code_t *engine);
void win32_unload_engine_code(engine_code_t *engine);
void win32_check_for_engine_code_update();

void *win32_mem_alloc(size_t size);
b32 win32_mem_free(void *address);

#endif
