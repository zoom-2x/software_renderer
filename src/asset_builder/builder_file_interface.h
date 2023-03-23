// ----------------------------------------------------------------------------------
// -- File: builder_file_interface.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-04-26 21:34:50
// -- Modified:
// ----------------------------------------------------------------------------------

#include "../gcsr_file_operations.h"

// ----------------------------------------------------------------------------------
// -- Simple file reading interface implementation (gcsr_file_operations.h).
// ----------------------------------------------------------------------------------

gc_file_t *read_file(char *filepath, char *mode)
{
    gc_file_t *requested_file = 0;
    FILE *f = fopen(filepath, mode);

    printf("read_file: %s\n", filepath);

    if (f)
    {
        fseek(f, 0, SEEK_END);
        size_t bytes = ftell(f);
        fseek(f, 0, SEEK_SET);

        requested_file = (gc_file_t *) malloc(sizeof(gc_file_t) + bytes);

        requested_file->data = requested_file + 1;
        requested_file->name = filepath;
        requested_file->bytes = bytes;
        requested_file->cursor = 0;

        fread(requested_file->data, requested_file->bytes, 1, f);
        fclose(f);
    }
    else
        printf("ERROR: Cannot open file {%s}.\n", filepath);

    return requested_file;
}

void free_file(gc_file_t *file) {
    free(file);
}