// ----------------------------------------------------------------------------------
// -- File: gcsr_rasterizer.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C
// -- Description:
// -- Created: 2021-03-27 16:17:07
// -- Modified:
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

// NOTE(gabic): only for powers of 2.
#define _mm_mod_epi32(val_4x, shift) (_mm_sub_epi32(val_4x, _mm_slli_epi32(_mm_srli_epi32(val_4x, shift), shift)))

__INLINE__ __m128 simd_tex_clamp(__m128 c) {
    return _mm_max_ps(_mm_min_ps(c, _mm_set1_ps(1.0f)), _mm_setzero_ps());
}

__INLINE__ __m128 simd_tex_repeat(__m128 c)
{
    c = _mm_sub_ps(c, _mm_floor_ps(c));
    c = _mm_and_ps(c, _mm_cmpge_ps(c, _mm_setzero_ps()));

    return c;
}

__m128 simd_tex_mirror(__m128 c)
{
    __m128 mask = _mm_cmplt_ps(c, _mm_setzero_ps());

    c = _mm_or_ps(
            _mm_and_ps(mask, _mm_mul_ps(c, _mm_set1_ps(-1.0f))),
            _mm_andnot_ps(mask, c));

    __m128i min = _mm_cvttps_epi32(c);
    __m128i max = _mm_add_epi32(min, _mm_set1_epi32(1));

    __m128i mmask = _mm_cmpgt_epi32(_mm_and_si128(min, _mm_set1_epi32(1)), _mm_setzero_si128());
    __m128 c1 = _mm_sub_ps(_mm_cvtepi32_ps(max), c);
    __m128 c2 = _mm_sub_ps(c, _mm_cvtepi32_ps(min));

    c = _mm_or_ps(
            _mm_and_ps(_mm_castsi128_ps(mmask), c1),
            _mm_andnot_ps(_mm_castsi128_ps(mmask), c2));

    return c;
}


void gl_sse_pipe_rasterize_point(thread_batch_memory_t *batch_memory, gc_screen_rect_t *box, u16 fragment_index_start, gc_primitive_t *primitive)
{
    OPTICK_EVENT("gl_pipe_rasterize_point");

    gc_processed_fragment_t *current_fragment = 0;

    u32 start_x = primitive->box.min.x;
    u32 end_x = primitive->box.max.x;
    u32 start_y = primitive->box.min.y;
    u32 end_y = primitive->box.max.y;

    if (primitive->box.min.x < box->min.x) start_x = box->min.x;
    if (primitive->box.max.x >= box->max.x) end_x = box->max.x - 1;
    if (primitive->box.min.y < box->min.y) start_y = box->min.y;
    if (primitive->box.max.y >= box->max.y) end_y = box->max.y - 1;

    u32 frag_col_min = (start_x - box->min.x) >> 1;
    u32 frag_col_max = (end_x - box->min.x) >> 1;
    u32 frag_row_min = (start_y - box->min.y) >> 1;
    u32 frag_row_max = (end_y - box->min.y) >> 1;

    __m128i pbox_min_x = _mm_set1_epi32(primitive->box.min.x - 1);
    __m128i pbox_min_y = _mm_set1_epi32(primitive->box.min.y - 1);
    __m128i pbox_max_x = _mm_set1_epi32(primitive->box.max.x + 1);
    __m128i pbox_max_y = _mm_set1_epi32(primitive->box.max.y + 1);

    __m128i masks = _mm_setr_epi32(0b11000000, 0b00110000, 0b00001100, 0b00000011);
    u32 tmp[GC_FRAG_SIZE];

    for (u32 r = frag_row_min; r <= frag_row_max; ++r)
    {
        u32 fy = r << GL_FRAG_HEIGHT_SHIFT;
        __m128i frag_coord_y = _mm_add_epi32(_mm_setr_epi32(fy, fy, fy + 1, fy + 1), _mm_set1_epi32(box->min.y));

        for (u32 c = frag_col_min; c <= frag_col_max; ++c)
        {
            u32 fx = c << GL_FRAG_WIDTH_SHIFT;

            __m128i frag_coord_x = _mm_add_epi32(_mm_setr_epi32(fx, fx + 1, fx, fx + 1), _mm_set1_epi32(box->min.x));

            __m128i mask_check = _mm_and_si128(masks,
                                    _mm_and_si128(
                                        _mm_and_si128(
                                            _mm_cmpgt_epi32(frag_coord_x, pbox_min_x),
                                            _mm_cmplt_epi32(frag_coord_x, pbox_max_x)),
                                        _mm_and_si128(
                                            _mm_cmpgt_epi32(frag_coord_y, pbox_min_y),
                                            _mm_cmplt_epi32(frag_coord_y, pbox_max_y))));

            _mm_store_si128((__m128i *) tmp, mask_check);
            u32 mask = tmp[0] | tmp[1] | tmp[2] | tmp[3];

            if (mask)
            {
                u32 frag_index = fragment_index_start + r * GL_BIN_FRAG_COLS + c;

                PUSH_FRAGMENT(current_fragment);
                SDL_assert(frag_index < GL_BIN_FRAGS);

                current_fragment->mask = mask;
                current_fragment->index = frag_index;
                current_fragment->x = fx + box->min.x;
                current_fragment->y = fy + box->min.y;
                current_fragment->discarded = false;
                current_fragment->primitive = primitive;
            }
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Line block rasterization routine.
// ----------------------------------------------------------------------------------

void gl_sse_pipe_rasterize_line(thread_batch_memory_t *batch_memory,
                                gc_screen_rect_t *box,
                                u16 fragment_index_start,
                                gc_primitive_t *primitive)
{
    OPTICK_EVENT("gl_sse_pipe_rasterize_line");

    gc_processed_fragment_t *current_fragment = 0;

    __m128i box_min_x = _mm_set1_epi32(box->min.x - 1);
    __m128i box_min_y = _mm_set1_epi32(box->min.y - 1);
    __m128i box_max_x = _mm_set1_epi32(box->max.x);
    __m128i box_max_y = _mm_set1_epi32(box->max.y);

    // __m128i line_min_x = _mm_set1_epi32(primitive->line.box_min_x - 1);
    // __m128i line_min_y = _mm_set1_epi32(primitive->line.box_min_y - 1);
    // __m128i line_max_x = _mm_set1_epi32(primitive->line.box_max_x + 1);
    // __m128i line_max_y = _mm_set1_epi32(primitive->line.box_max_y + 1);
    __m128 half_4x = _mm_set1_ps(0.5f);

    __m128 val_incr = _mm_set1_ps(primitive->line.a * 4);

    s32 check_mask[GC_FRAG_SIZE];
    s32 frag_index_vec[GC_FRAG_SIZE];
    s32 frag_x_vec[GC_FRAG_SIZE];
    s32 frag_y_vec[GC_FRAG_SIZE];
    s32 in_coord_vec[GC_FRAG_SIZE];
    s32 out_coord_vec[GC_FRAG_SIZE];

    // X variation.
    if (primitive->line.is_dx)
    {
        s32 start = box->min.x;
        s32 end = box->max.x;
        r32 start_val = primitive->line.a * start + primitive->line.b;

        __m128 xincr = _mm_setr_ps(0, 1, 2, 3);
        __m128i xincri = _mm_setr_epi32(0, 1, 2, 3);
        __m128 incr = _mm_mul_ps(xincr, _mm_set1_ps(primitive->line.a));
        __m128 computed_coord = _mm_add_ps(_mm_set1_ps(start_val), incr);
        __m128i bin_frag_cols = _mm_set1_epi32(GL_BIN_FRAG_COLS);

        // X input, Y output.
        for (s32 c = start; c < end; c += GC_FRAG_SIZE)
        {
            __m128i in_coord = _mm_add_epi32(_mm_set1_epi32(c), xincri);
            __m128i out_coord = _mm_cvttps_epi32(_mm_add_ps(computed_coord, half_4x));

            __m128i frag_col = _mm_srli_epi32(_mm_sub_epi32(in_coord, _mm_set1_epi32(box->min.x)), GL_FRAG_WIDTH_SHIFT);
            __m128i frag_row = _mm_srli_epi32(_mm_sub_epi32(out_coord, _mm_set1_epi32(box->min.y)), GL_FRAG_HEIGHT_SHIFT);
            __m128i frag_index = _mm_add_epi32(_mm_mullo_epi32(frag_row, bin_frag_cols), frag_col);

            s32 current_frag_index = -1;

            // __m128i check = _mm_and_si128(
            //                     _mm_and_si128(_mm_castps_si128(_mm_cmpge_ps(_mm_cvtepi32_ps(in_coord), line_min_x)), _mm_castps_si128(_mm_cmple_ps(_mm_cvtepi32_ps(in_coord), line_max_x))),
            //                     _mm_and_si128(_mm_cmpgt_epi32(out_coord, box_min_y), _mm_cmplt_epi32(out_coord, box_max_y)));

            __m128i check = _mm_and_si128(
                                _mm_and_si128(_mm_cmpgt_epi32(in_coord, _mm_set1_epi32(primitive->box.min.x - 1)), _mm_cmplt_epi32(in_coord, _mm_set1_epi32(primitive->box.max.x + 1))),
                                _mm_and_si128(_mm_cmpgt_epi32(out_coord, box_min_y), _mm_cmplt_epi32(out_coord, box_max_y)));

            __m128i frag_x = _mm_add_epi32(_mm_set1_epi32(box->min.x), _mm_slli_epi32(frag_col, GL_FRAG_WIDTH_SHIFT));
            __m128i frag_y = _mm_add_epi32(_mm_set1_epi32(box->min.y), _mm_slli_epi32(frag_row, GL_FRAG_HEIGHT_SHIFT));

            _mm_store_si128((__m128i *) check_mask, check);
            _mm_store_si128((__m128i *) frag_index_vec, frag_index);
            _mm_store_si128((__m128i *) in_coord_vec, in_coord);
            _mm_store_si128((__m128i *) out_coord_vec, out_coord);
            _mm_store_si128((__m128i *) frag_x_vec, frag_x);
            _mm_store_si128((__m128i *) frag_y_vec, frag_y);

            for (u8 i = 0; i < 4; ++i)
            {
                if (check_mask[i])
                {
                    s32 check_frag_index = fragment_index_start + frag_index_vec[i];

                    // Fragment push.
                    if (current_frag_index < 0 || current_frag_index != check_frag_index)
                    {
                        current_frag_index = check_frag_index;

                        PUSH_FRAGMENT(current_fragment);
                        SDL_assert(current_frag_index < GL_BIN_FRAGS);

                        current_fragment->mask = 0;
                        current_fragment->index = current_frag_index;
                        current_fragment->x = frag_x_vec[i];
                        current_fragment->y = frag_y_vec[i];
                        current_fragment->discarded = false;
                        current_fragment->primitive = primitive;
                    }

                    // Mask determination.
                    u8 mask = 0b00001100;

                    // x.
                    if (in_coord_vec[i] & 1)
                        mask = 0b00000011;

                    // y.
                    if ((out_coord_vec[i] & 1) == 0)
                        mask = mask << 4;

                    current_fragment->mask |= mask;
                }
            }

            computed_coord = _mm_add_ps(computed_coord, val_incr);
        }
    }
    else
    {
        s32 start = box->min.y;
        s32 end = box->max.y;
        r32 start_val = primitive->line.a * start + primitive->line.b;

        __m128 yincr = _mm_setr_ps(0, 1, 2, 3);
        __m128i yincri = _mm_setr_epi32(0, 1, 2, 3);
        __m128 incr = _mm_mul_ps(yincr, _mm_set1_ps(primitive->line.a));
        __m128 computed_coord = _mm_add_ps(_mm_set1_ps(start_val), incr);
        __m128i bin_frag_cols = _mm_set1_epi32(GL_BIN_FRAG_COLS);

        // Y input, X output.
        for (s32 c = start; c < end; c += GC_FRAG_SIZE)
        {
            __m128i in_coord = _mm_add_epi32(_mm_set1_epi32(c), yincri);
            __m128i out_coord = _mm_cvttps_epi32(_mm_add_ps(computed_coord, half_4x));

            __m128i frag_col = _mm_srli_epi32(_mm_sub_epi32(out_coord, _mm_set1_epi32(box->min.x)), GL_FRAG_WIDTH_SHIFT);
            __m128i frag_row = _mm_srli_epi32(_mm_sub_epi32(in_coord, _mm_set1_epi32(box->min.y)), GL_FRAG_HEIGHT_SHIFT);
            __m128i frag_index = _mm_add_epi32(_mm_mullo_epi32(frag_row, bin_frag_cols), frag_col);

            s32 current_frag_index = -1;

            // __m128i check = _mm_and_si128(
            //                     _mm_and_si128(_mm_castps_si128(_mm_cmpge_ps(_mm_cvtepi32_ps(in_coord), line_min_y)), _mm_castps_si128(_mm_cmple_ps(_mm_cvtepi32_ps(in_coord), line_max_y))),
            //                     _mm_and_si128(_mm_cmpgt_epi32(out_coord, box_min_x), _mm_cmplt_epi32(out_coord, box_max_x)));

            __m128i check = _mm_and_si128(
                                _mm_and_si128(_mm_cmpgt_epi32(in_coord, _mm_set1_epi32(primitive->box.min.y - 1)), _mm_cmplt_epi32(in_coord, _mm_set1_epi32(primitive->box.max.y + 1))),
                                _mm_and_si128(_mm_cmpgt_epi32(out_coord, box_min_x), _mm_cmplt_epi32(out_coord, box_max_x)));

            __m128i frag_x = _mm_add_epi32(_mm_set1_epi32(box->min.x), _mm_slli_epi32(frag_col, GL_FRAG_WIDTH_SHIFT));
            __m128i frag_y = _mm_add_epi32(_mm_set1_epi32(box->min.y), _mm_slli_epi32(frag_row, GL_FRAG_HEIGHT_SHIFT));

            _mm_store_si128((__m128i *) check_mask, check);
            _mm_store_si128((__m128i *) frag_index_vec, frag_index);
            _mm_store_si128((__m128i *) in_coord_vec, in_coord);
            _mm_store_si128((__m128i *) out_coord_vec, out_coord);
            _mm_store_si128((__m128i *) frag_x_vec, frag_x);
            _mm_store_si128((__m128i *) frag_y_vec, frag_y);

            for (u8 i = 0; i < 4; ++i)
            {
                if (check_mask[i])
                {
                    s32 check_frag_index = fragment_index_start + frag_index_vec[i];

                    // Fragment push.
                    if (current_frag_index < 0 || current_frag_index != check_frag_index)
                    {
                        current_frag_index = check_frag_index;

                        PUSH_FRAGMENT(current_fragment);
                        SDL_assert(current_frag_index < GL_BIN_FRAGS);

                        current_fragment->mask = 0;
                        current_fragment->index = current_frag_index;
                        current_fragment->x = frag_x_vec[i];
                        current_fragment->y = frag_y_vec[i];
                        current_fragment->discarded = false;
                        current_fragment->primitive = primitive;
                    }

                    // Mask determination.
                    u8 mask = 0b00001100;

                    // x.
                    if (out_coord_vec[i] & 1)
                        mask = 0b00000011;

                    // y.
                    if ((in_coord_vec[i] & 1) == 0)
                        mask = mask << 4;

                    current_fragment->mask |= mask;
                }
            }

            computed_coord = _mm_add_ps(computed_coord, val_incr);
        }
    }

}

// ----------------------------------------------------------------------------------
// -- Triangle rasterization routine.
// ----------------------------------------------------------------------------------

__INLINE__ void gl_sse_pipe_rasterize_triangle(thread_batch_memory_t *batch_memory,
                                               gc_screen_rect_t *box,
                                               u16 fragment_index_start,
                                               gc_primitive_t *primitive)
{
    OPTICK_EVENT("gl_sse_pipe_rasterize_triangle");

    gc_processed_fragment_t *current_fragment = 0;

    s64 fp_bx = box->min.x << FP_DEC_BIT;
    s64 fp_by = box->min.y << FP_DEC_BIT;

    s64 e1_start = primitive->triangle.l1a * fp_bx + primitive->triangle.l1b * fp_by + primitive->triangle.l1c;
    s64 e2_start = primitive->triangle.l2a * fp_bx + primitive->triangle.l2b * fp_by + primitive->triangle.l2c;
    s64 e3_start = primitive->triangle.l3a * fp_bx + primitive->triangle.l3b * fp_by + primitive->triangle.l3c;

    __m128i edge1_frag_dx_4x = _mm_set1_epi64x(primitive->triangle.frag_l1dx);
    __m128i edge1_frag_dy_4x = _mm_set1_epi64x(primitive->triangle.frag_l1dy);

    __m128i edge2_frag_dx_4x = _mm_set1_epi64x(primitive->triangle.frag_l2dx);
    __m128i edge2_frag_dy_4x = _mm_set1_epi64x(primitive->triangle.frag_l2dy);

    __m128i edge3_frag_dx_4x = _mm_set1_epi64x(primitive->triangle.frag_l3dx);
    __m128i edge3_frag_dy_4x = _mm_set1_epi64x(primitive->triangle.frag_l3dy);

    __m128i base_frag_e1a_4x = _mm_set_epi64x(e1_start, e1_start + primitive->triangle.l1dx);
    __m128i base_frag_e1b_4x = _mm_set_epi64x(e1_start + primitive->triangle.l1dy, e1_start + primitive->triangle.l1dx + primitive->triangle.l1dy);

    __m128i base_frag_e2a_4x = _mm_set_epi64x(e2_start, e2_start + primitive->triangle.l2dx);
    __m128i base_frag_e2b_4x = _mm_set_epi64x(e2_start + primitive->triangle.l2dy, e2_start + primitive->triangle.l2dx + primitive->triangle.l2dy);

    __m128i base_frag_e3a_4x = _mm_set_epi64x(e3_start, e3_start + primitive->triangle.l3dx);
    __m128i base_frag_e3b_4x = _mm_set_epi64x(e3_start + primitive->triangle.l3dy, e3_start + primitive->triangle.l3dx + primitive->triangle.l3dy);

    for (s32 y = box->min.y; y < box->max.y; y += GL_FRAG_HEIGHT)
    {
        __m128i current_frag_e1a_4x = base_frag_e1a_4x;
        __m128i current_frag_e1b_4x = base_frag_e1b_4x;

        __m128i current_frag_e2a_4x = base_frag_e2a_4x;
        __m128i current_frag_e2b_4x = base_frag_e2b_4x;

        __m128i current_frag_e3a_4x = base_frag_e3a_4x;
        __m128i current_frag_e3b_4x = base_frag_e3b_4x;

        u16 fragment_index = fragment_index_start;

        for (s32 x = box->min.x; x < box->max.x; x += GL_FRAG_WIDTH)
        {
            __m128i zeroi_4x = _mm_setzero_si128();

            __m128i cmpa_4x = _mm_and_si128(
                                _mm_and_si128(
                                    _mm_cmpgt_epi64(current_frag_e1a_4x, zeroi_4x),
                                    _mm_cmpgt_epi64(current_frag_e2a_4x, zeroi_4x)),
                                _mm_cmpgt_epi64(current_frag_e3a_4x, zeroi_4x));

            __m128i cmpb_4x = _mm_and_si128(
                                _mm_and_si128(
                                    _mm_cmpgt_epi64(current_frag_e1b_4x, zeroi_4x),
                                    _mm_cmpgt_epi64(current_frag_e2b_4x, zeroi_4x)),
                                _mm_cmpgt_epi64(current_frag_e3b_4x, zeroi_4x));

            // u32 a = _mm_movemask_ps(_mm_castsi128_ps(cmpa_4x));
            // u32 b = _mm_movemask_ps(_mm_castsi128_ps(cmpb_4x));

            u32 mask = (_mm_movemask_ps(_mm_castsi128_ps(cmpa_4x)) << GL_FRAG_MASK_SHIFT) |
                        _mm_movemask_ps(_mm_castsi128_ps(cmpb_4x));

            if (mask)
            {
                PUSH_FRAGMENT(current_fragment);
                SDL_assert(fragment_index < GL_BIN_FRAGS);

                current_fragment->mask = mask;
                current_fragment->index = fragment_index;
                current_fragment->x = x;
                current_fragment->y = y;
                current_fragment->discarded = false;
                current_fragment->primitive = primitive;
            }

            fragment_index++;

            current_frag_e1a_4x = _mm_add_epi64(current_frag_e1a_4x, edge1_frag_dx_4x);
            current_frag_e1b_4x = _mm_add_epi64(current_frag_e1b_4x, edge1_frag_dx_4x);

            current_frag_e2a_4x = _mm_add_epi64(current_frag_e2a_4x, edge2_frag_dx_4x);
            current_frag_e2b_4x = _mm_add_epi64(current_frag_e2b_4x, edge2_frag_dx_4x);

            current_frag_e3a_4x = _mm_add_epi64(current_frag_e3a_4x, edge3_frag_dx_4x);
            current_frag_e3b_4x = _mm_add_epi64(current_frag_e3b_4x, edge3_frag_dx_4x);
        }

        base_frag_e1a_4x = _mm_add_epi64(base_frag_e1a_4x, edge1_frag_dy_4x);
        base_frag_e1b_4x = _mm_add_epi64(base_frag_e1b_4x, edge1_frag_dy_4x);

        base_frag_e2a_4x = _mm_add_epi64(base_frag_e2a_4x, edge2_frag_dy_4x);
        base_frag_e2b_4x = _mm_add_epi64(base_frag_e2b_4x, edge2_frag_dy_4x);

        base_frag_e3a_4x = _mm_add_epi64(base_frag_e3a_4x, edge3_frag_dy_4x);
        base_frag_e3b_4x = _mm_add_epi64(base_frag_e3b_4x, edge3_frag_dy_4x);

        fragment_index_start += GL_TILE_FRAG_STRIDE;
    }
}

// ----------------------------------------------------------------------------------
// -- Computes the wireframe color for a specified single-pass-wireframe object.
// ----------------------------------------------------------------------------------

#if 0
__INLINE__ void gl_single_pass_wireframe_filter(gc_single_pass_wireframe_t *Wireframe,
                                            vec4 v1, vec4 v2, vec4 v3,
                                            r32 d1[4], r32 d2[4], r32 d3[4],
                                            u32 fragx[4], u32 fragy[4],
                                            r32 fragr[4], r32 fragg[4], r32 fragb[4], r32 fraga[4])
{
    vec4 wire_c = Wireframe->color;

    wire_c.r *= wire_c.a;
    wire_c.g *= wire_c.a;
    wire_c.b *= wire_c.a;

    b8 is_dotted = (Wireframe->type == SPW_DOTTED);

    __m128 zero_4x = _mm_setzero_ps();

    __m128 fragx_4x = _mm_cvtepi32_ps(_mm_loadu_si128((__m128i *) fragx));
    __m128 fragy_4x = _mm_cvtepi32_ps(_mm_loadu_si128((__m128i *) fragy));

    __m128 fragcr_4x = _mm_load_ps(fragr);
    __m128 fragcg_4x = _mm_load_ps(fragg);
    __m128 fragcb_4x = _mm_load_ps(fragb);
    __m128 fragca_4x = _mm_load_ps(fraga);

    __m128 one_minus_a_4x = _mm_set1_ps(1.0f - wire_c.a);
    __m128 wirecr_4x = _mm_set1_ps(wire_c.r);
    __m128 wirecg_4x = _mm_set1_ps(wire_c.g);
    __m128 wirecb_4x = _mm_set1_ps(wire_c.b);
    __m128 wireca_4x = _mm_set1_ps(wire_c.a);

    fragcr_4x = _mm_mul_ps(fragcr_4x, fragca_4x);
    fragcg_4x = _mm_mul_ps(fragcg_4x, fragca_4x);
    fragcb_4x = _mm_mul_ps(fragcb_4x, fragca_4x);

    vec4 lineP1 = v2;
    vec4 lineP2 = v3;

    vec4 points[3] = {v1, v2, v3};
    __ALIGN__ u32 indices[4];

    r32 px_4x[3] = {v1.x, v2.x, v3.x};
    r32 py_4x[3] = {v1.y, v2.y, v3.y};

    __m128i three_4x = _mm_set1_epi32(3);
    __m128 d1_4x = _mm_load_ps(d1);
    __m128 d2_4x = _mm_load_ps(d2);
    __m128 d3_4x = _mm_load_ps(d3);
    // points index, this specifies a set of two points from the vector (line).
    __m128i line_idx_4x = _mm_set1_epi32(1);
    __m128 distance_4x = d1_4x;

    // -- if (d2 < distance)

    __m128i cmp_d2_4x = _mm_castps_si128(_mm_cmplt_ps(d2_4x, distance_4x));

    distance_4x = _mm_or_ps(
                    _mm_and_ps(d2_4x, _mm_castsi128_ps(cmp_d2_4x)),
                    _mm_andnot_ps(_mm_castsi128_ps(cmp_d2_4x), distance_4x));

    line_idx_4x = _mm_or_si128(
                    _mm_andnot_si128(cmp_d2_4x, line_idx_4x),
                    _mm_and_si128(cmp_d2_4x, _mm_set1_epi32(2)));

    // -- if (d3 < distance)

    __m128i cmp_d3_4x = _mm_castps_si128(_mm_cmplt_ps(d3_4x, distance_4x));

    distance_4x = _mm_or_ps(
                    _mm_and_ps(d3_4x, _mm_castsi128_ps(cmp_d3_4x)),
                    _mm_andnot_ps(_mm_castsi128_ps(cmp_d3_4x), distance_4x));

    line_idx_4x = _mm_or_si128(
                    _mm_andnot_si128(cmp_d3_4x, line_idx_4x),
                    _mm_and_si128(cmp_d3_4x, _mm_set1_epi32(0)));

    // -- Edge P1.
    // ----------------------------------------------------------------------------------

    _mm_store_si128((__m128i *) indices, line_idx_4x);

    __m128 line_p1x_4x = _mm_setr_ps(
                            px_4x[indices[0]],
                            px_4x[indices[1]],
                            px_4x[indices[2]],
                            px_4x[indices[3]]);

    __m128 line_p1y_4x = _mm_setr_ps(
                            py_4x[indices[0]],
                            py_4x[indices[1]],
                            py_4x[indices[2]],
                            py_4x[indices[3]]);

    // -- Edge P2.
    // ----------------------------------------------------------------------------------

    line_idx_4x = _mm_add_epi32(line_idx_4x, _mm_set1_epi32(1));
    cmp_d3_4x = _mm_cmplt_epi32(line_idx_4x, three_4x);
    line_idx_4x = _mm_and_si128(line_idx_4x, cmp_d3_4x);

    _mm_store_si128((__m128i *) indices, line_idx_4x);

    __m128 line_p2x_4x = _mm_setr_ps(
                            px_4x[indices[0]],
                            px_4x[indices[1]],
                            px_4x[indices[2]],
                            px_4x[indices[3]]);

    __m128 line_p2y_4x = _mm_setr_ps(
                            py_4x[indices[0]],
                            py_4x[indices[1]],
                            py_4x[indices[2]],
                            py_4x[indices[3]]);

    // ----------------------------------------------------------------------------------

    __m128 I_4x = _mm_set1_ps(1.0f);
    __m128 d_max_4x = _mm_set1_ps(Wireframe->d_max);

    __m128 edgev_x_4x = _mm_sub_ps(line_p2x_4x, line_p1x_4x);
    __m128 edgev_y_4x = _mm_sub_ps(line_p2y_4x, line_p1y_4x);

    // -- vec2 normalization x4.
    // -- vec2 length x4.

    __m128 edgelen_4x = _mm_sqrt_ps(
                            _mm_add_ps(
                                _mm_mul_ps(edgev_x_4x, edgev_x_4x),
                                _mm_mul_ps(edgev_y_4x, edgev_y_4x)));

    edgev_x_4x = _mm_div_ps(edgev_x_4x, edgelen_4x);
    edgev_y_4x = _mm_div_ps(edgev_y_4x, edgelen_4x);

    // vec2 lineVector = {frag_x - lineP1.x, frag_y - lineP1.y};

    __m128 pointv_x_4x = _mm_sub_ps(fragx_4x, line_p1x_4x);
    __m128 pointv_y_4x = _mm_sub_ps(fragy_4x, line_p1y_4x);

    // r32 proj = vec2_dot(lineVector, edge);

    __m128 proj_s_4x = _mm_add_ps(
                        _mm_mul_ps(pointv_x_4x, edgev_x_4x),
                        _mm_mul_ps(pointv_y_4x, edgev_y_4x));

    // vec2 edgeDistanceVector = vec2_muls(edge, proj);

    __m128 edgedv_x_4x = _mm_mul_ps(edgev_x_4x, proj_s_4x);
    __m128 edgedv_y_4x = _mm_mul_ps(edgev_y_4x, proj_s_4x);

    // r32 edgeDistance = vec2_len(edgeDistanceVector);

    __m128 edgedvlen_4x = _mm_sqrt_ps(
                            _mm_add_ps(
                                _mm_mul_ps(edgedv_x_4x, edgedv_x_4x),
                                _mm_mul_ps(edgedv_y_4x, edgedv_y_4x)));

    // r32 rel = edgeDistance / fullEdgeDistance;

    __m128 ratio_4x = _mm_div_ps(edgedvlen_4x, edgelen_4x);
    __m128 v_4x = _mm_set1_ps((4 * Wireframe->n - 1) * PI);
    __ALIGN__ __m128 interval_4x = _mm_mul_ps(ratio_4x, v_4x);

    __m128 s_4x = _mm_setzero_ps();

    if (is_dotted)
        s_4x = sin_ps(interval_4x);
    else if (Wireframe->type == SPW_WAVE)
    {
        __m128 wave_offx_4x = _mm_set1_ps(Wireframe->wave_offset_x);
        __m128 wave_offy_4x = _mm_set1_ps(Wireframe->wave_offset_y);
        __m128 wave_scaling_4x = _mm_set1_ps(Wireframe->wave_scaling);

#if 0
        r32 sin_4[4];
        r32 sint1_4[4];

        __m128 st1_4x = _mm_sub_ps(interval_4x, wave_offx_4x);

        _mm_store_ps(sint1_4, st1_4x);
        sin_4[0] = t_sin(sint1_4[0]);
        sin_4[1] = t_sin(sint1_4[1]);
        sin_4[2] = t_sin(sint1_4[2]);
        sin_4[3] = t_sin(sint1_4[3]);
        __m128 sin_4x = _mm_load_ps(sin_4);

#else
        __m128 sin_4x = sin_ps(_mm_sub_ps(interval_4x, wave_offx_4x));
#endif
        s_4x = _mm_add_ps(sin_4x, wave_offy_4x);

        d_max_4x = _mm_mul_ps(s_4x, wave_scaling_4x);
    }

    __m128 one_4x = _mm_set1_ps(1.0f);
    __m128 exps_4x = _mm_set1_ps(-(2.0f / Wireframe->scaling));
    __m128 fcmp_4x = _mm_cmplt_ps(distance_4x, d_max_4x);

    if (!Wireframe->no_filter)
        I_4x = exp_ps(_mm_mul_ps(_mm_mul_ps(distance_4x, distance_4x), exps_4x));

    if (is_dotted)
    {
        __m128 dottedcmp_4x = _mm_cmpge_ps(s_4x, zero_4x);
        I_4x = _mm_and_ps(I_4x, dottedcmp_4x);
    }

    I_4x = _mm_and_ps(I_4x, fcmp_4x);
    __m128 one_minus_I_4x = _mm_sub_ps(one_4x, I_4x);

    wirecr_4x = _mm_add_ps(wirecr_4x, _mm_mul_ps(fragcr_4x, one_minus_a_4x));
    wirecg_4x = _mm_add_ps(wirecg_4x, _mm_mul_ps(fragcg_4x, one_minus_a_4x));
    wirecb_4x = _mm_add_ps(wirecb_4x, _mm_mul_ps(fragcb_4x, one_minus_a_4x));
    wireca_4x = _mm_add_ps(wireca_4x, _mm_mul_ps(fragca_4x, one_minus_a_4x));

    wirecr_4x = _mm_add_ps(_mm_mul_ps(I_4x, wirecr_4x),_mm_mul_ps(one_minus_I_4x, fragcr_4x));
    wirecg_4x = _mm_add_ps(_mm_mul_ps(I_4x, wirecg_4x),_mm_mul_ps(one_minus_I_4x, fragcg_4x));
    wirecb_4x = _mm_add_ps(_mm_mul_ps(I_4x, wirecb_4x),_mm_mul_ps(one_minus_I_4x, fragcb_4x));
    wireca_4x = _mm_add_ps(_mm_mul_ps(I_4x, wireca_4x),_mm_mul_ps(one_minus_I_4x, fragca_4x));

    _mm_store_ps(fragr, wirecr_4x);
    _mm_store_ps(fragg, wirecg_4x);
    _mm_store_ps(fragb, wirecb_4x);
    _mm_store_ps(fraga, wireca_4x);
}
#endif