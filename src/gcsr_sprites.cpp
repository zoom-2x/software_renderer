// ----------------------------------------------------------------------------------
// -- File: gcsr_sprites.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-07-28 15:42:29
// -- Modified: 2021-12-28 22:16:06
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

void gl_init_point_sprites()
{
#if 0
    gl_state_t *GL = global_gl;
    gc_point_sprite_t *PointCircle = GET_SPRITE(GL_SPRITE_POINT_CIRCLE);

    PointCircle->offx = 3;
    PointCircle->offy = 3;

    PointCircle->mask[0] = 0;
    PointCircle->mask[1] = 56;
    PointCircle->mask[2] = 68;
    PointCircle->mask[3] = 84;
    PointCircle->mask[4] = 68;
    PointCircle->mask[5] = 56;
    PointCircle->mask[6] = 0;
    PointCircle->mask[7] = 0;

    // ----------------------------------------------------------------------------------

    gc_point_sprite_t *PointSquare = GET_SPRITE(GL_SPRITE_POINT_SQUARE);

    PointSquare->offx = 3;
    PointSquare->offy = 3;

    PointSquare->mask[0] = 0;
    PointSquare->mask[1] = 124;
    PointSquare->mask[2] = 68;
    PointSquare->mask[3] = 84;
    PointSquare->mask[4] = 68;
    PointSquare->mask[5] = 124;
    PointSquare->mask[6] = 0;
    PointSquare->mask[7] = 0;

    // ----------------------------------------------------------------------------------

    gc_point_sprite_t *PointCross = GET_SPRITE(GL_SPRITE_POINT_CROSS);

    PointCross->offx = 3;
    PointCross->offy = 3;

    PointCross->mask[0] = 0;
    PointCross->mask[1] = 0;
    PointCross->mask[2] = 16;
    PointCross->mask[3] = 56;
    PointCross->mask[4] = 16;
    PointCross->mask[5] = 0;
    PointCross->mask[6] = 0;
    PointCross->mask[7] = 0;

    // ----------------------------------------------------------------------------------

    gc_point_sprite_t *PointTriangle = GET_SPRITE(GL_SPRITE_POINT_TRIANGLE);

    PointTriangle->offx = 3;
    PointTriangle->offy = 4;

    PointTriangle->mask[0] = 0;
    PointTriangle->mask[1] = 16;
    PointTriangle->mask[2] = 40;
    PointTriangle->mask[3] = 40;
    PointTriangle->mask[4] = 68;
    PointTriangle->mask[5] = 84;
    PointTriangle->mask[6] = 130;
    PointTriangle->mask[7] = 254;

#endif
}