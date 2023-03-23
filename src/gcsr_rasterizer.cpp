// ----------------------------------------------------------------------------------
// -- File: gcsr_rasterizer.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-03-27 16:12:38
// -- Modified: 2022-01-29 23:36:23
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

#if defined(GC_PIPE_SSE)
#include "simd/gcsr_sse_rasterizer.cpp" // sse rasterization routines
#elif defined(GC_PIPE_AVX)
#include "simd/gcsr_avx_rasterizer.cpp" // avx rasterization routines
#endif

#define gl_cube_samplef(sampler, i, output) \
{ \
    gl_texturef((sampler)->texture, (sampler)->u[i], (sampler)->v[i], (sampler)->mip_low, (output)); \
\
    if ((sampler)->texture->flags & TEXTURE_MIPS_FILTER) \
    { \
        gc_vec_t __tmp; \
\
        gl_texturef((sampler)->texture, (sampler)->u[i], (sampler)->v[i], (sampler)->mip_high, &__tmp); \
\
        (output)->c.r = (output)->c.r + (__tmp.c.r - (output)->c.r) * (sampler)->mip_interp; \
        (output)->c.g = (output)->c.g + (__tmp.c.g - (output)->c.g) * (sampler)->mip_interp; \
        (output)->c.b = (output)->c.b + (__tmp.c.b - (output)->c.b) * (sampler)->mip_interp; \
        (output)->c.a = (output)->c.a + (__tmp.c.a - (output)->c.a) * (sampler)->mip_interp; \
    } \
}

typedef gc_constant_t (* coverage_callback_t) (gc_primitive_t *primitive, gc_screen_rect_t *box);

__INLINE__ gc_constant_t gl_check_point_rect_coverage(gc_primitive_t *primitive, gc_screen_rect_t *box)
{
    OPTICK_EVENT("gl_check_point_rect_coverage");

    gc_constant_t res = GC_COVERAGE_NONE;
    s32 radius = 2;

    s32 pmin_x = primitive->base.pos[0] - radius;
    s32 pmin_y = primitive->base.pos[1] - radius;
    s32 pmax_x = primitive->base.pos[0] + radius;
    s32 pmax_y = primitive->base.pos[1] + radius;

    b8 check_0 = (pmin_x >= box->min.x && pmin_x < box->max.x) || (pmax_x >= box->min.x && pmax_x < box->max.x);
    b8 check_1 = (pmin_y >= box->min.y && pmin_y < box->max.y) || (pmax_y >= box->min.y && pmax_y < box->max.y);

    if (check_0 && check_1)
        res = GC_COVERAGE_PARTIAL;

    return res;
}

__INLINE__ gc_constant_t gl_check_line_rect_coverage(gc_primitive_t *primitive, gc_screen_rect_t *box)
{
    OPTICK_EVENT("gl_check_line_rect_coverage");

    gc_constant_t res = GC_COVERAGE_PARTIAL;

    if (primitive->line.is_dx)
    {
        s32 ymin = (s32) (primitive->line.a * box->min.x + primitive->line.b + 0.5f);
        s32 ymax = (s32) (primitive->line.a * box->max.x + primitive->line.b + 0.5f);

        if ((ymin < box->min.y || ymin > box->max.y) && (ymax < box->min.y || ymax > box->max.y))
            res = GC_COVERAGE_NONE;
    }
    else
    {
        s32 xmin = (s32) (primitive->line.a * box->min.y + primitive->line.b + 0.5f);
        s32 xmax = (s32) (primitive->line.a * box->max.y + primitive->line.b + 0.5f);

        if ((xmin < box->min.x || xmin > box->max.x) && (xmax < box->min.x || xmax > box->max.x))
            res = GC_COVERAGE_NONE;
    }

    return res;
}

__INLINE__ gc_constant_t gl_check_triangle_rect_coverage(gc_primitive_t *primitive, gc_screen_rect_t *box)
{
    OPTICK_EVENT("gl_check_triangle_rect_coverage");

    // ----------------------------------------------------------------------------------
    // -- Check convention:
    // --   >= 0 inside
    // --   < 0 outside
    // ----------------------------------------------------------------------------------

    gc_constant_t res = GC_COVERAGE_PARTIAL;

    s64 fp_p1x = (s64) FP_FROM_INT(box->min.x);
    s64 fp_p1y = (s64) FP_FROM_INT(box->min.y);

    s32 dx = box->max.x - box->min.x;
    s32 dy = box->max.y - box->min.y;

    s64 dx1 = (s64) dx * primitive->triangle.l1dx;
    s64 dx2 = (s64) dx * primitive->triangle.l2dx;
    s64 dx3 = (s64) dx * primitive->triangle.l3dx;

    s64 dy1 = (s64) dy * primitive->triangle.l1dy;
    s64 dy2 = (s64) dy * primitive->triangle.l2dy;
    s64 dy3 = (s64) dy * primitive->triangle.l3dy;

    s64 e1_start = primitive->triangle.l1a * fp_p1x + primitive->triangle.l1b * fp_p1y + primitive->triangle.l1c;
    s64 e2_start = primitive->triangle.l2a * fp_p1x + primitive->triangle.l2b * fp_p1y + primitive->triangle.l2c;
    s64 e3_start = primitive->triangle.l3a * fp_p1x + primitive->triangle.l3b * fp_p1y + primitive->triangle.l3c;

    s64 e1[4];

    e1[0] = e1_start;
    e1[1] = e1_start + dx1;
    e1[2] = e1_start + dx1 + dy1;
    e1[3] = e1_start + dy1;

    s64 e2[4];

    e2[0] = e2_start;
    e2[1] = e2_start + dx2;
    e2[2] = e2_start + dx2 + dy2;
    e2[3] = e2_start + dy2;

    s64 e3[4];

    e3[0] = e3_start;
    e3[1] = e3_start + dx3;
    e3[2] = e3_start + dx3 + dy3;
    e3[3] = e3_start + dy3;

    u8 a = ((e1[0] > 0) << 0) | ((e1[1] > 0) << 1) | ((e1[2] > 0) << 2) | ((e1[3] > 0) << 3);
    u8 b = ((e2[0] > 0) << 0) | ((e2[1] > 0) << 1) | ((e2[2] > 0) << 2) | ((e2[3] > 0) << 3);
    u8 c = ((e3[0] > 0) << 0) | ((e3[1] > 0) << 1) | ((e3[2] > 0) << 2) | ((e3[3] > 0) << 3);

    if (a == 0x0 || b == 0x0 || c == 0x0)
        res = GC_COVERAGE_NONE;
    else if (a == 0xf && b == 0xf && c == 0xf)
        res = GC_COVERAGE_TOTAL;

    return res;
}

// ----------------------------------------------------------------------------------
// -- Point block rasterization routine.
// ----------------------------------------------------------------------------------

#define POINT_MASK(cx, cy) \
    if ((cx) >= primitive->box.min.x && (cx) <= primitive->box.max.x && (cy) >= primitive->box.min.y && (cy) <= primitive->box.max.y) mask |= 0b1000; \
    if ((cx + 1) >= primitive->box.min.x && (cx + 1) <= primitive->box.max.x && (cy) >= primitive->box.min.y && (cy) <= primitive->box.max.y) mask |= 0b0100; \
    if ((cx) >= primitive->box.min.x && (cx) <= primitive->box.max.x && (cy + 1) >= primitive->box.min.y && (cy + 1) <= primitive->box.max.y) mask |= 0b0010; \
    if ((cx + 1) >= primitive->box.min.x && (cx + 1) <= primitive->box.max.x && (cy + 1) >= primitive->box.min.y && (cy + 1) <= primitive->box.max.y) mask |= 0b0001

void gl_pipe_rasterize_point(thread_batch_memory_t *batch_memory, gc_screen_rect_t *box, u16 fragment_index_start, gc_primitive_t *primitive)
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

    for (u32 r = frag_row_min; r <= frag_row_max; ++r)
    {
        s32 fy = r << 1;

        for (u32 c = frag_col_min; c <= frag_col_max; ++c)
        {
            s32 fx = c << 1;
            u8 mask = 0;

            POINT_MASK(fx + box->min.x, fy + box->min.y);

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
// -- Mask generation algorithm:
// ----------------------------------------------------------------------------------
// -- 2x2 fragment size
// -- LSB = least significant bit
// -- [x,0] => mask 1000
// -- [0,0]
// -- [0,x] => mask 0100
// -- [0,0]
// -- [0,0] => mask 0010
// -- [x,0]
// -- [0,0] => mask 0001
// -- [0,x]
// -- [x,x] => mask 1101
// -- [0,x]
// -- when x has LSB (even) = 0 => mask = 0010
// -- when x has LSB (odd) = 1 => mask = 0001
// -- when y has LSB (even) = 0 => mask = mask << 2
// -- when y has LSB (odd) = 1 => mask = mask
// ----------------------------------------------------------------------------------

#define CALC(coord) primitive->line.a * (coord) + primitive->line.b
#define TEST() \
    out_coord[0] = (s32) ((CALC(in_coord[0] - 0.5f) + CALC(in_coord[0] - 0.25f) + CALC(in_coord[0]) + CALC(in_coord[0] + 0.25f) + CALC(in_coord[0] + 0.5f)) * 0.2f + 0.5f); \
    out_coord[1] = (s32) ((CALC(in_coord[1] - 0.5f) + CALC(in_coord[1] - 0.25f) + CALC(in_coord[1]) + CALC(in_coord[1] + 0.25f) + CALC(in_coord[1] + 0.5f)) * 0.2f + 0.5f); \
    out_coord[2] = (s32) ((CALC(in_coord[2] - 0.5f) + CALC(in_coord[2] - 0.25f) + CALC(in_coord[2]) + CALC(in_coord[2] + 0.25f) + CALC(in_coord[2] + 0.5f)) * 0.2f + 0.5f); \
    out_coord[3] = (s32) ((CALC(in_coord[3] - 0.5f) + CALC(in_coord[3] - 0.25f) + CALC(in_coord[3]) + CALC(in_coord[3] + 0.25f) + CALC(in_coord[3] + 0.5f)) * 0.2f + 0.5f)

void gl_pipe_rasterize_line(thread_batch_memory_t *batch_memory, gc_screen_rect_t *box, u16 fragment_index_start, gc_primitive_t *primitive)
{
    OPTICK_EVENT("gl_pipe_rasterize_line");

    gc_processed_fragment_t *current_fragment = 0;

    // X variation.
    if (primitive->line.is_dx)
    {
        s32 start = box->min.x;
        s32 end = box->max.x;
        r32 start_val = primitive->line.a * start + primitive->line.b;
        r32 val_incr = primitive->line.a * 4;

        r32 computed_coord[4];

        computed_coord[0] = start_val;
        computed_coord[1] = start_val + primitive->line.a;
        computed_coord[2] = start_val + 2 * primitive->line.a;
        computed_coord[3] = start_val + 3 * primitive->line.a;

        // X input, Y output.
        for (s32 c = start; c < end; c += GC_FRAG_SIZE)
        {
            s32 in_coord[4];
            s32 out_coord[4];
            s32 frag_col[4];
            s32 frag_row[4];
            s32 frag_index[4];

            in_coord[0] = c;
            in_coord[1] = c + 1;
            in_coord[2] = c + 2;
            in_coord[3] = c + 3;

            out_coord[0] = (s32) (computed_coord[0] + 0.5f);
            out_coord[1] = (s32) (computed_coord[1] + 0.5f);
            out_coord[2] = (s32) (computed_coord[2] + 0.5f);
            out_coord[3] = (s32) (computed_coord[3] + 0.5f);

            frag_col[0] = (in_coord[0] - box->min.x) >> 1;
            frag_col[1] = (in_coord[1] - box->min.x) >> 1;
            frag_col[2] = (in_coord[2] - box->min.x) >> 1;
            frag_col[3] = (in_coord[3] - box->min.x) >> 1;

            frag_row[0] = (out_coord[0] - box->min.y) >> 1;
            frag_row[1] = (out_coord[1] - box->min.y) >> 1;
            frag_row[2] = (out_coord[2] - box->min.y) >> 1;
            frag_row[3] = (out_coord[3] - box->min.y) >> 1;

            frag_index[0] = frag_row[0] * GL_BIN_FRAG_COLS + frag_col[0];
            frag_index[1] = frag_row[1] * GL_BIN_FRAG_COLS + frag_col[1];
            frag_index[2] = frag_row[2] * GL_BIN_FRAG_COLS + frag_col[2];
            frag_index[3] = frag_row[3] * GL_BIN_FRAG_COLS + frag_col[3];

            s32 current_frag_index = -1;

            for (u8 i = 0; i < 4; ++i)
            {
                // if (in_coord[i] >= primitive->line.box_min_x && in_coord[i] <= primitive->line.box_max_x &&
                //     out_coord[i] >= box->min.y && out_coord[i] < box->max.y)

                if (in_coord[0] == 474 && out_coord[0] == 406)
                {
                    u8 a = 0;
                    a++;
                }

                if (in_coord[i] >= primitive->box.min.x && in_coord[i] <= primitive->box.max.x &&
                    out_coord[i] >= box->min.y && out_coord[i] < box->max.y)
                {
                    s32 check_frag_index = fragment_index_start + frag_index[i];

                    // Fragment push.
                    if (current_frag_index < 0 || current_frag_index != check_frag_index)
                    {
                        current_frag_index = check_frag_index;

                        PUSH_FRAGMENT(current_fragment);
                        SDL_assert(current_frag_index < GL_BIN_FRAGS);

                        current_fragment->mask = 0;
                        current_fragment->index = current_frag_index;
                        current_fragment->x = box->min.x + (frag_col[i] << 1);
                        current_fragment->y = box->min.y + (frag_row[i] << 1);
                        current_fragment->discarded = false;
                        current_fragment->primitive = primitive;
                    }

                    // Mask determination.
                    u8 mask = 0b0010;

                    // x.
                    if (in_coord[i] & 1)
                        mask = 0b0001;

                    // y.
                    if ((out_coord[i] & 1) == 0)
                        mask = mask << 2;

                    current_fragment->mask |= mask;

                    if (current_fragment->x == 474 && current_fragment->y == 406)
                    {
                        u8 a = 0;
                        a++;
                    }
                }
            }

            computed_coord[0] += val_incr;
            computed_coord[1] += val_incr;
            computed_coord[2] += val_incr;
            computed_coord[3] += val_incr;
        }
    }

    // Y variation.
    else
    {
        s32 start = box->min.y;
        s32 end = box->max.y;
        r32 start_val = primitive->line.a * start + primitive->line.b;
        r32 val_incr = primitive->line.a * 4;

        r32 computed_coord[4];
        A4SET(computed_coord, start_val, start_val + primitive->line.a, start_val + 2 * primitive->line.a, start_val + 3 * primitive->line.a);

        // Y input, X output.
        for (s32 c = start; c < end; c += GC_FRAG_SIZE)
        {
            s32 in_coord[4];
            s32 out_coord[4];
            s32 frag_row[4];
            s32 frag_col[4];
            s32 frag_index[4];

            A4SET(in_coord, c, c + 1, c + 2, c + 3);
            A4SET(out_coord, (s32) (computed_coord[0] + 0.5f),
                             (s32) (computed_coord[1] + 0.5f),
                             (s32) (computed_coord[2] + 0.5f),
                             (s32) (computed_coord[3] + 0.5f));

            // TEST();

            A4SET(frag_row, (in_coord[0] - box->min.y) >> 1,
                            (in_coord[1] - box->min.y) >> 1,
                            (in_coord[2] - box->min.y) >> 1,
                            (in_coord[3] - box->min.y) >> 1);

            A4SET(frag_col, (out_coord[0] - box->min.x) >> 1,
                            (out_coord[1] - box->min.x) >> 1,
                            (out_coord[2] - box->min.x) >> 1,
                            (out_coord[3] - box->min.x) >> 1);

            A4SET(frag_index, frag_row[0] * GL_BIN_FRAG_COLS + frag_col[0],
                              frag_row[1] * GL_BIN_FRAG_COLS + frag_col[1],
                              frag_row[2] * GL_BIN_FRAG_COLS + frag_col[2],
                              frag_row[3] * GL_BIN_FRAG_COLS + frag_col[3]);

            s32 current_frag_index = -1;

            for (u8 i = 0; i < 4; ++i)
            {
                // if (in_coord[i] >= primitive->line.box_min_y && in_coord[i] <= primitive->line.box_max_y &&
                //     out_coord[i] >= box->min.x && out_coord[i] < box->max.x)

                if (in_coord[i] >= primitive->box.min.y && in_coord[i] <= primitive->box.max.y &&
                    out_coord[i] >= box->min.x && out_coord[i] < box->max.x)
                {
                    s32 check_frag_index = fragment_index_start + frag_index[i];

                    // Fragment push.
                    if (current_frag_index < 0 || current_frag_index != check_frag_index)
                    {
                        current_frag_index = check_frag_index;

                        PUSH_FRAGMENT(current_fragment);
                        SDL_assert(current_frag_index < GL_BIN_FRAGS);

                        current_fragment->mask = 0;
                        current_fragment->index = current_frag_index;
                        current_fragment->x = box->min.x + (frag_col[i] << 1);
                        current_fragment->y = box->min.y + (frag_row[i] << 1);
                        current_fragment->discarded = false;
                        current_fragment->primitive = primitive;
                    }

                    // Mask determination.
                    u8 mask = 0b0010;

                    // x.
                    if (out_coord[i] & 1)
                        mask = 0b0001;

                    // y.
                    if ((in_coord[i] & 1) == 0)
                        mask = mask << 2;

                    current_fragment->mask |= mask;

                    if (current_fragment->x == 772 && current_fragment->y == 420)
                    {
                        u8 a = 0;
                        a++;
                    }
                }
            }

            computed_coord[0] += val_incr;
            computed_coord[1] += val_incr;
            computed_coord[2] += val_incr;
            computed_coord[3] += val_incr;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Triangle rasterization routine.
// ----------------------------------------------------------------------------------

void gl_pipe_rasterize_triangle(thread_batch_memory_t *batch_memory, gc_screen_rect_t *box, u16 fragment_index_start, gc_primitive_t *primitive)
{
    OPTICK_EVENT("gl_pipe_rasterize_triangle");

    gc_processed_fragment_t *current_fragment = 0;

    s64 fp_bx = box->min.x << FP_DEC_BIT;
    s64 fp_by = box->min.y << FP_DEC_BIT;

    s64 e1_start = primitive->triangle.l1a * fp_bx + primitive->triangle.l1b * fp_by + primitive->triangle.l1c;
    s64 e2_start = primitive->triangle.l2a * fp_bx + primitive->triangle.l2b * fp_by + primitive->triangle.l2c;
    s64 e3_start = primitive->triangle.l3a * fp_bx + primitive->triangle.l3b * fp_by + primitive->triangle.l3c;

    s64 base_frag_e1[4];
    s64 base_frag_e2[4];
    s64 base_frag_e3[4];

    A4SET(base_frag_e1, e1_start,
                        e1_start + primitive->triangle.l1dx,
                        e1_start + primitive->triangle.l1dy,
                        e1_start + primitive->triangle.l1dx + primitive->triangle.l1dy);

    A4SET(base_frag_e2, e2_start,
                        e2_start + primitive->triangle.l2dx,
                        e2_start + primitive->triangle.l2dy,
                        e2_start + primitive->triangle.l2dx + primitive->triangle.l2dy);

    A4SET(base_frag_e3, e3_start,
                        e3_start + primitive->triangle.l3dx,
                        e3_start + primitive->triangle.l3dy,
                        e3_start + primitive->triangle.l3dx + primitive->triangle.l3dy);

    s64 current_frag_e1[4];
    s64 current_frag_e2[4];
    s64 current_frag_e3[4];

    for (s32 y = box->min.y; y < box->max.y; y += GL_FRAG_HEIGHT)
    {
        s32 y_4x[4];

        A4SET(y_4x, y + GL_FRAG_OFFY_PIXEL0,
                    y + GL_FRAG_OFFY_PIXEL1,
                    y + GL_FRAG_OFFY_PIXEL2,
                    y + GL_FRAG_OFFY_PIXEL3);

        current_frag_e1[0] = base_frag_e1[0];
        current_frag_e1[1] = base_frag_e1[1];
        current_frag_e1[2] = base_frag_e1[2];
        current_frag_e1[3] = base_frag_e1[3];

        current_frag_e2[0] = base_frag_e2[0];
        current_frag_e2[1] = base_frag_e2[1];
        current_frag_e2[2] = base_frag_e2[2];
        current_frag_e2[3] = base_frag_e2[3];

        current_frag_e3[0] = base_frag_e3[0];
        current_frag_e3[1] = base_frag_e3[1];
        current_frag_e3[2] = base_frag_e3[2];
        current_frag_e3[3] = base_frag_e3[3];

        u16 fragment_index = fragment_index_start;

        for (s32 x = box->min.x; x < box->max.x; x += GL_FRAG_WIDTH)
        {
            s32 x_4x[4];

            A4SET(x_4x, x + GL_FRAG_OFFX_PIXEL0,
                        x + GL_FRAG_OFFX_PIXEL1,
                        x + GL_FRAG_OFFX_PIXEL2,
                        x + GL_FRAG_OFFX_PIXEL3);

            u8 mask = 0;

            if (current_frag_e1[0] > 0 && current_frag_e2[0] > 0 && current_frag_e3[0] > 0)
                mask |= (1 << 3);

            if (current_frag_e1[1] > 0 && current_frag_e2[1] > 0 && current_frag_e3[1] > 0)
                mask |= (1 << 2);

            if (current_frag_e1[2] > 0 && current_frag_e2[2] > 0 && current_frag_e3[2] > 0)
                mask |= (1 << 1);

            if (current_frag_e1[3] > 0 && current_frag_e2[3] > 0 && current_frag_e3[3] > 0)
                mask |= 1;

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

            current_frag_e1[0] += primitive->triangle.frag_l1dx;
            current_frag_e1[1] += primitive->triangle.frag_l1dx;
            current_frag_e1[2] += primitive->triangle.frag_l1dx;
            current_frag_e1[3] += primitive->triangle.frag_l1dx;

            current_frag_e2[0] += primitive->triangle.frag_l2dx;
            current_frag_e2[1] += primitive->triangle.frag_l2dx;
            current_frag_e2[2] += primitive->triangle.frag_l2dx;
            current_frag_e2[3] += primitive->triangle.frag_l2dx;

            current_frag_e3[0] += primitive->triangle.frag_l3dx;
            current_frag_e3[1] += primitive->triangle.frag_l3dx;
            current_frag_e3[2] += primitive->triangle.frag_l3dx;
            current_frag_e3[3] += primitive->triangle.frag_l3dx;
        }

        base_frag_e1[0] += primitive->triangle.frag_l1dy;
        base_frag_e1[1] += primitive->triangle.frag_l1dy;
        base_frag_e1[2] += primitive->triangle.frag_l1dy;
        base_frag_e1[3] += primitive->triangle.frag_l1dy;

        base_frag_e2[0] += primitive->triangle.frag_l2dy;
        base_frag_e2[1] += primitive->triangle.frag_l2dy;
        base_frag_e2[2] += primitive->triangle.frag_l2dy;
        base_frag_e2[3] += primitive->triangle.frag_l2dy;

        base_frag_e3[0] += primitive->triangle.frag_l3dy;
        base_frag_e3[1] += primitive->triangle.frag_l3dy;
        base_frag_e3[2] += primitive->triangle.frag_l3dy;
        base_frag_e3[3] += primitive->triangle.frag_l3dy;

        fragment_index_start += GL_BIN_FRAG_COLS;
    }
}

// ----------------------------------------------------------------------------------
// -- Computes the wireframe color for a specified single-pass-wireframe object.
// ----------------------------------------------------------------------------------

#if 0
__INLINE__ vec4 gl_single_pass_wireframe_filter(gc_single_pass_wireframe_t *Wireframe,
                                         vec4 v1, vec4 v2, vec4 v3,
                                         r32 d1, r32 d2, r32 d3,
                                         s32 fragX, s32 fragY,
                                         vec4 frag_c)
{
    vec4 wire_c = Wireframe->color;

    wire_c.r *= Wireframe->color.a;
    wire_c.g *= Wireframe->color.a;
    wire_c.b *= Wireframe->color.a;
    wire_c.a = Wireframe->color.a;

    frag_c.r *= frag_c.a;
    frag_c.g *= frag_c.a;
    frag_c.b *= frag_c.a;

    vec4 lineP1 = v2;
    vec4 lineP2 = v3;
    r32 distance = d1;

    if (d2 < distance)
    {
        lineP1 = v3;
        lineP2 = v1;
        distance = d2;
    }

    if (d3 < distance)
    {
        lineP1 = v1;
        lineP2 = v2;
        distance = d3;
    }

    r32 I = 0;
    r32 d_max = Wireframe->d_max;
    vec2 edgeVector = {lineP2.x - lineP1.x, lineP2.y - lineP1.y};
    vec2 edge = vec2_normalize(edgeVector);
    vec2 lineVector = {fragX - lineP1.x, fragY - lineP1.y};
    r32 proj = vec2_dot(lineVector, edge);
    vec2 edgeDistanceVector = vec2_muls(edge, proj);
    r32 fullEdgeDistance = vec2_len(edgeVector);
    r32 edgeDistance = vec2_len(edgeDistanceVector);
    r32 rel = edgeDistance / fullEdgeDistance;
    r32 interval = rel * (4 * Wireframe->n - 1) * PI;
    r32 s = 0;

    b8 isDotted = (Wireframe->type == SPW_DOTTED);

    if (isDotted)
        s = t_sin(interval);
    else if (Wireframe->type == SPW_WAVE)
    {
        s = t_sin(interval - Wireframe->wave_offset_x) + Wireframe->wave_offset_y;
        d_max = s * Wireframe->wave_scaling;
    }

    if (distance < d_max)
    {
        I = 1;

        if (!Wireframe->noFilter)
            I = expf( - (2 / Wireframe->scaling) * distance * distance);

        if (isDotted && s < 0)
            I = 0;
    }

    // ----------------------------------------------------------------------------------
    // NOTE(gabic): There is a problem when the line is on a primitive with transparency.
    // The wireframe color is computed using the fragment color, not the final compounded
    // transparent color. For this I should send the pixel to the transparency buffer
    // and mark it special so it can be processed as a spw line.
    // ----------------------------------------------------------------------------------
    // Transparent pixels are computed at the end using a separate transparancy buffer.
    // ----------------------------------------------------------------------------------

    // First determine the blended wire + frag color.

    r32 one_minus_a = 1.0f - wire_c.a;
    wire_c.r += frag_c.r * one_minus_a;
    wire_c.g += frag_c.g * one_minus_a;
    wire_c.b += frag_c.b * one_minus_a;
    wire_c.a += frag_c.a * one_minus_a;

    // Apply the attenuation function.
    one_minus_a = 1.0f - I;

    wire_c.r = I * wire_c.r + one_minus_a * frag_c.r;
    wire_c.g = I * wire_c.g + one_minus_a * frag_c.g;
    wire_c.b = I * wire_c.b + one_minus_a * frag_c.b;
    wire_c.a = I * wire_c.a + one_minus_a * frag_c.a;

    return wire_c;
}
#endif

#if 0
__INLINE__ void gl_spwColor(gl_fragpack_t *current_pack)
{
    gc_single_pass_wireframe_t *Wireframe = GCSR.gl->Config.Wireframe;

    vec4 wireColor = Wireframe->color;

    wireColor.r *= wireColor.a;
    wireColor.g *= wireColor.a;
    wireColor.b *= wireColor.a;
    // wireColor.a = Wireframe->color.a;

    for (u32 i = 0; i < current_pack->frag_count; ++i)
    {
        gl_fragment_pixel_t *CurrentPixelFragments = current_pack->pixels + i;
        gl_fragment_varyings_t *CurrentAttributeFragments = current_pack->varyings + i;

        for (u32 j = 0; j < GC_FRAG_SIZE; ++j)
        {
            r32 intensity = CurrentAttributeFragments->spw_intensity[j];

            vec4 wc = wireColor;

            vec4 fragColor = {
                CurrentPixelFragments->colorr[j],
                CurrentPixelFragments->colorg[j],
                CurrentPixelFragments->colorb[j],
                CurrentPixelFragments->colora[j]
            };

            if (CurrentPixelFragments->offset[j] == 677633)
            {
                u8 a = 0;
                a++;
            }

            // fragColor.r *= fragColor.a;
            // fragColor.g *= fragColor.a;
            // fragColor.b *= fragColor.a;

            // First determine the blended wire + frag color.

            r32 oneMinusA = 1.0f - wc.a;

            wc.r += fragColor.r * oneMinusA;
            wc.g += fragColor.g * oneMinusA;
            wc.b += fragColor.b * oneMinusA;
            wc.a += fragColor.a * oneMinusA;

            // Apply the attenuation function.
            oneMinusA = 1.0f - intensity;

            fragColor.r = intensity * wc.r + oneMinusA * fragColor.r;
            fragColor.g = intensity * wc.g + oneMinusA * fragColor.g;
            fragColor.b = intensity * wc.b + oneMinusA * fragColor.b;
            fragColor.a = intensity * wc.a + oneMinusA * fragColor.a;

            if (intensity > 0)
                fragColor.a = 1.0f;

            CurrentPixelFragments->colorr[j] = fragColor.r;
            CurrentPixelFragments->colorg[j] = fragColor.g;
            CurrentPixelFragments->colorb[j] = fragColor.b;
            CurrentPixelFragments->colora[j] = fragColor.a;
        }
    }
}
#endif

#if 0
__INLINE__ void gl_spwIntensity(gc_primitive_t *primitive,
                            r32 l1, r32 l2, r32 l3,
                            u32 fragX, u32 fragY, r32 *I)
{
    gc_single_pass_wireframe_t *Wireframe = GCSR.gl->Config.Wireframe;

    r32 *v1Pos = primitive->v1->pos;
    r32 *v2Pos = primitive->v2->pos;
    r32 *v3Pos = primitive->v3->pos;

    r32 spw_d1 = primitive->triangle.spw_d1 * l1;
    r32 spw_d2 = primitive->triangle.spw_d2 * l2;
    r32 spw_d3 = primitive->triangle.spw_d3 * l3;

    r32 *lineP1 = v2Pos;
    r32 *lineP2 = v3Pos;
    r32 distance = spw_d1;

    if (spw_d2 < distance)
    {
        lineP1 = v3Pos;
        lineP2 = v1Pos;
        distance = spw_d2;
    }

    if (spw_d3 < distance)
    {
        lineP1 = v1Pos;
        lineP2 = v2Pos;
        distance = spw_d3;
    }

    r32 d_max = Wireframe->d_max;
    vec2 edgeVector = {lineP2[0] - lineP1[0], lineP2[1] - lineP1[1]};
    r32 fullEdgeDistance = vec2_len(edgeVector);
    vec2 edge = vec2_normalize(edgeVector);

    vec2 lineVector = {fragX - lineP1[0], fragY - lineP1[1]};
    r32 proj = vec2_dot(lineVector, edge);
    vec2 edgeDistanceVector = vec2_muls(edge, proj);
    r32 edgeDistance = vec2_len(edgeDistanceVector);

    r32 rel = edgeDistance / fullEdgeDistance;
    r32 interval = rel * (4 * Wireframe->n - 1) * PI;
    r32 s = 0;

    b8 isDotted = (Wireframe->type == SPW_DOTTED);

    if (isDotted)
        s = t_sin(interval);
    else if (Wireframe->type == SPW_WAVE)
    {
        s = t_sin(interval - Wireframe->wave_offset_x) + Wireframe->wave_offset_y;
        d_max = s * Wireframe->wave_scaling;
    }

    if (distance < d_max)
    {
        *I = 1;

        if (!Wireframe->noFilter)
            *I = expf( - (2 / Wireframe->scaling) * distance * distance);

        if (isDotted && s < 0)
            *I = 0;
    }
}
#endif