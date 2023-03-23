// ----------------------------------------------------------------------------------
// -- File: gcsr_geometry.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-04-10 21:12:45
// -- Modified:
// ----------------------------------------------------------------------------------

#ifndef GCSR_GEOMETRY_H
#define GCSR_GEOMETRY_H

struct GlGeometryPlaneEquation
{
    r32 a;
    r32 b;
    r32 c;
};

struct GlGeometryLineEquation
{
    r32 a;
    r32 b;
    r32 c;
};

struct GlGeometryLine
{
    union
    {
        struct
        {
            vec2 p1;
            vec2 p2;
        };

        vec2 points[2];
    };

    vec2 vector;
};

struct GlGeometryTriangle
{
    union
    {
        struct
        {
            vec2 p1;
            vec2 p2;
            vec2 p3;
        };

        vec2 points[3];
    };

    vec2 vector[3];
    vec2 norm[3];
};

struct GlGeometryRectangle
{
    union
    {
        struct
        {
            vec2 p1;
            vec2 p2;
            vec2 p3;
            vec2 p4;
        };

        vec2 points[4];
    };

    vec2 vector[4];
    vec2 norm[4];
};

struct GlConvexProjectionLimits
{
    r32 min;
    r32 max;
};

// ----------------------------------------------------------------------------------
// -- Determines the intersection between two boxes.
// ----------------------------------------------------------------------------------

__INLINE__ b32 gl_boxIntersection(gc_screen_rect_t Box1, gc_screen_rect_t Box2, gc_screen_rect_t *Output)
{
    if ((Box1.min.x < Box2.min.x && Box1.max.x < Box2.min.x) ||
        (Box2.min.x < Box1.min.x && Box2.max.x < Box1.min.x) ||
        (Box1.min.y < Box2.min.y && Box1.max.y < Box2.min.y) ||
        (Box2.min.y < Box1.min.y && Box2.max.y < Box1.min.y))
        return false;

    if (Box1.min.x <= Box2.min.x)
    {
        Output->min.x = Box2.min.x;

        if (Box2.max.x <= Box1.max.x)
            Output->max.x = Box2.max.x;
        else
            Output->max.x = Box1.max.x;
    }
    else if (Box2.min.x <= Box1.min.x)
    {
        Output->min.x = Box1.min.x;

        if (Box1.max.x <= Box2.max.x)
            Output->max.x = Box1.max.x;
        else
            Output->max.x = Box2.max.x;
    }

    if (Box1.min.y <= Box2.min.y)
    {
        Output->min.y = Box2.min.y;

        if (Box2.max.y <= Box1.max.y)
            Output->max.y = Box2.max.y;
        else
            Output->max.y = Box1.max.y;
    }
    else if (Box2.min.y <= Box1.min.y)
    {
        Output->min.y = Box1.min.y;

        if (Box1.max.y <= Box2.max.y)
            Output->max.y = Box1.max.y;
        else
            Output->max.y = Box2.max.y;
    }

    return true;
}

__INLINE__ void gl_initTriangleGeometry(GlGeometryTriangle *triangle, vec2 p1, vec2 p2, vec2 p3)
{
    triangle->points[0] = p1;
    triangle->points[1] = p2;
    triangle->points[2] = p3;

    triangle->vector[0] = vec2_sub(triangle->p2, triangle->p1);
    triangle->vector[1] = vec2_sub(triangle->p3, triangle->p2);
    triangle->vector[2] = vec2_sub(triangle->p1, triangle->p3);

    triangle->norm[0] = vec2_perp(triangle->vector[0]);
    triangle->norm[1] = vec2_perp(triangle->vector[1]);
    triangle->norm[2] = vec2_perp(triangle->vector[2]);
}

__INLINE__ void gl_initRectangleGeometry(GlGeometryRectangle *Rectangle, vec2 p1, vec2 p2, vec2 p3, vec2 p4)
{
    Rectangle->points[0] = p1;
    Rectangle->points[1] = p2;
    Rectangle->points[2] = p3;
    Rectangle->points[3] = p4;

    Rectangle->vector[0] = vec2_sub(Rectangle->p2, Rectangle->p1);
    Rectangle->vector[1] = vec2_sub(Rectangle->p3, Rectangle->p2);
    Rectangle->vector[2] = vec2_sub(Rectangle->p4, Rectangle->p3);
    Rectangle->vector[3] = vec2_sub(Rectangle->p1, Rectangle->p4);

    Rectangle->norm[0] = vec2_perp(Rectangle->vector[0]);
    Rectangle->norm[1] = vec2_perp(Rectangle->vector[1]);
    Rectangle->norm[2] = vec2_perp(Rectangle->vector[2]);
    Rectangle->norm[3] = vec2_perp(Rectangle->vector[3]);
}

// ----------------------------------------------------------------------------------
// -- Separating axis theorem implementation.
// ----------------------------------------------------------------------------------

b32 gl_separatingAxisTheoremCheck(GlGeometryTriangle *triangle, GlGeometryRectangle *Tile)
{
    GlConvexProjectionLimits tileLimits;
    GlConvexProjectionLimits triLimits;

    // -- Project the shapes onto each of the triangle normals.
    // ----------------------------------------------------------------------------------

    u32 count = 0;

    for (u32 i = 0; i < 3; ++i)
    {
        vec2 projectionLine = triangle->norm[i];

        // -- Project the triangle.
        // ----------------------------------------------------------------------------------

        for (u32 k = 0; k < 3; ++k)
        {
            vec2 point = triangle->points[k];
            r32 proj = vec2_dot(point, projectionLine);

            if (k == 0)
            {
                triLimits.min = proj;
                triLimits.max = proj;
            }
            else
            {
                if (proj < triLimits.min)
                    triLimits.min = proj;

                if (proj > triLimits.max)
                    triLimits.max = proj;
            }
        }

        // -- Project the tile.
        // ----------------------------------------------------------------------------------

        for (u32 k = 0; k < 4; ++k)
        {
            vec2 point = Tile->points[k];
            r32 proj = vec2_dot(point, projectionLine);

            if (k == 0)
            {
                tileLimits.min = proj;
                tileLimits.max = proj;
            }
            else
            {
                if (proj < tileLimits.min)
                    tileLimits.min = proj;

                if (proj > tileLimits.max)
                    tileLimits.max = proj;
            }
        }

        // -- Check if both limits overlap.
        // ----------------------------------------------------------------------------------

        if ((triLimits.min >= tileLimits.min && triLimits.min <= tileLimits.max) ||
            (triLimits.max >= tileLimits.min && triLimits.max <= tileLimits.max) ||
            (tileLimits.min >= triLimits.min && tileLimits.min <= triLimits.max) ||
            (tileLimits.max >= triLimits.min && tileLimits.max <= triLimits.max))
            count++;
    }

    if (count == 3)
        return true;

    return false;
}

// ----------------------------------------------------------------------------------
// -- Establish the triangle bounding area and intersect it with the screen.
// ----------------------------------------------------------------------------------

#if 0
void gl_getTriangleBoundingArea(gc_screen_rect_t *Area, GlGeometryTriangle *triangle,
                                gc_screen_rect_t *Intersect)
{
    // *Area = *Intersect;

#if 1
    vec2i _vertices_[3] = {
        {(s32) triangle->p1.x, (s32) triangle->p1.y},
        {(s32) triangle->p2.x, (s32) triangle->p2.y},
        {(s32) triangle->p3.x, (s32) triangle->p3.y}
    };

    Area->min.x = _vertices_[0].x;
    Area->min.y = _vertices_[0].y;

    Area->max.x = _vertices_[0].x;
    Area->max.y = _vertices_[0].y;

    for (u32 i = 0; i < 3; ++i)
    {
        if (Area->min.x > _vertices_[i].x)
            Area->min.x = _vertices_[i].x;

        if (Area->min.y > _vertices_[i].y)
            Area->min.y = _vertices_[i].y;

        if (Area->max.x < _vertices_[i].x)
            Area->max.x = _vertices_[i].x;

        if (Area->max.y < _vertices_[i].y)
            Area->max.y = _vertices_[i].y;
    }

    if (Area->min.x < Intersect->min.x)
        Area->min.x = Intersect->min.x;

    if (Area->max.x < Intersect->min.x)
        Area->max.x = Intersect->min.x;

    if (Area->min.y < Intersect->min.y)
        Area->min.y = Intersect->min.y;

    if (Area->max.y < Intersect->min.y)
        Area->max.y = Intersect->min.y;

    if (Area->min.x >= Intersect->max.x)
        Area->min.x = Intersect->max.x;

    if (Area->max.x >= Intersect->max.x)
        Area->max.x = Intersect->max.x;

    if (Area->min.y >= Intersect->max.y)
        Area->min.y = Intersect->max.y;

    if (Area->max.y >= Intersect->max.y)
        Area->max.y = Intersect->max.y;

    // -- The resulting area should have the width a multiple of 4 (SIMD processing).

    Area->min.x &= ~3;
    Area->max.x = (Area->max.x & ~3) + 3;

    if (Area->min.x == Area->max.x || Area->min.y == Area->max.y)
        Area->noArea = true;

    if (!Area->noArea)
    {
        if (Area->min.x - 4 >= Intersect->min.x)
            Area->min.x -= 4;

        if (Area->min.y - 1 >= Intersect->min.y)
            Area->min.y--;

        if (Area->max.x + 4 <= Intersect->max.x)
            Area->max.x += 4;

        if (Area->max.y + 1 <= Intersect->max.y)
            Area->max.y++;
    }
#endif;
}
#endif

#endif