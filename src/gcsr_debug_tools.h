// ----------------------------------------------------------------------------------
// -- File: gcsr_debug_tools.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C
// -- Description:
// -- Created: 2021-09-15 22:37:45
// -- Modified: 2023-03-20 21:05:59
// ----------------------------------------------------------------------------------

#ifndef GCSR_DEBUG_TOOLS_H
#define GCSR_DEBUG_TOOLS_H

#ifdef GC_DEBUG_MODE
    #define DEBUG_SELECTED_SCENE SCENE_3D_PERSPECTIVE

    #define DEBUG_COLOR_COUNT 10
    #define DEBUG_MAX_INDICES 50

    #define DEBUG_MAX_MEDIAN_SLOTS 10

    #define DEBUG_MAX_COUNTERS 10
    #define DEBUG_MAX_MONITORS 20
    #define DEBUG_ROUTINE_MONITOR_SLOTS 50
    #define DEBUG_ROUTINE_CYCLES_FRAMES 60 * 1

    #define DEBUG_PRIMITIVE 1
    // #define DEBUG_MESH 8
    #define DEBUG_ENABLE_CAMERA_DEBUG 1
    // #define DEBUG_DISABLE_CAMERA_DAMPENING 1
    // #define DEBUG_ENABLE_BIN_DEBUG 1
    // #define DEBUG_ENABLE_FRAG_DEBUG 1

    // #define DEBUG_ENABLE_TRIANGLE_DEBUG 1
    // #define DEBUG_ENABLE_LINE_DEBUG 1

    // #define DEBUG_ENABLE_PERFCOUNTERS 1
    // #define DEBUG_ENABLE_CYCLES 1

    #define DEBUG_BIN_ROW 6
    #define DEBUG_BIN_COL 12

    // NOTE(gabic): Use this for 1 thread.
    #define DEBUG_PRIM_ID 27

    #ifdef DEBUG_ENABLE_TRIANGLE_DEBUG
    #define DEBUG_SHOW_ONLY_TRIANGLE() if (primitive_id != DEBUG_PRIM_ID) continue
    #else
    #define DEBUG_SHOW_ONLY_TRIANGLE()
    #endif

    #ifdef DEBUG_ENABLE_LINE_DEBUG
    #define DEBUG_SHOW_ONLY_LINE() if (primitive_id != DEBUG_PRIM_ID) continue
    #else
    #define DEBUG_SHOW_ONLY_LINE()
    #endif

    #define DEBUG_CAMERA_POSITION_X -8.581520f
    #define DEBUG_CAMERA_POSITION_Y -4.691881f
    #define DEBUG_CAMERA_POSITION_Z 4.403873f

    #define DEBUG_CAMERA_FOCUS_X 4.091715f
    #define DEBUG_CAMERA_FOCUS_Y 0.472213f
    #define DEBUG_CAMERA_FOCUS_Z -1.737888f

    #ifdef DEBUG_ENABLE_CAMERA_DEBUG
    #define DEBUG_SET_CAMERA_EYE(camera) \
    camera->eye.v3.x = DEBUG_CAMERA_POSITION_X; \
    camera->eye.v3.y = DEBUG_CAMERA_POSITION_Y; \
    camera->eye.v3.z = DEBUG_CAMERA_POSITION_Z

    #define DEBUG_SET_CAMERA_TARGET(camera) \
    camera->target.v3.x = DEBUG_CAMERA_FOCUS_X; \
    camera->target.v3.y = DEBUG_CAMERA_FOCUS_Y; \
    camera->target.v3.z = DEBUG_CAMERA_FOCUS_Z
    #else
    #define DEBUG_SET_CAMERA_EYE(camera)
    #define DEBUG_SET_CAMERA_TARGET(camera)
    #endif

    #define DEBUG_VEC(v, name) printf("%s: {%f, %f, %f, %f}\n", name, v.v4.x, v.v4.y, v.v4.z, v.v4.w)
    #define DEBUG_VECP(v, name) printf("%s: {%f, %f, %f, %f}\n", name, p->v4.x, p->v4.y, p->v4.z, p->v4.w)

    #define DEBUG_MATRIX(mat, name) \
    printf("%s\n", name); \
    printf("{%f, %f, %f, %f}\n", mat.data[0][0], mat.data[0][1], mat.data[0][2], mat.data[0][3]); \
    printf("{%f, %f, %f, %f}\n", mat.data[1][0], mat.data[1][1], mat.data[1][2], mat.data[1][3]); \
    printf("{%f, %f, %f, %f}\n", mat.data[2][0], mat.data[2][1], mat.data[2][2], mat.data[2][3]); \
    printf("{%f, %f, %f, %f}\n", mat.data[3][0], mat.data[3][1], mat.data[3][2], mat.data[3][3])

    #define DEBUG_MATRIXP(mat, name) \
    printf("%s\n", name); \
    printf("{%f, %f, %f, %f}\n", mat->data[0][0], mat->data[0][1], mat->data[0][2], mat->data[0][3]); \
    printf("{%f, %f, %f, %f}\n", mat->data[1][0], mat->data[1][1], mat->data[1][2], mat->data[1][3]); \
    printf("{%f, %f, %f, %f}\n", mat->data[2][0], mat->data[2][1], mat->data[2][2], mat->data[2][3]); \
    printf("{%f, %f, %f, %f}\n", mat->data[3][0], mat->data[3][1], mat->data[3][2], mat->data[3][3])

    #define DEBUG_ADDRESS(name, addr) (printf("%s: (0x%016llx)\n", name, (u64) addr))
    #define DEBUG_VEC2(name, v) (printf("%s: {%f, %f}\n", name, v.v2.x, v.v2.y))
    #define DEBUG_VEC3(name, v) (printf("%s: {%f, %f, %f}\n", name, (v).v3.x, (v).v3.y, (v).v3.z))
    #define DEBUG_VECT(name, v) (printf("%s: {%f, %f, %f, %f}\n", name, v.data[0], v.data[1], v.data[2], v.data[3]))
    #define DEBUG_CAMERA() \
    { \
        DEBUG_VEC3("[DEBUG] camera position", global_debug_camera->state.position); \
        DEBUG_VEC3("[DEBUG] camera focus", global_debug_camera->state.focus); \
    }

    // ----------------------------------------------------------------------------------
    // -- Macros.
    // ----------------------------------------------------------------------------------

    static u64 _debug_start = 0;

    #ifdef DEBUG_ENABLE_CYCLES
        __INLINE__ void debug_start_cycles() {
            _debug_start = __rdtsc();
        }
        __INLINE__ void debug_end_cycles(char *name)
        {
            u64 _debug_end = __rdtsc();
            printf("%s: %llu cycles\n", name, _debug_end - _debug_start);
        }
        #define DEBUG_StartCycles() \
            u64 StartCycleCount = __rdtsc();
        #define DEBUG_EndCycles(name) \
            u64 EndCycleCount = __rdtsc(); \
            printf("%s: %llu cycles\n", name, EndCycleCount - StartCycleCount);
    #else
        #define DEBUG_StartCycles()
        #define DEBUG_EndCycles(name)
    #endif

    #ifdef DEBUG_ENABLE_BIN_DEBUG
        #define DEBUG_SHOW_ONLY_BIN(row, col) if (row != DEBUG_BIN_ROW || col != DEBUG_BIN_COL) continue;
    #else
        #define DEBUG_SHOW_ONLY_BIN(row, col)
    #endif

    #if DEBUG_ENABLE_TILE_DEBUG == 1
        #define DEBUG_SHOW_ONLY_TILE(row, col, tile, Tiles) \
            u16 __dtile_index = (u16) (row * Tiles->cols + col); \
            if (__dtile_index != tile) \
                continue;
    #else
        #define DEBUG_SHOW_ONLY_TILE(row, col, tile, Tiles)
    #endif

    #ifdef DEBUG_ENABLE_TRIANGLE_DEBUG
        #define DEBUG_SHOW_ONLY_TRIANGLE_LINE(_id) \
        { \
            u32 __debug_count = sizeof(__DebugApi__->Tools.triangle_indices) / sizeof(u32); \
            b8 __debug_check = false; \
            for (u32 iii = 0; iii < __DebugApi__->Tools.triangle_indices_count; ++iii) \
            { \
                if (__DebugApi__->Tools.triangle_indices[iii] == _id) \
                { \
                    __debug_check = true; \
                    break; \
                } \
            } \
            if (!__debug_check) \
                continue; \
        }
    #else
        #define DEBUG_SHOW_ONLY_TRIANGLE_LINE(_id)
    #endif

#else
    #define DEBUG_CAMERA_YAW 0
    #define DEBUG_CAMERA_PITCH 0

    #define DEBUG_VEC(v, name)
    #define DEBUG_VECP(v, name)
    #define DEBUG_MATRIX(mat, name)
    #define DEBUG_MATRIXP(mat, name)

    #define DEBUG_SET_CAMERA_EYE(camera)
    #define DEBUG_SET_CAMERA_TARGET(camera)
    #define DEBUG_SHOW_ONLY_TILE(row, col, tile, Tiles)
    #define DEBUG_SHOW_ONLY_TRIANGLE()
    #define DEBUG_SHOW_ONLY_TRIANGLE_LINE(_id)
    #define DEBUG_SHOW_ONLY_BIN(row, col)
    #define DEBUG_SHOW_ONLY_LINE()

    #define DEBUG_StartCycles()
    #define DEBUG_EndCycles(name)
#endif

#endif