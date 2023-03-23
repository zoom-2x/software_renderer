// ----------------------------------------------------------------------------------
// -- File: gcsr_gl.cpp
// ----------------------------------------------------------------------------------
// -- Author: GC
// -- Description:
// -- Created: 2020-10-08 20:11:50
// -- Modified: 2022-05-23 21:23:41
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

void gl_destroy(gl_state_t *GL)
{}

// ----------------------------------------------------------------------------------
// -- GL initialization.
// ----------------------------------------------------------------------------------

void gc_change_resolution(u32 res_width, u32 res_height, u32 res_scaling)
{
    platform_api_t *API = get_platform_api();

    gc_delete_framebuffer(0);
    gc_delete_framebuffer(1);

    b8 scaling = res_scaling > 1;
    u32 fb0_flags = FB_FLAG_LSB | FB_FLAG_TRANSPARENCY;

    if (scaling)
        fb0_flags = 0;

    engine_framebuffer_config_t config;
    MEM_DESCRIPTION("framebuffer texture");
    gc_framebuffer_t *fb0 = gc_create_framebuffer(res_width, res_height, fb0_flags, 0);
    MEM_DESCRIPTION("framebuffer texture");
    texture2d_t *base_texture = gc_create_texture2d(fb0->tiled_width, fb0->tiled_height, 0, TEXTURE_FORMAT_RGBAU8, 0);
    gc_framebuffer_attach_color(0, base_texture);

    // Scaled resolution setup.
    if (scaling)
    {
        u32 scaled_width = res_width / res_scaling;
        u32 scaled_height = res_height / res_scaling;

        MEM_DESCRIPTION("framebuffer scaled texture");
        gc_framebuffer_t *fb1 = gc_create_framebuffer(scaled_width, scaled_height, FB_FLAG_COPY | FB_FLAG_LSB | FB_FLAG_TRANSPARENCY, 1);
        MEM_DESCRIPTION("framebuffer scaled texture");
        texture2d_t *scaled_texture = gc_create_texture2d(fb1->tiled_width, fb1->tiled_height, 0, TEXTURE_FORMAT_RGBAU8, 0);
        gc_framebuffer_attach_color(1, scaled_texture);
        SET_FRAMEBUFFER(1);
    }
    else
        SET_FRAMEBUFFER(0);

    // -- Base resolution switch.

    config.width = res_width;
    config.height = res_height;
    config.pixel_depth = 32;
    config.bytes_per_pixel = 4;
    config.pitch = fb0->tiled_width * config.bytes_per_pixel;
    config.aspect = (r32) config.width / config.height;
    config.video_memory = (u32 *) base_texture->mips->data;

    API->switch_resolution(&config);
}
// ----------------------------------------------------------------------------------
// -- Debug light mesh.
// ----------------------------------------------------------------------------------

void init_debug_light_mesh()
{
    // Light mesh, basic plane.
    size_t indices_bytes = MEM_SIZE_ALIGN(6 * sizeof(u32));
    size_t vertices_bytes = 4 * sizeof(asset_vertex_t);
    size_t debug_mesh_bytes = sizeof(mesh_t) + indices_bytes + vertices_bytes;
    MEM_LABEL("debug_light_mesh");
    GCSR.gl->debug_light_mesh = (mesh_t *) gc_mem_allocate(debug_mesh_bytes);

    GCSR.gl->debug_light_mesh->type = GL_MESH_TRIANGLE;
    GCSR.gl->debug_light_mesh->indices_count = 6;
    GCSR.gl->debug_light_mesh->indices = (u32 *) (GCSR.gl->debug_light_mesh + 1);
    GCSR.gl->debug_light_mesh->vertices = (asset_vertex_t *) ADDR_OFFSET(GCSR.gl->debug_light_mesh->indices, indices_bytes);

    asset_vertex_t *vertex = GCSR.gl->debug_light_mesh->vertices + 0;

    vertex->pos[0] = -1;
    vertex->pos[1] = 1;
    vertex->pos[2] = 0;
    vertex->pos[3] = 1;

    vertex = GCSR.gl->debug_light_mesh->vertices + 1;

    vertex->pos[0] = -1;
    vertex->pos[1] = -1;
    vertex->pos[2] = 0;
    vertex->pos[3] = 1;

    vertex = GCSR.gl->debug_light_mesh->vertices + 2;

    vertex->pos[0] = 1;
    vertex->pos[1] = -1;
    vertex->pos[2] = 0;
    vertex->pos[3] = 1;

    vertex = GCSR.gl->debug_light_mesh->vertices + 3;

    vertex->pos[0] = 1;
    vertex->pos[1] = 1;
    vertex->pos[2] = 0;
    vertex->pos[3] = 1;

    GCSR.gl->debug_light_mesh->indices[0] = 0;
    GCSR.gl->debug_light_mesh->indices[1] = 1;
    GCSR.gl->debug_light_mesh->indices[2] = 2;
    GCSR.gl->debug_light_mesh->indices[3] = 0;
    GCSR.gl->debug_light_mesh->indices[4] = 2;
    GCSR.gl->debug_light_mesh->indices[5] = 3;
}

// ----------------------------------------------------------------------------------
// -- Debug grid mesh.
// ----------------------------------------------------------------------------------

void init_debug_grid()
{
    gc_grid_mesh_t *grid_mesh = &GCSR.gl->debug_grid_mesh;

    u32 rows_center = grid_mesh->rows / 2;
    u32 cols_center = grid_mesh->cols / 2;

    r32 z_axis_start = grid_mesh->z_axis_height * 0.5f;
    r32 z_axis_end = -grid_mesh->z_axis_height * 0.5f;

    r32 start_y = grid_mesh->step * rows_center;
    r32 end_y = -grid_mesh->step * (grid_mesh->rows - rows_center);
    r32 start_x = -grid_mesh->step * cols_center;
    r32 end_x = grid_mesh->step * (grid_mesh->cols - cols_center);

    u32 arrow_vertices = 2 * 3;
    u32 arrow_indices = 6 * 3;

    u32 vertex_count = ((grid_mesh->rows + 1) << 1) + ((grid_mesh->cols + 1) << 1) + 2;

    size_t indices_bytes = MEM_SIZE_ALIGN((vertex_count + arrow_indices) * sizeof(u32));
    size_t vertices_bytes = (vertex_count + arrow_vertices) * sizeof(asset_vertex_t);
    size_t debug_grid_mesh_bytes = sizeof(mesh_t) + indices_bytes + vertices_bytes;
    MEM_LABEL("debug_grid_mesh");
    grid_mesh->mesh = (mesh_t *) gc_mem_allocate(debug_grid_mesh_bytes);

    grid_mesh->mesh->type = GL_MESH_LINE;
    grid_mesh->mesh->indices_count = vertex_count + arrow_indices;
    grid_mesh->mesh->indices = (u32 *) (grid_mesh->mesh + 1);
    grid_mesh->mesh->vertices = (asset_vertex_t *) ADDR_OFFSET(grid_mesh->mesh->indices, indices_bytes);

    u32 index = 0;
    u32 *index_pointer = grid_mesh->mesh->indices;
    asset_vertex_t *vertex_pointer = grid_mesh->mesh->vertices;

    r32 current_y = start_y;

    VINIT3(x_vector_xup, -2, 1, 0);
    v3_normalize(&x_vector_xup);
    VINIT3(x_vector_xdown, -2, -1, 0);
    v3_normalize(&x_vector_xdown);
    VINIT3(x_vector_zup, -2, 0, 1);
    v3_normalize(&x_vector_zup);
    VINIT3(x_vector_zdown, -2, 0, -1);
    v3_normalize(&x_vector_zdown);

    VINIT3(y_vector_xright, 1, -2, 0);
    v3_normalize(&y_vector_xright);
    VINIT3(y_vector_xleft, -1, -2, 0);
    v3_normalize(&y_vector_xleft);
    VINIT3(y_vector_zup, 0, -2, 1);
    v3_normalize(&y_vector_zup);
    VINIT3(y_vector_zdown, 0, -2, -1);
    v3_normalize(&y_vector_zdown);

    VINIT3(z_vector_xright, 1, 0, -2);
    v3_normalize(&z_vector_xright);
    VINIT3(z_vector_xleft, -1, 0, -2);
    v3_normalize(&z_vector_xleft);
    VINIT3(z_vector_yup, 0, 1, -2);
    v3_normalize(&z_vector_yup);
    VINIT3(z_vector_ydown, 0, -1, -2);
    v3_normalize(&z_vector_ydown);

    gc_vec_t x_axis_arrow_point;
    gc_vec_t y_axis_arrow_point;

    u32 x_axis_arrow_point_index = 0;
    u32 y_axis_arrow_point_index = 0;
    u32 z_axis_arrow_point_index = 0;

    // X axis.
    for (u8 i = 0; i <= grid_mesh->rows; ++i)
    {
        asset_vertex_t *v1 = vertex_pointer++;
        asset_vertex_t *v2 = vertex_pointer++;

        v1->pos[0] = start_x;
        v1->pos[1] = current_y;
        v1->pos[2] = 0;
        v1->pos[3] = 1;

        v2->pos[0] = end_x;
        v2->pos[1] = current_y;
        v2->pos[2] = 0;
        v2->pos[3] = 1;

        if (i == rows_center)
        {
            v1->data[0] = grid_mesh->x_color.data[0];
            v1->data[1] = grid_mesh->x_color.data[1];
            v1->data[2] = grid_mesh->x_color.data[2];
            v1->data[3] = grid_mesh->x_color.data[3];

            v2->data[0] = grid_mesh->x_color.data[0];
            v2->data[1] = grid_mesh->x_color.data[1];
            v2->data[2] = grid_mesh->x_color.data[2];
            v2->data[3] = grid_mesh->x_color.data[3];

            v1->pos[0] -= grid_mesh->axis_extension;
            v2->pos[0] += grid_mesh->axis_extension;

            x_axis_arrow_point.data[0] = v2->pos[0];
            x_axis_arrow_point.data[1] = v2->pos[1];
            x_axis_arrow_point.data[2] = v2->pos[2];
            x_axis_arrow_point_index = index + 1;
        }
        else
        {
            v1->data[0] = grid_mesh->base_color.data[0];
            v1->data[1] = grid_mesh->base_color.data[1];
            v1->data[2] = grid_mesh->base_color.data[2];
            v1->data[3] = grid_mesh->base_color.data[3];

            v2->data[0] = grid_mesh->base_color.data[0];
            v2->data[1] = grid_mesh->base_color.data[1];
            v2->data[2] = grid_mesh->base_color.data[2];
            v2->data[3] = grid_mesh->base_color.data[3];
        }

        current_y -= grid_mesh->step;
        *index_pointer++ = index++;
        *index_pointer++ = index++;
    }

    r32 current_x = start_x;

    // Y axis.
    for (u8 j = 0; j <= grid_mesh->cols; ++j)
    {
        asset_vertex_t *v1 = vertex_pointer++;
        asset_vertex_t *v2 = vertex_pointer++;

        v1->pos[0] = current_x;
        v1->pos[1] = start_y;
        v1->pos[2] = 0;
        v1->pos[3] = 1;

        v2->pos[0] = current_x;
        v2->pos[1] = end_y;
        v2->pos[2] = 0;
        v2->pos[3] = 1;

        if (j == cols_center)
        {
            v1->data[0] = grid_mesh->y_color.data[0];
            v1->data[1] = grid_mesh->y_color.data[1];
            v1->data[2] = grid_mesh->y_color.data[2];
            v1->data[3] = grid_mesh->y_color.data[3];

            v2->data[0] = grid_mesh->y_color.data[0];
            v2->data[1] = grid_mesh->y_color.data[1];
            v2->data[2] = grid_mesh->y_color.data[2];
            v2->data[3] = grid_mesh->y_color.data[3];

            v1->pos[1] += grid_mesh->axis_extension;
            v2->pos[1] -= grid_mesh->axis_extension;

            y_axis_arrow_point.data[0] = v1->pos[0];
            y_axis_arrow_point.data[1] = v1->pos[1];
            y_axis_arrow_point.data[2] = v1->pos[2];
            y_axis_arrow_point_index = index;
        }
        else
        {
            v1->data[0] = grid_mesh->base_color.data[0];
            v1->data[1] = grid_mesh->base_color.data[1];
            v1->data[2] = grid_mesh->base_color.data[2];
            v1->data[3] = grid_mesh->base_color.data[3];

            v2->data[0] = grid_mesh->base_color.data[0];
            v2->data[1] = grid_mesh->base_color.data[1];
            v2->data[2] = grid_mesh->base_color.data[2];
            v2->data[3] = grid_mesh->base_color.data[3];
        }

        current_x += grid_mesh->step;
        *index_pointer++ = index++;
        *index_pointer++ = index++;
    }

    // Z axis.
    asset_vertex_t *v1 = vertex_pointer++;
    asset_vertex_t *v2 = vertex_pointer++;

    v1->pos[0] = 0;
    v1->pos[1] = 0;
    v1->pos[2] = z_axis_start;
    v1->pos[3] = 1;

    v2->pos[0] = 0;
    v2->pos[1] = 0;
    v2->pos[2] = z_axis_end;
    v2->pos[3] = 1;

    v1->data[0] = grid_mesh->z_color.data[0];
    v1->data[1] = grid_mesh->z_color.data[1];
    v1->data[2] = grid_mesh->z_color.data[2];
    v1->data[3] = grid_mesh->z_color.data[3];

    v2->data[0] = grid_mesh->z_color.data[0];
    v2->data[1] = grid_mesh->z_color.data[1];
    v2->data[2] = grid_mesh->z_color.data[2];
    v2->data[3] = grid_mesh->z_color.data[3];

    z_axis_arrow_point_index = index;
    *index_pointer++ = index++;
    *index_pointer++ = index++;

    // -- Direction arrows X.

    r32 arrow_size = 0.15f;
    asset_vertex_t *x_arrow_0 = vertex_pointer++;
    asset_vertex_t *x_arrow_1 = vertex_pointer++;

    x_arrow_0->pos[0] = x_axis_arrow_point.data[0] + x_vector_xup.data[0] * arrow_size;
    x_arrow_0->pos[1] = x_axis_arrow_point.data[1] + x_vector_xup.data[1] * arrow_size;
    x_arrow_0->pos[2] = x_axis_arrow_point.data[2] + x_vector_xup.data[2] * arrow_size;
    x_arrow_0->pos[3] = 1;

    x_arrow_0->data[0] = grid_mesh->x_color.data[0];
    x_arrow_0->data[1] = grid_mesh->x_color.data[1];
    x_arrow_0->data[2] = grid_mesh->x_color.data[2];
    x_arrow_0->data[3] = grid_mesh->x_color.data[3];

    x_arrow_1->pos[0] = x_axis_arrow_point.data[0] + x_vector_xdown.data[0] * arrow_size;
    x_arrow_1->pos[1] = x_axis_arrow_point.data[1] + x_vector_xdown.data[1] * arrow_size;
    x_arrow_1->pos[2] = x_axis_arrow_point.data[2] + x_vector_xdown.data[2] * arrow_size;
    x_arrow_1->pos[3] = 1;

    x_arrow_1->data[0] = grid_mesh->x_color.data[0];
    x_arrow_1->data[1] = grid_mesh->x_color.data[1];
    x_arrow_1->data[2] = grid_mesh->x_color.data[2];
    x_arrow_1->data[3] = grid_mesh->x_color.data[3];

    u32 xarrow_idx[2];

    *index_pointer++ = x_axis_arrow_point_index;
    xarrow_idx[0] = index;
    *index_pointer++ = index++;
    *index_pointer++ = x_axis_arrow_point_index;
    xarrow_idx[1] = index;
    *index_pointer++ = index++;

    *index_pointer++ = xarrow_idx[0];
    *index_pointer++ = xarrow_idx[1];

    // -- Direction arrows Y.

    asset_vertex_t *y_arrow_0 = vertex_pointer++;
    asset_vertex_t *y_arrow_1 = vertex_pointer++;

    y_arrow_0->pos[0] = y_axis_arrow_point.data[0] + y_vector_xright.data[0] * arrow_size;
    y_arrow_0->pos[1] = y_axis_arrow_point.data[1] + y_vector_xright.data[1] * arrow_size;
    y_arrow_0->pos[2] = y_axis_arrow_point.data[2] + y_vector_xright.data[2] * arrow_size;
    y_arrow_0->pos[3] = 1;

    y_arrow_0->data[0] = grid_mesh->y_color.data[0];
    y_arrow_0->data[1] = grid_mesh->y_color.data[1];
    y_arrow_0->data[2] = grid_mesh->y_color.data[2];
    y_arrow_0->data[3] = grid_mesh->y_color.data[3];

    y_arrow_1->pos[0] = y_axis_arrow_point.data[0] + y_vector_xleft.data[0] * arrow_size;
    y_arrow_1->pos[1] = y_axis_arrow_point.data[1] + y_vector_xleft.data[1] * arrow_size;
    y_arrow_1->pos[2] = y_axis_arrow_point.data[2] + y_vector_xleft.data[2] * arrow_size;
    y_arrow_1->pos[3] = 1;

    y_arrow_1->data[0] = grid_mesh->y_color.data[0];
    y_arrow_1->data[1] = grid_mesh->y_color.data[1];
    y_arrow_1->data[2] = grid_mesh->y_color.data[2];
    y_arrow_1->data[3] = grid_mesh->y_color.data[3];

    u32 yarrow_idx[2];

    *index_pointer++ = y_axis_arrow_point_index;
    yarrow_idx[0] = index;
    *index_pointer++ = index++;
    *index_pointer++ = y_axis_arrow_point_index;
    yarrow_idx[1] = index;
    *index_pointer++ = index++;

    *index_pointer++ = yarrow_idx[0];
    *index_pointer++ = yarrow_idx[1];

    // -- Direction arrows Z.

    asset_vertex_t *z_arrow_0 = vertex_pointer++;
    asset_vertex_t *z_arrow_1 = vertex_pointer++;

    z_arrow_0->pos[0] = v1->pos[0] + z_vector_xright.data[0] * arrow_size;
    z_arrow_0->pos[1] = v1->pos[1] + z_vector_xright.data[1] * arrow_size;
    z_arrow_0->pos[2] = v1->pos[2] + z_vector_xright.data[2] * arrow_size;
    z_arrow_0->pos[3] = 1;

    z_arrow_0->data[0] = grid_mesh->z_color.data[0];
    z_arrow_0->data[1] = grid_mesh->z_color.data[1];
    z_arrow_0->data[2] = grid_mesh->z_color.data[2];
    z_arrow_0->data[3] = grid_mesh->z_color.data[3];

    z_arrow_1->pos[0] = v1->pos[0] + z_vector_xleft.data[0] * arrow_size;
    z_arrow_1->pos[1] = v1->pos[1] + z_vector_xleft.data[1] * arrow_size;
    z_arrow_1->pos[2] = v1->pos[2] + z_vector_xleft.data[2] * arrow_size;
    z_arrow_1->pos[3] = 1;

    z_arrow_1->data[0] = grid_mesh->z_color.data[0];
    z_arrow_1->data[1] = grid_mesh->z_color.data[1];
    z_arrow_1->data[2] = grid_mesh->z_color.data[2];
    z_arrow_1->data[3] = grid_mesh->z_color.data[3];

    u32 zarrow_idx[2];

    *index_pointer++ = z_axis_arrow_point_index;
    zarrow_idx[0] = index;
    *index_pointer++ = index++;
    *index_pointer++ = z_axis_arrow_point_index;
    zarrow_idx[1] = index;
    *index_pointer++ = index++;

    *index_pointer++ = zarrow_idx[0];
    *index_pointer++ = zarrow_idx[1];

    // -- Grid model init.

    GCSR.gl->debug_grid.meshes[0] = 0;
    GCSR.gl->debug_grid.meshes[1] = GCSR.gl->debug_grid_mesh.mesh;
    GCSR.gl->debug_grid.meshes[2] = 0;
    GCSR.gl->debug_grid.material = 0;
    // GCSR.gl->debug_grid.shader = 0;

    GCSR.gl->debug_grid.object.transforms.count = 0;

    GCSR.gl->debug_grid.object.position.data[0] = 0;
    GCSR.gl->debug_grid.object.position.data[1] = 0;
    GCSR.gl->debug_grid.object.position.data[2] = 0;
    GCSR.gl->debug_grid.object.position.data[3] = 1;

    GCSR.gl->debug_grid.object.rotation.data[0] = 0;
    GCSR.gl->debug_grid.object.rotation.data[1] = 0;
    GCSR.gl->debug_grid.object.rotation.data[2] = 0;

    GCSR.gl->debug_grid.object.scaling.data[0] = 1.0f;
    GCSR.gl->debug_grid.object.scaling.data[1] = 1.0f;
    GCSR.gl->debug_grid.object.scaling.data[2] = 1.0f;

    GCSR.gl->debug_grid.shader_id = SHADER_DEBUG_GRID;

    PUSH_LINE(&GCSR.gl->debug_grid);
}

void gl_initialize()
{
    // platform_api_t *API = get_platform_api();
    // API->open_file(&GCSR.gl->debug_file, "debug.txt", GC_FILE_WRITE);

    GCSR.gl->program_started = false;
    GCSR.gl->program_data = 0;

    size_t stack_size = GL_MAX_MATRIX_BUFFER_SIZE * sizeof(gc_mat_t) + sizeof(gc_matrix_stack_t);

    mem_set_chunk(MEMORY_PERMANENT);
    MEM_LABEL("matrix_buffer");
    GCSR.gl->matrix_buffer = (gc_matrix_stack_t *) gc_mem_allocate(stack_size);
    GCSR.gl->matrix_buffer->data = (gc_mat_t *) (GCSR.gl->matrix_buffer + 1);

    // ----------------------------------------------------------------------------------
    // -- Thread allocation.
    // ----------------------------------------------------------------------------------

    size_t multithreading_size = sizeof(gc_multithreading_t) +
                                 sizeof(void *) * GC_PIPE_NUM_THREADS;

    MEM_LABEL("threads");
    u8 *threads_pointer = (u8 *) gc_mem_allocate(multithreading_size);
    GCSR.gl->threads = (gc_multithreading_t *) threads_pointer;
    GCSR.gl->threads->pool = (SDL_Thread **) ADDR_OFFSET(threads_pointer, sizeof(gc_multithreading_t));

    GCSR.gl->threads->running_count = 0;
    GCSR.gl->threads->worker_condition = SDL_CreateCond();
    GCSR.gl->threads->worker_lock = SDL_CreateMutex();
    GCSR.gl->threads->scheduler_condition = SDL_CreateCond();
    GCSR.gl->threads->state = GL_RASTER_STATE_BACKEND;

#if GC_PIPE_NUM_THREADS > 1
    for (u32 i = 0; i < GC_PIPE_NUM_THREADS; ++i) {
        GCSR.gl->threads->pool[i] = SDL_CreateThread(gc_work, 0, (void *) i);
    }
#endif

    mem_restore_chunk();

    mem_set_chunk(MEMORY_TEMPORARY);
    da_create(GCSR.gl->triangle_queue, gc_model_t*, 20);
    da_create(GCSR.gl->line_queue, gc_model_t*, 20);
    da_create(GCSR.gl->point_queue, gc_model_t*, 20);
    mem_restore_chunk();

    // u32 cores = SDL_GetCPUCount();
    // u32 cacheLineSize = SDL_GetCPUCacheLineSize();
    // printf("Cores: %u\n", cores);
    // printf("L1 cache size: %u\n", cacheLineSize);
}

// ----------------------------------------------------------------------------------
// -- Matrix buffer functions.
// ----------------------------------------------------------------------------------

__INLINE__ void sse_matrix_stack_compose(gc_mat_t *out)
{
    if (MATRIX_BUFFER_COUNT() == 0)
        return;

    __ALIGN__ gc_mat_t tmp;

    gc_mat_t *m0 = 0;
    gc_mat_t *m1 = 0;

    if (MATRIX_BUFFER_COUNT() == 1)
    {
        m0 = MATRIX_BUFFER_GET(0);

        __m128 r0 = _mm_load_ps(m0->data[0]);
        __m128 r1 = _mm_load_ps(m0->data[1]);
        __m128 r2 = _mm_load_ps(m0->data[2]);
        __m128 r3 = _mm_load_ps(m0->data[3]);

        _mm_store_ps(out->data[0], r0);
        _mm_store_ps(out->data[1], r1);
        _mm_store_ps(out->data[2], r2);
        _mm_store_ps(out->data[3], r3);
    }
    else
    {
        for (u8 i = 1; i < MATRIX_BUFFER_COUNT(); ++i)
        {
            if (i == 1)
            {
                m0 = MATRIX_BUFFER_GET(0);
                m1 = MATRIX_BUFFER_GET(1);

                __m128 r0 = _mm_load_ps(m1->data[0]);
                __m128 r1 = _mm_load_ps(m1->data[1]);
                __m128 r2 = _mm_load_ps(m1->data[2]);
                __m128 r3 = _mm_load_ps(m1->data[3]);

                __m128 c0 = _mm_set1_ps(m0->data[0][0]);
                __m128 c1 = _mm_set1_ps(m0->data[0][1]);
                __m128 c2 = _mm_set1_ps(m0->data[0][2]);
                __m128 c3 = _mm_set1_ps(m0->data[0][3]);

                __m128 res_row_0 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                c0 = _mm_set1_ps(m0->data[1][0]);
                c1 = _mm_set1_ps(m0->data[1][1]);
                c2 = _mm_set1_ps(m0->data[1][2]);
                c3 = _mm_set1_ps(m0->data[1][3]);

                __m128 res_row_1 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                c0 = _mm_set1_ps(m0->data[2][0]);
                c1 = _mm_set1_ps(m0->data[2][1]);
                c2 = _mm_set1_ps(m0->data[2][2]);
                c3 = _mm_set1_ps(m0->data[2][3]);

                __m128 res_row_2 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                c0 = _mm_set1_ps(m0->data[3][0]);
                c1 = _mm_set1_ps(m0->data[3][1]);
                c2 = _mm_set1_ps(m0->data[3][2]);
                c3 = _mm_set1_ps(m0->data[3][3]);

                __m128 res_row_3 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                _mm_store_ps(tmp.data[0], res_row_0);
                _mm_store_ps(tmp.data[1], res_row_1);
                _mm_store_ps(tmp.data[2], res_row_2);
                _mm_store_ps(tmp.data[3], res_row_3);
            }
            else
            {
                m0 = &tmp;
                m1 = MATRIX_BUFFER_GET(i);

                __m128 r0 = _mm_load_ps(m1->data[0]);
                __m128 r1 = _mm_load_ps(m1->data[1]);
                __m128 r2 = _mm_load_ps(m1->data[2]);
                __m128 r3 = _mm_load_ps(m1->data[3]);

                __m128 c0 = _mm_set1_ps(m0->data[0][0]);
                __m128 c1 = _mm_set1_ps(m0->data[0][1]);
                __m128 c2 = _mm_set1_ps(m0->data[0][2]);
                __m128 c3 = _mm_set1_ps(m0->data[0][3]);

                __m128 res_row_0 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                c0 = _mm_set1_ps(m0->data[1][0]);
                c1 = _mm_set1_ps(m0->data[1][1]);
                c2 = _mm_set1_ps(m0->data[1][2]);
                c3 = _mm_set1_ps(m0->data[1][3]);

                __m128 res_row_1 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                c0 = _mm_set1_ps(m0->data[2][0]);
                c1 = _mm_set1_ps(m0->data[2][1]);
                c2 = _mm_set1_ps(m0->data[2][2]);
                c3 = _mm_set1_ps(m0->data[2][3]);

                __m128 res_row_2 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                c0 = _mm_set1_ps(m0->data[3][0]);
                c1 = _mm_set1_ps(m0->data[3][1]);
                c2 = _mm_set1_ps(m0->data[3][2]);
                c3 = _mm_set1_ps(m0->data[3][3]);

                __m128 res_row_3 = _mm_add_ps(
                                        _mm_add_ps(_mm_mul_ps(c0, r0), _mm_mul_ps(c1, r1)),
                                        _mm_add_ps(_mm_mul_ps(c2, r2), _mm_mul_ps(c3, r3)));

                _mm_store_ps(tmp.data[0], res_row_0);
                _mm_store_ps(tmp.data[1], res_row_1);
                _mm_store_ps(tmp.data[2], res_row_2);
                _mm_store_ps(tmp.data[3], res_row_3);
            }
        }
    }

    __m128 r0 = _mm_load_ps(tmp.data[0]);
    __m128 r1 = _mm_load_ps(tmp.data[1]);
    __m128 r2 = _mm_load_ps(tmp.data[2]);
    __m128 r3 = _mm_load_ps(tmp.data[3]);

    _mm_store_ps(out->data[0], r0);
    _mm_store_ps(out->data[1], r1);
    _mm_store_ps(out->data[2], r2);
    _mm_store_ps(out->data[3], r3);
}

__INLINE__ void gc_push_matrix(gc_mat_t *m)
{
    SDL_assert(GCSR.gl->matrix_buffer->count + 1 < GL_MAX_MATRIX_BUFFER_SIZE);
    gc_mat_t *data = GCSR.gl->matrix_buffer->data + GCSR.gl->matrix_buffer->count++;
    gl_mat4_copy(data, m);
}

void gc_compose_matrix(gc_mat_t *out)
{
    if (GCSR.gl->matrix_buffer->count > 0)
    {
        gc_mat_t *current = GCSR.gl->matrix_buffer->data;
        gl_mat4_copy(out, current++);

        if (GCSR.gl->matrix_buffer->count > 1)
        {
            for (u32 i = 1; i < GCSR.gl->matrix_buffer->count; i++)
            {
                gl_mat4_mul(out, current, out);
                current++;
            }
        }
    }
}

// ----------------------------------------------------------------------------------

void gl_pipeline_setup(gc_level_t *level)
{
    pipe_param_input_table_t *base = &GCSR.gl->pipeline.base;
    pipe_param_merged_table_t *overwrites = &GCSR.gl->pipeline.params.overwrites;

    GCSR.gl->pipeline.flags = 0;
    GCSR.gl->pipeline.params.shader_flags = 0;

    // ----------------------------------------------------------------------------------
    // -- Base overwrites settings.
    // ----------------------------------------------------------------------------------

    base->tone_mapping.value.u_byte = TONE_MAPPING_REINHARD;
    base->postprocessing.value.u_bool = false;
    base->saturation.value.u_float = 1.0;
    base->forced_opacity.value.u_float = 1;
    base->point_radius.value.u_integer = 1;

    VSET4(base->tint_color.value.u_vector, 1.0f, 1.0f, 1.0f, 1.0f);
    VSET4(base->solid_color.value.u_vector, 1.0f, 1.0f, 1.0f, 1.0f);
    VSET4(base->ambient_color.value.u_vector, 0.1f, 0.1f, 0.1f, 1.0f);
    VSET4(base->background_color.value.u_vector, 0.1f, 0.1f, 0.1f, 1.0f);
    VSET4(base->wireframe_color.value.u_vector, 1.0f, 1.0f, 1.0f, 1.0f);
    VSET4(base->point_color.value.u_vector, 1.0f, 1.0f, 1.0f, 1.0f);
    VSET2(base->uv_scaling.value.u_vector, 0, 0);

    overwrites->tone_mapping[0] = &base->tone_mapping.value;
    overwrites->postprocessing[0] = &base->postprocessing.value;
    overwrites->saturation[0] = &base->saturation.value;
    overwrites->forced_opacity[0] = &base->forced_opacity.value;
    overwrites->tint_color[0] = &base->tint_color.value;
    overwrites->solid_color[0] = &base->solid_color.value;
    overwrites->ambient_color[0] = &base->ambient_color.value;
    overwrites->background_color[0] = &base->background_color.value;
    overwrites->wireframe_color[0] = &base->wireframe_color.value;
    overwrites->point_color[0] = &base->point_color.value;
    overwrites->uv_scaling[0] = &base->uv_scaling.value;
    overwrites->point_radius[0] = &base->point_radius.value;

    // ----------------------------------------------------------------------------------
    // -- Level overwrites.
    // ----------------------------------------------------------------------------------

    PIPE_PARAM_COPY(0, 1);

    if (level->settings.overwrites.tone_mapping.overwrite)
        overwrites->tone_mapping[1] = &level->settings.overwrites.tone_mapping.value;

    if (level->settings.overwrites.postprocessing.overwrite)
        overwrites->postprocessing[1] = &level->settings.overwrites.postprocessing.value;

    if (level->settings.overwrites.saturation.overwrite)
        overwrites->saturation[1] = &level->settings.overwrites.saturation.value;

    if (level->settings.overwrites.tint_color.overwrite)
        overwrites->tint_color[1] = &level->settings.overwrites.tint_color.value;

    if (level->settings.overwrites.background_color.overwrite)
        overwrites->background_color[1] = &level->settings.overwrites.background_color.value;

    if (level->settings.overwrites.solid_color.overwrite)
        overwrites->solid_color[1] = &level->settings.overwrites.solid_color.value;

    if (level->settings.overwrites.ambient_color.overwrite)
        overwrites->ambient_color[1] = &level->settings.overwrites.ambient_color.value;

    if (level->settings.overwrites.wireframe_color.overwrite)
        overwrites->wireframe_color[1] = &level->settings.overwrites.wireframe_color.value;

    if (level->settings.overwrites.point_color.overwrite)
        overwrites->point_color[1] = &level->settings.overwrites.point_color.value;

    if (level->settings.overwrites.point_radius.overwrite)
        overwrites->point_radius[1] = &level->settings.overwrites.point_radius.value;

    PIPE_FLAG_ENABLE(GC_WINDING_CCW | GC_BACKFACE_CULL);

    // ----------------------------------------------------------------------------------
    // -- Level settings.
    // ----------------------------------------------------------------------------------

    gl_vec_copy(&GCSR.gl->camera.eye, &GCSR.gl->pipeline.params.world_camera_position);
    gl_vec_copy(&GCSR.gl->camera.axis_z, &GCSR.gl->pipeline.params.world_camera_direction);

    GCSR.gl->pipeline.flags = (level->settings.flags & GC_LEVEL_ALLOWED_FLAGS) |
                              (GCSR.gl->pipeline.flags & (~GC_LEVEL_ALLOWED_FLAGS));

    GCSR.gl->pipeline.params.lights = level->lights;
    GCSR.gl->pipeline.params.light_count = level->light_count;
}

// ----------------------------------------------------------------------------------
// -- 2D rendering routine.
// ----------------------------------------------------------------------------------

void gl_render_2D(gl_state_t *GL)
{}

// ----------------------------------------------------------------------------------
// -- 3D rendering routine.
// ----------------------------------------------------------------------------------

void gl_render_3d(gc_level_t *level)
{
    OPTICK_EVENT("gl_render_3d");

    if (!level)
        return;

    gl_update_lights(level->lights, level->light_count);
    update_camera(&GCSR.gl->camera);
    gl_pipeline_setup(level);

    // ----------------------------------------------------------------------------------
    // -- Shadow map generation - for each light that has "shadows" enabled.
    // ----------------------------------------------------------------------------------

    if (PIPE_FLAG(GC_MODE_NORMAL) && PIPE_FLAG(GC_SHADOW))
    {
        for (u8 i = 0; i < level->light_count; ++i)
        {
            gc_light_t *current_light = level->lights + i;

            // ----------------------------------------------------------------------------------
            // -- Directional light.
            // ----------------------------------------------------------------------------------

            if (current_light->type == GC_SUN_LIGHT && current_light->updated)
            {
                current_light->updated = false;
                texture2d_t *shadow_texture0 = (texture2d_t *) current_light->shadow_texture;

                SET_FRAMEBUFFER(2);
                gc_framebuffer_attach_color(2, shadow_texture0);
                gc_vec_t clear_color = {1, 1};
                THREAD_FRAMEBUFFER_CLEAR(clear_color);

                gc_compute_shadow_map_directional(level, current_light);

                // ----------------------------------------------------------------------------------
                // -- LSB to texture.
                // ----------------------------------------------------------------------------------

                if (FLAG(GCSR.gl->current_framebuffer->flags, FB_FLAG_LSB))
                {
                    gc_tile_buffer_t *current_tile = GCSR.gl->current_framebuffer->lsb.tiles;

                    for (u32 j = 0; j < GCSR.gl->current_framebuffer->total_bins; ++j)
                    {
                        GCSR.gl->current_framebuffer->save(GCSR.gl->current_framebuffer, current_tile);
                        current_tile++;
                    }
                }
            }

            // ----------------------------------------------------------------------------------
            // -- Spot light.
            // ----------------------------------------------------------------------------------

            else if (current_light->type == GC_POINT_LIGHT && current_light->updated)
            {
                SET_FRAMEBUFFER(2);
                current_light->updated = false;
                cube_texture_t *shadow_texture = (cube_texture_t *) current_light->shadow_texture;

                for (u8 face = 0; face < 6; ++face)
                {
                    texture2d_t *face_texture = shadow_texture->faces[face];
                    gc_framebuffer_attach_color(2, face_texture);

                    gc_vec_t clear_color = {1, 1};
                    THREAD_FRAMEBUFFER_CLEAR(clear_color);

                    gc_compute_point_shadow_map(level, current_light, (gl_cube_faces_t) face);

                    // ----------------------------------------------------------------------------------
                    // -- LSB to texture.
                    // ----------------------------------------------------------------------------------

                    if (FLAG(GCSR.gl->current_framebuffer->flags, FB_FLAG_LSB))
                    {
                        gc_tile_buffer_t *current_tile = GCSR.gl->current_framebuffer->lsb.tiles;

                        for (u32 j = 0; j < GCSR.gl->current_framebuffer->total_bins; ++j)
                        {
                            GCSR.gl->current_framebuffer->save(GCSR.gl->current_framebuffer, current_tile);
                            current_tile++;
                        }
                    }
                }

                shadow_texture = (cube_texture_t *) current_light->shadow_texture;
                gc_framebuffer_attach_color(2, shadow_texture->faces[CUBE_BOTTOM]);
            }
        }
    }

    // ----------------------------------------------------------------------------------
    // -- Main buffer pass.
    // ----------------------------------------------------------------------------------

#ifndef GL_DEBUG_SHADOW_MAP

    if (GCSR.state->settings.res_scaling > 1)
        SET_FRAMEBUFFER(1);
    else
        SET_FRAMEBUFFER(0);

    THREAD_FRAMEBUFFER_CLEAR(PIPE_PARAM_VALUE(1, background_color, u_vector));

    gl_set_projection_from_level(level);
    gc_mat4_lookat(&GCSR.gl->camera.eye,
                   &GCSR.gl->camera.target,
                   &GCSR.gl->camera.up,
                   GET_MATRIX(M_VIEW));

    gc_viewport(GCSR.gl->current_framebuffer->tiled_width, GCSR.gl->current_framebuffer->tiled_height, 0, 0);

    if (PIPE_FLAG(GC_MODE_NORMAL))
        render_triangle_queue(level->shader_id, GC_MODE_NORMAL);

    if (PIPE_FLAG(GC_MODE_WIREFRAME))
        render_triangle_queue(level->shader_id, GC_MODE_WIREFRAME);

    if (PIPE_FLAG(GC_MODE_POINT))
        render_triangle_queue(level->shader_id, GC_MODE_POINT);

    render_line_queue();
    render_point_queue();

    if (level->settings.debug_lights)
        level_debug_lights_pass(level);

    // ----------------------------------------------------------------------------------
    // -- Transparency processing.
    // ----------------------------------------------------------------------------------

    if (BUFFER_FLAG(FB_FLAG_TRANSPARENCY)) {
        THREAD_PROCESS_TRANSPARENCY();
    }

    // ----------------------------------------------------------------------------------
    // -- LSB to texture (linear screen buffer).
    // ----------------------------------------------------------------------------------

    if (FLAG(GCSR.gl->current_framebuffer->flags, FB_FLAG_LSB)) {
        THREAD_LSB_TO_TEXTURE();
    }

#endif
}
