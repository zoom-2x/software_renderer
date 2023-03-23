// ----------------------------------------------------------------------------------
// -- File: gcsr_animation.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-07 16:01:05
// -- Modified: 2022-12-07 16:01:05
// ----------------------------------------------------------------------------------

#ifndef GCSR_BEZIER_H
#define GCSR_BEZIER_H

// ----------------------------------------------------------------------------------
// -- Bezier curve.
// ----------------------------------------------------------------------------------

typedef struct
{
    r32 distance;
    r32 t;
} bezier_lut_sample_t;

typedef struct
{
    gc_vec_t p[3];
} bezier_point_t;

typedef struct
{
    // p0 = start point
    // p1 = first control point
    // p2 = second control point
    // p3 = end point

    gc_vec_t *p0;
    gc_vec_t *p1;
    gc_vec_t *p2;
    gc_vec_t *p3;
} bezier_section_t;

typedef enum
{
    BEZIER_DEBUG = 1,
    BEZIER_DEBUG_LUT = 2,
    BEZIER_DEBUG_TANGENTS = 4,
    BEZIER_DEBUG_POINTS = 8,
    BEZIER_CLOSED_PATH = 16,
    BEZIER_SYNCHRONIZE = 32,
    BEZIER_REVERSE = 64
} bezier_flag_t;

typedef struct
{
    u8 flags;

    u32 samples;
    u32 lut_samples;
    r32 global_scale;

    u32 dbgp_count;
    u32 dbgp_cycle_ms;
    r32 dbgp_step;
    r32 dbgp_current_ms;

    transform_t transform;

    u32 point_count;
    u32 section_count;

    bezier_point_t *points;
    bezier_section_t *sections;
    bezier_lut_sample_t *lut;

    mesh_t *points_mesh;
    mesh_t *tangents_mesh;
    mesh_t *base_mesh;

    gc_model_t *points_model;
    gc_model_t *tangents_model;
    gc_model_t *base_model;
    gc_model_t *debug_points_model;
} bezier_curve_t;

#endif