// ----------------------------------------------------------------------------------
// -- File: gcsr_gl_algorithms.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description: Various algorithms.
// -- Created: 2020-10-29 19:24:05
// -- Modified: 2022-01-23 22:22:15
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

__INLINE__ void gl_clip_triangle(gc_vset_t *set,
                                 u32 *set_count,
                                 gc_clipping_vertex_buffer_t *buffer,
                                 u32 interpolation_components)
{
    platform_api_t *API = get_platform_api();

    API->mem_copy(&buffer->data[0], &set->v[0], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
    API->mem_copy(&buffer->data[1], &set->v[1], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
    API->mem_copy(&buffer->data[2], &set->v[2], sizeof(gc_vertex_t), sizeof(gc_vertex_t));

    buffer->start = 0;
    buffer->count = 3;

    for (u32 i = 0; i < 2; ++i)
    {
        u32 count = buffer->count;
        u32 current = buffer->start;
        u32 dest = (current + count) % CLIPPING_VERTEX_BUFFER_COUNT;

        buffer->start = dest;
        buffer->count = 0;

        for (u32 j = 0; j < count; ++j)
        {
            u32 current_idx = (current + j) % CLIPPING_VERTEX_BUFFER_COUNT;
            u32 next_idx = (current + (j + 1) % count) % CLIPPING_VERTEX_BUFFER_COUNT;

            gc_vertex_t *current_vertex = &buffer->data[current_idx];
            gc_vertex_t *next_vertex = &buffer->data[next_idx];

            r32 BCP = 0;
            r32 BCQ = 0;

            // w + z = 0 (near)
            if (i == 0)
            {
                BCP = current_vertex->pos[3] + current_vertex->pos[2];
                BCQ = next_vertex->pos[3] + next_vertex->pos[2];
            }

            // w - z = 0 (far)
            else if (i == 1)
            {
                BCP = current_vertex->pos[3] - current_vertex->pos[2];
                BCQ = next_vertex->pos[3] - next_vertex->pos[2];
            }

            b32 outsideP = BCP < 0;
            b32 outsideQ = BCQ < 0;

            r32 t = BCP / (BCP - BCQ);

            // next point is on the clip plane.
            if (BCQ == 0 && !outsideP)
            {
                buffer->data[dest] = buffer->data[next_idx];

                dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
                buffer->count++;
            }
            else
            {
                if (t > 0 && t < 1)
                {
                    gl_interpolate_buffer(buffer, dest, current_idx, next_idx, t, interpolation_components);

                    dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
                    buffer->count++;
                }

                if (!outsideQ)
                {
                    buffer->data[dest] = buffer->data[next_idx];

                    dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
                    buffer->count++;
                }
            }
        }
    }

    // -- Add the new vertices into the current vertex buffer.

    if (buffer->count > 0)
    {
        for (u32 j = 0; (j + 2) < buffer->count; ++j)
        {
            gc_vset_t *next_set = set + (*set_count)++;
            SDL_assert(*set_count <= 21);

            u32 tidx1 = buffer->start + 0;
            u32 tidx2 = (buffer->start + j + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
            u32 tidx3 = (buffer->start + j + 2) % CLIPPING_VERTEX_BUFFER_COUNT;

            API->mem_copy(&next_set->v[0], &buffer->data[tidx1], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
            API->mem_copy(&next_set->v[1], &buffer->data[tidx2], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
            API->mem_copy(&next_set->v[2], &buffer->data[tidx3], sizeof(gc_vertex_t), sizeof(gc_vertex_t));

            u8 v1_clip_codes = gl_compute_clip_codes(&next_set->v[0]);
            u8 v2_clip_codes = gl_compute_clip_codes(&next_set->v[1]);
            u8 v3_clip_codes = gl_compute_clip_codes(&next_set->v[2]);

            next_set->area = 0;
            next_set->flags = 0;

            if (v1_clip_codes & v2_clip_codes & v3_clip_codes)
                next_set->flags |= GL_SET_DISCARDED;
        }
    }
}

__INLINE__ void gl_clip_line(gc_vset_t *set, u32 *set_count, gc_clipping_vertex_buffer_t *buffer, u32 interpolation_components)
{
    platform_api_t *API = get_platform_api();

    API->mem_copy(&buffer->data[0], &set->v[0], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
    API->mem_copy(&buffer->data[1], &set->v[1], sizeof(gc_vertex_t), sizeof(gc_vertex_t));

    buffer->start = 0;
    buffer->count = 2;

    for (u32 i = 0; i < 2; ++i)
    {
        u32 current = buffer->start;
        u32 dest = (current + buffer->count) % CLIPPING_VERTEX_BUFFER_COUNT;
        u32 nextStart = dest;

        // buffer->start = dest;
        buffer->count = 0;

        u32 current_idx = current;
        u32 next_idx = (current + 1) % CLIPPING_VERTEX_BUFFER_COUNT;

        gc_vertex_t *current_vertex = &buffer->data[current_idx];
        gc_vertex_t *next_vertex = &buffer->data[next_idx];

        r32 BCP = 0;
        r32 BCQ = 0;

        // w + z = 0 (near)
        if (i == 0)
        {
            BCP = current_vertex->pos[3] + current_vertex->pos[2];
            BCQ = next_vertex->pos[3] + next_vertex->pos[2];
        }

        // w - z = 0 (far)
        else if (i == 1)
        {
            BCP = current_vertex->pos[3] - current_vertex->pos[2];
            BCQ = next_vertex->pos[3] - next_vertex->pos[2];
        }

        b32 outsideP = BCP < 0;
        b32 outsideQ = BCQ < 0;

        if ((outsideP && outsideQ) ||
            (outsideP && BCQ == 0) ||
            (outsideQ && BCP == 0))
        {
            // no vertices inside.
            buffer->count = 0;
            buffer->start = current;

            break;
        }

        r32 t = BCP / (BCP - BCQ);

        // -- There is an intersection.

        if (t > 0 && t < 1)
        {
            if (!outsideP)
            {
                buffer->data[dest] = buffer->data[current_idx];
                dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
                buffer->count++;
            }

            gl_interpolate_buffer(buffer, dest, current_idx, next_idx, t, interpolation_components);
            dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
            buffer->count++;

            if (!outsideQ)
            {
                buffer->data[dest] = buffer->data[next_idx];
                dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
                buffer->count++;
            }
        }
        else
        {
            nextStart = buffer->start;
            buffer->count = 2;
        }

        buffer->start = nextStart;
    }

    // -- Add the new vertices into the current vertex buffer.

    if (buffer->count > 0)
    {
        gc_vset_t *next_set = set + (*set_count)++;
        SDL_assert(*set_count <= 21);

        u32 tidx1 = buffer->start + 0;
        u32 tidx2 = (buffer->start + 1) % CLIPPING_VERTEX_BUFFER_COUNT;

        API->mem_copy(&next_set->v[0], &buffer->data[tidx1], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
        API->mem_copy(&next_set->v[1], &buffer->data[tidx2], sizeof(gc_vertex_t), sizeof(gc_vertex_t));

        u8 v1_clip_codes = gl_compute_clip_codes(&next_set->v[0]);
        u8 v2_clip_codes = gl_compute_clip_codes(&next_set->v[1]);

        next_set->area = 0;
        next_set->flags = 0;

        if (v1_clip_codes & v2_clip_codes)
            next_set->flags |= GL_SET_DISCARDED;
    }
}

// __INLINE__ void gl_gb_clip_line(gc_vset_t *set, gc_vset_t *current_set, u32 *set_count, gc_clipping_vertex_buffer_t *buffer, u32 interpolation_components)
// {
//     platform_api_t *API = get_platform_api();

//     vec2 v1gb = {
//         current_set->v[0].pos[0] - GCSR.gl->current_framebuffer->res_offx,
//         current_set->v[0].pos[1] - GCSR.gl->current_framebuffer->res_offy
//     };

//     vec2 v2gb = {
//         current_set->v[1].pos[0] - GCSR.gl->current_framebuffer->res_offx,
//         current_set->v[1].pos[1] - GCSR.gl->current_framebuffer->res_offy
//     };

//     b8 gb_check = v1gb.x < FP_GUARD_BAND_MIN || v1gb.y < FP_GUARD_BAND_MIN ||
//                   v2gb.x < FP_GUARD_BAND_MIN || v2gb.y < FP_GUARD_BAND_MIN ||
//                   v1gb.x > FP_GUARD_BAND_MAX || v1gb.y > FP_GUARD_BAND_MAX ||
//                   v2gb.x > FP_GUARD_BAND_MAX || v2gb.y > FP_GUARD_BAND_MAX;

//     if (current_set->flags & GL_SET_DISCARDED || !gb_check)
//         return;

//     current_set->flags |= GL_SET_DISCARDED | GL_SET_FROM_GB_CLIP;

//     API->mem_copy(&buffer->data[0], &current_set->v[0], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
//     API->mem_copy(&buffer->data[1], &current_set->v[1], sizeof(gc_vertex_t), sizeof(gc_vertex_t));

//     buffer->data[0].pos[0] = v1gb.x;
//     buffer->data[0].pos[1] = v1gb.y;

//     buffer->data[1].pos[0] = v2gb.x;
//     buffer->data[1].pos[1] = v2gb.y;

//     buffer->start = 0;
//     buffer->count = 2;

//     // Top, bottom, left, right.
//     vec3 guard_box[4] =
//     {
//         {0, FP_INT_VAL, -FP_CONST_MIN},
//         {0, -FP_INT_VAL, FP_CONST_MAX},
//         {FP_INT_VAL, 0, -FP_CONST_MIN},
//         {-FP_INT_VAL, 0, FP_CONST_MAX}
//     };

//     for (u32 i = 0; i < 4; ++i)
//     {
//         u32 current = buffer->start;
//         u32 dest = (current + buffer->count) % CLIPPING_VERTEX_BUFFER_COUNT;
//         u32 nextStart = dest;

//         buffer->start = dest;
//         buffer->count = 0;

//         u32 current_idx = current;
//         u32 next_idx = (current + 1) % CLIPPING_VERTEX_BUFFER_COUNT;

//         gc_vertex_t *current_vertex = &buffer->data[current_idx];
//         gc_vertex_t *next_vertex = &buffer->data[next_idx];

//         r32 BCP = 0;
//         r32 BCQ = 0;

//         vec3 *Edge = &guard_box[i];

//         BCP = Edge->x * current_vertex->pos[0] + Edge->y * current_vertex->pos[1] + Edge->z;
//         BCQ = Edge->x * next_vertex->pos[0] + Edge->y * next_vertex->pos[1] + Edge->z;

//         b32 outsideP = BCP < 0;
//         b32 outsideQ = BCQ < 0;

//         // -- No vertices inside.

//         if ((outsideP && outsideQ) ||
//             (outsideP && BCQ == 0) ||
//             (outsideQ && BCP == 0))
//         {
//             buffer->count = 0;
//             buffer->start = current;

//             break;
//         }

//         r32 t = BCP / (BCP - BCQ);

//         // -- There is an intersection.

//         if (t > 0 && t < 1)
//         {
//             if (!outsideP)
//             {
//                 API->mem_copy(&buffer->data[dest], &buffer->data[current_idx], gcSize(gc_vertex_t), gcSize(gc_vertex_t));
//                 dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
//                 buffer->count++;
//             }

//             gl_interpolate_buffer(buffer, dest, current_idx, next_idx, t, interpolation_components);
//             dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
//             buffer->count++;

//             if (!outsideQ)
//             {
//                 API->mem_copy(&buffer->data[dest], &buffer->data[next_idx], gcSize(gc_vertex_t), gcSize(gc_vertex_t));
//                 dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
//                 buffer->count++;
//             }
//         }
//         else
//         {
//             nextStart = buffer->start;
//             buffer->count = 2;
//         }

//         buffer->start = nextStart;
//     }

//     if (buffer->count > 0)
//     {
//         // -- Add the new vertices into the current vertex buffer.

//         gc_vset_t *next_set = set + (*set_count)++;
//         SDL_assert(*set_count <= 21);

//         u32 tidx1 = buffer->start + 0;
//         u32 tidx2 = (buffer->start + 1) % CLIPPING_VERTEX_BUFFER_COUNT;

//         API->mem_copy(&next_set->v[0], &buffer->data[tidx1], gcSize(gc_vertex_t), gcSize(gc_vertex_t));
//         API->mem_copy(&next_set->v[1], &buffer->data[tidx2], gcSize(gc_vertex_t), gcSize(gc_vertex_t));

//         next_set->area = 0;
//         next_set->flags = GL_SET_FROM_GB_CLIP;

//         // The offset is already applied to the coordinates .

//         vec2i fp_v1_pos = {
//             FP_FROM_REAL(next_set->v[0].pos[0]),
//             FP_FROM_REAL(next_set->v[0].pos[1]),
//         };

//         vec2i fp_v2_pos = {
//             FP_FROM_REAL(next_set->v[1].pos[0]),
//             FP_FROM_REAL(next_set->v[1].pos[1]),
//         };

//         next_set->v1_posfp[0] = fp_v1_pos.x;
//         next_set->v1_posfp[1] = fp_v1_pos.y;

//         next_set->v2_posfp[0] = fp_v2_pos.x;
//         next_set->v2_posfp[1] = fp_v2_pos.y;

//         next_set->v[0].pos[0] += GCSR.gl->current_framebuffer->res_offx;
//         next_set->v[0].pos[1] += GCSR.gl->current_framebuffer->res_offy;
//         next_set->v[1].pos[0] += GCSR.gl->current_framebuffer->res_offx;
//         next_set->v[1].pos[1] += GCSR.gl->current_framebuffer->res_offy;
//     }
// }

// __INLINE__ void gl_gb_clip(gc_vset_t *set, gc_vset_t *current_set, u32 *set_count, gc_clipping_vertex_buffer_t *buffer, u32 interpolation_components)
// {
//     platform_api_t *API = get_platform_api();

//     vec2 v1gb = {
//         current_set->v[0].pos[0] - GCSR.gl->current_framebuffer->res_offx,
//         current_set->v[0].pos[1] - GCSR.gl->current_framebuffer->res_offy
//     };

//     vec2 v2gb = {
//         current_set->v[1].pos[0] - GCSR.gl->current_framebuffer->res_offx,
//         current_set->v[1].pos[1] - GCSR.gl->current_framebuffer->res_offy
//     };

//     vec2 v3gb = {
//         current_set->v[2].pos[0] - GCSR.gl->current_framebuffer->res_offx,
//         current_set->v[2].pos[1] - GCSR.gl->current_framebuffer->res_offy
//     };

//     b8 gb_check = v1gb.x < FP_GUARD_BAND_MIN || v1gb.y < FP_GUARD_BAND_MIN ||
//                   v2gb.x < FP_GUARD_BAND_MIN || v2gb.y < FP_GUARD_BAND_MIN ||
//                   v3gb.x < FP_GUARD_BAND_MIN || v3gb.y < FP_GUARD_BAND_MIN ||
//                   v1gb.x > FP_GUARD_BAND_MAX || v1gb.y > FP_GUARD_BAND_MAX ||
//                   v2gb.x > FP_GUARD_BAND_MAX || v2gb.y > FP_GUARD_BAND_MAX ||
//                   v3gb.x > FP_GUARD_BAND_MAX || v3gb.y > FP_GUARD_BAND_MAX;

//     if (current_set->flags & GL_SET_DISCARDED || !gb_check)
//         return;

//     current_set->flags |= GL_SET_DISCARDED | GL_SET_FROM_GB_CLIP;

//     API->mem_copy(&buffer->data[0], &current_set->v[0], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
//     API->mem_copy(&buffer->data[1], &current_set->v[1], sizeof(gc_vertex_t), sizeof(gc_vertex_t));
//     API->mem_copy(&buffer->data[2], &current_set->v[2], sizeof(gc_vertex_t), sizeof(gc_vertex_t));

//     buffer->data[0].pos[0] = v1gb.x;
//     buffer->data[0].pos[1] = v1gb.y;

//     buffer->data[1].pos[0] = v2gb.x;
//     buffer->data[1].pos[1] = v2gb.y;

//     buffer->data[2].pos[0] = v3gb.x;
//     buffer->data[2].pos[1] = v3gb.y;

//     buffer->start = 0;
//     buffer->count = 3;

//     // Top, bottom, left, right.
//     vec3 guard_box[4] =
//     {
//         {0, FP_INT_VAL, -FP_CONST_MIN},
//         {0, -FP_INT_VAL, FP_CONST_MAX},
//         {FP_INT_VAL, 0, -FP_CONST_MIN},
//         {-FP_INT_VAL, 0, FP_CONST_MAX}
//     };

//     for (u32 i = 0; i < 4; ++i)
//     {
//         u32 count = buffer->count;
//         u32 current = buffer->start;
//         u32 dest = (current + count) % CLIPPING_VERTEX_BUFFER_COUNT;

//         buffer->start = dest;
//         buffer->count = 0;

//         for (u32 j = 0; j < count; ++j)
//         {
//             u32 current_idx = (current + j) % CLIPPING_VERTEX_BUFFER_COUNT;
//             u32 next_idx = (current + (j + 1) % count) % CLIPPING_VERTEX_BUFFER_COUNT;

//             gc_vertex_t *current_vertex = &buffer->data[current_idx];
//             gc_vertex_t *next_vertex = &buffer->data[next_idx];

//             r32 BCP = 0;
//             r32 BCQ = 0;

//             vec3 *Edge = &guard_box[i];

//             BCP = Edge->x * current_vertex->pos[0] + Edge->y * current_vertex->pos[1] + Edge->z;
//             BCQ = Edge->x * next_vertex->pos[0] + Edge->y * next_vertex->pos[1] + Edge->z;

//             b32 outsideP = BCP < 0;
//             b32 outsideQ = BCQ < 0;

//             r32 t = BCP / (BCP - BCQ);

//             // next point is on the edge.
//             if (BCQ == 0 && !outsideP)
//             {
//                 API->mem_copy(&buffer->data[dest], &buffer->data[next_idx], gcSize(gc_vertex_t), gcSize(gc_vertex_t));

//                 dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
//                 buffer->count++;
//             }
//             else
//             {
//                 if (t > 0 && t < 1)
//                 {
//                     gl_interpolate_buffer(buffer, dest, current_idx, next_idx, t, interpolation_components);

//                     dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
//                     buffer->count++;
//                 }

//                 if (!outsideQ)
//                 {
//                     API->mem_copy(&buffer->data[dest], &buffer->data[next_idx], gcSize(gc_vertex_t), gcSize(gc_vertex_t));

//                     dest = (dest + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
//                     buffer->count++;
//                 }
//             }
//         }
//     }

//     if (buffer->count > 0)
//     {
//         // u32 old_tag = set->tag;
//         // s64 old_area = set->area;
//         b8 old_is_backface = current_set->flags & GL_SET_BACKFACE;

//         // -- Add the new vertices into the current vertex buffer.

//         for (u32 j = 0; (j + 2) < buffer->count; ++j)
//         {
//             gc_vset_t *next_set = set + (*set_count)++;
//             SDL_assert(*set_count <= 21);

//             u32 tidx1 = buffer->start + 0;
//             u32 tidx2 = (buffer->start + j + 1) % CLIPPING_VERTEX_BUFFER_COUNT;
//             u32 tidx3 = (buffer->start + j + 2) % CLIPPING_VERTEX_BUFFER_COUNT;

//             API->mem_copy(&next_set->v[0], &buffer->data[tidx1], gcSize(gc_vertex_t), gcSize(gc_vertex_t));
//             API->mem_copy(&next_set->v[1], &buffer->data[tidx2], gcSize(gc_vertex_t), gcSize(gc_vertex_t));
//             API->mem_copy(&next_set->v[2], &buffer->data[tidx3], gcSize(gc_vertex_t), gcSize(gc_vertex_t));

//             next_set->area = 0;
//             next_set->flags = GL_SET_FROM_GB_CLIP | old_is_backface;
//             // set->tag = old_tag;

//             // The offset is already applied to the coordinates .

//             vec2i fp_v1_pos = {
//                 FP_FROM_REAL(next_set->v[0].pos[0]),
//                 FP_FROM_REAL(next_set->v[0].pos[1]),
//             };

//             vec2i fp_v2_pos = {
//                 FP_FROM_REAL(next_set->v[1].pos[0]),
//                 FP_FROM_REAL(next_set->v[1].pos[1]),
//             };

//             vec2i fp_v3_pos = {
//                 FP_FROM_REAL(next_set->v[2].pos[0]),
//                 FP_FROM_REAL(next_set->v[2].pos[1]),
//             };

//             next_set->v1_posfp[0] = fp_v1_pos.x;
//             next_set->v1_posfp[1] = fp_v1_pos.y;

//             next_set->v2_posfp[0] = fp_v2_pos.x;
//             next_set->v2_posfp[1] = fp_v2_pos.y;

//             next_set->v3_posfp[0] = fp_v3_pos.x;
//             next_set->v3_posfp[1] = fp_v3_pos.y;

//             s64 fp_area = (s64) (fp_v2_pos.x - fp_v1_pos.x) * (fp_v3_pos.y - fp_v1_pos.y) -
//                           (s64) (fp_v2_pos.y - fp_v1_pos.y) * (fp_v3_pos.x - fp_v1_pos.x);

//             next_set->area = fp_area;

//             // NOTE(gabic): Sometimes the clipped triangle is so thin that
//             // the resulting points are so close that the snapped triangle
//             // has a zero area.

//             if (next_set->area == 0)
//             {
//                 next_set->flags |= GL_SET_DISCARDED;
//                 continue;
//             }

//             next_set->v[0].pos[0] += GCSR.gl->current_framebuffer->res_offx;
//             next_set->v[0].pos[1] += GCSR.gl->current_framebuffer->res_offy;
//             next_set->v[1].pos[0] += GCSR.gl->current_framebuffer->res_offx;
//             next_set->v[1].pos[1] += GCSR.gl->current_framebuffer->res_offy;
//             next_set->v[2].pos[0] += GCSR.gl->current_framebuffer->res_offx;
//             next_set->v[2].pos[1] += GCSR.gl->current_framebuffer->res_offy;

//             SDL_assert(fp_area != 0);
//         }
//     }
// }