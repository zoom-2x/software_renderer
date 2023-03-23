// ----------------------------------------------------------------------------------
// -- File: gcsr_parser.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-11-21 19:38:46
// -- Modified: 2022-04-29 21:15:21
// ----------------------------------------------------------------------------------

#ifndef GCSR_PARSER_H
#define GCSR_PARSER_H

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#ifndef GCSR_ENGINE_MATH_H
#define ONE_OVER_255 0.00392156862745098039f
#endif

void generate_mipmap(u32 *src, u32 *dest, u32 size);
void generate_mipmaps(loaded_bitmap_t *bitmap);
void generate_mipmapsf(loaded_bitmap_t *bitmap);
void *combine_cubemap_faces(cube_texture_t *texture, u32 mip_level, u32 *width, u32 *height, u32 *bytes);

// #define PARSER_MEMORY_POOL_SIZE Megabytes(10)
// #define FILE_NEXT_CHAR(File) (*((char *) File->data + File->cursor++))

// #define LOG_MEMORY_INSUFFICIENT(desc, required, current) printf("ERROR: [%s] Insufficient memory: required: %f current: %f\n")

void extract_pixel_format(bitmap_header_t *header, pixel_format_t *format)
{
    u32 masks[4] = {
        0x000000ff,
        0x0000ff00,
        0x00ff0000,
        0xff000000,
    };

    for (u32 i = 0; i < 4; ++i)
    {
        b8 set = false;

        if (header->red_mask == masks[i])
        {
            format->red_shift = i * 8;
            format->red_mask = header->red_mask;
            set = true;
        }
        else if (header->green_mask == masks[i])
        {
            format->green_shift = i * 8;
            format->green_mask = header->green_mask;
            set = true;
        }
        else if (header->blue_mask == masks[i])
        {
            format->blue_shift = i * 8;
            format->blue_mask = header->blue_mask;
            set = true;
        }

        if (!set)
        {
            format->alpha_shift = i * 8;
            format->alpha_mask = ~(format->red_mask | format->green_mask | format->blue_mask);
        }
    }
}

void init_pixel_format(pixel_format_t *format, u32 red_shift, u32 green_shift, u32 blue_shift, u32 alpha_shift)
{
    format->red_shift = red_shift;
    format->green_shift = green_shift;
    format->blue_shift = blue_shift;
    format->alpha_shift = alpha_shift;

    format->red_mask = 0xff << red_shift;
    format->green_mask = 0xff << green_shift;
    format->blue_mask = 0xff << blue_shift;
    format->alpha_mask = 0xff << alpha_shift;
}

__INLINE__ void unpack_color(u32 color, pixel_format_t *format, gc_vec_t *output)
{
    output->c.r = (r32) (((color & format->red_mask) >> format->red_shift) & 0xFF);
    output->c.g = (r32) (((color & format->green_mask) >> format->green_shift) & 0xFF);
    output->c.b = (r32) (((color & format->blue_mask) >> format->blue_shift) & 0xFF);
    output->c.a = (r32) (((color & format->alpha_mask) >> format->alpha_shift) & 0xFF);
}

__INLINE__ void pack_color(gc_vec_t *color, pixel_format_t *format, u32 *output)
{
    *output = ((u32) (color->c.r + 0.5f) << format->red_shift) |
              ((u32) (color->c.g + 0.5f) << format->green_shift) |
              ((u32) (color->c.b + 0.5f) << format->blue_shift) |
              ((u32) (color->c.a + 0.5f) << format->alpha_shift);
}

// ----------------------------------------------------------------------------------
// -- Loads a bitmap file (4 bytes per pixel only / rgba).
// ----------------------------------------------------------------------------------

b8 load_bitmap(char *filepath, loaded_bitmap_t *bitmap)
{
    b8 result = true;

    gc_file_t bitmap_file;
    open_file(&bitmap_file, filepath, GC_FILE_READ);

    if (bitmap_file.handle && bitmap_file.bytes > 0)
    {
        void *bitmap_content = malloc(bitmap_file.bytes);
        read_file(&bitmap_file, 0, bitmap_file.bytes, bitmap_content);

        bitmap_header_t *bitmap_header = (bitmap_header_t *) bitmap_content;
        u32 *source_data = (u32 *) ADDR_OFFSET(bitmap_content, bitmap_header->bitmap_offset);
        assert(bitmap_header->file_type == 0x4d42);

        b32 is_top_down = true;

        if (bitmap_header->height > 0)
            is_top_down = false;

        bitmap->width = bitmap_header->width;
        bitmap->height = is_top_down ? -bitmap_header->height : bitmap_header->height;
        bitmap->bytes_per_pixel = bitmap_header->bits_per_pixel >> 3;
        bitmap->pitch = bitmap->width * bitmap->bytes_per_pixel;
        bitmap->width_over_height = (r32) bitmap->width / bitmap->height;
        bitmap->components = bitmap->bytes_per_pixel;
        bitmap->bytes = sizeof(u32) * bitmap->width * bitmap->height;
        bitmap->mip_count = 0;
        bitmap->data = malloc(bitmap->bytes);

        // ----------------------------------------------------------------------------------
        // -- Copy only the bitmap data.
        // ----------------------------------------------------------------------------------

        u32 *source = (u32 *) source_data;
        u32 *dest = (u32 *) bitmap->data;

        for (u32 row = 0; row < bitmap->height; ++row)
        {
            for (u32 col = 0; col < bitmap->width; ++col) {
                *dest++ = *source++;
            }
        }

        // ----------------------------------------------------------------------------------
        // -- Rewrite the data to be top down.
        // ----------------------------------------------------------------------------------

        if (!is_top_down)
        {
            source = ADDR_OFFSET32(bitmap->data, (bitmap->height - 1) * (bitmap->width));
            dest = (u32 *) bitmap->data;

            u32 *source_row = source;
            u32 *dest_row = dest;
            u32 row = 0;

            while (source_row > dest_row)
            {
                u32 *source_pixel = source_row;
                u32 *dest_pixel = dest_row;
                u32 col = 0;

                for (u32 i = 0; i < bitmap->width; ++i)
                {
                    u32 tmp = *source_pixel;
                    *source_pixel = *dest_pixel;
                    *dest_pixel = tmp;

                    source_pixel++;
                    dest_pixel++;
                    col++;
                }

                source_row -= bitmap->width;
                dest_row += bitmap->width;
                row++;
            }
        }

        // ----------------------------------------------------------------------------------
        // -- Preprocess the color.
        // ----------------------------------------------------------------------------------

        pixel_format_t bitmap_pixel_format;
        pixel_format_t output_pixel_format;

        extract_pixel_format(bitmap_header, &bitmap_pixel_format);
        init_pixel_format(&output_pixel_format, GL_PIXEL_FORMAT_RED_SHIFT, GL_PIXEL_FORMAT_GREEN_SHIFT, GL_PIXEL_FORMAT_BLUE_SHIFT, GL_PIXEL_FORMAT_ALPHA_SHIFT);

        source = (u32 *) bitmap->data;
        u32 packed = 0;
        gc_vec_t color;

        for (u32 row = 0; row < bitmap->height; ++row)
        {
            u32 *pixel = source;

            for (u32 col = 0; col < bitmap->width; ++col)
            {
                unpack_color(*pixel, &bitmap_pixel_format, &color);

                color.c.r *= ONE_OVER_255;
                color.c.g *= ONE_OVER_255;
                color.c.b *= ONE_OVER_255;
                color.c.a *= ONE_OVER_255;

                // sRGB to linear + premultiplied alpha.
                color.c.r *= color.c.r * color.c.a;
                color.c.g *= color.c.g * color.c.a;
                color.c.b *= color.c.b * color.c.a;

                // Back to sRGB.
                color.c.r = sqrt(color.c.r) * 255;
                color.c.g = sqrt(color.c.g) * 255;
                color.c.b = sqrt(color.c.b) * 255;
                color.c.a *= 255;

                pack_color(&color, &output_pixel_format, &packed);

                *pixel++ = packed;
            }

            source += bitmap->width;
        }

        free(bitmap_content);
        close_file(&bitmap_file);
    }
    else
    {
        result = false;
        printf(B_ERROR(MISSING_FILE), filepath);
    }

    return result;
}

size_t compute_mip_bytes_int(u32 mip_count, u32 base_size)
{
    size_t bytes = mip_count * sizeof(texture_data_header_t);
    u32 mip_size = base_size;

    for (u8 i = 0; i < mip_count; ++i)
    {
        bytes += TEX_BYTES_INT(mip_size, mip_size);
        mip_size = mip_size >> 1;
    }

    return bytes;
}

size_t compute_mip_bytes_float(u32 mip_count, u32 base_size)
{
    size_t bytes = mip_count * sizeof(texture_data_header_t);
    u32 mip_size = base_size;

    for (u8 i = 0; i < mip_count; ++i)
    {
        bytes += TEX_BYTES_FLOAT(mip_size, mip_size);
        mip_size = mip_size >> 1;
    }

    return bytes;
}

b8 load_texture(char *filepath, loaded_texture_t *texture)
{
    loaded_bitmap_t bitmap;
    b8 loaded = load_bitmap(filepath, &bitmap);
    void *bitmap_data = bitmap.data;

    if (loaded && bitmap_data)
    {
        // The textures must be squared !
        assert(bitmap.width == bitmap.height);

        u32 mip_size = bitmap.width;
        u32 mip_count = TEX_MIP_COUNT(mip_size);

        texture2d_t *texture_object = gc_create_texture2d(mip_size, mip_size, mip_count, TEXTURE_FORMAT_RGBAU8, 0);
        texture->data = texture_object;

        texture->meta.type = ASSET_TEXTURE2D;
        texture->meta.data_bytes = texture_object->data_bytes;
        texture->meta.data_offset = 0;

        texture->meta.texture.width = bitmap.width;
        texture->meta.texture.height = bitmap.height;
        texture->meta.texture.mip_count = mip_count;
        texture->meta.texture.format = TEXTURE_FORMAT_RGBAU8;

        mip_size = bitmap.width;

        gc_texture_copy_image_to_level0(texture_object, (u32 *) bitmap_data);
        gc_texture_generate_mipmaps(texture_object);

        free(bitmap.data);
    }

    return loaded;
}

void load_cubemap(char cubepath[6][MAX_ASSET_FILEPATH_SIZE], loaded_texture_t *texture)
{
    loaded_bitmap_t bitmap_data[6];

    for (u8 i = 0; i < 6; ++i)
    {
        load_bitmap(cubepath[i], bitmap_data + i);

        if (!(bitmap_data + i))
            return;
    }

    u32 width = bitmap_data[0].width;
    u32 height = bitmap_data[0].height;

    assert(width == height);

    u32 mip_count = TEX_MIP_COUNT(width);
    cube_texture_t *texture_object = gc_create_cubemap_texture(width, height, mip_count, TEXTURE_FORMAT_RGBAU8, 0);

    texture->meta.type = ASSET_TEXTURE_CUBEMAP;
    texture->meta.data_bytes = texture_object->data_bytes;
    texture->meta.data_offset = 0;

    texture->meta.texture.width = width;
    texture->meta.texture.height = height;
    texture->meta.texture.mip_count = mip_count;
    texture->meta.texture.format = TEXTURE_FORMAT_RGBAU8;

    texture->data = texture_object;

    for (u8 i = 0; i < 6; ++i)
    {
        loaded_bitmap_t *bitmap = bitmap_data + i;
        texture2d_t *face = texture_object->faces[i];
        gc_texture_copy_image_to_level0(face, (u32 *) bitmap->data);
        gc_texture_generate_mipmaps(face);

        free(bitmap->data);
    }
}

// ----------------------------------------------------------------------------------
// -- Loads a bitmap file and transforms it in linear format.
// ----------------------------------------------------------------------------------
// -- NOTE(gabic): It's assumed that the bitmap size is a multiple of 2.
// ----------------------------------------------------------------------------------

b8 load_bitmapf(char *filepath, loaded_bitmap_t *bitmap)
{
    b8 result = true;

    gc_file_t bitmap_file;
    open_file(&bitmap_file, filepath, GC_FILE_READ);

    if (bitmap_file.handle && bitmap_file.bytes > 0)
    {
        void *bitmap_data = malloc(bitmap_file.bytes);
        read_file(&bitmap_file, 0, bitmap_file.bytes, bitmap_data);

        bitmap_header_t *bitmap_header = (bitmap_header_t *) bitmap_data;
        assert(bitmap_header->file_type == 0x4d42);

        b32 is_top_down = true;

        if (bitmap_header->height > 0)
            is_top_down = false;

        bitmap->width = bitmap_header->width;
        bitmap->height = is_top_down ? bitmap_header->height * -1 : bitmap_header->height;
        bitmap->bytes_per_pixel = bitmap_header->bits_per_pixel >> 3;
        bitmap->pitch = bitmap->width * bitmap->bytes_per_pixel;
        bitmap->width_over_height = (r32) bitmap->width / bitmap->height;
        bitmap->components = bitmap->bytes_per_pixel;
        bitmap->mip_count = 0;

        size_t data_bytes = bitmap->components * sizeof(r32) * bitmap->width * bitmap->height;

        bitmap->mip_count = (u32) log2f(bitmap->width) + 1;
        size_t mip_bytes = data_bytes;

        // First level is the base bitmap.
        for (u8 i = 1; i < bitmap->mip_count; ++i)
        {
            mip_bytes = mip_bytes >> 1;
            data_bytes += mip_bytes;
        }

        // Allocate memory for the bitmap + mipmaps (if enabled).
        bitmap->bytes = MEM_SIZE_ALIGN(data_bytes);
        bitmap->data = malloc(bitmap->bytes);

        // -- Copy the base bitmap data to the mipmap level 0.

        u32 tmp_offset = bitmap_header->bitmap_offset;
        s32 tmp_pitch = bitmap->width;

        if (!is_top_down)
        {
            tmp_offset += (bitmap->height - 1) * bitmap->pitch;
            tmp_pitch *= -1;
        }

        // u32 alpha_mask = ~(bitmap_header->red_mask | bitmap_header->green_mask | bitmap_header->blue_mask);

        pixel_format_t bitmap_pixel_format;
        pixel_format_t output_pixel_format;

        extract_pixel_format(bitmap_header, &bitmap_pixel_format);
        init_pixel_format(&output_pixel_format, GL_PIXEL_FORMAT_RED_SHIFT, GL_PIXEL_FORMAT_GREEN_SHIFT, GL_PIXEL_FORMAT_BLUE_SHIFT, GL_PIXEL_FORMAT_ALPHA_SHIFT);

        // bitmap->red_shift = bitmap_pixel_format.red_shift;
        // bitmap->green_shift = bitmap_pixel_format.green_shift;
        // bitmap->blue_shift = bitmap_pixel_format.blue_shift;
        // bitmap->alpha_shift = bitmap_pixel_format.alpha_shift;

        u32 *source = (u32 *) ADDR_OFFSET(bitmap_header, tmp_offset);
        texcolor_t *dest = (texcolor_t *) bitmap->data;
        gc_vec_t color;

        for (u32 row = 0; row < bitmap->height; ++row)
        {
            u32 *pixel = source;
            texcolor_t *dest_pixel = dest;

            for (u32 col = 0; col < bitmap->width; ++col)
            {
                // dest_pixel->r = (r32) (((*pixel & bitmap_header->red_mask) >> format.red_shift) & 0xFF);
                // dest_pixel->g = (r32) (((*pixel & bitmap_header->green_mask) >> format.green_shift) & 0xFF);
                // dest_pixel->b = (((*pixel & bitmap_header->blue_mask) >> format.blue_shift) & 0xFF);
                // dest_pixel->a = (((*pixel & alpha_mask) >> format.alpha_shift) & 0xFF);
                unpack_color(*pixel, &bitmap_pixel_format, &color);

                dest_pixel->r = color.c.r;
                dest_pixel->g = color.c.g;
                dest_pixel->b = color.c.b;
                dest_pixel->a = color.c.a;

                dest_pixel->r *= ONE_OVER_255;
                dest_pixel->g *= ONE_OVER_255;
                dest_pixel->b *= ONE_OVER_255;
                dest_pixel->a *= ONE_OVER_255;

                dest_pixel->r *= dest_pixel->r * dest_pixel->a;
                dest_pixel->g *= dest_pixel->g * dest_pixel->a;
                dest_pixel->b *= dest_pixel->b * dest_pixel->a;

                dest_pixel++;
                pixel++;
            }

            source += tmp_pitch;
            dest += bitmap->width;
        }

        generate_mipmapsf(bitmap);

        free(bitmap_data);
        close_file(&bitmap_file);
    }
    else
    {
        result = false;
        printf(B_ERROR(MISSING_FILE), filepath);
    }

    return result;
}

// ----------------------------------------------------------------------------------
// -- Generates a specified mip level.
// ----------------------------------------------------------------------------------

void generate_mipmap_float(texpixel_rgbaf_t *parent_data, texpixel_rgbaf_t *current_data, u32 mip_size)
{
    u32 parent_mip_size = mip_size << 1;
    texpixel_rgbaf_t *pixel = current_data;

    for (u32 row = 0; row < mip_size; ++row)
    {
        u32 parent_row = row << 1;
        texpixel_rgbaf_t *source_row_1 = parent_data + parent_row * parent_mip_size;
        texpixel_rgbaf_t *source_row_2 = parent_data + (parent_row + 1) * parent_mip_size;

        for (u32 col = 0; col < mip_size; ++col)
        {
            u32 parent_col = col << 1;

            texpixel_rgbaf_t *color_0 = source_row_1 + parent_col;
            texpixel_rgbaf_t *color_1 = source_row_1 + (parent_col + 1);
            texpixel_rgbaf_t *color_2 = source_row_2 + parent_col;
            texpixel_rgbaf_t *color_3 = source_row_2 + (parent_col + 1);

            // Average the color.
            pixel->r = (color_0->r + color_1->r + color_2->r + color_3->r) / 4;
            pixel->g = (color_0->g + color_1->g + color_2->g + color_3->g) / 4;
            pixel->b = (color_0->b + color_1->b + color_2->b + color_3->b) / 4;
            pixel->a = (color_0->a + color_1->a + color_2->a + color_3->a) / 4;

            pixel++;
        }
    }
}

void generate_mipmaps(loaded_bitmap_t *bitmap)
{
    if (!bitmap)
        return;

    u32 current_mip_width = bitmap->width;
    u32 current_mip_height = bitmap->height;

    u32 *prev_mip_pointer = (u32 *) bitmap->data;
    u32 *mip_pointer = prev_mip_pointer;

    pixel_format_t pixel_format;
    init_pixel_format(&pixel_format, GL_PIXEL_FORMAT_RED_SHIFT, GL_PIXEL_FORMAT_GREEN_SHIFT, GL_PIXEL_FORMAT_BLUE_SHIFT, GL_PIXEL_FORMAT_ALPHA_SHIFT);

    for (u8 i = 1; i < bitmap->mip_count; ++i)
    {
        size_t prev_mip_bytes = current_mip_width * current_mip_height * sizeof(u32);
        mip_pointer = (u32 *) ADDR_OFFSET(mip_pointer, prev_mip_bytes);
        u32 *current_mip_row = mip_pointer;

        u32 prev_mip_width = current_mip_width;

        current_mip_width = current_mip_width >> 1;
        current_mip_height = current_mip_height >> 1;

        gc_vec_t color[4];
        gc_vec_t mip_color;

        for (u32 row = 0; row < current_mip_height; ++row)
        {
            u32 *mip_pixel = current_mip_row;
            u32 prev_y = row << 1;

            for (u32 col = 0; col < current_mip_width; ++col)
            {
                u32 prev_x = col << 1;

                u32 *base_pixel_0 = prev_mip_pointer + (prev_y * prev_mip_width + prev_x);
                u32 *base_pixel_1 = prev_mip_pointer + (prev_y * prev_mip_width + (prev_x + 1));
                u32 *base_pixel_2 = prev_mip_pointer + ((prev_y + 1) * prev_mip_width + prev_x);
                u32 *base_pixel_3 = prev_mip_pointer + ((prev_y + 1) * prev_mip_width + (prev_x + 1));

                unpack_color(*base_pixel_0, &pixel_format, &color[0]);
                unpack_color(*base_pixel_1, &pixel_format, &color[1]);
                unpack_color(*base_pixel_2, &pixel_format, &color[2]);
                unpack_color(*base_pixel_3, &pixel_format, &color[3]);

                // Average the color.
                mip_color.c.r = (color[0].c.r + color[1].c.r + color[2].c.r + color[3].c.r) / 4;
                mip_color.c.g = (color[0].c.g + color[1].c.g + color[2].c.g + color[3].c.g) / 4;
                mip_color.c.b = (color[0].c.b + color[1].c.b + color[2].c.b + color[3].c.b) / 4;
                mip_color.c.a = (color[0].c.a + color[1].c.a + color[2].c.a + color[3].c.a) / 4;

                pack_color(&mip_color, &pixel_format, mip_pixel);
                mip_pixel++;
            }

            current_mip_row += current_mip_width;
        }

        prev_mip_pointer = mip_pointer;
    }
}

void generate_mipmapsf(loaded_bitmap_t *bitmap)
{
    if (!bitmap)
        return;

    u32 current_mip_width = bitmap->width;
    u32 current_mip_height = bitmap->height;

    texcolor_t *prev_mip_pointer = (texcolor_t *) bitmap->data;
    texcolor_t *mip_pointer = prev_mip_pointer;

    for (u8 i = 1; i < bitmap->mip_count; ++i)
    {
        size_t prev_mip_bytes = current_mip_width * current_mip_height * sizeof(texcolor_t);
        mip_pointer = (texcolor_t *) ADDR_OFFSET(mip_pointer, prev_mip_bytes);
        texcolor_t *current_mip_row = mip_pointer;

        u32 prev_mip_width = current_mip_width;

        current_mip_width = current_mip_width >> 1;
        current_mip_height = current_mip_height >> 1;

        for (u32 row = 0; row < current_mip_height; ++row)
        {
            texcolor_t *mip_pixel = current_mip_row;
            u32 prev_y = row << 1;

            for (u32 col = 0; col < current_mip_width; ++col)
            {
                u32 prev_x = col << 1;

                texcolor_t *base_pixel_0 = prev_mip_pointer + (prev_y * prev_mip_width + prev_x);
                texcolor_t *base_pixel_1 = prev_mip_pointer + (prev_y * prev_mip_width + (prev_x + 1));
                texcolor_t *base_pixel_2 = prev_mip_pointer + ((prev_y + 1) * prev_mip_width + prev_x);
                texcolor_t *base_pixel_3 = prev_mip_pointer + ((prev_y + 1) * prev_mip_width + (prev_x + 1));

                // Average the color.
                mip_pixel->r = (base_pixel_0->r + base_pixel_1->r + base_pixel_2->r + base_pixel_3->r) / 4;
                mip_pixel->g = (base_pixel_0->g + base_pixel_1->g + base_pixel_2->g + base_pixel_3->g) / 4;
                mip_pixel->b = (base_pixel_0->b + base_pixel_1->b + base_pixel_2->b + base_pixel_3->b) / 4;
                mip_pixel->a = (base_pixel_0->a + base_pixel_1->a + base_pixel_2->a + base_pixel_3->a) / 4;

                mip_pixel++;
            }

            current_mip_row += current_mip_width;
        }

        prev_mip_pointer = mip_pointer;
    }
}

// ----------------------------------------------------------------------------------
// -- Exports the given u32 data buffer to a specified bitmap file.
// ----------------------------------------------------------------------------------

void export_bitmap(gc_file_t *file, u32 *data, s32 width, s32 height, s32 components)
{
    bitmap_header_t header;

    assert(width && height);

    if (components == 4)
    {
        size_t data_bytes = width * height * sizeof(u32);

        header.file_type = 0x4D42;
        header.file_size = (u32) (sizeof(bitmap_header_t) + data_bytes);
        header.bitmap_offset = sizeof(bitmap_header_t);
        // header.size = sizeof(bitmap_header_t);
        header.size = 108; // v4 bitmap header !
        header.width = width;
        header.height = -height;
        header.bits_per_pixel = 32;
        header.size_of_bitmap = (u32) data_bytes;

        // Internal fixed bitmap pixel format is used.
        header.red_mask = (u32) 0xff << GL_PIXEL_FORMAT_RED_SHIFT;
        header.green_mask = (u32) 0xff << GL_PIXEL_FORMAT_GREEN_SHIFT;
        header.blue_mask = (u32) 0xff << GL_PIXEL_FORMAT_BLUE_SHIFT;
        header.alpha_mask = (u32) 0xff << GL_PIXEL_FORMAT_ALPHA_SHIFT;

        header.cstype = 0x73524742;
        header.planes = 1;
        header.compression = 3;

        write_file(file, sizeof(bitmap_header_t), &header);

        for (s32 row = 0; row < height; ++row)
        {
            u32 *pixel = data;

            for (s32 col = 0; col < width; ++col)
            {
                u32 pixel_color = *pixel++;
                write_file(file, sizeof(u32), &pixel_color);
            }

            data += width;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Exports the given r32 data buffer to a specified bitmap file.
// ----------------------------------------------------------------------------------

void export_bitmapf(gc_file_t *file, r32 *data, s32 width, s32 height, s32 components)
{
    bitmap_header_t header;
    u32 data_pitch = width * components;

    assert(width && height);

    if (components == 3)
    {
        bitmap_color_24_t pixel;

        s32 _cbytes = width * 3;
        s32 col_bytes_mult = MULTIPLE_OF(_cbytes, 4);
        s32 padding_bytes = col_bytes_mult - _cbytes;
        s32 total_padding_bytes = padding_bytes * height;

        size_t data_bytes = width * height * components;

        header.file_type = 0x4D42;
        header.file_size = (u32) (sizeof(bitmap_header_t) + data_bytes + total_padding_bytes);
        header.bitmap_offset = sizeof(bitmap_header_t);
        // header.size = sizeof(bitmap_header_t) - 14;
        header.size = 108; // v4 bitmap header !
        header.width = width;
        header.height = -height;
        header.bits_per_pixel = 24;
        header.size_of_bitmap = (u32) data_bytes + total_padding_bytes;
        header.planes = 1;

        write_file(file, sizeof(bitmap_header_t), &header);

        for (s32 row = 0; row < height; ++row)
        {
            r32 *data_row = data;

            for (s32 col = 0, col_bytes = 0; col < width; ++col, col_bytes += 3)
            {
                r32 r = *data_row++;
                r32 g = *data_row++;
                r32 b = *data_row++;

                r = r / (r + 1);
                g = g / (g + 1);
                b = b / (b + 1);

                // r = r > 1 ? 1 : r;
                // g = g > 1 ? 1 : g;
                // b = b > 1 ? 1 : b;

                r = pow(r, 1 / 2.2f);
                g = pow(g, 1 / 2.2f);
                b = pow(b, 1 / 2.2f);

                pixel.r = (u8) (r * 255);
                pixel.g = (u8) (g * 255);
                pixel.b = (u8) (b * 255);

                write_file(file, sizeof(bitmap_color_24_t), &pixel);

                // Padding.
                if (col == width - 1 && padding_bytes)
                {
                    for (u8 i = 0; i < padding_bytes; ++i)
                    {
                        u8 padding = 0;
                        write_file(file, 1, &padding);
                    }
                }
            }

            data += data_pitch;
        }
    }

    else if (components == 4)
    {
        size_t data_bytes = width * height * components;

        header.file_type = 0x4D42;
        header.file_size = (u32) (sizeof(bitmap_header_t) + data_bytes);
        header.bitmap_offset = sizeof(bitmap_header_t);
        // header.size = sizeof(bitmap_header_t);
        header.size = 108; // v4 bitmap header !
        header.width = width;
        header.height = -height;
        header.bits_per_pixel = 32;
        header.size_of_bitmap = (u32) data_bytes;
        header.red_mask = 0x00ff0000;
        header.green_mask = 0x0000ff00;
        header.blue_mask = 0x000000ff;
        header.alpha_mask = 0xff000000;
        header.cstype = 0x73524742;
        header.planes = 1;
        header.compression = 3;

        write_file(file, sizeof(bitmap_header_t), &header);

        for (s32 y = 0; y < height; ++y)
        {
            r32 *data_row = data;

            for (s32 x = 0; x < width; ++x)
            {
                r32 r = *data_row++;
                r32 g = *data_row++;
                r32 b = *data_row++;
                r32 a = *data_row++;

                r = r / (r + 1);
                g = g / (g + 1);
                b = b / (b + 1);

                r = pow(r, 1 / 2.2f);
                g = pow(g, 1 / 2.2f);
                b = pow(b, 1 / 2.2f);

                u8 r8 = (u8) (r * 255);
                u8 g8 = (u8) (g * 255);
                u8 b8 = (u8) (b * 255);
                u8 a8 = (u8) (a * 255);

                u32 pixel_color = a8 << 24 | r8 << 16 | g8 << 8 | b8;
                write_file(file, sizeof(u32), &pixel_color);
            }

            data += data_pitch;
        }
    }
}

void *scale_bitmap(char *filepath, loaded_bitmap_t *bitmap, u32 to_width, u32 to_height)
{
    void *data = 0;

    if (bitmap && bitmap->data)
    {
        data = malloc(bitmap->width * bitmap->height * sizeof(u32));
        u32 *pixel = (u32 *) data;
        u32 *src_pointer = (u32 *) bitmap->data;

        // Rescale (nearest neighbour).
        for (u32 y = 0; y < to_height; ++y)
        {
            r32 v = (r32) y / (to_height - 1);

            for (u32 x = 0; x < to_width; ++x)
            {
                r32 u = (r32) x / (to_width - 1);

                u32 texel_x = (u32) (u * (bitmap->width - 1));
                u32 texel_y = (u32) (v * (bitmap->height - 1));

                u32 offset = texel_y * bitmap->width + texel_x;
                u32 color = src_pointer[offset];

                *pixel++ = color;
            }
        }
    }

    return data;
}

void *scale_bitmap_float3_m(void *data, u32 width, u32 height, u32 mult)
{
    u32 new_width = width * mult;
    u32 new_height = height * mult;

    size_t bytes = new_width * new_height * 3 * sizeof(r32);
    void *memory = malloc(bytes);
    texpixel_rgbf_t *src_pixel = (texpixel_rgbf_t *) data;

    for (u32 y = 0; y < height; ++y)
    {
        u32 dest_start_y = y * mult;
        u32 offset = dest_start_y * new_width;

        for (u32 x = 0; x < width; ++x)
        {
            u32 dest_start_x = x * mult;
            texpixel_rgbf_t *dest_pixel = (texpixel_rgbf_t *) memory + offset + dest_start_x;

            for (u32 oy = 0; oy < mult; ++oy)
            {
                texpixel_rgbf_t *pixel = dest_pixel;

                for (u32 ox = 0; ox < mult; ++ox)
                {
                    pixel->r = src_pixel->r;
                    pixel->g = src_pixel->g;
                    pixel->b = src_pixel->b;

                    pixel++;
                }

                dest_pixel += new_width;
            }

            src_pixel++;
        }
    }

    return memory;
}

void _debug_basic_texture(char *filepath, loaded_texture_t *texture)
{
    char *filename = extract_file_name(filepath, 0);
    char *output_filepath = sb_next_buffer(&_string_buffer);

    gc_file_t output_bmp;
    texture2d_t *texture_object = (texture2d_t *) texture->data;

    for (u32 i = 0; i < texture_object->mip_count; ++i)
    {
        texture_mip_t *current_level = texture_object->mips + i;
        texture_data_header_t *texture_header = (texture_data_header_t *) current_level->header;

        sprintf(output_filepath, "%s%s.mip%u.%s", DEBUG_FOLDER, filename, i, "bmp");
        open_file(&output_bmp, output_filepath, GC_FILE_WRITE);
        export_bitmap(&output_bmp, (u32 *) current_level->data, texture_header->width, texture_header->height, 4);
        close_file(&output_bmp);
    }
}

void _debug_cubemap_texture(char *alias, loaded_texture_t *texture)
{
    char *output_filepath = sb_next_buffer(&_string_buffer);

    gc_file_t output_bmp;
    cube_texture_t *cubemap = (cube_texture_t *) texture->data;

    char names[6][20] = {
        "left",
        "right",
        "front",
        "back",
        "top",
        "bottom",
    };

    // ----------------------------------------------------------------------------------
    // -- Export the individual faces.
    // ----------------------------------------------------------------------------------

    for (u8 i = 0; i < 6; ++i)
    {
        texture2d_t *face = cubemap->faces[i];
        texture_mip_t *base_mip_level = face->mips + DEBUG_CUBEMAP_MIP_LEVEL;

        sprintf(output_filepath, "%s%s.%s.%s", DEBUG_FOLDER, alias, names[i], "bmp");
        open_file(&output_bmp, output_filepath, GC_FILE_WRITE);
        export_bitmap(&output_bmp, (u32 *) base_mip_level->data, base_mip_level->header->width, base_mip_level->header->height, 4);
        close_file(&output_bmp);
    }

    // ----------------------------------------------------------------------------------
    // -- Export all the faces as a single image.
    // ----------------------------------------------------------------------------------

    u32 width = 0;
    u32 height = 0;
    u32 bytes = 0;

    void *combined_cubemap = combine_cubemap_faces(cubemap, DEBUG_CUBEMAP_MIP_LEVEL, &width, &height, &bytes);
    sprintf(output_filepath, "%s%s.combined.%s", DEBUG_FOLDER, alias, "bmp");
    open_file(&output_bmp, output_filepath, GC_FILE_WRITE);
    export_bitmap(&output_bmp, (u32 *) combined_cubemap, width, height, 4);
    close_file(&output_bmp);
    free(combined_cubemap);
}

#define COPY_IMAGE(dest, src, width, height, pitch) \
for (u32 row = 0; row < height; ++row) \
{ \
    u32 *pixel = dest; \
\
    for (u32 col = 0; col < width; ++col) { \
        *pixel++ = *src++; \
    } \
\
    dest += (pitch); \
}

void *combine_cubemap_faces(cube_texture_t *cubemap, u32 mip_level, u32 *width, u32 *height, u32 *bytes)
{
    texture2d_t *first_face = cubemap->faces[0];
    texture_mip_t *selected_mip = first_face->mips + mip_level;

    u32 cube_width = selected_mip->header->width;
    u32 cube_height = selected_mip->header->height;

    *width = cube_width * 4;
    *height = cube_height * 3;
    *bytes = (*height) * (*width) * sizeof(u32);

    void *memory = malloc(*bytes);
    memset(memory, 0, *bytes);

    // ----------------------------------------------------------------------------------
    // -- Top.
    // ----------------------------------------------------------------------------------

    texture2d_t *top = cubemap->faces[CUBE_TOP];
    selected_mip = top->mips + mip_level;

    u32 *src = (u32 *) selected_mip->data;
    u32 *dest = (u32 *) ADDR_OFFSET32(memory, cube_width);

    COPY_IMAGE(dest, src, cube_width, cube_height, *width);

    // ----------------------------------------------------------------------------------
    // -- Left.
    // ----------------------------------------------------------------------------------

    texture2d_t *left = cubemap->faces[CUBE_LEFT];
    selected_mip = left->mips + mip_level;

    src = (u32 *) selected_mip->data;
    dest = (u32 *) ADDR_OFFSET32(memory, (*width) * cube_height);

    COPY_IMAGE(dest, src, cube_width, cube_height, *width);

    // ----------------------------------------------------------------------------------
    // -- Front.
    // ----------------------------------------------------------------------------------

    texture2d_t *front = cubemap->faces[CUBE_FRONT];
    selected_mip = front->mips + mip_level;

    src = (u32 *) selected_mip->data;
    dest = (u32 *) ADDR_OFFSET32(memory, (*width) * cube_height + cube_width);

    COPY_IMAGE(dest, src, cube_width, cube_height, *width);

    // ----------------------------------------------------------------------------------
    // -- Right.
    // ----------------------------------------------------------------------------------

    texture2d_t *right = cubemap->faces[CUBE_RIGHT];
    selected_mip = right->mips + mip_level;

    src = (u32 *) selected_mip->data;
    dest = (u32 *) ADDR_OFFSET32(memory, (*width) * cube_height + 2 * cube_width);

    COPY_IMAGE(dest, src, cube_width, cube_height, *width);

    // ----------------------------------------------------------------------------------
    // -- Back.
    // ----------------------------------------------------------------------------------

    texture2d_t *back = cubemap->faces[CUBE_BACK];
    selected_mip = back->mips + mip_level;

    src = (u32 *) selected_mip->data;
    dest = (u32 *) ADDR_OFFSET32(memory, (*width) * cube_height + 3 * cube_width);

    COPY_IMAGE(dest, src, cube_width, cube_height, *width);

    // ----------------------------------------------------------------------------------
    // -- Bottom.
    // ----------------------------------------------------------------------------------

    texture2d_t *bottom = cubemap->faces[CUBE_BOTTOM];
    selected_mip = bottom->mips + mip_level;

    src = (u32 *) selected_mip->data;
    dest = (u32 *) ADDR_OFFSET32(memory, 2 * cube_height * (*width) + cube_width);

    COPY_IMAGE(dest, src, cube_width, cube_height, *width);

    return memory;
}

#if 0
void debug_loaded_bitmap(char *filepath, loaded_bitmap_t *bitmap)
{
    char *filename = extract_file_name(filepath, 0);
    char *output_filepath = sb_next_buffer(&_string_buffer);
    char *mip_name = sb_next_buffer(&_string_buffer);

    gc_file_t output_bmp;

    // Base bitmap.
    sprintf(output_filepath, "%s%s.%s", DEBUG_FOLDER, filename, "bmp");
    open_file(&output_bmp, output_filepath, "wb");
    export_bitmap(&output_bmp, (u32 *) bitmap->data, bitmap->width, bitmap->height, bitmap->components);
    close_file(&output_bmp);

    u32 *base_pointer = (u32 *) bitmap->data;
    u32 *mip_pointer = base_pointer;

    // Mipmaps.
    if (bitmap->mip_count)
    {
        u32 current_mip_width = bitmap->width;
        u32 current_mip_height = bitmap->height;

        for (u8 i = 1; i < bitmap->mip_count; ++i)
        {
            size_t prev_mip_bytes = current_mip_width * current_mip_height * sizeof(u32);
            mip_pointer = (u32 *) ADDR_OFFSET(mip_pointer, prev_mip_bytes);

            current_mip_width = current_mip_width >> 1;
            current_mip_height = current_mip_height >> 1;

            sprintf(output_filepath, "%s%s.mip%u.%s", DEBUG_FOLDER, filename, i, "bmp");
            open_file(&output_bmp, output_filepath, "wb");
            export_bitmap(&output_bmp, (u32 *) mip_pointer, current_mip_width, current_mip_height, bitmap->components);
            close_file(&output_bmp);
        }
    }
}

void debug_loaded_bitmapf(char *filepath, loaded_bitmap_t *bitmap)
{
    char *filename = extract_file_name(filepath, 0);
    char *output_filepath = sb_next_buffer(&_string_buffer);
    char *mip_name = sb_next_buffer(&_string_buffer);

    gc_file_t output_bmp;

    // Base bitmap.
    sprintf(output_filepath, "%s%s.%s", DEBUG_FOLDER, filename, "bmp");
    open_file(&output_bmp, output_filepath, "wb");
    export_bitmapf(&output_bmp, (r32 *) bitmap->data, bitmap->width, bitmap->height, bitmap->components);
    close_file(&output_bmp);

    texcolor_t *base_pointer = (texcolor_t *) bitmap->data;
    texcolor_t *mip_pointer = base_pointer;

    // Mipmaps.
    if (bitmap->mip_count)
    {
        u32 current_mip_width = bitmap->width;
        u32 current_mip_height = bitmap->height;

        for (u8 i = 1; i < bitmap->mip_count; ++i)
        {
            size_t prev_mip_bytes = current_mip_width * current_mip_height * sizeof(texcolor_t);
            mip_pointer = (texcolor_t *) ADDR_OFFSET(mip_pointer, prev_mip_bytes);

            current_mip_width = current_mip_width >> 1;
            current_mip_height = current_mip_height >> 1;

            sprintf(output_filepath, "%s%s.mip%u.%s", DEBUG_FOLDER, filename, i, "bmp");
            open_file(&output_bmp, output_filepath, "wb");
            export_bitmapf(&output_bmp, (r32 *) mip_pointer, current_mip_width, current_mip_height, bitmap->components);
            close_file(&output_bmp);
        }
    }
}
#endif
#endif