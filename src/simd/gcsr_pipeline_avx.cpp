// ----------------------------------------------------------------------------------
// -- File: gcsr_pipeline_avx.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-08-15 22:01:07
// -- Modified:
// ----------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------
// -- Pixel shading routine.
// ----------------------------------------------------------------------------------

#ifndef GCSR_ROUTINE_gl_pipe_pixel_shading
#define GCSR_ROUTINE_gl_pipe_pixel_shading

void gl_pipe_pixel_shading(u32 threadId, gl_pixel_buffers_t *PixelBuffers, gl_program_t *Program, GlUniforms *uniforms)
{
    DEBUG_RM_Start(__DebugApi__, 4);

    gl_fragpack_t *Pack = PixelBuffers->Fragments[threadId];

    b8 use_global_color = global_gl->Config.flags & (GL_UNIF_MESH_COLOR | GL_UNIF_WIREFRAME_COLOR);
    u32 useSPW = (global_gl->Config.flags & GL_SPW) && global_gl->Config.Wireframe;

    if (use_global_color)
        return;

    r32 d1_8x[8];
    r32 d2_8x[8];
    r32 d3_8x[8];

    __m256 one_8x = _mm256_set1_ps(1.0f);
    __m256i xincr_8x = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);

    // -- Process each fragpack.
    // ----------------------------------------------------------------------------------

    for (u32 i = 0; i < GL_FRAGPACK_COUNT; ++i)
    {
        gl_fragpack_t *current_pack = Pack + i;

        if (!current_pack->frag_count)
            break;

        if (!use_global_color)
            Program->fs(current_pack, uniforms);
        else
        {}

        // -- If the mesh has a flat color specified (alpha > 0.0f) then
        // -- bypass the pixel shader.
#if 0
        if (!use_global_color)
        {
            gc_primitive_t *primitive = Fragment->primitive;
            GlFragmentShaderPack FragmentData;

            gc_vertex_t *v1 = primitive->v1;
            gc_vertex_t *v2 = primitive->v2;
            gc_vertex_t *v3 = primitive->v3;

            vec4 v1Pos = v1->pos;
            vec4 v2Pos = v2->pos;
            vec4 v3Pos = v3->pos;

            __m256 v1w_8x = _mm256_set1_ps(v1Pos.w);
            __m256 v2w_8x = _mm256_set1_ps(v2Pos.w);
            __m256 v3w_8x = _mm256_set1_ps(v3Pos.w);

            __ALIGN__ s32 xstart[8] = {
                Fragment->sx,
                Fragment->sx + 1,
                Fragment->sx + 2,
                Fragment->sx + 3,
                Fragment->sx + 4,
                Fragment->sx + 5,
                Fragment->sx + 6,
                Fragment->sx + 7
            };

            __m256 v1_uv_u_8x = _mm256_set1_ps(v1->uv.u);
            __m256 v1_uv_v_8x = _mm256_set1_ps(v1->uv.v);

            __m256 v2_uv_u_8x = _mm256_set1_ps(v2->uv.u);
            __m256 v2_uv_v_8x = _mm256_set1_ps(v2->uv.v);

            __m256 v3_uv_u_8x = _mm256_set1_ps(v3->uv.u);
            __m256 v3_uv_v_8x = _mm256_set1_ps(v3->uv.v);

            __m256 l1_8x = _mm256_load_ps(Fragment->l1_8x);
            __m256 l2_8x = _mm256_load_ps(Fragment->l2_8x);
            __m256 l3_8x = _mm256_load_ps(Fragment->l3_8x);

            __m256i pos_x_8x = _mm256_load_si256((__m256i *) xstart);
            __m256i pos_y_8x = _mm256_set1_epi32(Fragment->sy);

            // -- Attribute interpolation.

            __m256 one_over_w_8x = _mm256_div_ps(
                                        one_8x,
                                        _mm256_add_ps(
                                            _mm256_add_ps(
                                                _mm256_mul_ps(l1_8x, v1w_8x),
                                                _mm256_mul_ps(l2_8x, v2w_8x)),
                                            _mm256_mul_ps(l3_8x, v3w_8x)));

            __m256 uv_u_8x = _mm256_mul_ps(
                                        one_over_w_8x,
                                        _mm256_add_ps(
                                            _mm256_add_ps(
                                                _mm256_mul_ps(l1_8x, v1_uv_u_8x),
                                                _mm256_mul_ps(l2_8x, v2_uv_u_8x)),
                                            _mm256_mul_ps(l3_8x, v3_uv_u_8x)));

            __m256 uv_v_8x = _mm256_mul_ps(
                                        one_over_w_8x,
                                        _mm256_add_ps(
                                            _mm256_add_ps(
                                                _mm256_mul_ps(l1_8x, v1_uv_v_8x),
                                                _mm256_mul_ps(l2_8x, v2_uv_v_8x)),
                                            _mm256_mul_ps(l3_8x, v3_uv_v_8x)));

            _mm256_store_si256((__m256i *) FragmentData.pos_x_8x, pos_x_8x);
            _mm256_store_si256((__m256i *) FragmentData.pos_y_8x, pos_y_8x);

            _mm256_store_ps(FragmentData.uv_u_8x, uv_u_8x);
            _mm256_store_ps(FragmentData.uv_v_8x, uv_v_8x);

            FragmentData.outputr_8x = Fragment->colorr_8x;
            FragmentData.outputg_8x = Fragment->colorg_8x;
            FragmentData.outputb_8x = Fragment->colorb_8x;
            FragmentData.outputa_8x = Fragment->colora_8x;

            Program->fs(&FragmentData, uniforms);

            if (useSPW)
            {
                __m256 spw_d1_8x = _mm256_set1_ps(primitive->triangle.spw_d1);
                __m256 spw_d2_8x = _mm256_set1_ps(primitive->triangle.spw_d2);
                __m256 spw_d3_8x = _mm256_set1_ps(primitive->triangle.spw_d3);

                __m256 d1 = _mm256_mul_ps(spw_d1_8x, l1_8x);
                __m256 d2 = _mm256_mul_ps(spw_d2_8x, l2_8x);
                __m256 d3 = _mm256_mul_ps(spw_d3_8x, l3_8x);

                _mm256_store_ps((r32 *) &d1_8x, d1);
                _mm256_store_ps((r32 *) &d2_8x, d2);
                _mm256_store_ps((r32 *) &d3_8x, d3);

                gl_single_pass_wireframe_filter(
                            global_gl->Config.Wireframe,
                            v1Pos, v2Pos, v3Pos,
                            d1_8x, d2_8x, d3_8x,
                            FragmentData.pos_x_8x, FragmentData.pos_y_8x,
                            Fragment->colorr_8x, Fragment->colorg_8x, Fragment->colorb_8x, Fragment->colora_8x);
            }

            if (use_global_opacity)
            {
                __m256 a_8x = _mm256_set1_ps(global_opacity);
                _mm256_store_ps(Fragment->colora_8x, a_8x);
            }
        }
        else
        {
            __m256 r_8x = _mm256_set1_ps(global_color.r);
            __m256 g_8x = _mm256_set1_ps(global_color.g);
            __m256 b_8x = _mm256_set1_ps(global_color.b);
            __m256 a_8x = _mm256_set1_ps(global_color.a);

            _mm256_store_ps(Fragment->colorr_8x, r_8x);
            _mm256_store_ps(Fragment->colorg_8x, g_8x);
            _mm256_store_ps(Fragment->colorb_8x, b_8x);
            _mm256_store_ps(Fragment->colora_8x, a_8x);
        }
#endif
    }

    DEBUG_RM_End(__DebugApi__, 4);
}

#endif

// ----------------------------------------------------------------------------------
// -- Pixel output.
// ----------------------------------------------------------------------------------

__INLINE__ void gl_pipe_transparency_processing(gc_framebuffer_t *framebuffer);
__INLINE__ void gl_pipe_transparency_fill(gc_framebuffer_t *framebuffer, vec4 color, r32 z, u32 tx, u32 ty);

#ifndef GCSR_ROUTINE_gl_pipe_pixel_output
#define GCSR_ROUTINE_gl_pipe_pixel_output

void gl_pipe_pixel_output(u32 threadId, gc_framebuffer_t *framebuffer, gl_pixel_buffers_t *PixelBuffers)
{
    DEBUG_RM_Start(__DebugApi__, 5);

    gl_fragpack_t *Pack = PixelBuffers->Fragments[threadId];

    b8 is_z_test = (global_gl->Config.flags & GL_ZTEST) > 0;
    b8 is_z_write = (global_gl->Config.flags & GL_ZWRITE) > 0;
    b8 use_transparency_buffer = (global_gl->Config.flags & GL_TRANSPARENCY_BUFFER) > 0;
    b8 use_global_color = (global_gl->Config.flags & (GL_UNIF_MESH_COLOR | GL_UNIF_WIREFRAME_COLOR)) > 0;
    b8 use_global_opacity = (global_gl->Config.flags & GL_UNIF_MESH_OPACITY) > 0;

    r32 global_opacity = global_gl->Config.meshOpacity;
    vec4 global_color;
    u32 packed_global_color = 0x000000ff;

    if (global_gl->Config.flags & GL_UNIF_MESH_COLOR)
    {
        global_color = global_gl->Config.meshColor;
        global_color.r *= global_color.a;
        global_color.g *= global_color.a;
        global_color.b *= global_color.a;
        packed_global_color = gl_linear1_to_srgb(global_color);
    }

    if (global_gl->Config.flags & (GL_UNIF_WIREFRAME_COLOR))
    {
        global_color = global_gl->Config.wireframeColor;
        global_color.r *= global_color.a;
        global_color.g *= global_color.a;
        global_color.b *= global_color.a;
        packed_global_color = gl_linear1_to_srgb(global_color);
    }

    r32 *depthbuffer = (r32 *) framebuffer->depthbuffer->data;
    u32 *Pixel = (u32 *) framebuffer->VideoMemory;

    __ALIGN__ u32 packed_8x[GL_FRAGPACK_PIXELS];

    // -- Process each fragpack.

    for (u32 i = 0; i < GL_FRAGPACK_COUNT; ++i)
    {
        gl_fragpack_t *current_pack = Pack + i;
        gl_fragment_pixel_t *pixels = current_pack->pixels;

        if (!current_pack->frag_count)
            break;

        gl_linear1tosRGB_8x(current_pack->frag_count, pixels, packed_8x);

        u32 lastFrag = current_pack->frag_count - 1;

        // -- Fragpack fragments.

        for (u32 j = 0; j < current_pack->frag_count; ++j)
        {
            u32 excludeFrom = 0;
            gl_fragment_pixel_t *CurrentPixelFragments = pixels + j;

            if (j == lastFrag && current_pack->padding)
                excludeFrom = GC_FRAG_SIZE - current_pack->padding;

            for (u32 k = 0; k < GC_FRAG_SIZE; ++k)
            {
                if (excludeFrom && k >= excludeFrom)
                    continue;

                b8 z_check = true;
                u32 x = CurrentPixelFragments->pos_x[k];
                u32 y = CurrentPixelFragments->pos_y[k];
                u32 pixelOffset = CurrentPixelFragments->offset[k];
                r32 pixelZ = CurrentPixelFragments->z[k];

                if (is_z_test)
                    z_check = pixelZ <= depthbuffer[pixelOffset];

                if (!z_check)
                    continue;

                // -- Overwrite the fragment shader color.

                if (use_global_color)
                {
                    if (use_transparency_buffer)
                    {
                        if (global_color.a == 1.0f)
                        {
                            if (is_z_write)
                                depthbuffer[pixelOffset] = pixelZ;

                            Pixel[pixelOffset] = packed_global_color;
                        }
                        else
                            gl_pipe_transparency_push(threadId, PixelBuffers, x, y,
                                                    pixelOffset, global_color, pixelZ);
                    }
                    else
                    {
                        if (is_z_write)
                            depthbuffer[pixelOffset] = pixelZ;

                        Pixel[pixelOffset] = packed_global_color;
                    }
                }

                // -- Normal shader processing.

                else
                {
                    if (use_transparency_buffer)
                    {
                        vec4 color = {
                            CurrentPixelFragments->colorr[k],
                            CurrentPixelFragments->colorg[k],
                            CurrentPixelFragments->colorb[k],
                            CurrentPixelFragments->colora[k],
                        };

                        if (use_global_opacity)
                        {
                            color.a = global_opacity;
                            // Premultiplied alpha.
                            color.r *= color.a;
                            color.g *= color.a;
                            color.b *= color.a;
                        }

                        if (color.a == 1.0f)
                        {
                            if (is_z_write)
                                depthbuffer[pixelOffset] = pixelZ;

                            u32 _idx = j * GC_FRAG_SIZE + k;
                            Pixel[pixelOffset] = packed_8x[_idx];
                        }
                        else
                            gl_pipe_transparency_push(threadId, PixelBuffers, x, y,
                                                    pixelOffset, color, pixelZ);
                    }
                    else
                    {
                        if (is_z_write)
                            depthbuffer[pixelOffset] = pixelZ;

                        u32 _idx = j * GC_FRAG_SIZE + k;
                        Pixel[pixelOffset] = packed_8x[_idx];
                    }
                }
            }
        }
    }

    DEBUG_RM_End(__DebugApi__, 5);

    // -- Process each fragment from in the queue (4 pixels).
#if 0
    for (u32 i = 0; i < Fragments->count; ++i)
    {
        GlFragment *Fragment = &Fragments->data[i];
        u32 packed_colors[8];

        u32 x = Fragment->sx;
        u32 y = Fragment->sy;
        u32 offset = y * framebuffer->width + x;
        u32 *Pixel = (u32 *) framebuffer->VideoMemory + offset;

        gl_linear1tosRGB_8x(Fragment->colorr_8x,
                            Fragment->colorg_8x,
                            Fragment->colorb_8x,
                            Fragment->colora_8x,
                            packed_colors);

        // ----------------------------------------------------------------------------------
        // -- Basic "alpha blending", colors with transparency are considered invisible.
        // ----------------------------------------------------------------------------------

        if (!use_transparency_buffer)
        {
            if (isAlphaBlendEnabled)
            {
                for (u32 j = 0; j < 8; ++j)
                {
                    b8 z_check = true;

                    if (is_z_test)
                        z_check = Fragment->z[j] <= framebuffer->depthbuffer->data[offset];

                    if ((Fragment->mask & masks[j]) && z_check)
                    {
                        if (is_z_write)
                            framebuffer->depthbuffer->data[offset] = Fragment->z[j];

                        vec4 destColor = gl_srgb_to_linear1(*Pixel);
                        vec4 srcColor = gl_srgb_to_linear1(packed_colors[j]);
                        r32 one_minus_a = 1.0f - srcColor.a;

                        srcColor.r *= srcColor.a;
                        srcColor.g *= srcColor.a;
                        srcColor.b *= srcColor.a;

                        destColor.r = srcColor.r + one_minus_a * destColor.r;
                        destColor.g = srcColor.g + one_minus_a * destColor.g;
                        destColor.b = srcColor.b + one_minus_a * destColor.b;

                        u32 packed = gl_linear1_to_srgb(destColor);
                        *Pixel = packed;
                    }

                    offset++;
                    Pixel++;
                }
            }
            else
            {
                for (u32 j = 0; j < 8; ++j)
                {
                    b8 zCheck = true;

                    if (is_z_test)
                        zCheck = Fragment->z[j] <= framebuffer->depthbuffer->data[offset];

                    // if (Fragment->mask & masks[j] && zCheck)
                    if (Fragment->mask_8x[j] && zCheck)
                    {
                        if (is_z_write)
                            framebuffer->depthbuffer->data[offset] = Fragment->z[j];

                        *Pixel = packed_colors[j];
                    }

                    offset++;
                    Pixel++;
                }
            }
        }
        else
        {
            for (u32 j = 0; j < 8; ++j)
            {
                b8 zCheck = true;

                if (is_z_test)
                    zCheck = Fragment->z[j] <= framebuffer->depthbuffer->data[offset];

                // if (Fragment->mask & masks[j] && zCheck)
                if (Fragment->mask_8x[j] && zCheck)
                {
                    if (Fragment->colora_8x[j] < 1.0f)
                    {
                        vec4 tcolor = {Fragment->colorr_8x[j],
                                       Fragment->colorg_8x[j],
                                       Fragment->colorb_8x[j],
                                       Fragment->colora_8x[j]};

                        gl_pipe_transparency_fill(framebuffer,
                                                  tcolor,
                                                  Fragment->z[j],
                                                  Fragment->sx + j,
                                                  Fragment->sy);
                    }
                    else
                    {
                        if (is_z_write)
                            framebuffer->depthbuffer->data[offset] = Fragment->z[j];

                        *Pixel = packed_colors[j];
                    }
                }

                offset++;
                Pixel++;
            }
        }
    }
#endif
}

#ifndef GCSR_ROUTINE_gl_pipe_transparency_processing
#define GCSR_ROUTINE_gl_pipe_transparency_processing

void gl_pipe_transparency_processing(u32 threadId, gc_framebuffer_t *framebuffer, gl_pixel_buffers_t *PixelBuffers)
{
    DEBUG_RM_Start(__DebugApi__, 7);

    u32 is_z_write = global_gl->Config.flags & GL_ZWRITE;

    gl_transparency_buffer_t *Tbuffer = PixelBuffers->TransparencyBuffer;
    u32 *Video = (u32 *) framebuffer->VideoMemory;
    r32 *Depth = framebuffer->depthbuffer->data;

    u32 block_count = SDL_AtomicGet(&Tbuffer->queueCount);

    while (true)
    {
        u32 cursor = SDL_AtomicAdd(&Tbuffer->cursor, 1);

        if (cursor >= block_count)
            break;

        u32 blockIndex = Tbuffer->BlockQueue[cursor];
        gl_transparency_block_t *CurrentBlock = &Tbuffer->Blocks[blockIndex];

        for (u32 i = 0; i < CurrentBlock->pixelCount; ++i)
        {
            gl_transparency_pixel_stack_t *TPixel = &CurrentBlock->PixelQueue[i];
            SDL_assert(TPixel->count > 0);

            u32 *Pixel = Video + TPixel->offset;
            r32 *PixelDepth = Depth + TPixel->offset;
            r32 destZ = *PixelDepth;
            __ALIGN__ vec4 dstColor = gl_srgb_to_linear1(*Pixel);
            __ALIGN__ vec4 blended;
            __m128 blended_4x = _mm_setzero_ps();
            __m128 dstColor_4x = _mm_load_ps((r32 *) &dstColor);

            // -- Sort the pixels.

            u32 ordered[GL_MAX_TRANSPARENT_COLORS + 1];
            b8 switched = false;

            for (u32 j = 0; j < TPixel->count; ++j) {
                ordered[j] = j;
            }

            while (true)
            {
                switched = false;

                for (u32 k = 1; k < TPixel->count; ++k)
                {
                    u32 current = ordered[k];
                    u32 prev = ordered[k - 1];

                    if (TPixel->z[prev] > TPixel->z[current])
                    {
                        u32 tmp = ordered[k];
                        ordered[k] = ordered[k - 1];
                        ordered[k - 1] = tmp;

                        switched = true;
                    }
                }

                if (!switched)
                    break;
            }

            // -- Check if the top pixel is in front of the destination pixel.

            if (TPixel->z[ordered[0]] < destZ)
            {
                // -- Blend the pixels.

                r32 closest = 1.0f;

                for (u32 j = 0; j < TPixel->count; ++j)
                {
                    u32 current = ordered[j];
                    __m128 tColor_4x = _mm_load_ps((r32 *) &TPixel->rgba[current]);

                    if (TPixel->z[current] >= destZ)
                        continue;

                    __m128 oneMinusAlpha_4x = _mm_set1_ps(1.0f - M(blended_4x, 3));
                    blended_4x = _mm_add_ps(blended_4x, _mm_mul_ps(oneMinusAlpha_4x, tColor_4x));
                }

                // -- Add the destination color.

                __m128 oneMinusAlpha_4x = _mm_set1_ps(1.0f - M(blended_4x, 3));
                blended_4x = _mm_add_ps(blended_4x, _mm_mul_ps(dstColor_4x, oneMinusAlpha_4x));
                _mm_store_ps((r32 *) &blended, blended_4x);
                blended.a = 1.0f;

                if (blended.r > 1)
                    blended.r = 1;

                if (blended.g > 1)
                    blended.g = 1;

                if (blended.b > 1)
                    blended.b = 1;

                u32 packed = gl_linear1_to_srgb(blended);
                *Pixel = packed;

                if (is_z_write)
                    *PixelDepth = TPixel->z[ordered[0]];
            }

            CurrentBlock->pixelGrid[TPixel->gridIndex] = 0;

            TPixel->count = 0;
            TPixel->gridIndex = 0;
        }

        CurrentBlock->pixelCount = 0;
        CurrentBlock->inQueue = false;
    }

    DEBUG_RM_End(__DebugApi__, 7);
}
#endif

#ifndef GCSR_ROUTINE_gl_pipeTransparencyPush
#define GCSR_ROUTINE_gl_pipeTransparencyPush

void gl_pipe_transparency_push(u32 threadId, gl_pixel_buffers_t *PixelBuffers,
                             u32 px, u32 py, u32 pixelOffset, vec4 pixelColor, r32 pixelZ)
{
    DEBUG_RM_Start(__DebugApi__, 6);

    gl_transparency_buffer_t *Tbuffer = PixelBuffers->TransparencyBuffer;

    u32 blockCol = px / GL_BLOCK_SIZE;
    u32 blockRow = py / GL_BLOCK_SIZE;
    u32 blockOffset = blockRow * GL_TB_BLOCK_COUNT_COLS + blockCol;
    gl_transparency_block_t *CurrentBlock = &Tbuffer->Blocks[blockOffset];

    if (!CurrentBlock->inQueue)
    {
        u32 queueIndex = SDL_AtomicAdd(&Tbuffer->queueCount, 1);
        Tbuffer->BlockQueue[queueIndex] = blockOffset;

        CurrentBlock->inQueue = true;
    }

    u32 relX = px - CurrentBlock->sx;
    u32 relY = py - CurrentBlock->sy;
    u32 blockPixelOffset = relY * GL_BLOCK_SIZE + relX;

    u32 pixelQueueIndex = CurrentBlock->pixelGrid[blockPixelOffset];

    if (!pixelQueueIndex)
    {
        pixelQueueIndex = CurrentBlock->pixelCount++ + 1;
        CurrentBlock->pixelGrid[blockPixelOffset] = pixelQueueIndex;
    }

    gl_transparency_pixel_stack_t *TPixel = &CurrentBlock->PixelQueue[pixelQueueIndex - 1];
    TPixel->gridIndex = blockPixelOffset;

    // -- Replace the farthest color in the list.

    if (TPixel->count == GL_MAX_TRANSPARENT_COLORS)
    {
        u32 farthestIndex = 0;
        r32 farthestZ = 0;

        for (u32 i = 0; i < GL_MAX_TRANSPARENT_COLORS; ++i)
        {
            if (TPixel->z[i] > farthestZ)
            {
                farthestIndex = i;
                farthestZ = TPixel->z[i];
            }
        }

        TPixel->z[farthestIndex] = pixelZ;
        TPixel->rgba[farthestIndex] = pixelColor;
    }
    else
    {
        u32 colorIndex = TPixel->count++;

        TPixel->rgba[colorIndex] = pixelColor;
        TPixel->z[colorIndex] = pixelZ;
    }

    SDL_assert(TPixel->count <= (GL_MAX_TRANSPARENT_COLORS + 1));
    TPixel->offset = pixelOffset;

    DEBUG_RM_End(__DebugApi__, 6);
}

#endif

#endif