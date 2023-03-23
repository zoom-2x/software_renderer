// ----------------------------------------------------------------------------------
// -- File: gcsr_file_operations.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C
// -- Description:
// -- Created: 2022-04-26 20:27:06
// -- Modified: 2022-06-11 11:03:02
// ----------------------------------------------------------------------------------

#ifndef GCSR_FILE_INTERFACE_H
#define GCSR_FILE_INTERFACE_H

#include "gcsr_shared.h"
#define FILE_MAX_PATH 255

typedef enum
{
    GC_FILE_READ = 1,
    GC_FILE_WRITE = 2,
    GC_FILE_APPEND_DATA = 4
} file_access_t;

typedef struct
{
    u32 year;
    u32 month;
    u32 dayOfWeek;
    u32 day;
    u32 hour;
    u32 minute;
    u32 second;
    u32 milliseconds;
} user_time_t;

typedef struct
{
    b32 valid;
    void *handle;
} file_handle_t;

typedef struct
{
    large_number_t CreationTime;
    large_number_t LastAccessTime;
    large_number_t last_write_time;
    size_t Size;
    char filename[FILE_MAX_PATH];
} file_attributes_t;

typedef struct
{
    char *name;
    size_t bytes;
    void *handle;
    u64 write_offset;
} gc_file_t;

b32 file_exists(char *filepath);
u64 get_last_write_time(char *filepath);
u64 file_size(gc_file_t *file);
void open_file(gc_file_t *file, char *filepath, file_access_t access);
void read_file(gc_file_t *file, u64 offset, u64 bytes_to_read, void *dest_buffer);
void write_file(gc_file_t *file, u64 byte_count, void *source);
void close_file(gc_file_t *file);
void *find_first_file(char *path, file_attributes_t *attrs);
b8 find_next_file(void *handle, file_attributes_t *attrs);

#endif