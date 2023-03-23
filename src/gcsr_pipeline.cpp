// ----------------------------------------------------------------------------------
// -- File: gcsr_pipeline.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-05-15 21:51:27
// -- Modified: 2022-06-02 20:58:28
// ----------------------------------------------------------------------------------

void gl_framebuffer_to_tile_buffer(gc_tile_buffer_t *tile_buffer);
void gl_tile_buffer_to_framebuffer(gc_tile_buffer_t *tile_buffer);
void gl_fragment_varyings_setup(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer);
void _fragment_merge(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer, gc_transparency_bin_t *transparency_buffer);
void gl_pipe_transparency_push(gc_transparency_frag_t *transparency_frag, gc_processed_fragment_t *fragment, u32 transparency_mask);

extern global_vars_t GCSR;

#if defined(GL_PIPE_AVX)
#include "simd/gcsr_avx_pipeline.cpp"   // avx rasterization routines
#define gc_pipe_transparency_process _sse_transparency_process
#elif defined(GC_PIPE_SSE)
#include "simd/gcsr_sse_pipeline.cpp"   // sse rasterization routines
#define gc_pipe_transparency_process _sse_transparency_process
#else
#define gc_pipe_transparency_process _transparency_process
#endif

u32 thread_primitive_id[GC_PIPE_NUM_THREADS];

void gc_pipe_binning(u32 thread_id, gc_pipe_array_t *primitive_list, gc_mesh_type_t type)
{
    gc_bin_t *bins = GCSR.gl->current_framebuffer->bins;
    gc_framebuffer_t *framebuffer = GCSR.gl->current_framebuffer;
    pipe_memory_t *thread_memory = &GCSR.memory_manager->pipeline[thread_id];

    coverage_callback_t coverage_func = gl_check_triangle_rect_coverage;

    if (type == GL_MESH_LINE)
        coverage_func = gl_check_line_rect_coverage;
    else if (type == GL_MESH_POINT)
        coverage_func = gl_check_point_rect_coverage;

    for (u32 k = 0; k < primitive_list->count; ++k)
    {
        gc_primitive_t *primitive = (gc_primitive_t *) primitive_list->data + k;

        u32 bin_row_min = primitive->box.min.y / GL_BIN_HEIGHT;
        u32 bin_col_min = primitive->box.min.x / GL_BIN_WIDTH;
        u32 bin_row_max = primitive->box.max.y / GL_BIN_HEIGHT;
        u32 bin_col_max = primitive->box.max.x / GL_BIN_WIDTH;

        for (u32 row = bin_row_min; row <= bin_row_max; ++row)
        {
            for (u32 col = bin_col_min; col <= bin_col_max; ++col)
            {
                DEBUG_SHOW_ONLY_BIN(row, col);

                u16 bin_index = (u16) (row * framebuffer->bin_cols + col);
                gc_bin_t *current_bin = bins + bin_index;

                // u16 bsx = col * GL_BIN_WIDTH;
                // u16 bsy = row * GL_BIN_HEIGHT;

                SCREEN_RECT_SET(bin_box,
                    current_bin->x, current_bin->y,
                    current_bin->x + GL_BIN_WIDTH - 1,
                    current_bin->y + GL_BIN_HEIGHT - 1);

                // b8 is_over = false;
                gc_constant_t coverage = coverage_func(primitive, &bin_box);

                // if (primitive->type == GL_MESH_TRIANGLE)
                // else if (primitive->type == GL_MESH_LINE)
                //     coverage = gl_check_line_block_coverage(primitive, &bin_box);

                if (coverage == GC_COVERAGE_NONE)
                    continue;

                u32 dirty_count = SDL_AtomicAdd(&current_bin->dirty, 1);

                // The bin was not added to the queue.
                if (!dirty_count)
                {
                    u32 idx = SDL_AtomicAdd(&GCSR.gl->bin_queue.count, 1);
                    SDL_assert(idx <= GCSR.gl->current_framebuffer->total_bins);
                    GCSR.gl->bin_queue.bins[idx] = bin_index;
                }

                // Primitive linked list for the current thread.
                gc_bin_prim_list_t *list = &current_bin->list[thread_id];
                gc_bin_prim_link_t *link = (gc_bin_prim_link_t *) ((u8 *) thread_memory->data + thread_memory->cursor);
                thread_memory->cursor += sizeof(gc_bin_prim_link_t);
                SDL_assert(thread_memory->cursor <= thread_memory->bytes);

                link->next = 0;
                link->prim_idx = k;
                link->total_coverage = (coverage == GC_COVERAGE_TOTAL);

                if (!list->start)
                {
                    list->start = link;
                    list->last = link;
                }
                else
                    list->last->next = link;

                list->last = link;
                list->count++;
            }
        }
    }
}

void gl_pipe_frontend_point(u32 thread_id)
{
    OPTICK_EVENT("gl_pipe_frontend_point");

    pipe_memory_t *thread_memory = &GCSR.memory_manager->pipeline[thread_id];
    gc_pipe_array_t *primitive_list = (gc_pipe_array_t *) ADDR_OFFSET(thread_memory->data, thread_memory->cursor);
    thread_memory->cursor += sizeof(gc_pipe_array_t);
    GCSR.gl->primitives[thread_id] = primitive_list;

    primitive_list->data = primitive_list + 1;
    primitive_list->count = 0;

    SDL_assert(thread_memory->cursor <= thread_memory->bytes);

    gl_pipeline_state_t *pipeline = &GCSR.gl->pipeline;
    gc_shader_t *shader = GCSR.gl->pipeline.shader;

    gc_partition_t *partition = &GCSR.gl->partitions[thread_id];
    gc_vset_t current_set;

    for (u32 i = partition->start_index; i < partition->end_index; i += 1)
    {
        u32 primitive_id = thread_primitive_id[thread_id]++;

        current_set.area = 0;
        current_set.flags = 0;

        shader->read(&current_set,
                     GCSR.gl->current_arrays.indices,
                     GCSR.gl->current_arrays.vertices,
                     GCSR.gl->curr_idx + i);

        shader->vs(&current_set.v[0], &pipeline->params);
        u8 clip_codes = gl_compute_clip_codes(&current_set.v[0]);

        if (0xff & clip_codes)
            continue;

        gc_vertex_t *vertex_1 = &current_set.v[0];

        // ----------------------------------------------------------------------------------
        // -- Vertex transform.
        // ----------------------------------------------------------------------------------

        {
            OPTICK_EVENT("vertex transform");

            r32 one_over_w = 1.0f / vertex_1->pos[3];

            vertex_1->pos[0] *= one_over_w;
            vertex_1->pos[1] *= one_over_w;
            vertex_1->pos[2] *= one_over_w;
            vertex_1->pos[3] = one_over_w;

            gc_mat_t *m = GET_MATRIX(M_VIEWPORT);

            #ifndef GC_PIPE_SSE
                vertex_1->pos[0] = m->data[0][0] * vertex_1->pos[0] + m->data[0][3];
                vertex_1->pos[1] = m->data[1][1] * vertex_1->pos[1] + m->data[1][3];
                vertex_1->pos[2] = m->data[2][2] * vertex_1->pos[2] + m->data[2][3];
            #else
                __m128 t1_4x = _mm_setr_ps(m->data[0][0], m->data[1][1], m->data[2][2], 0);
                __m128 t2_4x = _mm_setr_ps(m->data[0][3], m->data[1][3], m->data[2][3], 0);

                r32 tmp_w = vertex_1->pos[3];
                __m128 vertex_4x = _mm_load_ps(vertex_1->pos);
                vertex_4x = _mm_add_ps(_mm_mul_ps(vertex_4x, t1_4x), t2_4x);
                _mm_store_ps(vertex_1->pos, vertex_4x);
                vertex_1->pos[3] = tmp_w;
            #endif
        }

        // ----------------------------------------------------------------------------------
        // -- Point setup.
        // ----------------------------------------------------------------------------------

        {
            OPTICK_EVENT("point setup");

            gc_primitive_t *primitive = (gc_primitive_t *) primitive_list->data + (primitive_list->count++);
            thread_memory->cursor += sizeof(gc_primitive_t);

            primitive->id = primitive_id;
            primitive->type = GL_MESH_POINT;

            gc_vertex_t *base_vertex = vertex_1;

            primitive->base.pos[0] = base_vertex->pos[0];
            primitive->base.pos[1] = base_vertex->pos[1];
            primitive->base.pos[2] = base_vertex->pos[2];
            primitive->base.pos[3] = base_vertex->pos[3];

            // Color setup.
            for (u32 ii = 0; ii < 4; ++ii) {
                primitive->base.data[ii] = base_vertex->data[ii];
            }

#ifdef DEBUG_PRIMITIVE
            primitive->v1.pos[0] = vertex_1->pos[0];
            primitive->v1.pos[1] = vertex_1->pos[1];
            primitive->v1.pos[2] = vertex_1->pos[2];
            primitive->v1.pos[3] = vertex_1->pos[3];

            for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii) {
                primitive->v1.data[ii] = vertex_1->data[ii];
            }
#endif
            primitive->point.radius = PIPE_PARAM_VALUE_INTEGER(1, point_radius);

            primitive->box.min.x = primitive->base.pos[0] - primitive->point.radius;
            primitive->box.max.x = primitive->base.pos[0] + primitive->point.radius;
            primitive->box.min.y = primitive->base.pos[1] - primitive->point.radius;
            primitive->box.max.y = primitive->base.pos[1] + primitive->point.radius;

            if (primitive->box.min.x < 0)
                primitive->box.min.x = 0;

            if (primitive->box.min.y < 0)
                primitive->box.min.y = 0;
        }
    }

    gc_pipe_binning(thread_id, primitive_list, GL_MESH_POINT);
}

void gl_pipe_frontend_line(u32 thread_id)
{
    OPTICK_EVENT("gl_pipe_frontend_line");

    pipe_memory_t *thread_memory = &GCSR.memory_manager->pipeline[thread_id];
    gc_pipe_array_t *primitive_list = (gc_pipe_array_t *) ADDR_OFFSET(thread_memory->data, thread_memory->cursor);
    thread_memory->cursor += sizeof(gc_pipe_array_t);
    GCSR.gl->primitives[thread_id] = primitive_list;

    primitive_list->data = primitive_list + 1;
    primitive_list->count = 0;

    SDL_assert(thread_memory->cursor <= thread_memory->bytes);

    gl_pipeline_state_t *pipeline = &GCSR.gl->pipeline;
    gc_shader_t *shader = GCSR.gl->pipeline.shader;

    u32 clip_planes = CLIP_NEAR | CLIP_FAR;

    gc_vset_t set_buffer[21];
    gc_clipping_vertex_buffer_t clip_buffer;
    gc_partition_t *partition = &GCSR.gl->partitions[thread_id];
    // u32 primitive_id = 0;

    for (u32 i = partition->start_index; i < partition->end_index; i += 2)
    {
        u32 primitive_id = thread_primitive_id[thread_id]++;

        gc_vset_t *current_set = &set_buffer[0];
        current_set->area = 0;
        current_set->flags = 0;

        u32 set_count = 0;
        u8 is_outside = 0xff;
        u8 is_clip = 0;

        shader->read(current_set,
                     GCSR.gl->current_arrays.indices,
                     GCSR.gl->current_arrays.vertices,
                     GCSR.gl->curr_idx + i);

        gc_vec_t check_vertices[2];

        set_count++;

        // ----------------------------------------------------------------------------------
        // -- Vertex shader.
        // ----------------------------------------------------------------------------------

        for (u32 j = 0; j < 2; ++j)
        {
            gc_vertex_t *vertex = &current_set->v[j];

            check_vertices[j].data[0] = vertex->pos[0];
            check_vertices[j].data[1] = vertex->pos[1];
            check_vertices[j].data[2] = vertex->pos[2];
            check_vertices[j].data[3] = vertex->pos[3];

            shader->vs(vertex, &pipeline->params);

            u8 clip_codes = gl_compute_clip_codes(vertex);

            is_outside &= clip_codes;
            is_clip |= clip_codes & clip_planes;
        }

        if (is_outside)
            continue;

        // ----------------------------------------------------------------------------------
        // -- Clipping.
        // ----------------------------------------------------------------------------------

        // TODO(gabic): guard band clipping !
        if (is_clip)
        {
            current_set->flags |= GL_SET_DISCARDED;
            gl_clip_line(current_set, &set_count, &clip_buffer, pipeline->varying_count);
        }

        // ----------------------------------------------------------------------------------
        // -- Transformations and setup.
        // ----------------------------------------------------------------------------------

        for (u32 j = 0; j < set_count; ++j)
        {
            current_set = &set_buffer[j];

            if (current_set->flags & GL_SET_DISCARDED)
                continue;

            gc_vertex_t *vertex_1 = &current_set->v[0];
            gc_vertex_t *vertex_2 = &current_set->v[1];

            // ----------------------------------------------------------------------------------
            // -- Vertex transform.
            // ----------------------------------------------------------------------------------

            {
                OPTICK_EVENT("vertex transform");

                r32 one_over_w = 1.0f / vertex_1->pos[3];

                vertex_1->pos[0] *= one_over_w;
                vertex_1->pos[1] *= one_over_w;
                vertex_1->pos[2] *= one_over_w;
                vertex_1->pos[3] = one_over_w;

                one_over_w = 1.0f / vertex_2->pos[3];

                vertex_2->pos[0] *= one_over_w;
                vertex_2->pos[1] *= one_over_w;
                vertex_2->pos[2] *= one_over_w;
                vertex_2->pos[3] = one_over_w;

                gc_mat_t *m = GET_MATRIX(M_VIEWPORT);

            #ifndef GC_PIPE_SSE
                vertex_1->pos[0] = m->data[0][0] * vertex_1->pos[0] + m->data[0][3];
                vertex_1->pos[1] = m->data[1][1] * vertex_1->pos[1] + m->data[1][3];
                vertex_1->pos[2] = m->data[2][2] * vertex_1->pos[2] + m->data[2][3];

                vertex_2->pos[0] = m->data[0][0] * vertex_2->pos[0] + m->data[0][3];
                vertex_2->pos[1] = m->data[1][1] * vertex_2->pos[1] + m->data[1][3];
                vertex_2->pos[2] = m->data[2][2] * vertex_2->pos[2] + m->data[2][3];
            #else
                __m128 t1_4x = _mm_setr_ps(m->data[0][0], m->data[1][1], m->data[2][2], 0);
                __m128 t2_4x = _mm_setr_ps(m->data[0][3], m->data[1][3], m->data[2][3], 0);

                r32 tmp_w = vertex_1->pos[3];
                __m128 vertex_4x = _mm_load_ps(vertex_1->pos);
                vertex_4x = _mm_add_ps(_mm_mul_ps(vertex_4x, t1_4x), t2_4x);
                _mm_store_ps(vertex_1->pos, vertex_4x);
                vertex_1->pos[3] = tmp_w;

                tmp_w = vertex_2->pos[3];
                vertex_4x = _mm_load_ps(vertex_2->pos);
                vertex_4x = _mm_add_ps(_mm_mul_ps(vertex_4x, t1_4x), t2_4x);
                _mm_store_ps(vertex_2->pos, vertex_4x);
                vertex_2->pos[3] = tmp_w;
            #endif
            }

            // ----------------------------------------------------------------------------------
            // -- Line setup.
            // ----------------------------------------------------------------------------------

            // if (primitive_id != 6 && primitive_id != 7)
            //     continue;

            // if (primitive_id == 0)
            //     continue;

            // if (primitive_id == 1)
            //     continue;

            DEBUG_SHOW_ONLY_LINE();

            {
                OPTICK_EVENT("line setup");

                gc_primitive_t *primitive = (gc_primitive_t *) primitive_list->data + (primitive_list->count++);
                thread_memory->cursor += sizeof(gc_primitive_t);

                primitive->id = primitive_id;
                primitive->type = GL_MESH_LINE;

                vertex_1->pos[2] -= 0.0001f;
                vertex_2->pos[2] -= 0.0001f;

                gc_vertex_t *base_vertex = vertex_1;

                // Compute which is the closest vertex from the origin and choose that as base.
                r32 c1 = vertex_1->pos[0] * vertex_1->pos[0] + vertex_1->pos[1] * vertex_1->pos[1];
                r32 c2 = vertex_2->pos[0] * vertex_2->pos[0] + vertex_2->pos[1] * vertex_2->pos[1];

                if (c2 < c1)
                    base_vertex = vertex_2;

                primitive->base.pos[0] = base_vertex->pos[0];
                primitive->base.pos[1] = base_vertex->pos[1];
                primitive->base.pos[2] = base_vertex->pos[2];
                primitive->base.pos[3] = base_vertex->pos[3];

                // Color setup.
                for (u32 ii = 0; ii < 4; ++ii) {
                    primitive->base.data[ii] = base_vertex->data[ii];
                }

#ifdef DEBUG_PRIMITIVE
                primitive->v1.pos[0] = vertex_1->pos[0];
                primitive->v1.pos[1] = vertex_1->pos[1];
                primitive->v1.pos[2] = vertex_1->pos[2];
                primitive->v1.pos[3] = vertex_1->pos[3];

                for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii) {
                    primitive->v1.data[ii] = vertex_1->data[ii];
                }

                primitive->v2.pos[0] = vertex_2->pos[0];
                primitive->v2.pos[1] = vertex_2->pos[1];
                primitive->v2.pos[2] = vertex_2->pos[2];
                primitive->v2.pos[3] = vertex_2->pos[3];

                for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii) {
                    primitive->v2.data[ii] = vertex_2->data[ii];
                }
#endif

                // ----------------------------------------------------------------------------------
                // -- Determine the line's bounding box.
                // ----------------------------------------------------------------------------------

                r32 min_x = vertex_1->pos[0];
                r32 min_y = vertex_1->pos[1];
                r32 max_x = vertex_1->pos[0];
                r32 max_y = vertex_1->pos[1];

                if (vertex_2->pos[0] < min_x)
                    min_x = vertex_2->pos[0];
                else if (vertex_2->pos[0] > max_x)
                    max_x = vertex_2->pos[0];

                if (vertex_2->pos[1] < min_y)
                    min_y = vertex_2->pos[1];
                else if (vertex_2->pos[1] > max_y)
                    max_y = vertex_2->pos[1];

                primitive->box.min.x = (s32) (min_x + 0.5f);
                primitive->box.max.x = (s32) (max_x + 0.5f);
                primitive->box.min.y = (s32) (min_y + 0.5f);
                primitive->box.max.y = (s32) (max_y + 0.5f);

                // ----------------------------------------------------------------------------------
                // -- Crop the triangle area to the screen.
                // ----------------------------------------------------------------------------------

                if (primitive->box.min.x < 0)
                    primitive->box.min.x = 0;

                if (primitive->box.min.y < 0)
                    primitive->box.min.y = 0;

                if (primitive->box.max.x >= (s32) GCSR.gl->current_framebuffer->tiled_width)
                    primitive->box.max.x = GCSR.gl->current_framebuffer->tiled_width - 1;

                if (primitive->box.max.y >= (s32) GCSR.gl->current_framebuffer->tiled_height)
                    primitive->box.max.y = GCSR.gl->current_framebuffer->tiled_height - 1;

                // -- Data used to determine if a point is within the line's range.

                primitive->line.box_min_x = min_x;
                primitive->line.box_min_y = min_y;
                primitive->line.box_max_x = max_x;
                primitive->line.box_max_y = max_y;

                // ----------------------------------------------------------------------------------
                // -- Line equation setup.
                // ----------------------------------------------------------------------------------

                primitive->line.is_dx = false;

                r32 dx = primitive->line.box_max_x - primitive->line.box_min_x;
                r32 dy = primitive->line.box_max_y - primitive->line.box_min_y;

                if (dx >= dy)
                    primitive->line.is_dx = true;

                primitive->line.la = vertex_2->pos[1] - vertex_1->pos[1];
                primitive->line.lb = vertex_1->pos[0] - vertex_2->pos[0];
                // primitive->line.lc = vertex_1->pos[1] * vertex_2->pos[0] - vertex_1->pos[0] * vertex_2->pos[1];
                primitive->line.lc = vertex_1->pos[1] * (vertex_2->pos[0] - vertex_1->pos[0]) + vertex_1->pos[0] * (vertex_1->pos[1] - vertex_2->pos[1]);

                if (primitive->line.is_dx)
                {
                    primitive->line.a = -primitive->line.la / primitive->line.lb;
                    primitive->line.b = -primitive->line.lc / primitive->line.lb;
                }
                else
                {
                    primitive->line.a = -primitive->line.lb / primitive->line.la;
                    primitive->line.b = -primitive->line.lc / primitive->line.la;
                }

                // ----------------------------------------------------------------------------------
                // -- Partial derivatives setup.
                // ----------------------------------------------------------------------------------

                if (primitive->line.is_dx)
                    primitive->line.interp_z = (vertex_2->pos[2] - vertex_1->pos[2]) / (vertex_2->pos[0] - vertex_1->pos[0]);
                else
                    primitive->line.interp_z = (vertex_2->pos[2] - vertex_1->pos[2]) / (vertex_2->pos[1] - vertex_1->pos[1]);
            }
        }
    }

    gc_pipe_binning(thread_id, primitive_list, GL_MESH_LINE);
}

void gl_pipe_frontend_triangle(u32 thread_id)
{
    OPTICK_EVENT("gl_pipe_frontend_triangle");

    pipe_memory_t *thread_memory = &GCSR.memory_manager->pipeline[thread_id];
    gc_pipe_array_t *primitive_list = (gc_pipe_array_t *) ADDR_OFFSET(thread_memory->data, thread_memory->cursor);
    thread_memory->cursor += sizeof(gc_pipe_array_t);
    GCSR.gl->primitives[thread_id] = primitive_list;

    primitive_list->data = primitive_list + 1;
    primitive_list->count = 0;

    SDL_assert(thread_memory->cursor <= thread_memory->bytes);

    gl_pipeline_state_t *pipeline = &GCSR.gl->pipeline;
    gc_shader_t *shader = GCSR.gl->pipeline.shader;

    // u32 clip_planes = CLIP_NEAR | CLIP_FAR;
    u32 clip_planes = CLIP_NEAR;

    gc_vset_t set_buffer[21];
    gc_clipping_vertex_buffer_t clip_buffer;

    gc_partition_t *partition = &GCSR.gl->partitions[thread_id];
    b8 do_backface_culling = FLAG(GCSR.gl->pipeline.flags, GC_BACKFACE_CULL);
    // u32 primitive_id = 0;

    for (u32 i = partition->start_index; i < partition->end_index; i += 3)
    {
        u32 primitive_id = thread_primitive_id[thread_id]++;
        gc_vset_t *current_set = &set_buffer[0];

        current_set->area = 0;
        current_set->flags = 0;

        u32 set_count = 0;
        u8 is_outside = 0xff;
        u8 is_clip = 0;

        shader->read(current_set,
                     GCSR.gl->current_arrays.indices,
                     GCSR.gl->current_arrays.vertices,
                     GCSR.gl->curr_idx + i);

        set_count++;

        // ----------------------------------------------------------------------------------
        // -- Vertex shader.
        // ----------------------------------------------------------------------------------

        for (u32 j = 0; j < 3; ++j)
        {
            gc_vertex_t *vertex = &current_set->v[j];

            shader->vs(vertex, &pipeline->params);

            u8 clip_codes = gl_compute_clip_codes(vertex);

            is_outside &= clip_codes;
            is_clip |= clip_codes & clip_planes;
        }

        if (is_outside)
            continue;

        // ----------------------------------------------------------------------------------
        // -- Clipping.
        // ----------------------------------------------------------------------------------

        // TODO(gabic): guard band clipping !
        if (is_clip)
        {
            current_set->flags |= GL_SET_DISCARDED;
            gl_clip_triangle(current_set, &set_count, &clip_buffer, pipeline->varying_count);
        }

        // if (primitive_id != 1)
        //     continue;

        // ----------------------------------------------------------------------------------
        // -- Transformations and setup.
        // ----------------------------------------------------------------------------------

        for (u32 j = 0; j < set_count; ++j)
        {
            current_set = &set_buffer[j];

            if (current_set->flags & GL_SET_DISCARDED)
                continue;

            gc_vertex_t *vertex_1 = &current_set->v[0];
            gc_vertex_t *vertex_2 = &current_set->v[1];
            gc_vertex_t *vertex_3 = &current_set->v[2];

            // printf("v1pos: {%f, %f, %f, %f}\n", vertex_1->pos[0], vertex_1->pos[1], vertex_1->pos[2], vertex_1->pos[3]);
            // printf("v2pos: {%f, %f, %f, %f}\n", vertex_2->pos[0], vertex_2->pos[1], vertex_2->pos[2], vertex_2->pos[3]);
            // printf("v3pos: {%f, %f, %f, %f}\n", vertex_3->pos[0], vertex_3->pos[1], vertex_3->pos[2], vertex_3->pos[3]);

            // ----------------------------------------------------------------------------------
            // -- Vertex transform.
            // ----------------------------------------------------------------------------------

            s64 fp_area = 0;
            s64 v1fpx, v1fpy, v2fpx, v2fpy, v3fpx, v3fpy;

            {
                OPTICK_EVENT("vertex transform");

                r32 one_over_w = 1.0f / vertex_1->pos[3];

                vertex_1->pos[0] *= one_over_w;
                vertex_1->pos[1] *= one_over_w;
                vertex_1->pos[2] *= one_over_w;
                vertex_1->pos[3] = one_over_w;

                one_over_w = 1.0f / vertex_2->pos[3];

                vertex_2->pos[0] *= one_over_w;
                vertex_2->pos[1] *= one_over_w;
                vertex_2->pos[2] *= one_over_w;
                vertex_2->pos[3] = one_over_w;

                one_over_w = 1.0f / vertex_3->pos[3];

                vertex_3->pos[0] *= one_over_w;
                vertex_3->pos[1] *= one_over_w;
                vertex_3->pos[2] *= one_over_w;
                vertex_3->pos[3] = one_over_w;

                gc_mat_t *m = GET_MATRIX(M_VIEWPORT);

#ifndef GC_PIPE_SSE
                vertex_1->pos[0] = m->data[0][0] * vertex_1->pos[0] + m->data[0][3];
                vertex_1->pos[1] = m->data[1][1] * vertex_1->pos[1] + m->data[1][3];
                vertex_1->pos[2] = m->data[2][2] * vertex_1->pos[2] + m->data[2][3];

                vertex_2->pos[0] = m->data[0][0] * vertex_2->pos[0] + m->data[0][3];
                vertex_2->pos[1] = m->data[1][1] * vertex_2->pos[1] + m->data[1][3];
                vertex_2->pos[2] = m->data[2][2] * vertex_2->pos[2] + m->data[2][3];

                vertex_3->pos[0] = m->data[0][0] * vertex_3->pos[0] + m->data[0][3];
                vertex_3->pos[1] = m->data[1][1] * vertex_3->pos[1] + m->data[1][3];
                vertex_3->pos[2] = m->data[2][2] * vertex_3->pos[2] + m->data[2][3];
#else
                __m128 t1_4x = _mm_setr_ps(m->data[0][0], m->data[1][1], m->data[2][2], 0);
                __m128 t2_4x = _mm_setr_ps(m->data[0][3], m->data[1][3], m->data[2][3], 0);

                r32 tmp_w = vertex_1->pos[3];
                __m128 vertex_4x = _mm_load_ps(vertex_1->pos);
                vertex_4x = _mm_add_ps(_mm_mul_ps(vertex_4x, t1_4x), t2_4x);
                _mm_store_ps(vertex_1->pos, vertex_4x);
                vertex_1->pos[3] = tmp_w;

                tmp_w = vertex_2->pos[3];
                vertex_4x = _mm_load_ps(vertex_2->pos);
                vertex_4x = _mm_add_ps(_mm_mul_ps(vertex_4x, t1_4x), t2_4x);
                _mm_store_ps(vertex_2->pos, vertex_4x);
                vertex_2->pos[3] = tmp_w;

                tmp_w = vertex_3->pos[3];
                vertex_4x = _mm_load_ps(vertex_3->pos);
                vertex_4x = _mm_add_ps(_mm_mul_ps(vertex_4x, t1_4x), t2_4x);
                _mm_store_ps(vertex_3->pos, vertex_4x);
                vertex_3->pos[3] = tmp_w;
#endif

                if (PIPE_FLAG(GC_FORCE_DEPTH_ONE))
                {
                    vertex_1->pos[2] = 1.0f;
                    vertex_2->pos[2] = 1.0f;
                    vertex_3->pos[2] = 1.0f;
                }

                v1fpx = FP_FROM_REAL(vertex_1->pos[0]);
                v1fpy = FP_FROM_REAL(vertex_1->pos[1]);

                v2fpx = FP_FROM_REAL(vertex_2->pos[0]);
                v2fpy = FP_FROM_REAL(vertex_2->pos[1]);

                v3fpx = FP_FROM_REAL(vertex_3->pos[0]);
                v3fpy = FP_FROM_REAL(vertex_3->pos[1]);

                fp_area = (v2fpx - v1fpx) * (v3fpy - v1fpy) - (v2fpy - v1fpy) * (v3fpx - v1fpx);

                if (!fp_area)
                    continue;
            }

            // ----------------------------------------------------------------------------------
            // -- Winding check / triangle culling.
            // ----------------------------------------------------------------------------------

            b8 is_backface = false;

            {
                OPTICK_EVENT("triangle culling");

                if (FLAG(GCSR.gl->pipeline.flags, GC_WINDING_CW) && fp_area < 0)
                {
                    if (do_backface_culling)
                        continue;
                    else
                        is_backface = true;
                }
                else if (FLAG(GCSR.gl->pipeline.flags, GC_WINDING_CCW) && fp_area > 0)
                {
                    if (do_backface_culling)
                        continue;
                    else
                        is_backface = true;
                }
            }

            // ----------------------------------------------------------------------------------
            // -- Triangle setup.
            // ----------------------------------------------------------------------------------

            DEBUG_SHOW_ONLY_TRIANGLE();

            {
                OPTICK_EVENT("triangle setup");

                gc_primitive_t *primitive = (gc_primitive_t *) primitive_list->data + (primitive_list->count++);
                thread_memory->cursor += sizeof(gc_primitive_t);
                s32 _s = 1;

                primitive->id = primitive_id;
                primitive->type = GL_MESH_TRIANGLE;
                primitive->is_backface = is_backface;
                primitive->triangle.flags = 0;

                gc_vertex_t *base_vertex = vertex_1;

                // Compute which is the closest vertex from the origin and choose that as base.
                r32 c1 = vertex_1->pos[0] * vertex_1->pos[0] + vertex_1->pos[1] * vertex_1->pos[1];
                r32 c2 = vertex_2->pos[0] * vertex_2->pos[0] + vertex_2->pos[1] * vertex_2->pos[1];
                r32 c3 = vertex_3->pos[0] * vertex_3->pos[0] + vertex_3->pos[1] * vertex_3->pos[1];

                if (c2 < c1)
                {
                    c1 = c2;
                    base_vertex = vertex_2;
                }

                if (c3 < c1)
                    base_vertex = vertex_3;

                // ----------------------------------------------------------------------------------
                // -- Copy the first vertex in the triangle for the varyings interpolation.
                // ----------------------------------------------------------------------------------

                primitive->base.pos[0] = base_vertex->pos[0];
                primitive->base.pos[1] = base_vertex->pos[1];
                primitive->base.pos[2] = base_vertex->pos[2];
                primitive->base.pos[3] = base_vertex->pos[3];

                for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii) {
                    primitive->base.data[ii] = base_vertex->pos[3] * base_vertex->data[ii];
                }

#ifdef DEBUG_PRIMITIVE
                primitive->v1.pos[0] = vertex_1->pos[0];
                primitive->v1.pos[1] = vertex_1->pos[1];
                primitive->v1.pos[2] = vertex_1->pos[2];
                primitive->v1.pos[3] = vertex_1->pos[3];

                for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii) {
                    primitive->v1.data[ii] = vertex_1->data[ii];
                }

                primitive->v2.pos[0] = vertex_2->pos[0];
                primitive->v2.pos[1] = vertex_2->pos[1];
                primitive->v2.pos[2] = vertex_2->pos[2];
                primitive->v2.pos[3] = vertex_2->pos[3];

                for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii) {
                    primitive->v2.data[ii] = vertex_2->data[ii];
                }

                primitive->v3.pos[0] = vertex_3->pos[0];
                primitive->v3.pos[1] = vertex_3->pos[1];
                primitive->v3.pos[2] = vertex_3->pos[2];
                primitive->v3.pos[3] = vertex_3->pos[3];

                for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii) {
                    primitive->v3.data[ii] = vertex_3->data[ii];
                }
#endif

                // ----------------------------------------------------------------------------------
                // -- Determine the triangle's bounding box.
                // ----------------------------------------------------------------------------------

                r32 min_x = vertex_1->pos[0];
                r32 min_y = vertex_1->pos[1];
                r32 max_x = vertex_1->pos[0];
                r32 max_y = vertex_1->pos[1];

                if (vertex_2->pos[0] < min_x)
                    min_x = vertex_2->pos[0];
                else if (vertex_2->pos[0] > max_x)
                    max_x = vertex_2->pos[0];

                if (vertex_2->pos[1] < min_y)
                    min_y = vertex_2->pos[1];
                else if (vertex_2->pos[1] > max_y)
                    max_y = vertex_2->pos[1];

                if (vertex_3->pos[0] < min_x)
                    min_x = vertex_3->pos[0];
                else if (vertex_3->pos[0] > max_x)
                    max_x = vertex_3->pos[0];

                if (vertex_3->pos[1] < min_y)
                    min_y = vertex_3->pos[1];
                else if (vertex_3->pos[1] > max_y)
                    max_y = vertex_3->pos[1];

                primitive->box.min.x = (s32) floorf(min_x);
                primitive->box.max.x = (s32) floorf(max_x);
                primitive->box.min.y = (s32) floorf(min_y);
                primitive->box.max.y = (s32) floorf(max_y);

                // ----------------------------------------------------------------------------------
                // -- Crop the triangle area to the screen.
                // ----------------------------------------------------------------------------------

                if (primitive->box.min.x < 0)
                    primitive->box.min.x = 0;

                if (primitive->box.min.y < 0)
                    primitive->box.min.y = 0;

                if (primitive->box.max.x >= (s32) GCSR.gl->current_framebuffer->tiled_width)
                    primitive->box.max.x = GCSR.gl->current_framebuffer->tiled_width - 1;

                if (primitive->box.max.y >= (s32) GCSR.gl->current_framebuffer->tiled_height)
                    primitive->box.max.y = GCSR.gl->current_framebuffer->tiled_height - 1;

                if (fp_area < 0)
                    _s = -1;

                // ----------------------------------------------------------------------------------
                // -- Rasterization equation setup.
                // ----------------------------------------------------------------------------------

                primitive->triangle.l1a = _s * ((s64) v2fpy - v3fpy);
                primitive->triangle.l1b = _s * ((s64) v3fpx - v2fpx);
                primitive->triangle.l1c = _s * ((s64) v2fpx * v3fpy - (s64) v2fpy * v3fpx);

                primitive->triangle.l2a = _s * ((s64) v3fpy - v1fpy);
                primitive->triangle.l2b = _s * ((s64) v1fpx - v3fpx);
                primitive->triangle.l2c = _s * ((s64) v3fpx * v1fpy - (s64) v3fpy * v1fpx);

                primitive->triangle.l3a = _s * ((s64) v1fpy - v2fpy);
                primitive->triangle.l3b = _s * ((s64) v2fpx - v1fpx);
                primitive->triangle.l3c = _s * ((s64) v1fpx * v2fpy - (s64) v1fpy * v2fpx);

                primitive->triangle.l1dx = primitive->triangle.l1a << FP_DEC_BIT;
                primitive->triangle.l2dx = primitive->triangle.l2a << FP_DEC_BIT;
                primitive->triangle.l3dx = primitive->triangle.l3a << FP_DEC_BIT;

                primitive->triangle.l1dy = primitive->triangle.l1b << FP_DEC_BIT;
                primitive->triangle.l2dy = primitive->triangle.l2b << FP_DEC_BIT;
                primitive->triangle.l3dy = primitive->triangle.l3b << FP_DEC_BIT;

                primitive->triangle.frag_l1dx = primitive->triangle.l1dx << 1;
                primitive->triangle.frag_l1dy = primitive->triangle.l1dy << 1;
                primitive->triangle.frag_l2dx = primitive->triangle.l2dx << 1;
                primitive->triangle.frag_l2dy = primitive->triangle.l2dy << 1;
                primitive->triangle.frag_l3dx = primitive->triangle.l3dx << 1;
                primitive->triangle.frag_l3dy = primitive->triangle.l3dy << 1;

                // ----------------------------------------------------------------------------------
                // -- Top-left convention check.
                // ----------------------------------------------------------------------------------

                if (primitive->triangle.l1a > 0 || (primitive->triangle.l1a == 0 && primitive->triangle.l1b > 0))
                    primitive->triangle.l1c++;

                if (primitive->triangle.l2a > 0 || (primitive->triangle.l2a == 0 && primitive->triangle.l2b > 0))
                    primitive->triangle.l2c++;

                if (primitive->triangle.l3a > 0 || (primitive->triangle.l3a == 0 && primitive->triangle.l3b > 0))
                    primitive->triangle.l3c++;

                primitive->triangle.fp_area = _s * fp_area;
                r32 area = primitive->triangle.fp_area * FP_ONE_OVER_2DEC_VAL;
                r32 one_over_area = 1.0f / area;
                primitive->triangle.one_over_area = _s * one_over_area;

                // ----------------------------------------------------------------------------------
                // -- Barycentric setup.
                // ----------------------------------------------------------------------------------

                // primitive->triangle.interp_1.v3.x = (vertex_2->pos[1] - vertex_3->pos[1]) * primitive->triangle.one_over_area;
                // primitive->triangle.interp_1.v3.y = (vertex_3->pos[0] - vertex_2->pos[0]) * primitive->triangle.one_over_area;
                // primitive->triangle.interp_1.v3.z = (vertex_2->pos[0] * vertex_3->pos[1] - vertex_2->pos[1] * vertex_3->pos[0]) * primitive->triangle.one_over_area;

                // primitive->triangle.interp_2.v3.x = (vertex_3->pos[1] - vertex_1->pos[1]) * primitive->triangle.one_over_area;
                // primitive->triangle.interp_2.v3.y = (vertex_1->pos[0] - vertex_3->pos[0]) * primitive->triangle.one_over_area;
                // primitive->triangle.interp_2.v3.z = (vertex_3->pos[0] * vertex_1->pos[1] - vertex_3->pos[1] * vertex_1->pos[0]) * primitive->triangle.one_over_area;

                // primitive->triangle.interp_3.v3.x = (vertex_1->pos[1] - vertex_2->pos[1]) * primitive->triangle.one_over_area;
                // primitive->triangle.interp_3.v3.y = (vertex_2->pos[0] - vertex_1->pos[0]) * primitive->triangle.one_over_area;
                // primitive->triangle.interp_3.v3.z = (vertex_1->pos[0] * vertex_2->pos[1] - vertex_1->pos[1] * vertex_2->pos[0]) * primitive->triangle.one_over_area;

                // ----------------------------------------------------------------------------------
                // -- Partial derivatives setup.
                // ----------------------------------------------------------------------------------

                r32 dy12 = vertex_1->pos[1] - vertex_2->pos[1];
                r32 dy31 = vertex_3->pos[1] - vertex_1->pos[1];
                r32 dx21 = vertex_2->pos[0] - vertex_1->pos[0];
                r32 dx13 = vertex_1->pos[0] - vertex_3->pos[0];

                r32 tmp_d1 = vertex_3->pos[2] - vertex_1->pos[2];
                r32 tmp_d2 = vertex_2->pos[2] - vertex_1->pos[2];

                primitive->triangle.interp_z.x = (dy12 * tmp_d1 + dy31 * tmp_d2) * primitive->triangle.one_over_area;
                primitive->triangle.interp_z.y = (dx21 * tmp_d1 + dx13 * tmp_d2) * primitive->triangle.one_over_area;

                tmp_d1 = vertex_3->pos[3] - vertex_1->pos[3];
                tmp_d2 = vertex_2->pos[3] - vertex_1->pos[3];

                primitive->triangle.interp_w.x = (dy12 * tmp_d1 + dy31 * tmp_d2) * primitive->triangle.one_over_area;
                primitive->triangle.interp_w.y = (dx21 * tmp_d1 + dx13 * tmp_d2) * primitive->triangle.one_over_area;

                for (u32 ii = 0; ii < GCSR.gl->pipeline.varying_count; ++ii)
                {
                    tmp_d1 = vertex_3->data[ii] * vertex_3->pos[3] - vertex_1->data[ii] * vertex_1->pos[3];
                    tmp_d2 = vertex_2->data[ii] * vertex_2->pos[3] - vertex_1->data[ii] * vertex_1->pos[3];

                    primitive->triangle.interp_varying[ii].x = (dy12 * tmp_d1 + dy31 * tmp_d2) * primitive->triangle.one_over_area;
                    primitive->triangle.interp_varying[ii].y = (dx21 * tmp_d1 + dx13 * tmp_d2) * primitive->triangle.one_over_area;
                }
            }
        }
    }

    gc_pipe_binning(thread_id, primitive_list, GL_MESH_TRIANGLE);

    // ----------------------------------------------------------------------------------
    // -- Primitive binning.
    // ----------------------------------------------------------------------------------

    // gc_bin_t *bins = GCSR.gl->current_framebuffer->bins;
    // gc_framebuffer_t *framebuffer = GCSR.gl->current_framebuffer;

    // for (u32 k = 0; k < primitive_list->count; ++k)
    // {
    //     gc_primitive_t *primitive = (gc_primitive_t *) primitive_list->data + k;

    //     u32 bin_row_min = primitive->box.min.y / GL_BIN_HEIGHT;
    //     u32 bin_col_min = primitive->box.min.x / GL_BIN_WIDTH;
    //     u32 bin_row_max = primitive->box.max.y / GL_BIN_HEIGHT;
    //     u32 bin_col_max = primitive->box.max.x / GL_BIN_WIDTH;

    //     for (u32 row = bin_row_min; row <= bin_row_max; ++row)
    //     {
    //         for (u32 col = bin_col_min; col <= bin_col_max; ++col)
    //         {
    //             DEBUG_SHOW_ONLY_BIN(row, col);

    //             u16 bin_index = (u16) (row * framebuffer->bin_cols + col);
    //             gc_bin_t *current_bin = bins + bin_index;

    //             // u16 bsx = col * GL_BIN_WIDTH;
    //             // u16 bsy = row * GL_BIN_HEIGHT;

    //             SCREEN_RECT_SET(bin_box,
    //                 current_bin->x, current_bin->y,
    //                 current_bin->x + GL_BIN_WIDTH - 1,
    //                 current_bin->y + GL_BIN_HEIGHT - 1);

    //             // b8 is_over = false;
    //             gc_constant_t coverage = gl_check_triangle_rect_coverage(primitive, &bin_box);

    //             // if (primitive->type == GL_MESH_TRIANGLE)
    //             // else if (primitive->type == GL_MESH_LINE)
    //             //     coverage = gl_check_line_block_coverage(primitive, &bin_box);

    //             if (coverage == GC_COVERAGE_NONE)
    //                 continue;

    //             u32 dirty_count = SDL_AtomicAdd(&current_bin->dirty, 1);

    //             // The bin was not added to the queue.
    //             if (!dirty_count)
    //             {
    //                 u32 idx = SDL_AtomicAdd(&GCSR.gl->bin_queue.count, 1);
    //                 SDL_assert(idx <= GCSR.gl->current_framebuffer->total_bins);
    //                 GCSR.gl->bin_queue.bins[idx] = bin_index;
    //             }

    //             // Primitive linked list for the current thread.
    //             gc_bin_prim_list_t *list = &current_bin->list[thread_id];
    //             gc_bin_prim_link_t *link = (gc_bin_prim_link_t *) ((u8 *) thread_memory->data + thread_memory->cursor);
    //             thread_memory->cursor += sizeof(gc_bin_prim_link_t);
    //             SDL_assert(thread_memory->cursor <= thread_memory->bytes);

    //             link->next = 0;
    //             link->prim_idx = k;
    //             link->total_coverage = (coverage == GC_COVERAGE_TOTAL);

    //             if (!list->start)
    //             {
    //                 list->start = link;
    //                 list->last = link;
    //             }
    //             else
    //                 list->last->next = link;

    //             list->last = link;
    //             list->count++;
    //         }
    //     }
    // }
}

__INLINE__ void gl_pipe_transparency_push(gc_transparency_frag_t *transparency_frag,
                                          gc_processed_fragment_t *fragment,
                                          u32 transparency_mask)
{
    u8 masks[4] = GC_FRAGMENT_MASKS;

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        if ((transparency_mask & masks[i]) && fragment->a[i] > 0)
        {
            // gc_vec_t color = {
            //     fragment->r[i],
            //     fragment->g[i],
            //     fragment->b[i],
            //     fragment->a[i]
            // };

            // color.r = 1.0f;
            // color.g = 1.0f;
            // color.b = 1.0f;
            // color.a = 1.0f;

            // The color to be pushed onto the transparency stack.
            // u32 packed = gl_pack(color);
            // gc_vec_t test_color = gl_unpack(packed);

            // First color in the stack.
            if (transparency_frag->count[i] == 0)
            {
                u8 count = transparency_frag->count[i]++;

                transparency_frag->stack_r[count][i] = fragment->r[i];
                transparency_frag->stack_g[count][i] = fragment->g[i];
                transparency_frag->stack_b[count][i] = fragment->b[i];
                transparency_frag->stack_a[count][i] = fragment->a[i];

                transparency_frag->z[count][i] = fragment->z[i];
            }

            // Insert the new pixel color based on it's depth value.
            else
            {
                s8 insert_index = 0;

                for (u8 k = 0; k < transparency_frag->count[i]; ++k)
                {
                    if (fragment->z[i] <= transparency_frag->z[k][i])
                    {
                        insert_index = k;
                        break;
                    }

                    insert_index++;
                }

                if (insert_index < MAX_TRANSPARENCY_STACK)
                {
                    s8 push_index = insert_index;
                    s8 end_index = MAX_TRANSPARENCY_STACK - 1;
                    s8 end_cursor = end_index;

                    while (true)
                    {
                        if (end_cursor < end_index)
                        {
                            transparency_frag->stack_r[end_cursor + 1][i] = transparency_frag->stack_r[end_cursor][i];
                            transparency_frag->stack_g[end_cursor + 1][i] = transparency_frag->stack_g[end_cursor][i];
                            transparency_frag->stack_b[end_cursor + 1][i] = transparency_frag->stack_b[end_cursor][i];
                            transparency_frag->stack_a[end_cursor + 1][i] = transparency_frag->stack_a[end_cursor][i];

                            transparency_frag->z[end_cursor + 1][i] = transparency_frag->z[end_cursor][i];
                        }

                        transparency_frag->stack_r[end_cursor][i] = 0;
                        transparency_frag->stack_g[end_cursor][i] = 0;
                        transparency_frag->stack_b[end_cursor][i] = 0;
                        transparency_frag->stack_a[end_cursor][i] = 0;

                        transparency_frag->z[end_cursor][i] = 0;

                        end_cursor--;

                        if (end_cursor < push_index)
                            break;
                    }

                    // transparency_frag->stack[push_index][i] = packed;
                    transparency_frag->stack_r[push_index][i] = fragment->r[i];
                    transparency_frag->stack_g[push_index][i] = fragment->g[i];
                    transparency_frag->stack_b[push_index][i] = fragment->b[i];
                    transparency_frag->stack_a[push_index][i] = fragment->a[i];

                    transparency_frag->z[push_index][i] = fragment->z[i];
                    transparency_frag->count[i]++;
                }
            }
        }
    }
}

__INLINE__ void gl_push_fragments_full(thread_batch_memory_t *batch_memory,
                                       gc_screen_rect_t *box,
                                       u16 fragment_index_start,
                                       gc_primitive_t *primitive)
{
    OPTICK_EVENT("gl_push_fragments_full");

    gc_processed_fragment_t *current_fragment = 0;

    for (s32 y = box->min.y; y < box->max.y; y += GL_FRAG_HEIGHT)
    {
        u16 fragment_index = fragment_index_start;

        for (s32 x = box->min.x; x < box->max.x; x += GL_FRAG_WIDTH)
        {
            PUSH_FRAGMENT(current_fragment);

            current_fragment->mask = GL_FRAGMENT_FULL_MASK;
            current_fragment->index = fragment_index;
            current_fragment->x = x;
            current_fragment->y = y;
            current_fragment->discarded = false;
            current_fragment->primitive = primitive;

            fragment_index++;
        }

        fragment_index_start += GL_BIN_FRAG_COLS;
    }
}

void gl_fragment_varyings_setup_point(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer)
{
    OPTICK_EVENT("gl_fragment_varyings_setup_point");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;
    r32 *screen_depth = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = &tile_buffer->fragments[fragment->index];
        screen_depth = screen_fragment->z;

#if 1
        // Points are always in front of everything (for debugging).
        fragment->z[0] = 0;
        fragment->z[1] = 0;
        fragment->z[2] = 0;
        fragment->z[3] = 0;
#else
        fragment->z[0] = fragment->primitive->base.pos[2];
        fragment->z[1] = fragment->primitive->base.pos[2];
        fragment->z[2] = fragment->primitive->base.pos[2];
        fragment->z[3] = fragment->primitive->base.pos[2];

        r32 z_check = fragment->z[0] <= screen_depth[0] ||
                      fragment->z[1] <= screen_depth[1] ||
                      fragment->z[2] <= screen_depth[2] ||
                      fragment->z[3] <= screen_depth[3];

        if (!z_check)
        {
            fragment->discarded = true;
            continue;
        }
#endif

        fragment->shadow[0] = 1;
        fragment->shadow[1] = 1;
        fragment->shadow[2] = 1;
        fragment->shadow[3] = 1;
    }
}

void gl_fragment_varyings_setup_line(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer)
{
    OPTICK_EVENT("gl_fragment_varyings_setup_line");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;
    r32 *screen_depth = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = &tile_buffer->fragments[fragment->index];
        screen_depth = screen_fragment->z;

        if (fragment->primitive->line.is_dx)
        {
            r32 dx = fragment->x - fragment->primitive->base.pos[0];

            fragment->z[0] = fragment->primitive->base.pos[2] + dx * fragment->primitive->line.interp_z;
            fragment->z[1] = fragment->z[0] + fragment->primitive->line.interp_z;
            fragment->z[2] = fragment->z[0];
            fragment->z[3] = fragment->z[1];
        }
        else
        {
            r32 dy = fragment->y - fragment->primitive->base.pos[1];

            fragment->z[0] = fragment->primitive->base.pos[2] + dy * fragment->primitive->line.interp_z;
            fragment->z[1] = fragment->z[0];
            fragment->z[2] = fragment->z[0] + fragment->primitive->line.interp_z;
            fragment->z[3] = fragment->z[2];
        }

        r32 z_check = fragment->z[0] <= screen_depth[0] ||
                      fragment->z[1] <= screen_depth[1] ||
                      fragment->z[2] <= screen_depth[2] ||
                      fragment->z[3] <= screen_depth[3];

        if (!z_check)
        {
            fragment->discarded = true;
            continue;
        }

        fragment->shadow[0] = 1;
        fragment->shadow[1] = 1;
        fragment->shadow[2] = 1;
        fragment->shadow[3] = 1;
    }
}

void gl_fragment_varyings_setup_triangle(gc_fragments_array_t *fragments_array, gc_tile_buffer_t *tile_buffer)
{
    OPTICK_EVENT("gl_fragment_varyings_setup_triangle");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;
    r32 *screen_depth = 0;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = tile_buffer->fragments + fragment->index;
        screen_depth = screen_fragment->z;

        r32 dx = fragment->x - fragment->primitive->base.pos[0];
        r32 dy = fragment->y - fragment->primitive->base.pos[1];

        fragment->z[0] = fragment->primitive->base.pos[2] + dx * fragment->primitive->triangle.interp_z.x + dy * fragment->primitive->triangle.interp_z.y;
        fragment->z[1] = fragment->z[0] + fragment->primitive->triangle.interp_z.x;
        fragment->z[2] = fragment->z[0] + fragment->primitive->triangle.interp_z.y;
        fragment->z[3] = fragment->z[0] + fragment->primitive->triangle.interp_z.x + fragment->primitive->triangle.interp_z.y;

        r32 z_check = fragment->z[0] <= screen_depth[0] ||
                      fragment->z[1] <= screen_depth[1] ||
                      fragment->z[2] <= screen_depth[2] ||
                      fragment->z[3] <= screen_depth[3];

        if (!z_check)
        {
            fragment->discarded = true;
            continue;
        }

        fragment->shadow[0] = 1;
        fragment->shadow[1] = 1;
        fragment->shadow[2] = 1;
        fragment->shadow[3] = 1;

        if (GCSR.gl->pipeline.varying_count)
        {
            r32 base_w00 = fragment->primitive->base.pos[3] + dx * fragment->primitive->triangle.interp_w.x + dy * fragment->primitive->triangle.interp_w.y;

            r32 w00 = 1.0f / base_w00;
            r32 w01 = 1.0f / (base_w00 + fragment->primitive->triangle.interp_w.x);
            r32 w10 = 1.0f / (base_w00 + fragment->primitive->triangle.interp_w.y);
            r32 w11 = 1.0f / (base_w00 + fragment->primitive->triangle.interp_w.x + fragment->primitive->triangle.interp_w.y);

            // Varyings setup.
            for (u16 k = 0; k < GCSR.gl->pipeline.varying_count; ++k)
            {
                r32 base = fragment->primitive->base.data[k] +
                               dx * fragment->primitive->triangle.interp_varying[k].x +
                               dy * fragment->primitive->triangle.interp_varying[k].y;

                r32 varying_linear_00 = base;
                r32 varying_linear_01 = base + fragment->primitive->triangle.interp_varying[k].x;
                r32 varying_linear_10 = base + fragment->primitive->triangle.interp_varying[k].y;
                r32 varying_linear_11 = base + fragment->primitive->triangle.interp_varying[k].x + fragment->primitive->triangle.interp_varying[k].y;

                fragment->varyings[k][0] = varying_linear_00 * w00;
                fragment->varyings[k][1] = varying_linear_01 * w01;
                fragment->varyings[k][2] = varying_linear_10 * w10;
                fragment->varyings[k][3] = varying_linear_11 * w11;

                // Assume {u} k = 0 and {v} k = 1.
                if (k == 0)
                {
                    // udx
                    r32 varying_linear_A = varying_linear_01 + fragment->primitive->triangle.interp_varying[k].x;
                    r32 varying_linear_B = varying_linear_11 + fragment->primitive->triangle.interp_varying[k].x;
                    // udy
                    r32 varying_linear_C = varying_linear_10 + fragment->primitive->triangle.interp_varying[k].y;
                    r32 varying_linear_D = varying_linear_11 + fragment->primitive->triangle.interp_varying[k].y;

                    r32 wA = 1.0f / (base_w00 + 2 * fragment->primitive->triangle.interp_w.x);
                    r32 wB = 1.0f / (base_w00 + 2 * fragment->primitive->triangle.interp_w.x + fragment->primitive->triangle.interp_w.y);
                    r32 wC = 1.0f / (base_w00 + 2 * fragment->primitive->triangle.interp_w.y);
                    r32 wD = 1.0f / (base_w00 + fragment->primitive->triangle.interp_w.x + 2 * fragment->primitive->triangle.interp_w.y);

                    fragment->dudx[0] = fragment->varyings[k][1] - fragment->varyings[k][0];
                    fragment->dudx[1] = varying_linear_A * wA - fragment->varyings[k][1];
                    fragment->dudx[2] = fragment->varyings[k][3] - fragment->varyings[k][2];
                    fragment->dudx[3] = varying_linear_B * wB - fragment->varyings[k][3];

                    fragment->dudy[0] = fragment->varyings[k][2] - fragment->varyings[k][0];
                    fragment->dudy[1] = fragment->varyings[k][3] - fragment->varyings[k][1];
                    fragment->dudy[2] = varying_linear_C * wC - fragment->varyings[k][2];
                    fragment->dudy[3] = varying_linear_D * wD - fragment->varyings[k][3];
                }
                else if (k == 1)
                {
                    // vdx
                    r32 varying_linear_A = varying_linear_01 + fragment->primitive->triangle.interp_varying[k].x;
                    r32 varying_linear_B = varying_linear_11 + fragment->primitive->triangle.interp_varying[k].x;
                    // vdy
                    r32 varying_linear_C = varying_linear_10 + fragment->primitive->triangle.interp_varying[k].y;
                    r32 varying_linear_D = varying_linear_11 + fragment->primitive->triangle.interp_varying[k].y;

                    r32 wA = 1.0f / (base_w00 + 2 * fragment->primitive->triangle.interp_w.x);
                    r32 wB = 1.0f / (base_w00 + 2 * fragment->primitive->triangle.interp_w.x + fragment->primitive->triangle.interp_w.y);
                    r32 wC = 1.0f / (base_w00 + 2 * fragment->primitive->triangle.interp_w.y);
                    r32 wD = 1.0f / (base_w00 + fragment->primitive->triangle.interp_w.x + 2 * fragment->primitive->triangle.interp_w.y);

                    fragment->dvdx[0] = fragment->varyings[k][1] - fragment->varyings[k][0];
                    fragment->dvdx[1] = varying_linear_A * wA - fragment->varyings[k][1];
                    fragment->dvdx[2] = fragment->varyings[k][3] - fragment->varyings[k][2];
                    fragment->dvdx[3] = varying_linear_B * wB - fragment->varyings[k][3];

                    fragment->dvdy[0] = fragment->varyings[k][2] - fragment->varyings[k][0];
                    fragment->dvdy[1] = fragment->varyings[k][3] - fragment->varyings[k][1];
                    fragment->dvdy[2] = varying_linear_C * wC - fragment->varyings[k][2];
                    fragment->dvdy[3] = varying_linear_D * wD - fragment->varyings[k][3];
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Merges the shader results with the tilebuffer.
// ----------------------------------------------------------------------------------

__INLINE__ void _fragment_merge(gc_fragments_array_t *fragments_array,
                                  gc_tile_buffer_t *tile_buffer,
                                  gc_transparency_bin_t *transparency_buffer)
{
    OPTICK_EVENT("_fragment_merge");

    gc_fragment_t *screen_fragment = 0;
    gc_processed_fragment_t *fragment = 0;
    u8 masks[GC_FRAG_SIZE] = GC_FRAGMENT_MASKS;

    b8 is_shadow = PIPE_FLAG(GC_SHADOW_PASS);
    b8 is_transparency = PIPE_FLAG(GC_TRANSPARENCY);

    pipe_param_merged_table_t *overwrites = &GCSR.gl->pipeline.params.overwrites;
    r32 forced_opacity = overwrites->forced_opacity[2]->u_float;

    FRAGMENT_BATCH_LOOP(fragments_array, i)
    {
        fragment = (gc_processed_fragment_t *) fragments_array->data + i;
        screen_fragment = tile_buffer->fragments + fragment->index;

        if (fragment->discarded)
            continue;

        u32 final_mask = (fragment->z[0] <= screen_fragment->z[0] ? masks[0] : 0) |
                         (fragment->z[1] <= screen_fragment->z[1] ? masks[1] : 0) |
                         (fragment->z[2] <= screen_fragment->z[2] ? masks[2] : 0) |
                         (fragment->z[3] <= screen_fragment->z[3] ? masks[3] : 0);

        final_mask = fragment->mask & final_mask;

        if (is_shadow)
        {
            for (u8 j = 0; j < GC_FRAG_SIZE; ++j)
            {
                if (final_mask & masks[j])
                {
                    screen_fragment->r[j] = fragment->r[j];
                    screen_fragment->g[j] = fragment->g[j];
                    screen_fragment->z[j] = fragment->z[j];
                }
            }
        }
        else
        {
            if (!is_shadow && is_transparency)
            {
                if (forced_opacity < 1.0f)
                {
                    fragment->r[0] *= forced_opacity;
                    fragment->r[1] *= forced_opacity;
                    fragment->r[2] *= forced_opacity;
                    fragment->r[3] *= forced_opacity;

                    fragment->g[0] *= forced_opacity;
                    fragment->g[1] *= forced_opacity;
                    fragment->g[2] *= forced_opacity;
                    fragment->g[3] *= forced_opacity;

                    fragment->b[0] *= forced_opacity;
                    fragment->b[1] *= forced_opacity;
                    fragment->b[2] *= forced_opacity;
                    fragment->b[3] *= forced_opacity;

                    fragment->a[0] *= forced_opacity;
                    fragment->a[1] *= forced_opacity;
                    fragment->a[2] *= forced_opacity;
                    fragment->a[3] *= forced_opacity;
                }

                gc_transparency_frag_t *transparency_frag = transparency_buffer->frags + fragment->index;

                u32 transparent_mask = (fragment->a[0] < 1 ? masks[0] : 0) |
                                       (fragment->a[1] < 1 ? masks[1] : 0) |
                                       (fragment->a[2] < 1 ? masks[2] : 0) |
                                       (fragment->a[3] < 1 ? masks[3] : 0);

                transparent_mask &= final_mask;
                final_mask &= ~transparent_mask;
                transparency_buffer->dirty = true;

                gl_pipe_transparency_push(transparency_frag, fragment, transparent_mask);
            }

            for (u8 j = 0; j < GC_FRAG_SIZE; ++j)
            {
                if (final_mask & masks[j])
                {
                    screen_fragment->r[j] = fragment->r[j];
                    screen_fragment->g[j] = fragment->g[j];
                    screen_fragment->b[j] = fragment->b[j];
                    screen_fragment->z[j] = fragment->z[j];
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Transparency processing.
// ----------------------------------------------------------------------------------

__INLINE__ void _transparency_process(u32 thread_id)
{
    OPTICK_EVENT("_transparency_process");

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

        // Current frag color.
        r32 r_4x[4];
        r32 g_4x[4];
        r32 b_4x[4];
        r32 a_4x[4];

        // Compounded color.
        gc_processed_fragment_t _fragment;
        gc_processed_fragment_t *fragment = &_fragment;

        r32 blendeda_4x[4];
        r32 blendedz_4x[4];

        r32 tmp_a[4];
        r32 mask[4];
        r32 final_mask[4];

        A4SET(tmp_a, 0, 0, 0, 0);
        A4SET(mask, 0, 0, 0, 0);
        A4SET(final_mask, 0, 0, 0, 0);

        for (u16 j = 0; j < GL_BIN_FRAGS; ++j)
        {
            gc_transparency_frag_t *frag = tbin->frags + j;
            gc_fragment_t *screen_fragment = tile_buffer->fragments + j;

            if (frag->count[0] || frag->count[1] || frag->count[2] || frag->count[3])
            {
                fragment->r[0] = 0;
                fragment->r[1] = 0;
                fragment->r[2] = 0;
                fragment->r[3] = 0;

                fragment->g[0] = 0;
                fragment->g[1] = 0;
                fragment->g[2] = 0;
                fragment->g[3] = 0;

                fragment->b[0] = 0;
                fragment->b[1] = 0;
                fragment->b[2] = 0;
                fragment->b[3] = 0;

                blendeda_4x[0] = 0;
                blendeda_4x[1] = 0;
                blendeda_4x[2] = 0;
                blendeda_4x[3] = 0;

                final_mask[0] = frag->count[0] && (frag->z[0][0] <= screen_fragment->z[0]) ? 1 : 0;
                final_mask[1] = frag->count[1] && (frag->z[0][1] <= screen_fragment->z[1]) ? 1 : 0;
                final_mask[2] = frag->count[2] && (frag->z[0][2] <= screen_fragment->z[2]) ? 1 : 0;
                final_mask[3] = frag->count[3] && (frag->z[0][3] <= screen_fragment->z[3]) ? 1 : 0;

                mask[0] = final_mask[0];
                mask[1] = final_mask[1];
                mask[2] = final_mask[2];
                mask[3] = final_mask[3];

                blendedz_4x[0] = mask[0] ? frag->z[0][0] : screen_fragment->z[0];
                blendedz_4x[1] = mask[1] ? frag->z[0][1] : screen_fragment->z[1];
                blendedz_4x[2] = mask[2] ? frag->z[0][2] : screen_fragment->z[2];
                blendedz_4x[3] = mask[3] ? frag->z[0][3] : screen_fragment->z[3];

                // ----------------------------------------------------------------------------------
                // -- Process the frag transparency stack.
                // ----------------------------------------------------------------------------------

                for (u16 k = 0; k < MAX_TRANSPARENCY_STACK; ++k)
                {
                    mask[0] *= (frag->z[k][0] <= screen_fragment->z[0]) ? 1 : 0;
                    mask[1] *= (frag->z[k][1] <= screen_fragment->z[1]) ? 1 : 0;
                    mask[2] *= (frag->z[k][2] <= screen_fragment->z[2]) ? 1 : 0;
                    mask[3] *= (frag->z[k][3] <= screen_fragment->z[3]) ? 1 : 0;

                    r_4x[0] = frag->stack_r[k][0] * mask[0];
                    r_4x[1] = frag->stack_r[k][1] * mask[1];
                    r_4x[2] = frag->stack_r[k][2] * mask[2];
                    r_4x[3] = frag->stack_r[k][3] * mask[3];

                    g_4x[0] = frag->stack_g[k][0] * mask[0];
                    g_4x[1] = frag->stack_g[k][1] * mask[1];
                    g_4x[2] = frag->stack_g[k][2] * mask[2];
                    g_4x[3] = frag->stack_g[k][3] * mask[3];

                    b_4x[0] = frag->stack_b[k][0] * mask[0];
                    b_4x[1] = frag->stack_b[k][1] * mask[1];
                    b_4x[2] = frag->stack_b[k][2] * mask[2];
                    b_4x[3] = frag->stack_b[k][3] * mask[3];

                    a_4x[0] = frag->stack_a[k][0] * mask[0];
                    a_4x[1] = frag->stack_a[k][1] * mask[1];
                    a_4x[2] = frag->stack_a[k][2] * mask[2];
                    a_4x[3] = frag->stack_a[k][3] * mask[3];

                    tmp_a[0] = 1.0f - blendeda_4x[0];
                    tmp_a[1] = 1.0f - blendeda_4x[1];
                    tmp_a[2] = 1.0f - blendeda_4x[2];
                    tmp_a[3] = 1.0f - blendeda_4x[3];

                    fragment->r[0] += r_4x[0] * tmp_a[0];
                    fragment->r[1] += r_4x[1] * tmp_a[1];
                    fragment->r[2] += r_4x[2] * tmp_a[2];
                    fragment->r[3] += r_4x[3] * tmp_a[3];

                    fragment->g[0] += g_4x[0] * tmp_a[0];
                    fragment->g[1] += g_4x[1] * tmp_a[1];
                    fragment->g[2] += g_4x[2] * tmp_a[2];
                    fragment->g[3] += g_4x[3] * tmp_a[3];

                    fragment->b[0] += b_4x[0] * tmp_a[0];
                    fragment->b[1] += b_4x[1] * tmp_a[1];
                    fragment->b[2] += b_4x[2] * tmp_a[2];
                    fragment->b[3] += b_4x[3] * tmp_a[3];

                    blendeda_4x[0] += a_4x[0] * tmp_a[0];
                    blendeda_4x[1] += a_4x[1] * tmp_a[1];
                    blendeda_4x[2] += a_4x[2] * tmp_a[2];
                    blendeda_4x[3] += a_4x[3] * tmp_a[3];
                }

                // ----------------------------------------------------------------------------------
                // -- Final blend with the bin_frag color.
                // ----------------------------------------------------------------------------------

                tmp_a[0] = 1.0f - blendeda_4x[0];
                tmp_a[1] = 1.0f - blendeda_4x[1];
                tmp_a[2] = 1.0f - blendeda_4x[2];
                tmp_a[3] = 1.0f - blendeda_4x[3];

                fragment->r[0] += (screen_fragment->r[0] * tmp_a[0]) * final_mask[0];
                fragment->r[1] += (screen_fragment->r[1] * tmp_a[1]) * final_mask[1];
                fragment->r[2] += (screen_fragment->r[2] * tmp_a[2]) * final_mask[2];
                fragment->r[3] += (screen_fragment->r[3] * tmp_a[3]) * final_mask[3];

                fragment->g[0] += (screen_fragment->g[0] * tmp_a[0]) * final_mask[0];
                fragment->g[1] += (screen_fragment->g[1] * tmp_a[1]) * final_mask[1];
                fragment->g[2] += (screen_fragment->g[2] * tmp_a[2]) * final_mask[2];
                fragment->g[3] += (screen_fragment->g[3] * tmp_a[3]) * final_mask[3];

                fragment->b[0] += (screen_fragment->b[0] * tmp_a[0]) * final_mask[0];
                fragment->b[1] += (screen_fragment->b[1] * tmp_a[1]) * final_mask[1];
                fragment->b[2] += (screen_fragment->b[2] * tmp_a[2]) * final_mask[2];
                fragment->b[3] += (screen_fragment->b[3] * tmp_a[3]) * final_mask[3];

                // ----------------------------------------------------------------------------------
                // -- Write to the bin.
                // ----------------------------------------------------------------------------------

                if (final_mask[0])
                {
                    screen_fragment->r[0] = fragment->r[0];
                    screen_fragment->g[0] = fragment->g[0];
                    screen_fragment->b[0] = fragment->b[0];
                    screen_fragment->z[0] = blendedz_4x[0];
                }

                if (final_mask[1])
                {
                    screen_fragment->r[1] = fragment->r[1];
                    screen_fragment->g[1] = fragment->g[1];
                    screen_fragment->b[1] = fragment->b[1];
                    screen_fragment->z[1] = blendedz_4x[1];
                }

                if (final_mask[2])
                {
                    screen_fragment->r[2] = fragment->r[2];
                    screen_fragment->g[2] = fragment->g[2];
                    screen_fragment->b[2] = fragment->b[2];
                    screen_fragment->z[2] = blendedz_4x[2];
                }

                if (final_mask[3])
                {
                    screen_fragment->r[3] = fragment->r[3];
                    screen_fragment->g[3] = fragment->g[3];
                    screen_fragment->b[3] = fragment->b[3];
                    screen_fragment->z[3] = blendedz_4x[3];
                }
            }
        }

        memset(tbin, 0, sizeof(gc_transparency_bin_t));
        tbin->dirty = false;
    }
}

// ----------------------------------------------------------------------------------
// -- Backend processing.
// ----------------------------------------------------------------------------------

__INLINE__ void gl_pipe_backend(u32 thread_id)
{
    OPTICK_EVENT("gl_pipe_backend");

    thread_batch_memory_t batch_memory;

    u32 queue_count = SDL_AtomicGet(&GCSR.gl->bin_queue.count);
    pipe_memory_t *thread_memory = &GCSR.memory_manager->pipeline[thread_id];
    gc_framebuffer_t *current_framebuffer = GET_FRAMEBUFFER();
    b8 has_transparency = BUFFER_FLAG(FB_FLAG_TRANSPARENCY);

    gc_transparency_bin_t *transparency_bins = (gc_transparency_bin_t *) current_framebuffer->transparency;
    gc_transparency_bin_t *transparency_buffer = 0;

    // Fragment list allocation.
    gc_fragments_array_t *fragments_array = (gc_fragments_array_t *) ADDR_OFFSET(thread_memory->data, thread_memory->cursor);
    fragments_array->data = fragments_array + 1;
    fragments_array->count = 0;
    thread_memory->cursor += sizeof(gc_fragments_array_t);
    size_t return_cursor = thread_memory->cursor;

    batch_memory.memory = thread_memory;
    batch_memory.fragments = fragments_array;

    while (true)
    {
        u32 queue_item_index = SDL_AtomicAdd(&GCSR.gl->bin_queue.r_cursor, 1);

        if (queue_item_index >= queue_count)
            break;

        u32 bin_index = GCSR.gl->bin_queue.bins[queue_item_index];
        gc_bin_t *current_bin = GCSR.gl->current_framebuffer->bins + bin_index;

        if (has_transparency)
            transparency_buffer = transparency_bins + bin_index;

        gc_tile_buffer_t *tile_buffer = current_framebuffer->lsb.tiles + bin_index;

        // Start the primitive processing.
        for (u8 i = 0; i < GC_PIPE_NUM_THREADS; ++i)
        {
            gc_pipe_array_t *thread_primitives = GCSR.gl->primitives[i];
            gc_bin_prim_list_t *primitive_list = &current_bin->list[i];
            struct gc_bin_prim_link_s *list_item = primitive_list->start;
            b8 last_buffer = (i == GC_PIPE_NUM_THREADS - 1);

            // ----------------------------------------------------------------------------------
            // -- Process the bin's primitives and fill a fragment buffer.
            // ----------------------------------------------------------------------------------

            while (primitive_list->count)
            {
                gc_screen_rect_t box;
                gc_primitive_t *primitive = (gc_primitive_t *) thread_primitives->data + list_item->prim_idx;

                // ----------------------------------------------------------------------------------
                // -- The primitive covers the whole tile.
                // ----------------------------------------------------------------------------------

                if (list_item->total_coverage)
                {
                    box.min.x = tile_buffer->x;
                    box.min.y = tile_buffer->y;
                    box.max.x = tile_buffer->x + GL_BIN_WIDTH;
                    box.max.y = tile_buffer->y + GL_BIN_HEIGHT;

                    gl_push_fragments_full(&batch_memory, &box, 0, primitive);
                }

                // ----------------------------------------------------------------------------------
                // -- Split the tile in 8x8 blocks and evaluate the coverage for each block.
                // ----------------------------------------------------------------------------------

                else
                {
                    // Relative values to the current bin.
                    s32 block_row_min = (primitive->box.min.y - current_bin->y) >> GL_BIN_BLOCK_SHIFT;
                    s32 block_row_max = (primitive->box.max.y - current_bin->y) >> GL_BIN_BLOCK_SHIFT;
                    s32 block_col_min = (primitive->box.min.x - current_bin->x) >> GL_BIN_BLOCK_SHIFT;
                    s32 block_col_max = (primitive->box.max.x - current_bin->x) >> GL_BIN_BLOCK_SHIFT;

                    if (block_row_min < 0) block_row_min = 0;
                    if (block_col_min < 0) block_col_min = 0;
                    if (block_row_max >= GL_BIN_BLOCK_ROWS) block_row_max = GL_BIN_BLOCK_ROWS - 1;
                    if (block_col_max >= GL_BIN_BLOCK_COLS) block_col_max = GL_BIN_BLOCK_COLS - 1;

                    s32 block_min_x = current_bin->x + (block_col_min << GL_BIN_BLOCK_SHIFT);
                    s32 block_max_x = current_bin->x + ((block_col_max + 1) << GL_BIN_BLOCK_SHIFT);
                    s32 block_min_y = current_bin->y + (block_row_min << GL_BIN_BLOCK_SHIFT);
                    s32 block_max_y = current_bin->y + ((block_row_max + 1) << GL_BIN_BLOCK_SHIFT);

                    for (s32 by = block_min_y; by < block_max_y; by += GL_BIN_BLOCK_SIZE)
                    {
                        for (s32 bx = block_min_x; bx < block_max_x; bx += GL_BIN_BLOCK_SIZE)
                        {
                            SCREEN_RECT_SET(coverage_block, bx, by, bx + GL_BIN_BLOCK_SIZE - 1, by + GL_BIN_BLOCK_SIZE - 1);
                            gc_constant_t coverage = GCSR.gl->pinterface.coverage(primitive, &coverage_block);

                            if (coverage == GC_COVERAGE_NONE) {
                                continue;
                            }
                            // Only for triangles.
                            else if (coverage == GC_COVERAGE_TOTAL)
                            {
                                box.min.x = bx;
                                box.min.y = by;
                                box.max.x = bx + GL_BIN_BLOCK_SIZE;
                                box.max.y = by + GL_BIN_BLOCK_SIZE;

                                u16 block_rel_x = bx - current_bin->x;
                                u16 block_rel_y = by - current_bin->y;

                                u16 fragment_index_start = GET_FRAGMENT_INDEX(block_rel_x, block_rel_y);
                                gl_push_fragments_full(&batch_memory, &box, fragment_index_start, primitive);
                            }
                            else
                            {
                                box.min.x = bx;
                                box.min.y = by;
                                box.max.x = bx + GL_BIN_BLOCK_SIZE;
                                box.max.y = by + GL_BIN_BLOCK_SIZE;

                                u16 block_rel_x = bx - current_bin->x;
                                u16 block_rel_y = by - current_bin->y;

                                u16 fragment_index_start = GET_FRAGMENT_INDEX(block_rel_x, block_rel_y);
                                GCSR.gl->pinterface.rasterization(&batch_memory, &box, fragment_index_start, primitive);
                            }
                        }
                    }
                }

                // ----------------------------------------------------------------------------------
                // -- If the batch is full process the fragments.
                // ----------------------------------------------------------------------------------

                if (fragments_array->count > GL_FRAGMENT_BATCH_THRESHOLD)
                {
                    PROCESS_FRAGMENT_BATCH(fragments_array, tile_buffer, transparency_buffer);
                    fragments_array->count = 0;
                    thread_memory->cursor = return_cursor;
                }

                list_item = list_item->next;
                primitive_list->count--;
            }

            if (fragments_array->count && last_buffer)
            {
                PROCESS_FRAGMENT_BATCH_LAST(fragments_array, tile_buffer, transparency_buffer);
                fragments_array->count = 0;
                thread_memory->cursor = return_cursor;
            }
        }

        SDL_assert(thread_memory->cursor <= thread_memory->bytes);
    }
}

// ----------------------------------------------------------------------------------
// -- Copy the secondary buffer to the primary buffer (scaling).
// ----------------------------------------------------------------------------------

#if 0
void scale_framebuffer(gc_framebuffer_slot_t to_slot, gc_framebuffer_slot_t from_slot)
{
    gc_framebuffer_t *primary = GCSR.gl->framebuffers[to_slot];
    gc_framebuffer_t *secondary = GCSR.gl->framebuffers[from_slot];

    u32 *dest = primary->video_memory;
    u32 *src = secondary->video_memory;

    r32 primary_one_over_width = 1.0f / (primary->width - 1);
    r32 primary_one_over_height = 1.0f / (primary->height - 1);

    while (true)
    {
        u32 bin_index = SDL_AtomicAdd(&primary->bin_cursor, 1);

        if (bin_index >= primary->total_bins)
            break;

        gc_bin_t *current_bin = primary->bins + bin_index;

        u32 sx = current_bin->x;
        u32 sy = current_bin->y;
        u32 ex = current_bin->x + GL_BIN_WIDTH;
        u32 ey = current_bin->y + GL_BIN_HEIGHT;

        u32 *bin_dest_pointer = dest + sy * primary->tiled_width + sx;

#ifdef GC_PIPE_SSE

        __m128i secondary_stride_4x = _mm_set1_epi32(secondary->tiled_width);
        __m128 primary_width_4x = _mm_set1_ps(primary->width);
        __m128 secondary_width_4x = _mm_set1_ps(secondary->width - 1);
        __m128 secondary_height_4x = _mm_set1_ps(secondary->height - 1);
        __m128 primary_one_over_width_4x = _mm_set1_ps(primary_one_over_width);

        r32 tu[4];
        r32 tv[4];

        for (u32 row = sy; row < ey; ++row)
        {
            u32 *dest_pointer = bin_dest_pointer;

            for (u32 col = sx; col < ex; col += GC_FRAG_SIZE)
            {
                __m128 col_4x = _mm_setr_ps(col, col + 1, col + 2, col + 3);
                __m128 mask_4x = _mm_cmplt_ps(col_4x, primary_width_4x);
                col_4x = _mm_and_ps(col_4x, mask_4x);
                r32 v = (r32) row * primary_one_over_height;
                __m128 u_4x = _mm_mul_ps(col_4x, primary_one_over_width_4x);

                __m128i x_4x = _mm_cvttps_epi32(_mm_mul_ps(u_4x, secondary_width_4x));
                __m128i y_4x = _mm_set1_epi32((u32) (v * (secondary->height - 1)));

                __m128i offset_4x = _mm_add_epi32(_mm_mullo_epi32(y_4x, secondary_stride_4x), x_4x);

                *dest_pointer++ = src[_mm_extract_epi32(offset_4x, 0)];
                *dest_pointer++ = src[_mm_extract_epi32(offset_4x, 1)];
                *dest_pointer++ = src[_mm_extract_epi32(offset_4x, 2)];
                *dest_pointer++ = src[_mm_extract_epi32(offset_4x, 3)];
            }

            bin_dest_pointer += primary->tiled_width;
        }
#else
        for (u32 row = sy; row < ey; ++row)
        {
            u32 *dest_pointer = bin_dest_pointer;

            for (u32 col = sx; col < ex; ++col)
            {
                if (col >= primary->width)
                    break;

                r32 u = (r32) col * primary_one_over_width;
                r32 v = (r32) row * primary_one_over_height;

                u32 x = (u32) (u * (secondary->width - 1));
                u32 y = (u32) (v * (secondary->height - 1));

                u32 offset = y * secondary->tiled_width + x;
                *dest_pointer++ = src[offset];

                // NOTE(gabic): Pentru filtrare dar merge mai greu, mai trebuie studiata problema.
                // gl_texture(&secondary_texture, u, v, &color, GL_FILTER_BILINEAR);
                // *dest_pointer++ = gl_pack(color);
            }

            bin_dest_pointer += primary->tiled_width;
        }
#endif
    }
}
#endif

// ----------------------------------------------------------------------------------
// -- Work routine.
// ----------------------------------------------------------------------------------

int gc_work(void *data)
{
    OPTICK_THREAD("Worker");
    u32 thread_id = (u32) data;

    while (true)
    {
        // -- Idle state.

        SDL_LockMutex(GCSR.gl->threads->worker_lock);

        if (GCSR.gl->threads->running_count > 0)
            GCSR.gl->threads->running_count--;

        if (GCSR.gl->threads->running_count == 0)
            SDL_CondSignal(GCSR.gl->threads->scheduler_condition);

        SDL_CondWait(GCSR.gl->threads->worker_condition, GCSR.gl->threads->worker_lock);
        SDL_UnlockMutex(GCSR.gl->threads->worker_lock);

        // -- Frontend job.

        if (GCSR.gl->threads->state == GL_RASTER_STATE_FRONTEND) {
            GCSR.gl->pinterface.frontend(thread_id);
        }

        // -- Backend job.

        else if (GCSR.gl->threads->state == GL_RASTER_STATE_BACKEND) {
            gl_pipe_backend(thread_id);
        }

        // // -- Scaling job.

        // else if (GCSR.gl->threads->state == GL_RASTER_STATE_SCALING) {
        //     scale_framebuffer(PRIMARY_BUFFER, SECONDARY_BUFFER);
        // }

        else if (GCSR.gl->threads->state == GL_RASTER_STATE_TRANSPARENCY) {
            gc_pipe_transparency_process(thread_id);
        }

        else if (GCSR.gl->threads->state == GL_RASTER_STATE_LSB_TO_TEXTURE) {
            lsb_to_texture(thread_id);
        }

        else if (GCSR.gl->threads->state == GL_RASTER_STATE_CLEAR_LSB) {
            gc_framebuffer_clear(thread_id);
        }
    }

    return 1;
}

void gl_draw_arrays(u32 *indices, asset_vertex_t *vertices, u32 count, gc_mesh_type_t type)
{
    OPTICK_EVENT("gl_draw_arrays");

    if (PIPE_FLAG(GC_MODE_NORMAL) && PIPE_FLAG(GC_SHADOW))
        GCSR.gl->pipeline.varying_count += 4;

    GCSR.gl->current_arrays.indices = indices;
    GCSR.gl->current_arrays.vertices = vertices;
    GCSR.gl->current_arrays.count = count;

    u32 prim_vertices = 3;

    if (type == GL_MESH_POINT)
    {
        prim_vertices = 1;
        GCSR.gl->pinterface.vertices = 1;
        GCSR.gl->pinterface.frontend = gl_pipe_frontend_point;
        GCSR.gl->pinterface.coverage = gl_check_point_rect_coverage;

        #if defined(GC_PIPE_AVX)
        GCSR.gl->pinterface.varyings_setup = gl_sse_fragment_varyings_setup_point;
        GCSR.gl->pinterface.rasterization = gl_sse_pipe_rasterize_point;
        #elif defined(GC_PIPE_SSE)
        GCSR.gl->pinterface.varyings_setup = gl_sse_fragment_varyings_setup_point;
        GCSR.gl->pinterface.rasterization = gl_sse_pipe_rasterize_point;
        #else
        GCSR.gl->pinterface.varyings_setup = gl_fragment_varyings_setup_point;
        GCSR.gl->pinterface.rasterization = gl_pipe_rasterize_point;
        #endif
    }
    else if (type == GL_MESH_LINE)
    {
        prim_vertices = 2;
        GCSR.gl->pinterface.vertices = 2;
        GCSR.gl->pinterface.frontend = gl_pipe_frontend_line;
        GCSR.gl->pinterface.coverage = gl_check_line_rect_coverage;

        #if defined(GC_PIPE_AVX)
        GCSR.gl->pinterface.varyings_setup = gl_sse_fragment_varyings_setup_line;
        GCSR.gl->pinterface.rasterization = gl_sse_pipe_rasterize_line;
        #elif defined(GC_PIPE_SSE)
        GCSR.gl->pinterface.varyings_setup = gl_sse_fragment_varyings_setup_line;
        GCSR.gl->pinterface.rasterization = gl_sse_pipe_rasterize_line;
        #else
        GCSR.gl->pinterface.varyings_setup = gl_fragment_varyings_setup_line;
        GCSR.gl->pinterface.rasterization = gl_pipe_rasterize_line;
        #endif
    }
    else if (type == GL_MESH_TRIANGLE)
    {
        GCSR.gl->pinterface.vertices = 3;
        GCSR.gl->pinterface.frontend = gl_pipe_frontend_triangle;
        GCSR.gl->pinterface.coverage = gl_check_triangle_rect_coverage;

        #if defined(GC_PIPE_AVX)
        GCSR.gl->pinterface.varyings_setup = gl_sse_fragment_varyings_setup_triangle;
        GCSR.gl->pinterface.rasterization = gl_sse_pipe_rasterize_triangle;
        #elif defined(GC_PIPE_SSE)
        GCSR.gl->pinterface.varyings_setup = gl_sse_fragment_varyings_setup_triangle;
        GCSR.gl->pinterface.rasterization = gl_sse_pipe_rasterize_triangle;
        #else
        GCSR.gl->pinterface.varyings_setup = gl_fragment_varyings_setup_triangle;
        GCSR.gl->pinterface.rasterization = gl_pipe_rasterize_triangle;
        #endif
    }

    GCSR.gl->curr_idx = 0;
    u32 batch_vertex_count = prim_vertices * GL_PRIM_BATCH * GC_PIPE_NUM_THREADS;
    GCSR.gl->threads->running_count = 0;

    // Reset the primitive index.
    for (u8 i = 0; i < GC_PIPE_NUM_THREADS; ++i) {
        thread_primitive_id[i] = 0;
    }

    while (GCSR.gl->curr_idx < count)
    {
        u32 batch = batch_vertex_count;

        if (GCSR.gl->curr_idx + batch_vertex_count > count)
            batch = count - GCSR.gl->curr_idx;

        // ----------------------------------------------------------------------------------
        // -- Partitioning.
        // ----------------------------------------------------------------------------------

        u32 vertices_per_thread = prim_vertices * (batch / (GC_PIPE_NUM_THREADS * prim_vertices));
        u32 remaining_prims = (batch % (GC_PIPE_NUM_THREADS * prim_vertices)) / prim_vertices;

        GCSR.gl->partitions[0].start_index = 0;
        GCSR.gl->partitions[0].end_index = vertices_per_thread;

        if (remaining_prims)
        {
            GCSR.gl->partitions[0].end_index += prim_vertices;
            remaining_prims--;
        }

        for (u8 i = 1; i < GC_PIPE_NUM_THREADS; ++i)
        {
            GCSR.gl->partitions[i].start_index = GCSR.gl->partitions[i - 1].end_index;
            GCSR.gl->partitions[i].end_index = GCSR.gl->partitions[i].start_index + vertices_per_thread;

            if (remaining_prims)
            {
                GCSR.gl->partitions[i].end_index += prim_vertices;
                remaining_prims--;
            }
        }

        gl_thread_mem_reset();

        // ----------------------------------------------------------------------------------
        // -- Frontend / backend job scheduling.
        // ----------------------------------------------------------------------------------

        #if GC_PIPE_NUM_THREADS == 1
            GCSR.gl->pinterface.frontend(0);
            gl_pipe_backend(0);
        #else
            THREAD_START(GL_RASTER_STATE_FRONTEND);
            THREAD_START(GL_RASTER_STATE_BACKEND);
        #endif

        GCSR.gl->curr_idx += batch_vertex_count;

        // ----------------------------------------------------------------------------------
        // -- Bin reset.
        // ----------------------------------------------------------------------------------

        for (u32 i = 0; i < GCSR.gl->current_framebuffer->total_bins; ++i)
        {
            gc_bin_t *bin = &GCSR.gl->current_framebuffer->bins[i];

            for (u32 k = 0; k < GC_PIPE_NUM_THREADS; ++k)
            {
                bin->list[k].start = 0;
                bin->list[k].last = 0;
                bin->list[k].count = 0;
            }

            SDL_AtomicSet(&bin->dirty, 0);
        }

        // ----------------------------------------------------------------------------------
        // -- Bin queue reset.
        // ----------------------------------------------------------------------------------

        SDL_AtomicSet(&GCSR.gl->bin_queue.count, 0);
        SDL_AtomicSet(&GCSR.gl->bin_queue.r_cursor, 0);
    }
}