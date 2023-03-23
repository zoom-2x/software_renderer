// ----------------------------------------------------------------------------------
// -- File: gcsr_macros.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-12-17 21:51:15
// -- Modified: 2022-11-16 19:15:21
// ----------------------------------------------------------------------------------

#ifndef GCSR_MACROS_H
#define GCSR_MACROS_H

#ifdef __cplusplus
#ifdef ENABLE_OPTICK
#include "optick/optick.h"
#else
#define OPTICK_EVENT(name)
#define OPTICK_THREAD(name)
#define OPTICK_FRAME(name)
#endif
#else
#define OPTICK_EVENT(name)
#define OPTICK_THREAD(name)
#define OPTICK_FRAME(name)
#endif

#include "xmmintrin.h"

// ----------------------------------------------------------------------------------
// -- Memory management.
// ----------------------------------------------------------------------------------

#if _MSC_VER
#define ALIGN(s) __declspec(align(s))
#define __INLINE__ inline
#else
#define ALIGN(s)
#define __INLINE__
#endif

// The smallest possible memory size that can be allocated by the memory manager.
// All allocations will be a multiple of this value (in bytes).
#define USE_MEMORY_ALIGN_SIZE 1
#define MEMORY_ALIGN_SIZE 16
#define MEMORY_ALIGN_SHIFT 4
#define MEMORY_BLOCK_OVERHEAD sizeof(memory_block_t)
// A block must have a minimum of 16 cells + the header overhead.
#define MIN_BLOCK_BYTES (MEMORY_ALIGN_SIZE << 4) + MEMORY_BLOCK_OVERHEAD
#define BLOCK_CORRUPTION_CHECK 2139291415
#define MAX_MEMORY_BLOCKS_TO_REMEMBER 64

#define __ALIGN__ ALIGN(MEMORY_ALIGN_SIZE)
#define MEM_SIZE_ALIGN(size) (((size) + MEMORY_ALIGN_SIZE - 1) & ~(MEMORY_ALIGN_SIZE - 1))
// #define MEM_SIZE_ALIGN(size) ((((size) >> MEMORY_ALIGN_SHIFT) + 1) << MEMORY_ALIGN_SHIFT)
#define ADDRESS_ALIGN(addr) (void *) (((u64) addr + MEMORY_ALIGN_SIZE - 1) & ~(MEMORY_ALIGN_SIZE - 1))

// Aligns the given size to a multiple of GC_FRAG_SIZE.
#define BUFFER_SIZE_ALIGN(v) ((v + (GC_FRAG_SIZE - 1)) & ~(GC_FRAG_SIZE - 1))

#define GET_BLOCK(data) ((memory_block_t *) data - 1)
#define GET_DATA_LOCATION(data) ((GET_BLOCK(data))->location)
#define GET_CHUNK(type) ((memory_chunk_t *) &(get_memory_manager()->chunks[type]))
#define GET_BLOCK_CHUNK(block) GET_CHUNK(block->location)
#define GET_BLOCK_BYTES(data) GET_BLOCK(data)->total_bytes
#define BLOCK_CHECK(data) (GET_BLOCK(data)->bcc == BLOCK_CORRUPTION_CHECK)
#define DEBUG_BLOCK(data) (global_debug_block = GET_BLOCK(data))

// ----------------------------------------------------------------------------------
// -- Engine.
// ----------------------------------------------------------------------------------

#define TONE_MAPPING_CLAMP 1
#define TONE_MAPPING_REINHARD 2
#define TONE_MAPPING_FILMIC 3
#define TONE_MAPPING_ACES 4
#define TONE_MAPPING_ACES_APROX 5

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)

#define BytesToKilo(value) (r32) value / 1024
#define BytesToMega(value) (r32) value / (1024 * 1024)
#define BytesToGiga(value) (r32) value / (1024 * 1024 * 1024)

#define internal static
#define local_persist static
#define global_variable static

#define MAX_STRING_LENGTH 2048

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define matCount(Array) (sizeof(Array) / sizeof((Array)[0][0]))

// ----------------------------------------------------------------------------------
// -- The pixel format will be fixed, the packed bitmaps channels will be
// -- rearranged to match the fixed format.
// ----------------------------------------------------------------------------------

#define GL_PIXEL_FORMAT SDL_PIXELFORMAT_RGBA8888

#define GL_PIXEL_FORMAT_RED_SHIFT 24
#define GL_PIXEL_FORMAT_GREEN_SHIFT 16
#define GL_PIXEL_FORMAT_BLUE_SHIFT 8
#define GL_PIXEL_FORMAT_ALPHA_SHIFT 0

#define GC_PIXEL_FORMAT_RMASK 0xff000000
#define GC_PIXEL_FORMAT_GMASK 0x00ff0000
#define GC_PIXEL_FORMAT_BMASK 0x0000ff00
#define GC_PIXEL_FORMAT_AMASK 0x000000ff

#define GC_PIXEL_FORMAT_OVERR 0.000000000233743705f
#define GC_PIXEL_FORMAT_OVERG 0.00000005983838848039f
#define GC_PIXEL_FORMAT_OVERB 0.00001531862745098039f
#define GC_PIXEL_FORMAT_OVERA 0.00392156862745098039f

// ----------------------------------------------------------------------------------

#define ADDR_OFFSET(addr, off) ((u8 *) (addr) + (off))
#define ADDR_OFFSET32(addr, off) ((u32 *) (addr) + (off))
#define MULTIPLE_OF(val, mult) ((val + (mult - 1)) & (~(mult - 1)))
#define MESH_BYTES(count) (sizeof(mesh_t) + (count) * (sizeof(u32) + sizeof(asset_vertex_t)))

// ----------------------------------------------------------------------------------
// -- Graphics library.
// ----------------------------------------------------------------------------------

#define GL_MAX_MATRIX_BUFFER_SIZE 15
#define CLIPPING_VERTEX_BUFFER_COUNT 20

// ----------------------------------------------------------------------------------
// -- Fragment setup.
// ----------------------------------------------------------------------------------

#ifdef GL_PIPE_AVX
    #define GL_FRAG_FORMAT8_4x2 1   // AVX
#else
    #define GL_FRAG_FORMAT4_2x2 1   // SSE
    // #define GL_FRAG_FORMAT4_4x1 1   // SSE
#endif

#if defined(GL_FRAG_FORMAT8_4x2)

    #define GC_FRAG_SIZE 8
    #define GL_FRAG_SHIFT 3
    #define GL_FRAG_WIDTH 4
    #define GL_FRAG_HEIGHT 2
    #define GL_FRAG_WIDTH_SHIFT 2
    #define GL_FRAG_HEIGHT_SHIFT 1

    #define GL_FRAG_OFFX_PIXEL0 0
    #define GL_FRAG_OFFX_PIXEL1 1
    #define GL_FRAG_OFFX_PIXEL2 2
    #define GL_FRAG_OFFX_PIXEL3 3
    #define GL_FRAG_OFFX_PIXEL4 0
    #define GL_FRAG_OFFX_PIXEL5 1
    #define GL_FRAG_OFFX_PIXEL6 2
    #define GL_FRAG_OFFX_PIXEL7 3

    #define GL_FRAG_OFFY_PIXEL0 0
    #define GL_FRAG_OFFY_PIXEL1 0
    #define GL_FRAG_OFFY_PIXEL2 0
    #define GL_FRAG_OFFY_PIXEL3 0
    #define GL_FRAG_OFFY_PIXEL4 1
    #define GL_FRAG_OFFY_PIXEL5 1
    #define GL_FRAG_OFFY_PIXEL6 1
    #define GL_FRAG_OFFY_PIXEL7 1

#elif defined(GL_FRAG_FORMAT4_2x2)

    #define GC_FRAG_SIZE 4
    #define GL_FRAG_SHIFT 2
    #define GL_FRAG_WIDTH 2
    #define GL_FRAG_HEIGHT 2
    #define GL_FRAG_WIDTH_SHIFT 1
    #define GL_FRAG_HEIGHT_SHIFT 1

    #define GL_FRAG_DEPTH_SHIFTX 1
    #define GL_FRAG_DEPTH_SHIFTY 1

    #define GL_FRAG_OFFX_PIXEL0 0
    #define GL_FRAG_OFFX_PIXEL1 1
    #define GL_FRAG_OFFX_PIXEL2 0
    #define GL_FRAG_OFFX_PIXEL3 1

    #define GL_FRAG_OFFY_PIXEL0 0
    #define GL_FRAG_OFFY_PIXEL1 0
    #define GL_FRAG_OFFY_PIXEL2 1
    #define GL_FRAG_OFFY_PIXEL3 1

#elif defined(GL_FRAG_FORMAT4_4x1)

    #define GC_FRAG_SIZE 4
    #define GL_FRAG_SHIFT 2
    #define GL_FRAG_WIDTH 4
    #define GL_FRAG_HEIGHT 1
    #define GL_FRAG_WIDTH_SHIFT 2
    #define GL_FRAG_HEIGHT_SHIFT 0

    #define GL_FRAG_DEPTH_SHIFTX 0
    #define GL_FRAG_DEPTH_SHIFTY 0

    #define GL_FRAG_OFFX_PIXEL0 0
    #define GL_FRAG_OFFX_PIXEL1 1
    #define GL_FRAG_OFFX_PIXEL2 2
    #define GL_FRAG_OFFX_PIXEL3 3

    #define GL_FRAG_OFFY_PIXEL0 0
    #define GL_FRAG_OFFY_PIXEL1 0
    #define GL_FRAG_OFFY_PIXEL2 0
    #define GL_FRAG_OFFY_PIXEL3 0

#endif

// ----------------------------------------------------------------------------------
// -- New pipeline.
// ----------------------------------------------------------------------------------

#define GL_PRIM_BATCH 12000

// NOTE(gabic): Multiple of 8
#define GL_BIN_WIDTH 64
#define GL_BIN_HEIGHT 64
#define GL_BIN_SIZE (GL_BIN_WIDTH * GL_BIN_HEIGHT)
#define GL_BIN_WIDTH_SHIFT 6
#define GL_BIN_HEIGHT_SHIFT 6

#define GL_SIMD_SHIFT 2

#define GL_BIN_BLOCK_SIZE 16
#define GL_BIN_BLOCK_TOTAL_SIZE 256
#define GL_BIN_BLOCK_TOTAL_SHIFT 8
#define GL_BIN_BLOCK_SHIFT 4
#define GL_BIN_BLOCK_COLS (GL_BIN_WIDTH >> GL_BIN_BLOCK_SHIFT)
#define GL_BIN_BLOCK_ROWS (GL_BIN_HEIGHT >> GL_BIN_BLOCK_SHIFT)
#define GL_BIN_TOTAL_BLOCKS (GL_BIN_BLOCK_COLS * GL_BIN_BLOCK_ROWS)

#define GL_BIN_BLOCK_FRAG_ROWS (GL_BIN_BLOCK_SIZE >> GL_FRAG_HEIGHT_SHIFT)
#define GL_BIN_BLOCK_FRAG_COLS (GL_BIN_BLOCK_SIZE >> GL_FRAG_WIDTH_SHIFT)
#define GL_BIN_BLOCK_FRAGS (GL_BIN_BLOCK_FRAG_ROWS * GL_BIN_BLOCK_FRAG_COLS)
#define GL_TILE_FRAG_STRIDE (GL_BIN_WIDTH >> GL_FRAG_WIDTH_SHIFT)

#define GL_BIN_FRAGS (GL_BIN_SIZE >> GL_FRAG_SHIFT)
#define GL_BIN_FRAG_ROWS (GL_BIN_HEIGHT >> GL_FRAG_HEIGHT_SHIFT)
#define GL_BIN_FRAG_COLS (GL_BIN_WIDTH >> GL_FRAG_WIDTH_SHIFT)

#define GL_FRAGMENT_BATCH_PARTITIONING 1
#define GL_FRAGMENT_BATCH_THRESHOLD 32
#define GL_FRAGMENT_BATCH_THRESHOLD_SHIFT 5

// 2x2 pixel fragment.
#define GL_FRAG_MASK_PIXEL0 0b11000000
#define GL_FRAG_MASK_PIXEL1 0b00110000
#define GL_FRAG_MASK_PIXEL2 0b00001100
#define GL_FRAG_MASK_PIXEL3 0b00000011
#define GL_FRAG_MASK_SHIFT GC_FRAG_SIZE

#ifdef GC_PIPE_SSE
#define GL_FRAGMENT_FULL_MASK 0b11111111
#define GC_FRAGMENT_MASKS {GL_FRAG_MASK_PIXEL0, GL_FRAG_MASK_PIXEL1, GL_FRAG_MASK_PIXEL2, GL_FRAG_MASK_PIXEL3}
#else
#define GL_FRAGMENT_FULL_MASK 0b1111
#define GC_FRAGMENT_MASKS {0b1000, 0b0100, 0b0010, 0b0001}
#endif

// ----------------------------------------------------------------------------------
// -- Vertex buffer attribute sizes.
// ----------------------------------------------------------------------------------

#define VA_POS 4
#define VA_UV 2
#define VA_NORM 3
#define VA_WORLD_POS 3
#define VA_VIEW_DIR 3
#define VA_LIGHT_DIR 3
#define VA_TANGENT 4
#define VA_COLOR 4
#define VA_BARY 3

#define VERTEX_ATTRIBUTE_SIZE 20
#define VARYINGS_BUFFER_SIZE (BUFFER_SIZE_ALIGN((VA_BARY + VA_UV + VA_NORM + VA_WORLD_POS + VA_VIEW_DIR + VA_LIGHT_DIR + VA_TANGENT)))

// ----------------------------------------------------------------------------------

#define GET_SPRITE(type) (&GCSR.gl->rasterization->point_sprites[(type) - 1])
#define GET_SHADER(id) (&GCSR.state->shader_table[(id) - 1])
#define GET_PROGRAM(id) (&GCSR.state->program_table[(id) - 1])

#define PUSH_TRIANGLE(model) \
    u32 queue_index = da_push(GCSR.gl->triangle_queue, gc_model_t*); \
    GCSR.gl->triangle_queue[queue_index] = (model);

#define PUSH_LINE(model) \
    u32 queue_index = da_push(GCSR.gl->line_queue, gc_model_t*); \
    GCSR.gl->line_queue[queue_index] = (model);

#define PUSH_POINT(model) \
    u32 queue_index = da_push(GCSR.gl->point_queue, gc_model_t*); \
    GCSR.gl->point_queue[queue_index] = (model);

// #define GET_UPDATE(id) (GCSR.state->update_table[id - 1])
// #define GET_SETUP(id) (GCSR.state->setup_table[id - 1])
#define SET_SHADER(id) \
{ \
    GCSR.gl->pipeline.shader = &GCSR.state->shader_table[id - 1]; \
    GCSR.gl->pipeline.varying_count = GCSR.gl->pipeline.shader->varying_count; \
}

#define PUSH_PROGRAM(id) \
{ \
    GCSR.gl->pipeline.stack_shader = GCSR.gl->pipeline.shader; \
    GCSR.gl->pipeline.stack_varying_count = GCSR.gl->pipeline.varying_count; \
    SET_SHADER(id); \
}

#define POP_PROGRAM() \
{ \
    GCSR.gl->pipeline.shader = GCSR.gl->pipeline.stack_shader; \
    GCSR.gl->pipeline.varying_count = GCSR.gl->pipeline.stack_varying_count; \
    GCSR.gl->pipeline.stack_shader = 0; \
    GCSR.gl->pipeline.stack_varying_count = 0; \
}

#define GET_FRAMEBUFFER() GCSR.gl->current_framebuffer
#define SET_FRAMEBUFFER(slot) GCSR.gl->current_framebuffer = GCSR.gl->framebuffers[slot]
#define FRAMEBUFFER_SLOT(slot) GCSR.gl->framebuffers[slot]

#define TEX_CHECK(framebuffer, x, y) framebuffer->depth_memory[y * framebuffer->tiled_width + x]
#define GET_MATRIX(matrix) &GCSR.gl->matrices[matrix]

#define FLAG(flags, flag) ((flags & (flag)) > 0)
#define FLAG_ENABLE(flags, flag) (flags |= (flag))
#define FLAG_DISABLE(flags, flag) (flags &= ~(flag))

#define BUFFER_FLAG(check) (FLAG(GCSR.gl->current_framebuffer->flags, (check)))

#define PIPE_FLAG(check) (FLAG(GCSR.pipeline->flags, (check)))
#define PIPE_FLAG_ENABLE(flag) (FLAG_ENABLE(GCSR.pipeline->flags, (flag)))
#define PIPE_FLAG_DISABLE(flag) (FLAG_DISABLE(GCSR.pipeline->flags, (flag)))

#define GET_LEVEL_UPDATE(id) (GCSR.state->level_update_table[((s32) (id - 1) >= 0) ? (id - 1) : 0])

#define MODEL_TRANSFORM_RESET(model) (model)->object.transforms.count = 0
#define MODEL_PUSH_BASE_TRANSFORM(model) model_transform_push((model), &(model)->object.position, &(model)->object.rotation, &(model)->object.scaling, (model)->object.rtype)

#define TARGET_FRAME_TIME 16.66f
#define ONE_OVER_TARGET_FRAME_TIME 0.060024009f
#define CAMERA_ANGULAR_SPEED 360

#define MATRIX_BUFFER_RESET() (GCSR.gl->matrix_buffer->count = 0)
#define MATRIX_BUFFER_COUNT() (GCSR.gl->matrix_buffer->count)
#define MATRIX_BUFFER_INCR() (GCSR.gl->matrix_buffer->count++; SDL_assert((GCSR.gl->matrix_buffer->count + 1) < GL_MAX_MATRIX_BUFFER_SIZE))
#define MATRIX_BUFFER_GET(index) (GCSR.gl->matrix_buffer->data + index)
#define MATRIX_BUFFER_NEXT() (GCSR.gl->matrix_buffer->data + GCSR.gl->matrix_buffer->count++)
#define MATRIX_BUFFER_CHECK() SDL_assert(GCSR.gl->matrix_buffer->count <= GL_MAX_MATRIX_BUFFER_SIZE)

// ----------------------------------------------------------------------------------
// -- Pipeline memory management.
// ----------------------------------------------------------------------------------

#define gl_thread_mem_reset() \
{ \
    for (u8 i = 0; i < GC_PIPE_NUM_THREADS; ++i) { \
        pipe_memory_t *__thread_memory = &GCSR.memory_manager->pipeline[i]; \
        GCSR.gl->primitives[i] = 0; \
        __thread_memory->cursor = 0; \
    } \
}

#define gl_thread_mem_push(thread_id, type, mem) \
{ \
    pipe_memory_t *__thread_memory = &GCSR.memory_manager->pipeline[thread_id]; \
    mem = (type *) ((u8 *) __thread_memory->data + __thread_memory->cursor); \
    __thread_memory->cursor += sizeof(type); \
    SDL_assert(__thread_memory->cursor <= __thread_memory->bytes); \
}

#define gl_thread_mem_vertex_array_push(thread_id) \
{ \
    OPTICK_EVENT("gl_thread_mem_vertex_array_push"); \
    pipe_memory_t *__thread_memory = &GCSR.memory_manager->pipeline[thread_id]; \
    GCSR.gl->vertices[thread_id] = (gc_pipe_array_t *) ((u8 *) __thread_memory->data + __thread_memory->cursor); \
    GCSR.gl->vertices[thread_id]->data = GCSR.gl->vertices[thread_id] + 1; \
    GCSR.gl->vertices[thread_id]->count = 0; \
    __thread_memory->cursor += sizeof(gc_pipe_array_t); \
    SDL_assert(__thread_memory->cursor <= __thread_memory->bytes); \
}

#define gl_thread_mem_primitive_array_push(thread_id) \
{ \
    OPTICK_EVENT("gl_thread_mem_primitive_array_push"); \
    pipe_memory_t *__thread_memory = &global_memory_manager->pipeline[thread_id]; \
    GCSR.gl->primitives[thread_id] = (gc_pipe_array_t *) ((u8 *) __thread_memory->data + __thread_memory->cursor); \
    GCSR.gl->primitives[thread_id]->data = GCSR.gl->primitives[thread_id] + 1; \
    GCSR.gl->primitives[thread_id]->count = 0; \
    __thread_memory->cursor += sizeof(gc_pipe_array_t); \
    SDL_assert(__thread_memory->cursor <= __thread_memory->bytes); \
}

#define gl_thread_mem_vset_push(thread_id, set) \
{ \
    pipe_memory_t *__thread_memory = &global_memory_manager->pipeline[thread_id]; \
    __thread_memory->cursor += sizeof(gc_vset_t); \
    SDL_assert(__thread_memory->cursor <= __thread_memory->bytes); \
    set = (gc_vset_t *) GCSR.gl->vertices[thread_id]->data + GCSR.gl->vertices[thread_id]->count; \
    GCSR.gl->vertices[thread_id]->count++; \
}

#define gl_thread_mem_primitive_push(thread_id, primitive) \
{ \
    pipe_memory_t *__thread_memory = &global_memory_manager->pipeline[thread_id]; \
    __thread_memory->cursor += sizeof(gc_primitive_t); \
    SDL_assert(__thread_memory->cursor <= __thread_memory->bytes); \
    primitive = (gc_primitive_t *) GCSR.gl->primitives[thread_id]->data + GCSR.gl->primitives[thread_id]->count; \
    GCSR.gl->primitives[thread_id]->count++; \
}

#define gl_thread_mem_fragment_push(thread_id, array, fragment) \
{ \
    pipe_memory_t *__thread_memory = &global_memory_manager->pipeline[thread_id]; \
    __thread_memory->cursor += sizeof(gc_processed_fragment_t); \
    SDL_assert(__thread_memory->cursor <= __thread_memory->bytes); \
    fragment = (gc_processed_fragment_t *) array->frag_list + array->frag_count; \
    array->frag_count++; \
}

#define GET_FRAGMENT_INDEX(x, y) ((y >> GL_FRAG_HEIGHT_SHIFT) * GL_TILE_FRAG_STRIDE + (x >> GL_FRAG_WIDTH_SHIFT))

#define PUSH_FRAGMENT(frag) \
{ \
    frag = (gc_processed_fragment_t *) batch_memory->fragments->data + batch_memory->fragments->count++; \
    batch_memory->memory->cursor += sizeof(gc_processed_fragment_t); \
}

#if defined(GC_PIPE_AVX)
#define gc_fragment_merge _avx_fragment_merge
#elif defined(GC_PIPE_SSE)
#define gc_fragment_merge _sse_fragment_merge
#else
#define gc_fragment_merge _fragment_merge
#endif

#ifdef GL_FRAGMENT_BATCH_PARTITIONING
    #define FRAGMENT_BATCH_LOOP(fragments_array, index) for (u32 index = fragments_array->start; index < fragments_array->end; ++index)

    #define PROCESS_FRAGMENT_BATCH(fragments_array, tile_buffer, transparency_buffer) \
    { \
        u32 batch_partitions = fragments_array->count >> GL_FRAGMENT_BATCH_THRESHOLD_SHIFT; \
    \
        if (fragments_array->count % GL_FRAGMENT_BATCH_THRESHOLD) \
            batch_partitions++; \
    \
        for (u16 t = 0; t < batch_partitions; ++t) \
        { \
            fragments_array->start = t * GL_FRAGMENT_BATCH_THRESHOLD; \
            fragments_array->end = fragments_array->start + GL_FRAGMENT_BATCH_THRESHOLD; \
    \
            if (fragments_array->end > fragments_array->count) \
                fragments_array->end = fragments_array->count; \
    \
            GCSR.gl->pinterface.varyings_setup(fragments_array, tile_buffer); \
            GCSR.gl->pipeline.shader->fs(fragments_array, &GCSR.gl->pipeline.params); \
    \
            gc_fragment_merge(fragments_array, tile_buffer, transparency_buffer); \
        } \
    }

    #define PROCESS_FRAGMENT_BATCH_LAST(fragments_array, tile_buffer, transparency_buffer) \
    { \
        fragments_array->start = 0; \
        fragments_array->end = fragments_array->count; \
    \
        GCSR.gl->pinterface.varyings_setup(fragments_array, tile_buffer); \
        GCSR.gl->pipeline.shader->fs(fragments_array, &GCSR.gl->pipeline.params); \
    \
        gc_fragment_merge(fragments_array, tile_buffer, transparency_buffer); \
    }
#else
    #define FRAGMENT_BATCH_LOOP(fragments_array, index) for (u32 index = 0; index < fragments_array->count; ++index)

    #define PROCESS_FRAGMENT_BATCH(fragments_array, tile_buffer, transparency_buffer) \
    { \
        GCSR.gl->pinterface.varyings_setup(fragments_array, tile_buffer); \
        GCSR.gl->pipeline.shader->fs(fragments_array, &GCSR.gl->pipeline.params); \
      \
        gc_fragment_merge(fragments_array, tile_buffer, transparency_buffer); \
    }

    #define PROCESS_FRAGMENT_BATCH_LAST(fragments_array, tile_buffer, transparency_buffer) \
    { \
        GCSR.gl->pinterface.varyings_setup(fragments_array, tile_buffer); \
        GCSR.gl->pipeline.shader->fs(fragments_array, &GCSR.gl->pipeline.params); \
      \
        gc_fragment_merge(fragments_array, tile_buffer, transparency_buffer); \
    }
#endif

// ----------------------------------------------------------------------------------
// -- Thread macros.
// ----------------------------------------------------------------------------------

#define THREAD_START(work_state) \
{ \
    SDL_LockMutex(GCSR.gl->threads->worker_lock); \
    GCSR.gl->threads->state = work_state; \
    GCSR.gl->threads->running_count = GC_PIPE_NUM_THREADS; \
    SDL_CondBroadcast(GCSR.gl->threads->worker_condition); \
    SDL_CondWait(GCSR.gl->threads->scheduler_condition, GCSR.gl->threads->worker_lock); \
    SDL_UnlockMutex(GCSR.gl->threads->worker_lock); \
}

#if GC_PIPE_NUM_THREADS == 1
#define THREAD_FRAMEBUFFER_CLEAR(color) \
    GCSR.gl->current_framebuffer->lsb.clear_color = color; \
    SDL_AtomicSet(&GCSR.gl->current_framebuffer->lsb.cursor, 0); \
    gc_framebuffer_clear(0);
#else
#define THREAD_FRAMEBUFFER_CLEAR(color) \
    GCSR.gl->current_framebuffer->lsb.clear_color = color; \
    SDL_AtomicSet(&GCSR.gl->current_framebuffer->lsb.cursor, 0); \
    THREAD_START(GL_RASTER_STATE_CLEAR_LSB);
#endif

#if GC_PIPE_NUM_THREADS == 1
#define THREAD_PROCESS_TRANSPARENCY() \
    SDL_AtomicSet(&GCSR.gl->current_framebuffer->transparency_cursor, 0); \
    gc_pipe_transparency_process(0);
#else
#define THREAD_PROCESS_TRANSPARENCY() \
    SDL_AtomicSet(&GCSR.gl->current_framebuffer->transparency_cursor, 0); \
    THREAD_START(GL_RASTER_STATE_TRANSPARENCY);
#endif


#if GC_PIPE_NUM_THREADS == 1
#define THREAD_LSB_TO_TEXTURE() \
    SDL_AtomicSet(&GCSR.gl->current_framebuffer->lsb.cursor, 0); \
    lsb_to_texture(0);
#else
#define THREAD_LSB_TO_TEXTURE() \
    SDL_AtomicSet(&GCSR.gl->current_framebuffer->lsb.cursor, 0); \
    THREAD_START(GL_RASTER_STATE_LSB_TO_TEXTURE);
#endif

// ----------------------------------------------------------------------------------

#define LINSTEP(low, high, v) (v - low) / (high - low)

#define PREMULT_ALPHA(color) { \
    (color).c.r *= (color).c.a; \
    (color).c.g *= (color).c.a; \
    (color).c.b *= (color).c.a; \
}

#define EXIT_ALERT(msg) \
{ \
    printf(msg); \
    exit(EXIT_FAILURE); \
}

#define EXIT_ALERT_ARGS(msg, param) \
{ \
    printf(msg, param); \
    exit(EXIT_FAILURE); \
}

// ----------------------------------------------------------------------------------
// -- Mipmap test colors.
// ----------------------------------------------------------------------------------

#define hex2float(r, g, b) {r / 255.0f, g / 255.0f, b / 255.0f}

float test_colors[][3] = {
    hex2float(0xff, 0x00, 0x00),
    hex2float(0x00, 0xff, 0x00),
    hex2float(0x00, 0x00, 0xff),
    hex2float(0x00, 0x8c, 0xcc),
    hex2float(0x63, 0x00, 0xcc),
    hex2float(0xcc, 0x00, 0xae),
    hex2float(0xcc, 0x9b, 0x00),
    hex2float(0xef, 0xef, 0x00),
    hex2float(0x5e, 0xd3, 0x00),
    hex2float(0xff, 0x38, 0xb2),
    hex2float(0xcc, 0xad, 0xff),
    hex2float(0x2b, 0x60, 0x6e),
    hex2float(0x27, 0x97, 0x5a),
    hex2float(0x70, 0x1c, 0x63),
    hex2float(0x80, 0x6b, 0x22),
};

#ifdef GL_DEBUG_MIPMAPS
#define DEBUG_MIP(sample, lod_low) \
{ \
    sample.r[0] *= test_colors[lod_low][0]; \
    sample.g[0] *= test_colors[lod_low][1]; \
    sample.b[0] *= test_colors[lod_low][2]; \
\
    sample.r[1] *= test_colors[lod_low][0]; \
    sample.g[1] *= test_colors[lod_low][1]; \
    sample.b[1] *= test_colors[lod_low][2]; \
\
    sample.r[2] *= test_colors[lod_low][0]; \
    sample.g[2] *= test_colors[lod_low][1]; \
    sample.b[2] *= test_colors[lod_low][2]; \
\
    sample.r[3] *= test_colors[lod_low][0]; \
    sample.g[3] *= test_colors[lod_low][1]; \
    sample.b[3] *= test_colors[lod_low][2]; \
}
#else
#define DEBUG_MIP(sample, lod_low)
#endif

// ----------------------------------------------------------------------------------
// -- Profiler colors.
// ----------------------------------------------------------------------------------

#define FRAGMENT_SHADER_COLOR 0xffff0000
#define GL_SAMPLE_COLOR 0xffff0000

#endif