// ----------------------------------------------------------------------------------
// -- File: gcsr_vertex_stream.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-03-16 21:21:44
// -- Modified: 2022-03-16 21:21:46
// ----------------------------------------------------------------------------------

// pos
void gl_vstream_point(gc_vset_t *buffer, u32 *indices, asset_vertex_t *vertices, u32 offset)
{
    OPTICK_EVENT("gl_vstream_point");

    indices += offset;

    gc_vertex_t *v0 = &buffer->v[0];

    u32 v1_idx = indices[0];

    asset_vertex_t *src_vertex1 = &vertices[v1_idx];

#ifdef GC_PIPE_SSE

    __m128 pos = _mm_load_ps(src_vertex1->pos);
    _mm_store_ps(v0->pos, pos);

#else

    // -- Vertex 0.

    v0->pos[0] = src_vertex1->pos[0];
    v0->pos[1] = src_vertex1->pos[1];
    v0->pos[2] = src_vertex1->pos[2];
#endif
}

// pos
void gl_vstream_line(gc_vset_t *buffer, u32 *indices, asset_vertex_t *vertices, u32 offset)
{
    OPTICK_EVENT("gl_vstream_line");

    indices += offset;

    gc_vertex_t *v0 = &buffer->v[0];
    gc_vertex_t *v1 = &buffer->v[1];

    u32 v1_idx = indices[0];
    u32 v2_idx = indices[1];

    asset_vertex_t *src_vertex1 = &vertices[v1_idx];
    asset_vertex_t *src_vertex2 = &vertices[v2_idx];

#ifdef GC_PIPE_SSE

    __m128 pos = _mm_load_ps(src_vertex1->pos);
    _mm_store_ps(v0->pos, pos);
    pos = _mm_load_ps(src_vertex2->pos);
    _mm_store_ps(v1->pos, pos);

#else

    // -- Vertex 0.

    v0->pos[0] = src_vertex1->pos[0];
    v0->pos[1] = src_vertex1->pos[1];
    v0->pos[2] = src_vertex1->pos[2];
    v0->pos[3] = src_vertex1->pos[3];

    // -- Vertex 1.

    v1->pos[0] = src_vertex2->pos[0];
    v1->pos[1] = src_vertex2->pos[1];
    v1->pos[2] = src_vertex2->pos[2];
    v1->pos[3] = src_vertex2->pos[3];
#endif
}

// pos, color
void gl_vstream_line_color(gc_vset_t *buffer, u32 *indices, asset_vertex_t *vertices, u32 offset)
{
    OPTICK_EVENT("gl_vstream_line_color");

    indices += offset;

    gc_vertex_t *v0 = &buffer->v[0];
    gc_vertex_t *v1 = &buffer->v[1];

    u32 v1_idx = indices[0];
    u32 v2_idx = indices[1];

    asset_vertex_t *src_vertex1 = &vertices[v1_idx];
    asset_vertex_t *src_vertex2 = &vertices[v2_idx];

#ifdef GC_PIPE_SSE
    _mm_store_ps(v0->pos, _mm_load_ps(src_vertex1->pos));
    _mm_store_ps(v0->data, _mm_load_ps(src_vertex1->data));

    _mm_store_ps(v1->pos, _mm_load_ps(src_vertex2->pos));
    _mm_store_ps(v1->data, _mm_load_ps(src_vertex2->data));

#else

    // -- Vertex 0.

    v0->pos[0] = src_vertex1->pos[0];
    v0->pos[1] = src_vertex1->pos[1];
    v0->pos[2] = src_vertex1->pos[2];

    v0->data[0] = src_vertex1->data[0];
    v0->data[1] = src_vertex1->data[1];
    v0->data[2] = src_vertex1->data[2];
    v0->data[3] = src_vertex1->data[3];

    // -- Vertex 1.

    v1->pos[0] = src_vertex2->pos[0];
    v1->pos[1] = src_vertex2->pos[1];
    v1->pos[2] = src_vertex2->pos[2];

    v1->data[0] = src_vertex2->data[0];
    v1->data[1] = src_vertex2->data[1];
    v1->data[2] = src_vertex2->data[2];
    v1->data[3] = src_vertex2->data[3];
#endif
}

// pos, uv
void gl_vstream_0(gc_vset_t *buffer, u32 *indices, asset_vertex_t *vertices, u32 offset)
{
    OPTICK_EVENT("gl_vstream_0");

    indices += offset;

    gc_vertex_t *v0 = &buffer->v[0];
    gc_vertex_t *v1 = &buffer->v[1];
    gc_vertex_t *v2 = &buffer->v[2];

    // u32 v1_idx = indices[0] - 1;
    // u32 v2_idx = indices[1] - 1;
    // u32 v3_idx = indices[2] - 1;

    u32 v1_idx = indices[0];
    u32 v2_idx = indices[1];
    u32 v3_idx = indices[2];

    asset_vertex_t *src_vertex1 = &vertices[v1_idx];
    asset_vertex_t *src_vertex2 = &vertices[v2_idx];
    asset_vertex_t *src_vertex3 = &vertices[v3_idx];

#ifdef GC_PIPE_SSE

    __m128 pos = _mm_load_ps(src_vertex1->pos);
    __m128 data0 = _mm_load_ps(src_vertex1->data);

    _mm_store_ps(v0->pos, pos);
    _mm_store_ps(v0->data, data0);

    pos = _mm_load_ps(src_vertex2->pos);
    data0 = _mm_load_ps(src_vertex2->data);

    _mm_store_ps(v1->pos, pos);
    _mm_store_ps(v1->data, data0);

    pos = _mm_load_ps(src_vertex3->pos);
    data0 = _mm_load_ps(src_vertex3->data);

    _mm_store_ps(v2->pos, pos);
    _mm_store_ps(v2->data, data0);

#else

    // -- Vertex 0.

    v0->pos[0] = src_vertex1->pos[0];
    v0->pos[1] = src_vertex1->pos[1];
    v0->pos[2] = src_vertex1->pos[2];

    v0->data[0] = src_vertex1->data[0];
    v0->data[1] = src_vertex1->data[1];

    // -- Vertex 1.

    v1->pos[0] = src_vertex2->pos[0];
    v1->pos[1] = src_vertex2->pos[1];
    v1->pos[2] = src_vertex2->pos[2];

    v1->data[0] = src_vertex2->data[0];
    v1->data[1] = src_vertex2->data[1];

    // -- Vertex 2.

    v2->pos[0] = src_vertex3->pos[0];
    v2->pos[1] = src_vertex3->pos[1];
    v2->pos[2] = src_vertex3->pos[2];

    v2->data[0] = src_vertex3->data[0];
    v2->data[1] = src_vertex3->data[1];
#endif
}

// pos, uv, normal, tangent
void gl_vstream_1(gc_vset_t *buffer, u32 *indices, asset_vertex_t *vertices, u32 offset)
{
    OPTICK_EVENT("gl_vstream_1");

    indices += offset;

    gc_vertex_t *v0 = &buffer->v[0];
    gc_vertex_t *v1 = &buffer->v[1];
    gc_vertex_t *v2 = &buffer->v[2];

    // u32 v1_idx = indices[0] - 1;
    // u32 v2_idx = indices[1] - 1;
    // u32 v3_idx = indices[2] - 1;

    u32 v1_idx = indices[0];
    u32 v2_idx = indices[1];
    u32 v3_idx = indices[2];

    asset_vertex_t *src_vertex1 = &vertices[v1_idx];
    asset_vertex_t *src_vertex2 = &vertices[v2_idx];
    asset_vertex_t *src_vertex3 = &vertices[v3_idx];

#ifdef GC_PIPE_SSE

    __m128 pos = _mm_load_ps(src_vertex1->pos);
    __m128 data0 = _mm_load_ps(src_vertex1->data);
    __m128 data1 = _mm_load_ps(src_vertex1->data + 4);
    __m128 data2 = _mm_load_ps(src_vertex1->data + 8);

    _mm_store_ps(v0->pos, pos);
    _mm_store_ps(v0->data, data0);
    _mm_store_ps(v0->data + 4, data1);
    _mm_store_ps(v0->data + 8, data2);

    pos = _mm_load_ps(src_vertex2->pos);
    data0 = _mm_load_ps(src_vertex2->data);
    data1 = _mm_load_ps(src_vertex2->data + 4);
    data2 = _mm_load_ps(src_vertex2->data + 8);

    _mm_store_ps(v1->pos, pos);
    _mm_store_ps(v1->data, data0);
    _mm_store_ps(v1->data + 4, data1);
    _mm_store_ps(v1->data + 8, data2);

    pos = _mm_load_ps(src_vertex3->pos);
    data0 = _mm_load_ps(src_vertex3->data);
    data1 = _mm_load_ps(src_vertex3->data + 4);
    data2 = _mm_load_ps(src_vertex3->data + 8);

    _mm_store_ps(v2->pos, pos);
    _mm_store_ps(v2->data, data0);
    _mm_store_ps(v2->data + 4, data1);
    _mm_store_ps(v2->data + 8, data2);

#else

    // -- Vertex 0.

    v0->pos[0] = src_vertex1->pos[0];
    v0->pos[1] = src_vertex1->pos[1];
    v0->pos[2] = src_vertex1->pos[2];

    v0->data[0] = src_vertex1->data[0];
    v0->data[1] = src_vertex1->data[1];

    v0->data[2] = src_vertex1->data[2];
    v0->data[3] = src_vertex1->data[3];
    v0->data[4] = src_vertex1->data[4];

    v0->data[5] = src_vertex1->data[5];
    v0->data[6] = src_vertex1->data[6];
    v0->data[7] = src_vertex1->data[7];
    v0->data[8] = src_vertex1->data[8];

    // -- Vertex 1.

    v1->pos[0] = src_vertex2->pos[0];
    v1->pos[1] = src_vertex2->pos[1];
    v1->pos[2] = src_vertex2->pos[2];

    v1->data[0] = src_vertex2->data[0];
    v1->data[1] = src_vertex2->data[1];

    v1->data[2] = src_vertex2->data[2];
    v1->data[3] = src_vertex2->data[3];
    v1->data[4] = src_vertex2->data[4];

    v1->data[5] = src_vertex2->data[5];
    v1->data[6] = src_vertex2->data[6];
    v1->data[7] = src_vertex2->data[7];
    v1->data[8] = src_vertex2->data[8];

    // -- Vertex 2.

    v2->pos[0] = src_vertex3->pos[0];
    v2->pos[1] = src_vertex3->pos[1];
    v2->pos[2] = src_vertex3->pos[2];

    v2->data[0] = src_vertex3->data[0];
    v2->data[1] = src_vertex3->data[1];

    v2->data[2] = src_vertex3->data[2];
    v2->data[3] = src_vertex3->data[3];
    v2->data[4] = src_vertex3->data[4];

    v2->data[5] = src_vertex3->data[5];
    v2->data[6] = src_vertex3->data[6];
    v2->data[7] = src_vertex3->data[7];
    v2->data[8] = src_vertex3->data[8];
#endif
}

// pos, uv
void gl_vstream_2(gc_vset_t *buffer, u32 *indices, asset_vertex_t *vertices, u32 offset)
{
    OPTICK_EVENT("gl_vstream_2");

    indices += offset;

    gc_vertex_t *v0 = &buffer->v[0];
    gc_vertex_t *v1 = &buffer->v[1];
    gc_vertex_t *v2 = &buffer->v[2];

    // u32 v1_idx = indices[0] - 1;
    // u32 v2_idx = indices[1] - 1;
    // u32 v3_idx = indices[2] - 1;

    u32 v1_idx = indices[0];
    u32 v2_idx = indices[1];
    u32 v3_idx = indices[2];

    asset_vertex_t *src_vertex1 = &vertices[v1_idx];
    asset_vertex_t *src_vertex2 = &vertices[v2_idx];
    asset_vertex_t *src_vertex3 = &vertices[v3_idx];

#ifdef GC_PIPE_SSE
    __m128 pos = _mm_load_ps(src_vertex1->pos);
    __m128 data0 = _mm_load_ps(src_vertex1->data);

    _mm_store_ps(v0->pos, pos);
    _mm_store_ps(v0->data, data0);

    pos = _mm_load_ps(src_vertex2->pos);
    data0 = _mm_load_ps(src_vertex2->data);

    _mm_store_ps(v1->pos, pos);
    _mm_store_ps(v1->data, data0);

    pos = _mm_load_ps(src_vertex3->pos);
    data0 = _mm_load_ps(src_vertex3->data);

    _mm_store_ps(v2->pos, pos);
    _mm_store_ps(v2->data, data0);
#else

    // -- Vertex 0.

    v0->pos[0] = src_vertex1->pos[0];
    v0->pos[1] = src_vertex1->pos[1];
    v0->pos[2] = src_vertex1->pos[2];

    v0->data[0] = src_vertex1->data[0];
    v0->data[1] = src_vertex1->data[1];

    // -- Vertex 1.

    v1->pos[0] = src_vertex2->pos[0];
    v1->pos[1] = src_vertex2->pos[1];
    v1->pos[2] = src_vertex2->pos[2];

    v1->data[0] = src_vertex2->data[0];
    v1->data[1] = src_vertex2->data[1];

    // -- Vertex 2.

    v2->pos[0] = src_vertex3->pos[0];
    v2->pos[1] = src_vertex3->pos[1];
    v2->pos[2] = src_vertex3->pos[2];

    v2->data[0] = src_vertex3->data[0];
    v2->data[1] = src_vertex3->data[1];
#endif
}