// ---------------------------------------------------------------------------------
// -- File: gcsr_common_routines.cpp
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description:
// -- Created: 2020-06-25 23:04:16
// -- Modified: 2021-12-28 22:12:21
// ---------------------------------------------------------------------------------

extern global_vars_t GCSR;

#define get_memory_manager() GCSR.memory_manager
#define get_engine_memory_pool() &GCSR.core->memory
#define get_input_bindings() &GCSR.core->bindings
#define get_screen_buffer() &GCSR.core->main_window.buffer
#define get_platform_api() &GCSR.core->API
