// ---------------------------------------------------------------------------------
// -- File: win32_file.cpp
// ---------------------------------------------------------------------------------
// -- Author:
// -- Description:
// -- Created:
// -- Modified:
// ---------------------------------------------------------------------------------

#define MAX_FILEPATH 1024

char FILEPATH_BUFFER[MAX_FILEPATH];

void win32_get_cwd(char *pathname, size_t size) {
    GetCurrentDirectory((DWORD) size, pathname);
}

void Win32_CreateFile(char *filePath)
{
    HANDLE file = CreateFileA(filePath, GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

    if (file == INVALID_HANDLE_VALUE)
        Win32_LogLastError();
    else
        CloseHandle(file);
}

b32 win32_file_exists(char *filePath)
{
    b32 result = true;
    WIN32_FILE_ATTRIBUTE_DATA Ignored;

    if(!GetFileAttributesEx(filePath, GetFileExInfoStandard, &Ignored))
        result = false;

    return result;
}

large_number_t win32_get_last_write_time(char *filePath)
{
    large_number_t result;
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (GetFileAttributesEx(filePath, GetFileExInfoStandard, &data))
    {
        result.lowPart = data.ftLastWriteTime.dwLowDateTime;
        result.highPart = data.ftLastWriteTime.dwHighDateTime;
    }

    return result;
}

b32 win32_get_file_attributes(char *filePath, file_attributes_t *Attributes)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    b32 result = GetFileAttributesExA(filePath, GetFileExInfoStandard, &data);

    if (Attributes && result)
    {
        Attributes->CreationTime.lowPart = data.ftCreationTime.dwLowDateTime;
        Attributes->CreationTime.highPart = data.ftCreationTime.dwHighDateTime;

        Attributes->LastAccessTime.lowPart = data.ftLastAccessTime.dwLowDateTime;
        Attributes->LastAccessTime.highPart = data.ftLastAccessTime.dwHighDateTime;

        Attributes->last_write_time.lowPart = data.ftLastWriteTime.dwLowDateTime;
        Attributes->last_write_time.highPart = data.ftLastWriteTime.dwHighDateTime;

        Attributes->Size.lowPart = data.nFileSizeLow;
        Attributes->Size.highPart = data.nFileSizeHigh;
    }

    if (!result)
        Win32_LogLastError();

    return result;
}

void win32_get_readable_time(large_number_t time, user_time_t *readtime)
{
    SDL_assert(readtime);

    FILETIME TmpFileTime;
    SYSTEMTIME TmpSysTime;

    TmpFileTime.dwLowDateTime = time.lowPart;
    TmpFileTime.dwHighDateTime = time.highPart;

    FileTimeToSystemTime(&TmpFileTime, &TmpSysTime);
    readtime->year = TmpSysTime.wYear;
    readtime->month = TmpSysTime.wMonth;
    readtime->dayOfWeek = TmpSysTime.wDayOfWeek;
    readtime->day = TmpSysTime.wDay;
    readtime->hour = TmpSysTime.wHour;
    readtime->minute = TmpSysTime.wMinute;
    readtime->second = TmpSysTime.wSecond;
    readtime->milliseconds = TmpSysTime.wMilliseconds;
}

file_handle_t win32_open_file(char *filePath, file_access_t access)
{
    file_handle_t result;

    DWORD desiredAccess = GENERIC_READ;
    // DWORD sharedMode = FILE_SHARE_READ;

    if (access == GC_FILE_WRITE)
        desiredAccess = GENERIC_WRITE;
    else if (access == (GC_FILE_READ | GC_FILE_WRITE))
        desiredAccess = GENERIC_READ | GENERIC_WRITE;

    HANDLE file = CreateFileA(filePath, desiredAccess, 0, 0, OPEN_ALWAYS, 0, 0);

    if (file == INVALID_HANDLE_VALUE) {
        Win32_LogLastError();
    }
    else
    {
        result.handle = file;
        result.valid = true;
    }

    return result;
}

large_number_t win32_file_size(file_handle_t File)
{
    large_number_t res;

    if (File.valid)
    {
        LARGE_INTEGER tmp;
        BOOL status = GetFileSizeEx(File.handle, &tmp);

        if (status)
        {
            res.lowPart = tmp.LowPart;
            res.highPart = tmp.HighPart;
        }
    }

    return res;
}

void win32_read_from_file(file_handle_t handle, u64 offset, u64 byteCount, void *buffer)
{
    // GetFileSizeEx
    if (handle.valid)
    {
        DWORD bytesRead = 0;
        OVERLAPPED Overlapped;
        Overlapped.Offset = offset;
        // For 64 bit ?
        // Overlapped.Offset = (u32) (Offset & 0xFFFFFFFF);
        // Overlapped.OffsetHigh = (u32) ((Offset >> 32) & 0xFFFFFFFF);

        b32 result = ReadFile(handle.handle, buffer, byteCount, &bytesRead, &Overlapped);

        if (!result)
            Win32_LogLastError();
    }
}

void win32_write_to_file(file_handle_t handle, u32 offset, u32 byteCount, void *source)
{
    if (handle.valid)
    {
        DWORD bytesWritten = 0;
        OVERLAPPED Overlapped;
        Overlapped.Offset = offset;
        // For 64 bit ?
        // Overlapped.Offset = (u32) (Offset & 0xFFFFFFFF);
        // Overlapped.OffsetHigh = (u32) ((Offset >> 32) & 0xFFFFFFFF);

        b32 result = WriteFile(handle.handle, source, byteCount, &bytesWritten, &Overlapped);

        if (!result)
            Win32_LogLastError();
    }
}

void win32_close_file(file_handle_t *handle)
{
    if (handle->valid)
    {
        b32 result = CloseHandle(handle->handle);

        if (!result) {
            Win32_LogLastError();
        }
        else
        {
            handle->valid = false;
            handle->handle = 0;
        }
    }
}
