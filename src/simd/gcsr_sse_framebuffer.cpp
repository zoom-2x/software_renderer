// ----------------------------------------------------------------------------------
// -- File: gcsr_sse_framebuffer.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-10-06 09:39:19
// -- Modified: 2022-10-06 09:39:20
// ----------------------------------------------------------------------------------

void _sse_framebuffer_clear(u32 thread_id)
{
    OPTICK_EVENT("_sse_framebuffer_clear");

    gc_framebuffer_t *current_framebuffer = GET_FRAMEBUFFER();
    gc_vec_t color = current_framebuffer->lsb.clear_color;

    __m128 r_4x = _mm_set1_ps(color.c.r);
    __m128 g_4x = _mm_set1_ps(color.c.g);
    __m128 b_4x = _mm_set1_ps(color.c.b);

    while (true)
    {
        u16 tile_index = SDL_AtomicAdd(&current_framebuffer->lsb.cursor, 1);

        if (tile_index >= current_framebuffer->total_bins)
            break;

        gc_tile_buffer_t *selected_tile = current_framebuffer->lsb.tiles + tile_index;
        gc_fragment_t *fragment = selected_tile->fragments;

        for (u32 i = 0; i < GL_BIN_FRAGS; ++i)
        {
            _mm_store_ps(fragment->r, r_4x);
            _mm_store_ps(fragment->g, g_4x);
            _mm_store_ps(fragment->b, b_4x);
            _mm_store_ps(fragment->z, _mm_set1_ps(1.0f));

            fragment++;
        }
    }
}

void _sse_copy_texture2d_to_framebuffer_rgbau8(texture2d_t *texture)
{}

void _sse_copy_texture2d_to_framebuffer_rgbaf(texture2d_t *texture)
{}

void _sse_copy_texture2d_to_framebuffer_rgf(texture2d_t *texture)
{}