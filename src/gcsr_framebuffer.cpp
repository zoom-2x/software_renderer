// ----------------------------------------------------------------------------------
// -- File: gcsr_framebuffer.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-08-15 13:55:49
// -- Modified: 2022-08-15 13:55:50
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_AVX)
#include "simd/gcsr_avx_framebuffer.cpp"
#define gc_framebuffer_clear _sse_framebuffer_clear
#elif defined(GC_PIPE_SSE)
#include "simd/gcsr_sse_framebuffer.cpp"
#define gc_framebuffer_clear _sse_framebuffer_clear
#else
#define gc_framebuffer_clear _framebuffer_clear
#endif

__INLINE__ void clear_set(gc_vec_t color)
{
    gc_framebuffer_t *framebuffer = GET_FRAMEBUFFER();
    framebuffer->lsb.clear_color = color;
}

void _framebuffer_clear(u32 thread_id)
{
    OPTICK_EVENT("_framebuffer_clear");

    gc_framebuffer_t *current_framebuffer = GET_FRAMEBUFFER();
    gc_vec_t color = current_framebuffer->lsb.clear_color;

    while (true)
    {
        u16 tile_index = SDL_AtomicAdd(&current_framebuffer->lsb.cursor, 1);

        if (tile_index >= current_framebuffer->total_bins)
            break;

        gc_tile_buffer_t *selected_tile = current_framebuffer->lsb.tiles + tile_index;
        gc_fragment_t *fragment = selected_tile->fragments;

        for (u32 i = 0; i < GL_BIN_FRAGS; ++i)
        {
            fragment->r[0] = color.c.r;
            fragment->r[1] = color.c.r;
            fragment->r[2] = color.c.r;
            fragment->r[3] = color.c.r;

            fragment->g[0] = color.c.g;
            fragment->g[1] = color.c.g;
            fragment->g[2] = color.c.g;
            fragment->g[3] = color.c.g;

            fragment->b[0] = color.c.b;
            fragment->b[1] = color.c.b;
            fragment->b[2] = color.c.b;
            fragment->b[3] = color.c.b;

            fragment->z[0] = 1;
            fragment->z[1] = 1;
            fragment->z[2] = 1;
            fragment->z[3] = 1;

            fragment++;
        }
    }
}

void lsb_to_texture(u32 thread_id)
{
    gc_framebuffer_t *current_framebuffer = GET_FRAMEBUFFER();

    while (true)
    {
        u16 tile_index = SDL_AtomicAdd(&current_framebuffer->lsb.cursor, 1);

        if (tile_index >= current_framebuffer->total_bins)
            break;

        gc_tile_buffer_t *selected_tile = current_framebuffer->lsb.tiles + tile_index;
        GCSR.gl->current_framebuffer->save(GCSR.gl->current_framebuffer, selected_tile);
    }
}

gc_framebuffer_t *gc_create_framebuffer(u32 width, u32 height, u32 flags, u32 slot)
{
    u32 tiled_width = MULTIPLE_OF(width, GL_BIN_WIDTH);
    u32 tiled_height = MULTIPLE_OF(height, GL_BIN_HEIGHT);
    // u32 total_tiled_pixels = tiled_width * tiled_height;

    u16 bin_rows = tiled_height / GL_BIN_HEIGHT;
    u16 bin_cols = tiled_width / GL_BIN_WIDTH;
    u16 total_bins = bin_rows * bin_cols;

    // size_t depth_memory_bytes = MEM_SIZE_ALIGN(sizeof(r32) * total_tiled_pixels);
    size_t bins_bytes = sizeof(gc_bin_t) * total_bins;
    size_t transparency_bytes = sizeof(gc_transparency_bin_t) * total_bins;
    size_t lsb_bytes = sizeof(gc_tile_buffer_t) * total_bins;

    if (!(flags & FB_FLAG_TRANSPARENCY))
        transparency_bytes = 0;

    size_t framebuffer_bytes = sizeof(gc_framebuffer_t);

    if (FLAG(flags, FB_FLAG_LSB))
    {
        framebuffer_bytes += bins_bytes + lsb_bytes;

        if (FLAG(flags, FB_FLAG_TRANSPARENCY))
            framebuffer_bytes += transparency_bytes;
    }

    mem_set_chunk(MEMORY_TEMPORARY);
    MEM_LABEL("framebuffer");
    gc_framebuffer_t *framebuffer = (gc_framebuffer_t *) gc_mem_allocate(framebuffer_bytes);
    mem_restore_chunk();

    if (FLAG(flags, FB_FLAG_LSB))
    {
        framebuffer->bins = (gc_bin_t *) ADDR_OFFSET(framebuffer, sizeof(gc_framebuffer_t));
        framebuffer->lsb.tiles = (gc_tile_buffer_t *) ADDR_OFFSET(framebuffer->bins, bins_bytes);
        framebuffer->lsb.bytes = lsb_bytes;

        if (FLAG(flags, FB_FLAG_TRANSPARENCY))
            framebuffer->transparency = (gc_transparency_bin_t *) ADDR_OFFSET(framebuffer->lsb.tiles, lsb_bytes);
    }

    // ----------------------------------------------------------------------------------
    // -- Framebuffer setup.
    // ----------------------------------------------------------------------------------

    if (GCSR.state->GL.framebuffers[slot])
        gc_mem_free(GCSR.state->GL.framebuffers[slot]);
    else
        GCSR.state->GL.framebuffer_count++;

    GCSR.state->GL.framebuffers[slot] = framebuffer;

    framebuffer->color = 0;

    framebuffer->bytes = framebuffer_bytes;
    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->aspect = (r32) width / height;

    framebuffer->flags = flags;
    framebuffer->bin_rows = bin_rows;
    framebuffer->bin_cols = bin_cols;
    framebuffer->total_bins = total_bins;
    framebuffer->total_bins = total_bins;

    framebuffer->frag_rows = MULTIPLE_OF(height, GL_FRAG_HEIGHT) >> GL_FRAG_HEIGHT_SHIFT;
    framebuffer->frag_cols = MULTIPLE_OF(width, GL_FRAG_WIDTH) >> GL_FRAG_WIDTH_SHIFT;

    framebuffer->tiled_width = tiled_width;
    framebuffer->tiled_height = tiled_height;

    SDL_AtomicSet(&framebuffer->bin_cursor, 0);
    SDL_AtomicSet(&framebuffer->transparency_cursor, 0);

    // ----------------------------------------------------------------------------------
    // -- LSB setup.
    // ----------------------------------------------------------------------------------

    if (FLAG(flags, FB_FLAG_LSB))
    {
        u32 idx = 0;

        for (u32 row = 0; row < bin_rows; ++row)
        {
            for (u32 col = 0; col < bin_cols; ++col)
            {
                gc_tile_buffer_t *tile = framebuffer->lsb.tiles + idx++;

                tile->x = col * GL_BIN_WIDTH;
                tile->y = row * GL_BIN_HEIGHT;
            }
        }

        // ----------------------------------------------------------------------------------
        // -- Bin allocation.
        // ----------------------------------------------------------------------------------

        idx = 0;

        for (u32 row = 0; row < bin_rows; ++row)
        {
            for (u32 col = 0; col < bin_cols; ++col)
            {
                gc_bin_t *bin = framebuffer->bins + idx;

                bin->x = col * GL_BIN_WIDTH;
                bin->y = row * GL_BIN_HEIGHT;

                for (u32 k = 0; k < GC_PIPE_NUM_THREADS; ++k)
                {
                    bin->list[k].start = 0;
                    bin->list[k].last = 0;
                    bin->list[k].count = 0;
                }

                idx++;
                SDL_AtomicSet(&bin->dirty, 0);
            }
        }

        idx = 0;

        if (FLAG(flags, FB_FLAG_TRANSPARENCY))
        {
            for (u32 row = 0; row < bin_rows; ++row)
            {
                for (u32 col = 0; col < bin_cols; ++col)
                {
                    gc_transparency_bin_t *tbin = framebuffer->transparency + idx;

                    tbin->x = col * GL_BIN_WIDTH;
                    tbin->y = row * GL_BIN_HEIGHT;

                    idx++;
                }
            }
        }
    }

    return framebuffer;
}

void gc_delete_framebuffer(u32 slot)
{
    if (GCSR.state->GL.framebuffers[slot])
    {
        // TODO(gabic): De terminat.
    }
}

void gc_framebuffer_attach_color(u32 slot, texture2d_t *texture)
{
    if (!GCSR.state->GL.framebuffers[slot])
        return;

    if (texture)
    {
        gc_framebuffer_t *framebuffer = GCSR.state->GL.framebuffers[slot];
        framebuffer->color = texture;

        SDL_assert(texture->mips->header->width == framebuffer->tiled_width &&
                   texture->mips->header->height == framebuffer->tiled_height);

        if (texture->settings.format == TEXTURE_FORMAT_RGBAU8)
        {
            #if defined(GC_PIPE_AVX)
            // framebuffer->load = load_tile_buffer_rgbau8;
            framebuffer->save = save_tile_buffer_rgbau8;
            #elif defined(GC_PIPE_SSE)
            // framebuffer->load = load_tile_buffer_rgbau8;
            framebuffer->save = sse_save_tile_buffer_rgbau8;
            #else
            // framebuffer->load = load_tile_buffer_rgbau8;
            framebuffer->save = save_tile_buffer_rgbau8;
            #endif
        }
        else if (texture->settings.format == TEXTURE_FORMAT_RGBAF)
        {
            #if defined(GC_PIPE_AVX)
            // framebuffer->load = load_tile_buffer_rgbaf;
            framebuffer->save = save_tile_buffer_rgbaf;
            #elif defined(GC_PIPE_SSE)
            // framebuffer->load = load_tile_buffer_rgbaf;
            framebuffer->save = save_tile_buffer_rgbaf;
            #else
            // framebuffer->load = load_tile_buffer_rgbaf;
            framebuffer->save = save_tile_buffer_rgbaf;
            #endif
        }
        else if (texture->settings.format == TEXTURE_FORMAT_RGBF)
        {
            #if defined(GC_PIPE_AVX)
            // framebuffer->load = load_tile_buffer_rgbf;
            framebuffer->save = save_tile_buffer_rgbf;
            #elif defined(GC_PIPE_SSE)
            // framebuffer->load = load_tile_buffer_rgbf;
            framebuffer->save = save_tile_buffer_rgbf;
            #else
            // framebuffer->load = load_tile_buffer_rgbf;
            framebuffer->save = save_tile_buffer_rgbf;
            #endif
        }
        else if (texture->settings.format == TEXTURE_FORMAT_RGF)
        {
            #if defined(GC_PIPE_AVX)
            // framebuffer->load = load_tile_buffer_rgf;
            framebuffer->save = save_tile_buffer_rgf;
            #elif defined(GC_PIPE_SSE)
            // framebuffer->load = load_tile_buffer_rgf;
            framebuffer->save = save_tile_buffer_rgf;
            #else
            // framebuffer->load = load_tile_buffer_rgf;
            framebuffer->save = save_tile_buffer_rgf;
            #endif
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Copy the specified framebuffer to the main framebuffer.
// ----------------------------------------------------------------------------------

void _copy_texture2d_to_framebuffer_rgbau8(texture2d_t *texture)
{
    gc_framebuffer_t *main_framebuffer = GCSR.gl->framebuffers[0];
    texture2d_t *dst_texture = main_framebuffer->color;

    u32 src_width = texture->mips->header->width;
    u32 src_height = texture->mips->header->height;
    u32 *src_pointer = (u32 *) texture->mips->data;

    u32 dst_width = dst_texture->mips->header->width;
    u32 dst_height = dst_texture->mips->header->height;
    u32 *dst_pointer = (u32 *) dst_texture->mips->data;

    for (u32 y = 0; y < dst_height; ++y)
    {
        r32 v = (r32) y / dst_height;

        for (u32 x = 0; x < dst_width; ++x)
        {
            r32 u = (r32) x / dst_width;

            u32 src_texel_x = (u32) (u * (src_width - 1));
            u32 src_texel_y = (u32) (v * (src_height - 1));
            u32 src_offset = src_texel_y * (src_width) + src_texel_x;

            u32 *src_pixel = src_pointer + src_offset;
            *dst_pointer++ = *src_pixel;
        }
    }
}

void _copy_texture2d_to_framebuffer_rgbaf(texture2d_t *texture)
{
    gc_framebuffer_t *main_framebuffer = GCSR.gl->framebuffers[0];
    texture2d_t *dst_texture = main_framebuffer->color;

    u32 src_width = texture->mips->header->width - 1;
    u32 src_height = texture->mips->header->height - 1;
    texpixel_rgbaf_t *src_pointer = (texpixel_rgbaf_t *) texture->mips->data;

    u32 dst_width = dst_texture->mips->header->width - 1;
    u32 dst_height = dst_texture->mips->header->height - 1;
    u32 *dst_pointer = (u32 *) dst_texture->mips->data;

    for (u32 y = 0; y < dst_height; ++y)
    {
        r32 v = (r32) y / dst_height;

        for (u32 x = 0; x < dst_width; ++x)
        {
            r32 u = (r32) x / dst_width;

            u32 src_texel_x = (u32) (u * src_width);
            u32 src_texel_y = (u32) (v * src_height);
            u32 src_offset = src_texel_y * (src_width + 1) + src_texel_x;

            // The color is already tone-mapped and gamma-corrected.
            texpixel_rgbaf_t *src_pixel = src_pointer + src_offset;

            *dst_pointer++ = ((u32) (src_pixel->r * 255)) << GL_PIXEL_FORMAT_RED_SHIFT |
                             ((u32) (src_pixel->g * 255)) << GL_PIXEL_FORMAT_GREEN_SHIFT |
                             ((u32) (src_pixel->b * 255)) << GL_PIXEL_FORMAT_BLUE_SHIFT |
                             0xff << GL_PIXEL_FORMAT_ALPHA_SHIFT;
        }
    }
}

void _copy_texture2d_to_framebuffer_rgbf(texture2d_t *texture)
{}

void _copy_texture2d_to_framebuffer_rgf(texture2d_t *texture)
{
    gc_framebuffer_t *main_framebuffer = GCSR.gl->framebuffers[0];
    texture2d_t *dst_texture = main_framebuffer->color;

    u32 src_width = texture->mips->header->width - 1;
    u32 src_height = texture->mips->header->height - 1;
    texpixel_rgf_t *src_pointer = (texpixel_rgf_t *) texture->mips->data;

    u32 dst_width = dst_texture->mips->header->width;
    u32 dst_height = dst_texture->mips->header->height;
    u32 *dst_pointer = (u32 *) dst_texture->mips->data;

    for (u32 y = 0; y < dst_height; ++y)
    {
        r32 v = (r32) y / (dst_height - 1);

        for (u32 x = 0; x < dst_width; ++x)
        {
            r32 u = (r32) x / (dst_width - 1);

            u32 src_texel_x = (u32) (u * src_width);
            u32 src_texel_y = (u32) (v * src_height);
            u32 src_offset = src_texel_y * (src_width + 1) + src_texel_x;

            // NOTE(gabic): For now this format is used for the shadow depth buffer.
            texpixel_rgf_t *src_pixel = src_pointer + src_offset;

            // if (src_pixel->r == 1)
            //     src_pixel->r = 0;

            u32 channel = (u32) (src_pixel->r * 255);
            *dst_pointer++ = (channel << GL_PIXEL_FORMAT_RED_SHIFT) |
                             (channel << GL_PIXEL_FORMAT_GREEN_SHIFT) |
                             (channel << GL_PIXEL_FORMAT_BLUE_SHIFT) |
                             (0xff << GL_PIXEL_FORMAT_ALPHA_SHIFT);

            // u8 a = 0;
        }
    }
}

__INLINE__ void gc_copy_framebuffer(gc_framebuffer_t *framebuffer)
{
    gc_framebuffer_t *main_framebuffer = GCSR.gl->framebuffers[0];

    if (framebuffer != main_framebuffer)
    {
        texture2d_t *src_texture = framebuffer->color;

        if (src_texture->settings.format == TEXTURE_FORMAT_RGBAU8)
            _copy_texture2d_to_framebuffer_rgbau8(framebuffer->color);
        if (src_texture->settings.format == TEXTURE_FORMAT_RGBAF)
            _copy_texture2d_to_framebuffer_rgbaf(framebuffer->color);
        else if (src_texture->settings.format == TEXTURE_FORMAT_RGBF)
            _copy_texture2d_to_framebuffer_rgbf(framebuffer->color);
        else if (src_texture->settings.format == TEXTURE_FORMAT_RGF)
            _copy_texture2d_to_framebuffer_rgf(framebuffer->color);
        // else if (src_texture->settings.format == TEXTURE_FORMAT_SHADOW)
        //     _copy_texture2d_to_framebuffer_shadow(framebuffer->color);
    }
}