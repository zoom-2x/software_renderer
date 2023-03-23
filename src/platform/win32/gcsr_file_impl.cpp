// ----------------------------------------------------------------------------------
// -- File: gcsr_file_impl.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-08-24 11:31:29
// -- Modified:
// ----------------------------------------------------------------------------------

void win32_log_last_error()
{
    DWORD error = GetLastError();

    if (error)
    {
        char err[255];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, 0, err, 255, 0);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, err);
    }
}

b32 file_exists(char *filepath)
{
    b32 result = true;
    WIN32_FILE_ATTRIBUTE_DATA Ignored;

    if(!GetFileAttributesEx(filepath, GetFileExInfoStandard, &Ignored))
        result = false;

    return result;
}

u64 get_last_write_time(char *filepath)
{
    u64 result = 0;
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (GetFileAttributesEx(filepath, GetFileExInfoStandard, &data))
    {
        result = ((u64) data.ftLastWriteTime.dwHighDateTime << 32) | data.ftLastWriteTime.dwLowDateTime;
        // result.lowPart = data.ftLastWriteTime.dwLowDateTime;
        // result.highPart = data.ftLastWriteTime.dwHighDateTime;
    }

    return result;
}

u64 file_size(gc_file_t *file)
{
    u64 size = 0;

    if (file->handle)
    {
        LARGE_INTEGER tmp;
        BOOL status = GetFileSizeEx(file->handle, &tmp);

        if (status)
            size = ((u64) tmp.HighPart << 32) | tmp.LowPart;
    }

    return size;
}

void open_file(gc_file_t *file, char *filepath, file_access_t access)
{
    DWORD desiredAccess = GENERIC_READ;
    DWORD dwCreationDisposition = OPEN_ALWAYS;
    // DWORD sharedMode = FILE_SHARE_READ;

    if (access == GC_FILE_WRITE)
    {
        desiredAccess = GENERIC_WRITE;

        if (file_exists(filepath))
            dwCreationDisposition = TRUNCATE_EXISTING;
    }
    else if (access == (GC_FILE_READ | GC_FILE_WRITE))
        desiredAccess = GENERIC_READ | GENERIC_WRITE;

    HANDLE _handle = CreateFileA(filepath, desiredAccess, 0, 0, dwCreationDisposition, 0, 0);

    file->name = filepath;
    file->handle = 0;
    file->bytes = 0;
    file->write_offset = 0;

    if (_handle == INVALID_HANDLE_VALUE) {
        win32_log_last_error();
    }
    else
    {
        file->name = filepath;
        file->handle = _handle;
        file->bytes = file_size(file);
    }
}

void read_file(gc_file_t *file, u64 offset, u64 bytes_to_read, void *dest_buffer)
{
    if (file->handle)
    {
        DWORD bytesRead = 0;
        OVERLAPPED Overlapped;

        Overlapped.Internal = 0;
        Overlapped.InternalHigh = 0;
        Overlapped.hEvent = 0;

        // For 64 bit ?
        Overlapped.Offset = (u32) (offset & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32) ((offset >> 32) & 0xFFFFFFFF);

        b32 result = ReadFile(file->handle, dest_buffer, bytes_to_read, &bytesRead, &Overlapped);

        if (!result)
            win32_log_last_error();
    }
}

void write_file(gc_file_t *file, u64 byte_count, void *source)
{
    if (file->handle)
    {
        DWORD bytesWritten = 0;
        OVERLAPPED Overlapped;

        Overlapped.Internal = 0;
        Overlapped.InternalHigh = 0;
        Overlapped.hEvent = 0;

        // For 64 bit ?
        Overlapped.Offset = (u32) (file->write_offset & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32) ((file->write_offset >> 32) & 0xFFFFFFFF);

        b32 result = WriteFile(file->handle, source, byte_count, &bytesWritten, &Overlapped);
        file->write_offset += byte_count;

        if (!result)
            win32_log_last_error();
    }
}

void close_file(gc_file_t *file)
{
    if (file->handle)
    {
        b32 result = CloseHandle(file->handle);

        if (!result) {
            win32_log_last_error();
        }
        else
        {
            file->name = 0;
            file->bytes = 0;
            file->handle = 0;
        }
    }
}

void _init_attributes(WIN32_FIND_DATA *from, file_attributes_t *to)
{
    to->CreationTime.lowPart = from->ftCreationTime.dwLowDateTime;
    to->CreationTime.highPart = from->ftCreationTime.dwHighDateTime;

    to->LastAccessTime.lowPart = from->ftLastAccessTime.dwLowDateTime;
    to->LastAccessTime.highPart = from->ftLastAccessTime.dwHighDateTime;

    to->last_write_time.lowPart = from->ftLastWriteTime.dwLowDateTime;
    to->last_write_time.highPart = from->ftLastWriteTime.dwHighDateTime;

    to->Size = ((u64) from->nFileSizeHigh << 32) | from->nFileSizeLow;

    strncpy(to->filename, from->cFileName, FILE_MAX_PATH);
}

void *find_first_file(char *path, file_attributes_t *attrs)
{
    void *handle = 0;

    WIN32_FIND_DATA data;
    handle = FindFirstFile(path, &data);

    if (handle != INVALID_HANDLE_VALUE)
        _init_attributes(&data, attrs);

    return handle;
}

b8 find_next_file(void *handle, file_attributes_t *attrs)
{
    WIN32_FIND_DATA data;

    b8 result = FindNextFile(handle, &data);

    if (result)
        _init_attributes(&data, attrs);
    else
        FindClose(handle);

    return result;
}