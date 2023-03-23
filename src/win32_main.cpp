// ---------------------------------------------------------------------------------
// -- File: win32_main.cpp
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description: Application entry point.
// -- Created:
// -- Modified:
// ---------------------------------------------------------------------------------

#include <stdio.h>
// #define SDL_MAIN_HANDLED

#include <windows.h>

#include "gcsr_settings.h"
#include "gcsr_macros.h"
#include "gcsr_engine.h"
#include "win32_main.h"

#if GC_PLATFORM == GC_WIN32
#include <windows.h>
#include "platform/win32/gcsr_file_impl.cpp"
#elif GC_PLATFORM == GC_LINUX
#include "platform/linux/gcsr_file_impl.cpp"
#endif

#include "win32_common.cpp"
// #include "win32_file.cpp"
#include "win32_engine.cpp"

extern engine_core_t gcsr_engine;

void win32_get_exe_filename(char *filePath, u32 pathSize, char *filename, u32 filenameSize)
{
    u32 nameSize = GetModuleFileNameA(0, filePath, (DWORD) pathSize);

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Win32_GetExeFilepath: Destination buffer is smaller than the filePath size: %d < %d\n",
                        pathSize, nameSize);
    }
    else
    {
        char *cursor = filePath;
        char *pastSlash = cursor;

        // -- Search for slashes.

        while (*cursor)
        {
            if (*cursor == '\\')
                pastSlash = cursor + 1;

            cursor++;
        }

        // -- Copy the file name.

        u32 copiedBytes = 0;
        char *pathLastByte = pastSlash;

        while (*pastSlash && copiedBytes < filenameSize)
        {
            copiedBytes++;
            *filename++ = *pastSlash++;
        }

        *filename = '\0';
        u32 lastIndex = (u32) (pathLastByte - filePath);
        filePath[lastIndex] = '\0';
    }

    int a = 1;

    a++;
}

// ---------------------------------------------------------------------------------
// -- memory management routines.
// ---------------------------------------------------------------------------------

void *win32_mem_alloc(size_t size) {
    return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void win32_mem_copy(void *dest, void *src, size_t dest_size, size_t bytes_to_copy)
{
    size_t size = bytes_to_copy > dest_size ? dest_size : bytes_to_copy;
    CopyMemory(dest, src, size);
}

b32 win32_mem_free(void *address) {
    return VirtualFree(address, 0, MEM_RELEASE);
}

void win32_mem_clear(void *address, size_t length) {
    SecureZeroMemory(address, length);
}

void win32_mem_set(void *address, u8 val, size_t count) {
    memset(address, val, count);
}

internal win32_engine_code_t dll_handler;

void win32_initialize_file_system(engine_file_system_t *file_system)
{
    if (file_system)
    {
        strncpy_s(file_system->platform_library_name, MAX_STRING_LENGTH, "gcsr_engine_core.dll", _TRUNCATE);
        strncpy_s(file_system->platform_library_tmp_name, MAX_STRING_LENGTH, "gcsr_engine_core_tmp.dll", _TRUNCATE);
        strncpy_s(file_system->lock_filename, MAX_STRING_LENGTH, "lock.tmp", _TRUNCATE);

        win32_get_exe_filename(file_system->exe_directory, MAX_STRING_LENGTH, file_system->exe_filename, MAX_STRING_LENGTH);

        // size_t ExeDirectoryLen = strLen(file_system->exe_directory, MAX_STRING_LENGTH);
        // size_t ExeFilenameLen = strLen(file_system->exe_filename, MAX_STRING_LENGTH);

        strncpy_s(file_system->game_code_library_path, MAX_STRING_LENGTH, file_system->exe_directory, _TRUNCATE);
        strncat_s(file_system->game_code_library_path, MAX_STRING_LENGTH, file_system->platform_library_name, _TRUNCATE);

        strncpy_s(file_system->gamecode_library_tmp_path, MAX_STRING_LENGTH, file_system->exe_directory, _TRUNCATE);
        strncat_s(file_system->gamecode_library_tmp_path, MAX_STRING_LENGTH, file_system->platform_library_tmp_name, _TRUNCATE);

        strncpy_s(file_system->lock_filepath, MAX_STRING_LENGTH, file_system->exe_directory, _TRUNCATE);
        strncat_s(file_system->lock_filepath, MAX_STRING_LENGTH, file_system->lock_filename, _TRUNCATE);
    }
}

void win32_load_engine_code(engine_code_t *Code)
{
    engine_file_system_t *file_system = &Code->file_system;
    BOOL res = CopyFile(file_system->game_code_library_path, file_system->gamecode_library_tmp_path, FALSE);
    u32 errcode = GetLastError();

    if (errcode)
        SDL_Log("Copy error code %u", errcode);

    if (res)
    {
        dll_handler.handler = LoadLibraryA(file_system->gamecode_library_tmp_path);

        if (dll_handler.handler)
        {
            Code->last_write_time = get_last_write_time(file_system->game_code_library_path);

            Code->handler = (void *) &dll_handler;
            get_engine_api_t get_engine_api = (get_engine_api_t) GetProcAddress(dll_handler.handler, "get_engine_api");

            Code->API = get_engine_api(&gcsr_engine);

            if (Code->API)
                Code->is_valid = true;
        }
    }
}

void win32_unload_engine_code(engine_code_t *Engine)
{
    if (Engine->handler)
    {
        win32_engine_code_t *handler = (win32_engine_code_t *) Engine->handler;
        FreeLibrary(handler->handler);
        handler->handler = 0;
    }

    Engine->handler = 0;
    Engine->API = 0;
    Engine->is_valid = false;
}

void Win32_Yield() {
    Sleep(0);
}

void Win32_InitializeBackbuffer()
{}

void win32_initialize_api()
{
    platform_api_t *API = &gcsr_engine.API;

    API->yield = Win32_Yield;

    // API->get_wall_clock = engine_get_wall_clock;
    // API->get_seconds_elapsed = engine_get_seconds_elapsed;
    // API->get_ms_elapsed = engine_get_ms_elapsed;
    API->switch_resolution = switch_resolution;
    API->update_engine = update_engine;

    API->mem_allocate = win32_mem_alloc;
    API->mem_copy = win32_mem_copy;
    API->mem_free = win32_mem_free;
    API->mem_clear = win32_mem_clear;
    API->mem_set = win32_mem_set;

    API->file_exists = file_exists;
    API->get_last_write_time = get_last_write_time;
    API->file_size = file_size;
    API->open_file = open_file;
    API->read_file = read_file;
    API->write_file = write_file;
    API->close_file = close_file;
    API->find_first_file = find_first_file;
    API->find_next_file = find_next_file;

    // API->get_exe_filename = win32_get_exe_filename;
    // API->get_cwd = win32_get_cwd;
    // API->get_file_attributes = win32_get_file_attributes;
    // API->get_last_write_time = win32_get_last_write_time;
    // API->get_readable_time = win32_get_readable_time;
    // API->file_exists = win32_file_exists;
    // API->open_file = win32_open_file;
    // API->file_size = win32_file_size;
    // API->read_file = win32_read_from_file;
    // API->write_to_file = win32_write_to_file;
    // API->close_file = win32_close_file;
}

// ---------------------------------------------------------------------------------
// -- Entry point.
// ---------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    // engine_memory_config_t memory;
    // engine_resolution_config_t resolution;

    // memory.permanent = Megabytes(256);
    // memory.temporary = Megabytes(128);
    // memory.assets = Megabytes(128);

    // resolution.r_width = 1280;
    // resolution.r_height = 800;
    // resolution.fps = 300;
    // resolution.fps = 30;

    // resolution.r_width = 1600;
    // resolution.r_width = 600;
    // resolution.r_height = 480;
    // resolution.fps = 20;

    win32_initialize_memory();
    // win32_initialize_main_window(&resolution);
    win32_initialize_api();
    win32_run();

    return 0;
}