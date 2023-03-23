// ----------------------------------------------------------------------------------
// -- File: gcsr_pipeline_sse.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-08-15 21:18:11
// -- Modified: 2022-11-07 20:23:59
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

void gl_sse_fragment_varyings_setup_point(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer)
{
    OPTICK_EVENT("gl_sse_fragment_varyings_setup_line");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = &tile_buffer->fragments[fragment->index];

#if 1
        _mm_store_ps(fragment->z, _mm_setzero_ps());
#else
        __m128 fragment_z = _mm_set1_ps(fragment->primitive->base->pos[2]);
        r32 z_check = _mm_movemask_ps(_mm_cmple_ps(fragment_z, screen_z));

        if (!z_check)
        {
            fragment->discarded = true;
            continue;
        }

        _mm_store_ps(fragment->z, fragment_z);
        _mm_store_ps(fragment->shadow, _mm_set1_ps(1.0f));
#endif
    }
}

void gl_sse_fragment_varyings_setup_line(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer)
{
    OPTICK_EVENT("gl_sse_fragment_varyings_setup_line");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = &tile_buffer->fragments[fragment->index];

        __m128 screen_z = _mm_load_ps(screen_fragment->z);
        __m128 interp_z = _mm_set1_ps(fragment->primitive->line.interp_z);
        __m128 fragment_z = _mm_set1_ps(fragment->primitive->base.pos[2]);

        if (fragment->primitive->line.is_dx)
        {
            r32 dx = fragment->x - fragment->primitive->base.pos[0];
            __m128 dx_4x = _mm_setr_ps(dx, dx + 1, dx, dx + 1);
            fragment_z = _mm_add_ps(fragment_z, _mm_mul_ps(dx_4x, interp_z));
        }
        else
        {
            r32 dy = fragment->y - fragment->primitive->base.pos[1];
            __m128 dy_4x = _mm_setr_ps(dy, dy, dy + 1, dy + 1);
            fragment_z = _mm_add_ps(fragment_z, _mm_mul_ps(dy_4x, interp_z));
        }

        r32 z_check = _mm_movemask_ps(_mm_cmple_ps(fragment_z, screen_z));

        if (!z_check)
        {
            fragment->discarded = true;
            continue;
        }

        _mm_store_ps(fragment->z, fragment_z);
        _mm_store_ps(fragment->shadow, _mm_set1_ps(1.0f));
    }
}

void gl_sse_fragment_varyings_setup_triangle(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer)
{
    OPTICK_EVENT("gl_sse_fragment_varyings_setup_triangle");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = &tile_buffer->fragments[fragment->index];

        __m128 dz_4x = _mm_setr_ps(0,
                                   fragment->primitive->triangle.interp_z.x,
                                   fragment->primitive->triangle.interp_z.y,
                                   fragment->primitive->triangle.interp_z.x + fragment->primitive->triangle.interp_z.y);

        r32 dx = fragment->x - fragment->primitive->base.pos[0];
        r32 dy = fragment->y - fragment->primitive->base.pos[1];

        __m128 screen_z_4x = _mm_load_ps(screen_fragment->z);
        __m128 start_z_4x = _mm_set1_ps(fragment->primitive->base.pos[2] + dx * fragment->primitive->triangle.interp_z.x + dy * fragment->primitive->triangle.interp_z.y);
        __m128 fragment_z_4x = _mm_add_ps(start_z_4x, dz_4x);
        __m128 zcmp_4x = _mm_cmple_ps(fragment_z_4x, screen_z_4x);
        u32 z_check = _mm_movemask_ps(zcmp_4x);

        if (!z_check)
        {
            fragment->discarded = true;
            continue;
        }

        _mm_store_ps(fragment->z, fragment_z_4x);
        _mm_store_ps(fragment->shadow, _mm_set1_ps(1.0f));

        if (GCSR.gl->pipeline.varying_count)
        {
            __m128 base_dw_4x = _mm_setr_ps(0,
                                   fragment->primitive->triangle.interp_w.x,
                                   fragment->primitive->triangle.interp_w.y,
                                   fragment->primitive->triangle.interp_w.x + fragment->primitive->triangle.interp_w.y);

            __m128 center_w_4x = _mm_add_ps(_mm_set1_ps(fragment->primitive->base.pos[3] + dx * fragment->primitive->triangle.interp_w.x + dy * fragment->primitive->triangle.interp_w.y), base_dw_4x);
            __m128 left_w_4x = _mm_div_ps(_mm_set1_ps(1.0f), _mm_add_ps(center_w_4x, _mm_set1_ps(fragment->primitive->triangle.interp_w.x)));
            __m128 down_w_4x = _mm_div_ps(_mm_set1_ps(1.0f), _mm_add_ps(center_w_4x, _mm_set1_ps(fragment->primitive->triangle.interp_w.y)));
            // __m128 left_w_4x = _mm_rcp_ps(_mm_add_ps(center_w_4x, _mm_set1_ps(fragment->primitive->triangle.interp_w.x)));
            // __m128 down_w_4x = _mm_rcp_ps(_mm_add_ps(center_w_4x, _mm_set1_ps(fragment->primitive->triangle.interp_w.y)));

            center_w_4x = _mm_div_ps(_mm_set1_ps(1.0f), center_w_4x);

            // Varyings setup.
            for (u16 k = 0; k < GCSR.gl->pipeline.varying_count; ++k)
            {
                __m128 center_dvar_4x = _mm_setr_ps(0,
                                        fragment->primitive->triangle.interp_varying[k].x,
                                        fragment->primitive->triangle.interp_varying[k].y,
                                        fragment->primitive->triangle.interp_varying[k].x + fragment->primitive->triangle.interp_varying[k].y);

                __m128 center_var_4x = _mm_add_ps(_mm_set1_ps(fragment->primitive->base.data[k] + dx * fragment->primitive->triangle.interp_varying[k].x + dy * fragment->primitive->triangle.interp_varying[k].y), center_dvar_4x);
                __m128 base_fragment_var_4x = _mm_mul_ps(center_var_4x, center_w_4x);
                _mm_store_ps(fragment->varyings[k], base_fragment_var_4x);

                if (k == 0)
                {
                    __m128 left_var_4x = _mm_mul_ps(_mm_add_ps(center_var_4x, _mm_set1_ps(fragment->primitive->triangle.interp_varying[k].x)), left_w_4x);
                    __m128 down_var_4x = _mm_mul_ps(_mm_add_ps(center_var_4x, _mm_set1_ps(fragment->primitive->triangle.interp_varying[k].y)), down_w_4x);

                    _mm_store_ps(fragment->dudx, _mm_sub_ps(left_var_4x, base_fragment_var_4x));
                    _mm_store_ps(fragment->dudy, _mm_sub_ps(down_var_4x, base_fragment_var_4x));
                }
                else if (k == 1)
                {
                    __m128 left_var_4x = _mm_mul_ps(_mm_add_ps(center_var_4x, _mm_set1_ps(fragment->primitive->triangle.interp_varying[k].x)), left_w_4x);
                    __m128 down_var_4x = _mm_mul_ps(_mm_add_ps(center_var_4x, _mm_set1_ps(fragment->primitive->triangle.interp_varying[k].y)), down_w_4x);

                    _mm_store_ps(fragment->dvdx, _mm_sub_ps(left_var_4x, base_fragment_var_4x));
                    _mm_store_ps(fragment->dvdy, _mm_sub_ps(down_var_4x, base_fragment_var_4x));
                }
            }
        }
    }
}

__INLINE__ void _sse_fragment_merge(gc_fragments_array_t *fragments_array,
                                    gc_tile_buffer_t *tile_buffer,
                                    gc_transparency_bin_t *transparency_buffer)
{
    OPTICK_EVENT("_sse_fragment_merge");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;

    b8 is_shadow = PIPE_FLAG(GC_SHADOW_PASS);
    b8 is_transparency = PIPE_FLAG(GC_TRANSPARENCY);

    pipe_param_merged_table_t *overwrites = &GCSR.gl->pipeline.params.overwrites;
    r32 forced_opacity = overwrites->forced_opacity[2]->u_float;

    __m128 base_mask_4x = _mm_castsi128_ps(
                            _mm_setr_epi32(GL_FRAG_MASK_PIXEL0,
                                           GL_FRAG_MASK_PIXEL1,
                                           GL_FRAG_MASK_PIXEL2,
                                           GL_FRAG_MASK_PIXEL3));

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = &tile_buffer->fragments[fragment->index];

        if (fragment->discarded)
            continue;

        __m128 fragment_mask_4x = _mm_castsi128_ps(_mm_set1_epi32(fragment->mask));
        __m128 fragment_z_4x = _mm_load_ps(fragment->z);
        __m128 screen_z_4x = _mm_load_ps(screen_fragment->z);

        __m128 final_mask_4x = _mm_castsi128_ps(_mm_cmpgt_epi32(_mm_castps_si128(_mm_and_ps(fragment_mask_4x, base_mask_4x)), _mm_setzero_si128()));
        final_mask_4x = _mm_and_ps(final_mask_4x, _mm_cmple_ps(fragment_z_4x, screen_z_4x));

        if (is_shadow)
        {
            __m128 screen_color_r = _mm_load_ps(screen_fragment->r);
            __m128 screen_color_g = _mm_load_ps(screen_fragment->g);

            __m128 fragment_color_r = _mm_load_ps(fragment->r);
            __m128 fragment_color_g = _mm_load_ps(fragment->g);

            screen_color_r = _mm_or_ps(
                                _mm_and_ps(final_mask_4x, fragment_color_r),
                                _mm_andnot_ps(final_mask_4x, screen_color_r));

            screen_color_g = _mm_or_ps(
                                _mm_and_ps(final_mask_4x, fragment_color_g),
                                _mm_andnot_ps(final_mask_4x, screen_color_g));

            screen_z_4x = _mm_or_ps(
                            _mm_and_ps(final_mask_4x, fragment_z_4x),
                            _mm_andnot_ps(final_mask_4x, screen_z_4x));

            _mm_store_ps(screen_fragment->r, screen_color_r);
            _mm_store_ps(screen_fragment->g, screen_color_g);
            _mm_store_ps(screen_fragment->z, screen_z_4x);
        }
        else
        {
            sse_color_t fragment_color;
            sse_color_t screen_color;

            fragment_color.r = _mm_load_ps(fragment->r);
            fragment_color.g = _mm_load_ps(fragment->g);
            fragment_color.b = _mm_load_ps(fragment->b);
            fragment_color.a = _mm_load_ps(fragment->a);

            screen_color.r = _mm_load_ps(screen_fragment->r);
            screen_color.g = _mm_load_ps(screen_fragment->g);
            screen_color.b = _mm_load_ps(screen_fragment->b);

            if (is_transparency)
            {
                if (forced_opacity < 1.0f)
                {
                    __m128 forced_opacity_4x = _mm_set1_ps(forced_opacity);

                    fragment_color.r = _mm_mul_ps(fragment_color.r, forced_opacity_4x);
                    fragment_color.g = _mm_mul_ps(fragment_color.g, forced_opacity_4x);
                    fragment_color.b = _mm_mul_ps(fragment_color.b, forced_opacity_4x);
                    fragment_color.a = _mm_mul_ps(fragment_color.a, forced_opacity_4x);
                }

                gc_transparency_frag_t *transparency_frag = transparency_buffer->frags + fragment->index;
                __m128 transparent_mask = _mm_and_ps(_mm_cmplt_ps(fragment_color.a, _mm_set1_ps(1.0f)), final_mask_4x);
                final_mask_4x = _mm_andnot_ps(transparent_mask, final_mask_4x);

                transparency_buffer->dirty = true;

                transparent_mask = _mm_and_ps(transparent_mask, base_mask_4x);
                s32 tmask = _mm_extract_ps(transparent_mask, 0) |
                            _mm_extract_ps(transparent_mask, 1) |
                            _mm_extract_ps(transparent_mask, 2) |
                            _mm_extract_ps(transparent_mask, 3);

                _mm_store_ps(fragment->r, fragment_color.r);
                _mm_store_ps(fragment->g, fragment_color.g);
                _mm_store_ps(fragment->b, fragment_color.b);
                _mm_store_ps(fragment->a, fragment_color.a);

                gl_pipe_transparency_push(transparency_frag, fragment, tmask);
            }

            screen_color.r = _mm_or_ps(
                                _mm_and_ps(final_mask_4x, fragment_color.r),
                                _mm_andnot_ps(final_mask_4x, screen_color.r));

            screen_color.g = _mm_or_ps(
                                _mm_and_ps(final_mask_4x, fragment_color.g),
                                _mm_andnot_ps(final_mask_4x, screen_color.g));

            screen_color.b = _mm_or_ps(
                                _mm_and_ps(final_mask_4x, fragment_color.b),
                                _mm_andnot_ps(final_mask_4x, screen_color.b));

            screen_z_4x = _mm_or_ps(
                            _mm_and_ps(final_mask_4x, fragment_z_4x),
                            _mm_andnot_ps(final_mask_4x, screen_z_4x));

            _mm_store_ps(screen_fragment->r, screen_color.r);
            _mm_store_ps(screen_fragment->g, screen_color.g);
            _mm_store_ps(screen_fragment->b, screen_color.b);
            _mm_store_ps(screen_fragment->z, screen_z_4x);
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Transparency processing routine.
// ----------------------------------------------------------------------------------

__INLINE__ void _sse_transparency_process(u32 thread_id)
{
    OPTICK_EVENT("_sse_transparency_process");

    gc_framebuffer_t *current_framebuffer = GET_FRAMEBUFFER();

    while (true)
    {
        u16 bin_index = SDL_AtomicAdd(&current_framebuffer->transparency_cursor, 1);

        if (bin_index >= current_framebuffer->total_bins)
            break;

        gc_bin_t *current_bin = current_framebuffer->bins + bin_index;
        gc_transparency_bin_t *tbin = current_framebuffer->transparency + bin_index;
        gc_tile_buffer_t *tile_buffer = current_framebuffer->lsb.tiles + bin_index;

        if (!tbin->dirty)
            continue;

        tile_buffer->x = current_bin->x;
        tile_buffer->y = current_bin->y;

        sse_color_t computed_color;
        sse_color_t screen_color;
        sse_color_t stack_color;

        for (u16 j = 0; j < GL_BIN_FRAGS; ++j)
        {
            gc_transparency_frag_t *frag = tbin->frags + j;
            gc_fragment_t *screen_fragment = tile_buffer->fragments + j;

            if (frag->count[0] || frag->count[1] || frag->count[2] || frag->count[3])
            {
                computed_color.r = _mm_setzero_ps();
                computed_color.g = _mm_setzero_ps();
                computed_color.b = _mm_setzero_ps();
                computed_color.a = _mm_setzero_ps();

                screen_color.r = _mm_load_ps(screen_fragment->r);
                screen_color.g = _mm_load_ps(screen_fragment->g);
                screen_color.b = _mm_load_ps(screen_fragment->b);

                __m128i count = _mm_setr_epi32(frag->count[0], frag->count[1], frag->count[2], frag->count[3]);
                __m128 frag_z = _mm_load_ps(frag->z[0]);
                __m128 screen_z = _mm_load_ps(screen_fragment->z);

                __m128 mask = _mm_and_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(count, _mm_setzero_si128())), _mm_cmple_ps(frag_z, screen_z));
                __m128 stack_mask = mask;
                __m128 blended_z = _mm_or_ps(_mm_and_ps(mask, frag_z), _mm_andnot_ps(mask, screen_z));

                // -- Process the frag transparency stack.

                for (u16 k = 0; k < MAX_TRANSPARENCY_STACK; ++k)
                {
                    stack_color.r = _mm_load_ps(frag->stack_r[k]);
                    stack_color.g = _mm_load_ps(frag->stack_g[k]);
                    stack_color.b = _mm_load_ps(frag->stack_b[k]);
                    stack_color.a = _mm_load_ps(frag->stack_a[k]);

                    __m128 stack_frag_z = _mm_load_ps(frag->z[k]);
                    __m128 interp = _mm_sub_ps(_mm_set1_ps(1.0f), computed_color.a);

                    stack_mask = _mm_and_ps(stack_mask, _mm_cmple_ps(stack_frag_z, screen_z));

                    computed_color.r = _mm_add_ps(computed_color.r, _mm_mul_ps(_mm_and_ps(stack_color.r, stack_mask), interp));
                    computed_color.g = _mm_add_ps(computed_color.g, _mm_mul_ps(_mm_and_ps(stack_color.g, stack_mask), interp));
                    computed_color.b = _mm_add_ps(computed_color.b, _mm_mul_ps(_mm_and_ps(stack_color.b, stack_mask), interp));
                    computed_color.a = _mm_add_ps(computed_color.a, _mm_mul_ps(_mm_and_ps(stack_color.a, stack_mask), interp));
                }

                // -- Final blend with the bin_frag color.

                __m128 interp = _mm_sub_ps(_mm_set1_ps(1.0f), computed_color.a);

                computed_color.r = _mm_add_ps(computed_color.r, _mm_and_ps(mask, _mm_mul_ps(screen_color.r, interp)));
                computed_color.g = _mm_add_ps(computed_color.g, _mm_and_ps(mask, _mm_mul_ps(screen_color.g, interp)));
                computed_color.b = _mm_add_ps(computed_color.b, _mm_and_ps(mask, _mm_mul_ps(screen_color.b, interp)));

                // -- Add the unmodified screen colors.

                computed_color.r = _mm_or_ps(_mm_and_ps(mask, computed_color.r), _mm_andnot_ps(mask, screen_color.r));
                computed_color.g = _mm_or_ps(_mm_and_ps(mask, computed_color.g), _mm_andnot_ps(mask, screen_color.g));
                computed_color.b = _mm_or_ps(_mm_and_ps(mask, computed_color.b), _mm_andnot_ps(mask, screen_color.b));

                // -- Write to the bin.

                _mm_store_ps(screen_fragment->r, computed_color.r);
                _mm_store_ps(screen_fragment->g, computed_color.g);
                _mm_store_ps(screen_fragment->b, computed_color.b);
                _mm_store_ps(screen_fragment->z, blended_z);
            }
        }

        memset(tbin, 0, sizeof(gc_transparency_bin_t));
        tbin->dirty = false;
    }
}