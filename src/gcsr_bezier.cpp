// ----------------------------------------------------------------------------------
// -- File: gcsr_animation.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-07 16:08:35
// -- Modified: 2022-12-07 16:08:35
// ----------------------------------------------------------------------------------

// global.
__INLINE__ r32 bezier_tg(u32 section_count, r32 tc)
{
    r32 t = clamp(0, 1, (tc / section_count));
    return t;
}

// curve.
__INLINE__ r32 bezier_tc(u32 section_count, r32 tg)
{
    r32 t = clamp(0, section_count, (tg * section_count));
    return t;
}

// local.
__INLINE__ r32 bezier_tl(u32 section_count, r32 tc)
{
    r32 t = tc - floorf(tc);

    t = t < 0 ? 0 : t;
    t = (tc >= section_count) ? 1 : t;

    return t;
}

__INLINE__ u32 bezier_section(bezier_curve_t *curve, r32 tc)
{
    u32 section_index = (u32) tc;
    section_index = section_index >= curve->section_count ? curve->section_count - 1 : section_index;

    return section_index;
}

__INLINE__ r32 bezier_length(bezier_curve_t *curve)
{
    bezier_lut_sample_t *lut = curve->lut + (curve->lut_samples - 1);
    r32 arclen = lut->distance;

    return arclen;
}

__INLINE__ r32 bezier_loop_clamp(r32 t)
{
    if (t < 0)
        t = 1.0f + t;
    else if (t > 1)
        t -= floorf(t);

    return t;
}

r32 bezier_sync_to_lut_from_tg(bezier_curve_t *curve, r32 tg)
{
    if (!curve || !curve->lut_samples)
        return 0;

    tg = clamp(0, 1, tg);
    r32 t = tg;

    r32 arclen = bezier_length(curve);
    r32 search_distance = tg * arclen;

    u32 li = 0;
    u32 ri = curve->lut_samples - 1;

    while (true)
    {
        u32 i = li + ((ri - li) >> 1);

        if (li == ri - 1)
            break;

        bezier_lut_sample_t *search_lut = curve->lut + i;

        if (search_lut->distance == search_distance)
        {
            li = ri - 1;
            break;
        }
        else
        {
            if (search_distance > search_lut->distance)
                li = i;
            else
                ri = i;
        }
    }

    bezier_lut_sample_t *left_lut = curve->lut + li;
    bezier_lut_sample_t *right_lut = curve->lut + ri;

    r32 t_local = (search_distance - left_lut->distance) / (right_lut->distance - left_lut->distance);
    t = (1 - t_local) * left_lut->t + t_local * right_lut->t;

    return t;
}

gc_vec_t bezier_compute_point(bezier_curve_t *curve, r32 tg)
{
    VINIT4(res, 0, 0, 0, 0);

    if (curve->flags & BEZIER_REVERSE)
        tg = 1.0f - tg;

    r32 tc = bezier_tc(curve->section_count, tg);
    u32 section_index = bezier_section(curve, tc);
    r32 t = bezier_tl(curve->section_count, tc);

    r32 a = 1 - t;
    r32 t1 = a * a * a;
    r32 t2 = 3 * t * a * a;
    r32 t3 = 3 * t * t * a;
    r32 t4 = t * t * t;

    bezier_section_t *section = curve->sections + section_index;

    res.v3.x = t1 * section->p0->v3.x +
               t2 * section->p1->v3.x +
               t3 * section->p2->v3.x +
               t4 * section->p3->v3.x;

    res.v3.y = t1 * section->p0->v3.y +
               t2 * section->p1->v3.y +
               t3 * section->p2->v3.y +
               t4 * section->p3->v3.y;

    res.v3.z = t1 * section->p0->v3.z +
               t2 * section->p1->v3.z +
               t3 * section->p2->v3.z +
               t4 * section->p3->v3.z;

    return res;
}

gc_vec_t bezier_compute_first_derivative(bezier_curve_t *curve, r32 tg)
{
    VINIT4(res, 0, 0, 0, 0);

    if (curve->flags & BEZIER_REVERSE)
        tg = 1.0f - tg;

    r32 tc = bezier_tc(curve->section_count, tg);
    u32 section_index = bezier_section(curve, tc);
    r32 t = bezier_tl(curve->section_count, tc);

    r32 t1 = -3 * t * t + 6 * t - 3;
    r32 t2 = 9 * t * t - 12 * t + 3;
    r32 t3 = -9 * t * t + 6 * t;
    r32 t4 = 3 * t * t;

    bezier_section_t *section = curve->sections + section_index;

    res.v3.x = t1 * section->p0->v3.x +
               t2 * section->p1->v3.x +
               t3 * section->p2->v3.x +
               t4 * section->p3->v3.x;

    res.v3.y = t1 * section->p0->v3.y +
               t2 * section->p1->v3.y +
               t3 * section->p2->v3.y +
               t4 * section->p3->v3.y;

    res.v3.z = t1 * section->p0->v3.z +
               t2 * section->p1->v3.z +
               t3 * section->p2->v3.z +
               t4 * section->p3->v3.z;

    if (curve->flags & BEZIER_REVERSE)
    {
        res.v3.x *= -1;
        res.v3.y *= -1;
        res.v3.z *= -1;
    }

    return res;
}
