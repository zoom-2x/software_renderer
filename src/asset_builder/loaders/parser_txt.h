// ----------------------------------------------------------------------------------
// -- File: parser_txt.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-11-27 22:29:40
// -- Modified: 2021-11-27 22:29:42
// ----------------------------------------------------------------------------------

#ifndef GCSR_PARSER_TEXT_H
#define GCSR_PARSER_TEXT_H

typedef struct
{
    u32 count;
    u32 keys_bytes;
    u32 values_bytes;

    void *keys;
    void *values;
} parser_text_data_t;

void *parse_text(gc_file_t *File)
{
    size_t total = sizeof(parser_text_data_t) + Kilobytes(500) + Megabytes(1);
    parser_text_data_t *data = (parser_text_data_t *) malloc(total);
    memset(data, 0, total);

    u8 *keys = (u8 *) (data + 1);
    u8 *values = (u8 *) (data + 1) + Kilobytes(500);

    data->keys = keys;
    data->values = values;

    data->count = 0;
    data->keys_bytes = 0;
    data->values_bytes = 0;

    string_array_t tmp_buffer;
    string_array_t *buffer = &tmp_buffer;

    // -- Read the file.

    while (File->cursor < File->bytes)
    {
        STRARR_RESET_MARKERS(buffer);
        b8 res = parser_read_line(File, buffer);

        if (res)
        {
            string_buffer_t *LineBuffer = STRARR_GET_LAST_STRING(buffer);

            // Comment.
            if (LineBuffer->buffer[0] == '#')
                continue;

            parser_tokenize_line(buffer, LineBuffer, '=');
            string_marker_t *Marker = STRARR_GET_LAST_MARKER(buffer);
            u32 start = Marker->start;

            if (Marker->len != 2)
            {
                STRARR_REWIND_MARKERS(buffer);
                continue;
            }

            string_buffer_t *key = &buffer->array[start];
            string_buffer_t *value = &buffer->array[start + 1];

            // -- Empty keys won't be copied.

            if (value->bytes)
            {
                u32 total_key_bytes = key->bytes + sizeof(u32);
                u32 total_value_bytes = value->bytes + sizeof(u32);

                // If the buffer is too small then signal it and stop the parsing.
                if ((data->keys_bytes + total_key_bytes) > Kilobytes(500) ||
                    (data->values_bytes + total_value_bytes) > Megabytes(1))
                {
                    printf("Text buffer is too small for this dataset, please adjust it's size !\n");
                    break;
                }

                var_char_t *K = (var_char_t *) keys;
                var_char_t *V = (var_char_t *) values;

                char *key_string = (char *) (K + 1);
                char *value_string = (char *) (V + 1);

                K->bytes = key->bytes;
                V->bytes = value->bytes;

                strncpy(key_string, key->buffer, K->bytes);
                strncpy(value_string, value->buffer, V->bytes);

                keys += total_key_bytes;
                values += total_value_bytes;

                data->count++;
                data->keys_bytes += total_key_bytes;
                data->values_bytes += total_value_bytes;
            }
        }
    }

    return data;
}

#endif