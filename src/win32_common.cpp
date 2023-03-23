// ---------------------------------------------------------------------------------
// -- String routines.
// ---------------------------------------------------------------------------------

__INLINE__ size_t strLen(char *Str, size_t size)
{
    size_t result = 0;

    while (*Str++ != '\0' && result++ < size);

    return result;
}

__INLINE__ void catStrings(char *Dest, size_t destSize, size_t destLen, char *Src)
{
    size_t count = destLen;
    size_t max = destSize - 1;
    Dest += destLen;

    while (count++ < max && *Src != '\0') {
        *Dest++ = *Src++;
    }

    *Dest = '\0';
}

// ---------------------------------------------------------------------------------
// -- Logging.
// ---------------------------------------------------------------------------------

void Win32_LogLastError()
{
    DWORD error = GetLastError();

    if (error)
    {
        char err[255];
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, 0, err, 255, 0);
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, err);
    }
}