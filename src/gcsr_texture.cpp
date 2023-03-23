// ----------------------------------------------------------------------------------
// -- File: gcsr_texture.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-07-24 12:00:20
// -- Modified: 2022-07-24 12:00:21
// ----------------------------------------------------------------------------------

#include "gcsr_texture.h"
#if defined(GC_PIPE_SSE)
#include "simd/gcsr_sse_texture.h"
#include "simd/gcsr_sse_texture.cpp"
#elif defined(GC_PIPE_AVX)
#include "simd/gcsr_avx_texture.h"
#include "simd/gcsr_avx_texture.cpp"
#endif

r32 _tex_clamp_s(r32 c)
{
    c = c < 0 ? 0 : c;
    return c;
}

r32 _tex_repeat_s(r32 c)
{
    c = c - floor(c);
    c = c < 0 ? 0 : c;
    return c;
}

r32 _tex_mirror_s(r32 c)
{
    c = c < 0 ? -c : c;

    s32 min = (s32) c;
    s32 max = min + 1;

    if (min % 2)
        c = max - c;
    else
        c = c - min;

    return c;
}

void _tex_clamp(r32 *in_u, r32 *in_v, r32 *out_u, r32 *out_v)
{
    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        out_u[i] = in_u[i];
        out_v[i] = in_v[i];

        if (in_u[i] < 0)
            out_u[i] = 0;
        else if (in_u[i] > 1)
            out_u[i] = 1;

        if (in_v[i] < 0)
            out_v[i] = 0;
        else if (in_v[i] > 1)
            out_v[i] = 1;
    }
}

void _tex_repeat(r32 *in_u, r32 *in_v, r32 *out_u, r32 *out_v)
{
    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        out_u[i] = in_u[i] - floor(in_u[i]);
        out_v[i] = in_v[i] - floor(in_v[i]);

        if (out_u[i] < 0)
            out_u[i] = 0;

        if (out_v[i] < 0)
            out_v[i] = 0;
    }
}

void _tex_mirror(r32 *in_u, r32 *in_v, r32 *out_u, r32 *out_v)
{
    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        out_u[i] = in_u[i] < 0 ? -in_u[i] : in_u[i];
        out_v[i] = in_v[i] < 0 ? -in_v[i] : in_v[i];

        s32 min_u = (s32) out_u[i];
        s32 max_u = min_u + 1;

        s32 min_v = (s32) out_v[i];
        s32 max_v = min_v + 1;

        if (min_u % 2)
            out_u[i] = max_u - out_u[i];
        else
            out_u[i] = out_u[i] - min_u;

        if (min_v % 2)
            out_v[i] = max_v - out_v[i];
        else
            out_v[i] = out_v[i] - min_v;

        if (out_u[i] < 0)
            out_u[i] = 0;
        else if (out_u[i] > 1)
            out_u[i] = 1;

        if (out_v[i] < 0)
            out_v[i] = 0;
        else if (out_v[i] > 1)
            out_v[i] = 1;
    }
}

#define ABS_MASK ~(1 << 31)

__INLINE__ void gc_compute_lod(u32 width, u32 height, u32 mip_count, r32 *dudx, r32 *dudy, r32 *dvdx, r32 *dvdy, lod_t *lod)
{
    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        lod->low[i] = 0;
        lod->high[i] = 0;
        lod->interp[i] = 0;

        r32 tdudx = dudx[i] * width;
        r32 tdudy = dudy[i] * width;
        r32 tdvdx = dvdx[i] * height;
        r32 tdvdy = dvdy[i] * height;

        u32 *p = (u32 *) &tdudx;
        *p &= ABS_MASK;
        p = (u32 *) &tdvdx;
        *p &= ABS_MASK;
        p = (u32 *) &tdudy;
        *p &= ABS_MASK;
        p = (u32 *) &tdvdy;
        *p &= ABS_MASK;

        r32 t1 = tdudx * tdudx + tdvdx * tdvdx;
        r32 t2 = tdudy * tdudy + tdvdy * tdvdy;

        r32 scale = t1 >= t2 ? t1 : t2;
        r32 mip_lod = 0.5f * log2f(scale);

        if (mip_lod < 0)
            mip_lod = 0;
        else if (mip_lod > mip_count - 1)
            mip_lod = mip_count - 1;

        r32 low = FAST_FLOOR16(mip_lod);
        r32 high = FAST_CEIL16(mip_lod);
        r32 interp = mip_lod - low;

        lod->low[i] = low;
        lod->high[i] = high;
        lod->interp[i] = interp;
    }
}

// __INLINE__ void gc_compute_lod(texture2d_t *texture, fv2_t *texcoord, u32 *lod_min, u32 *lod_max, r32 *lod_interp)
__INLINE__ void gc_compute_lod_old(u32 width, u32 height, u32 mip_count, r32 *u, r32 *v, lod_t *lod)
{
    lod->low[0] = 0;
    lod->low[1] = 0;
    lod->low[2] = 0;
    lod->low[3] = 0;

    lod->high[0] = 0;
    lod->high[1] = 0;
    lod->high[2] = 0;
    lod->high[3] = 0;

    lod->interp[0] = 0;
    lod->interp[1] = 0;
    lod->interp[2] = 0;
    lod->interp[3] = 0;

    r32 dudx = (u[1] - u[0]) * width;
    r32 dvdx = (v[1] - v[0]) * height;
    r32 dudy = (u[2] - u[0]) * width;
    r32 dvdy = (v[2] - v[0]) * height;

    u32 *p = (u32 *) &dudx;
    *p &= ABS_MASK;
    p = (u32 *) &dvdx;
    *p &= ABS_MASK;
    p = (u32 *) &dudy;
    *p &= ABS_MASK;
    p = (u32 *) &dvdy;
    *p &= ABS_MASK;

    r32 t1 = dudx * dudx + dvdx * dvdx;
    r32 t2 = dudy * dudy + dvdy * dvdy;

    r32 scale = t1 >= t2 ? t1 : t2;
    r32 mip_lod = 0.5f * log2f(scale);

    if (mip_lod < 0)
        mip_lod = 0;
    else if (mip_lod > mip_count - 1)
        mip_lod = mip_count - 1;

    r32 low = FAST_FLOOR16(mip_lod);
    r32 high = FAST_CEIL16(mip_lod);
    r32 interp = mip_lod - low;

    lod->low[0] = low;
    lod->low[1] = low;
    lod->low[2] = low;
    lod->low[3] = low;

    lod->high[0] = high;
    lod->high[1] = high;
    lod->high[2] = high;
    lod->high[3] = high;

    lod->interp[0] = interp;
    lod->interp[1] = interp;
    lod->interp[2] = interp;
    lod->interp[3] = interp;
}

// #define gc_texture_compute_lod(texture, u, v, lod) gc_compute_lod((texture)->mips->header->width, (texture)->mips->header->height, (texture)->mip_count, u, v, lod)
// #define gl_cubemap_compute_lod(texture, u, v, lod) gc_compute_lod((texture)->faces[0]->mips->header->width, (texture)->faces[0]->mips->header->height, (texture)->mip_count, u, v, lod)
#define gc_texture_compute_lod(texture, fragment, lod) gc_compute_lod((texture)->mips->header->width, (texture)->mips->header->height, (texture)->mip_count, fragment->dudx, fragment->dudy, fragment->dvdx, fragment->dvdy, lod)
#define gl_cubemap_compute_lod(texture, fragment, lod) gc_compute_lod((texture)->faces[0]->mips->header->width, (texture)->faces[0]->mips->header->height, (texture)->mip_count, fragment->dudx, fragment->dudy, fragment->dvdx, fragment->dvdy, lod)

// NOTE(gabic): Momentan nu stiu cum ar merge facut aici cu simd.
void cube_uv_from(gc_vec_t *v, gc_vec_t *texcoord, u32 *face_index)
{
    // -- Generate the uv coordinates from the input fragment vectors.

    r32 absx = v->v3.x < 0 ? -v->v3.x : v->v3.x;
    r32 absy = v->v3.y < 0 ? -v->v3.y : v->v3.y;
    r32 absz = v->v3.z < 0 ? -v->v3.z : v->v3.z;

    b8 positive_x = v->v3.x > 0 ? true : false;
    b8 positive_y = v->v3.y > 0 ? true : false;
    b8 positive_z = v->v3.z > 0 ? true : false;

    r32 max_axis = 0;
    r32 tu = 0;
    r32 tv = 0;

    // Left.
    if (!positive_x && absx >= absy && absx >= absz)
    {
        max_axis = absx;
        tu = v->v3.y;
        tv = -v->v3.z;
        *face_index = CUBE_LEFT;
    }
    // Right.
    else if (positive_x && absx >= absy && absx >= absz)
    {
        max_axis = absx;
        tu = -v->v3.y;
        tv = -v->v3.z;
        *face_index = CUBE_RIGHT;
    }
    // Front.
    else if (positive_y && absy >= absx && absy >= absz)
    {
        max_axis = absy;
        tu = v->v3.x;
        tv = -v->v3.z;
        *face_index = CUBE_FRONT;
    }
    // Back.
    else if (!positive_y && absy >= absx && absy >= absz)
    {
        max_axis = absy;
        tu = -v->v3.x;
        tv = -v->v3.z;
        *face_index = CUBE_BACK;
    }
    // Top.
    else if (positive_z && absz >= absx && absz >= absy)
    {
        max_axis = absz;
        tu = v->v3.x;
        tv = v->v3.y;
        *face_index = CUBE_TOP;
    }
    // Bottom.
    else if (!positive_z && absz >= absx && absz >= absy)
    {
        max_axis = absz;
        tu = v->v3.x;
        tv = -v->v3.y;
        *face_index = CUBE_BOTTOM;
    }

    texcoord->v2.x = 0.5f * (tu / max_axis + 1.0f);
    texcoord->v2.y = 0.5f * (tv / max_axis + 1.0f);
}

void cube_uv_from_vec(fv3_t *v, fv2_t *texcoord, u32 *face_index, b8 warp)
{
    // -- Generate the uv coordinates from the input fragment vectors.

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 absx = v->x[i] < 0 ? -v->x[i] : v->x[i];
        r32 absy = v->y[i] < 0 ? -v->y[i] : v->y[i];
        r32 absz = v->z[i] < 0 ? -v->z[i] : v->z[i];

        b8 positive_x = v->x[i] > 0 ? true : false;
        b8 positive_y = v->y[i] > 0 ? true : false;
        b8 positive_z = v->z[i] > 0 ? true : false;

        r32 max_axis = 0;
        r32 tu = 0;
        r32 tv = 0;

        // Left.
        if (!positive_x && absx >= absy && absx >= absz)
        {
            max_axis = absx;
            tu = v->y[i];
            tv = -v->z[i];
            face_index[i] = CUBE_LEFT;
        }
        // Right.
        else if (positive_x && absx >= absy && absx >= absz)
        {
            max_axis = absx;
            tu = -v->y[i];
            tv = -v->z[i];
            face_index[i] = CUBE_RIGHT;
        }
        // Front.
        else if (positive_y && absy >= absx && absy >= absz)
        {
            max_axis = absy;
            tu = v->x[i];
            tv = -v->z[i];
            face_index[i] = CUBE_FRONT;
        }
        // Back.
        else if (!positive_y && absy >= absx && absy >= absz)
        {
            max_axis = absy;
            tu = -v->x[i];
            tv = -v->z[i];
            face_index[i] = CUBE_BACK;
        }
        // Top.
        else if (positive_z && absz >= absx && absz >= absy)
        {
            max_axis = absz;
            tu = v->x[i];
            tv = v->y[i];
            face_index[i] = CUBE_TOP;
        }
        // Bottom.
        else if (!positive_z && absz >= absx && absz >= absy)
        {
            max_axis = absz;
            tu = v->x[i];
            tv = -v->y[i];
            face_index[i] = CUBE_BOTTOM;
        }

        r32 inv = 1.0f / max_axis;
        r32 tmp_u = tu * inv;
        r32 tmp_v = tv * inv;

        #define warp_constant 0.0145f

        if (warp)
        {
            tmp_u = warp_constant * (tmp_u * tmp_u * tmp_u) + tmp_u;
            tmp_v = warp_constant * (tmp_v * tmp_v * tmp_v) + tmp_v;

            if (tmp_u > 1) tmp_u = 1;
            if (tmp_v > 1) tmp_v = 1;
            if (tmp_u < -1) tmp_u = -1;
            if (tmp_v < -1) tmp_v = -1;
        }

        texcoord->x[i] = 0.5f * (tmp_u + 1.0f);
        texcoord->y[i] = 0.5f * (tmp_v + 1.0f);
    }
}

// ----------------------------------------------------------------------------------
// -- Texture clear.
// ----------------------------------------------------------------------------------

void _clear_rgbau8(texture2d_t *texture, gc_vec_t color)
{
    u32 total = texture->mips->header->width * texture->mips->header->height;
    u32 *pixel_pointer = (u32 *) texture->mips->data;
    u32 packed = gl_pack(color);

    for (u32 i = 0; i < total; ++i) {
        *pixel_pointer++ = packed;
    }
}

void _clear_rgbaf(texture2d_t *texture, gc_vec_t color)
{
    u32 total = texture->mips->header->width * texture->mips->header->height;
    texpixel_rgbaf_t *pixel = (texpixel_rgbaf_t *) texture->mips->data;

    for (u32 i = 0; i < total; ++i)
    {
        pixel->r = color.c.r;
        pixel->g = color.c.g;
        pixel->b = color.c.b;
        pixel->a = color.c.a;

        pixel++;
    }
}

void _clear_rgbf(texture2d_t *texture, gc_vec_t color)
{
    u32 total = texture->mips->header->width * texture->mips->header->height;
    texpixel_rgbf_t *pixel = (texpixel_rgbf_t *) texture->mips->data;

    for (u32 i = 0; i < total; ++i)
    {
        pixel->r = color.c.r;
        pixel->g = color.c.g;
        pixel->b = color.c.b;

        pixel++;
    }
}

void _clear_rgf(texture2d_t *texture, gc_vec_t color)
{
    u32 total = texture->mips->header->width * texture->mips->header->height;
    texpixel_rgf_t *pixel = (texpixel_rgf_t *) texture->mips->data;

    for (u32 i = 0; i < total; ++i)
    {
        pixel->r = color.c.r;
        pixel->g = color.c.g;

        pixel++;
    }
}

// ----------------------------------------------------------------------------------
// -- Shadow map routines.
// ----------------------------------------------------------------------------------

// r32 shadow_map_sample_single(texture2d_t *shadow_map, r32 u, r32 v, r32 compare)
// {
//     OPTICK_EVENT();

//     texture_mip_t *texture = shadow_map->mips;
//     texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

//     u32 map_size = texture->header->width - 1;
//     u32 map_pitch = texture->header->width;

//     r32 texel_u = _tex_clamp(u);
//     r32 texel_v = _tex_clamp(v);

//     r32 texel_x = texel_u * map_size;
//     r32 texel_y = texel_v * map_size;

//     u32 x = (u32) (texel_x);
//     u32 y = (u32) (texel_y);

//     TEX_OFFSET(offset, map_pitch, x, y);
//     texpixel_rgf_t *sample = texture_pointer + offset;
//     r32 cmp = compare > sample->r ? 0 : 1;

//     return cmp;
// }

// TODO(gabic): simd
// NOTE(gabic): shadow_map_sun_sample_linear() e acelasi lucru !
void shadow_map_sample_filter_single(cube_texture_t *shadow_map, u32 *face_index, r32 *u, r32 *v, r32 *compare, r32 *out)
{
    OPTICK_EVENT("shadow_map_sample_filter_single");

    r32 tmp_u[GC_FRAG_SIZE];
    r32 tmp_v[GC_FRAG_SIZE];

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    _tex_clamp(u, v, tmp_u, tmp_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        texture2d_t *face = shadow_map->faces[face_index[i]];
        texture_mip_t *texture = face->mips;
        texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

        u32 map_size = texture->header->width - 1;
        u32 map_pitch = texture->header->width;

        local_u[0] = tmp_u[i];
        local_v[0] = tmp_v[i];

        local_u[1] = tmp_u[i] + texture->header->tex_du;
        local_v[1] = tmp_v[i];

        local_u[2] = tmp_u[i];
        local_v[2] = tmp_v[i] + texture->header->tex_dv;

        local_u[3] = local_u[1];
        local_v[3] = local_v[2];

        _tex_clamp(local_u, local_v, local_u, local_v);

        r32 texel_x0 = local_u[0] * map_size;
        r32 texel_y0 = local_v[0] * map_size;

        r32 texel_x1 = local_u[1] * map_size;
        r32 texel_y1 = local_v[1] * map_size;

        r32 texel_x2 = local_u[2] * map_size;
        r32 texel_y2 = local_v[2] * map_size;

        r32 texel_x3 = local_u[3] * map_size;
        r32 texel_y3 = local_v[3] * map_size;

        u32 x0 = (u32) (texel_x0);
        u32 y0 = (u32) (texel_y0);

        u32 x1 = (u32) (texel_x1);
        u32 y1 = (u32) (texel_y1);

        u32 x2 = (u32) (texel_x2);
        u32 y2 = (u32) (texel_y2);

        u32 x3 = (u32) (texel_x3);
        u32 y3 = (u32) (texel_y3);

        r32 s = texel_x0 - x0;
        r32 t = texel_y0 - y0;

        TEX_OFFSET(offset_0, map_pitch, x0, y0);
        TEX_OFFSET(offset_1, map_pitch, x1, y1);
        TEX_OFFSET(offset_2, map_pitch, x2, y2);
        TEX_OFFSET(offset_3, map_pitch, x3, y3);

        texpixel_rgf_t *sample_0 = texture_pointer + offset_0;
        texpixel_rgf_t *sample_1 = texture_pointer + offset_1;
        texpixel_rgf_t *sample_2 = texture_pointer + offset_2;
        texpixel_rgf_t *sample_3 = texture_pointer + offset_3;

        r32 c0 = compare[i] > sample_0->r ? 0 : 1;
        r32 c1 = compare[i] > sample_1->r ? 0 : 1;
        r32 c2 = compare[i] > sample_2->r ? 0 : 1;
        r32 c3 = compare[i] > sample_3->r ? 0 : 1;

        out[i] = (c0 + (c1 - c0) * s) * (1 - t) + (c2 + (c3 - c2) * s) * t;
    }
}

void shadow_map_sun_sample(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("shadow_map_sun_sample");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    u32 map_size = texture->header->width - 1;
    u32 map_pitch = texture->header->width;

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    _tex_clamp(u, v, local_u, local_v);

    r32 texel_x0 = local_u[0] * map_size;
    r32 texel_y0 = local_v[0] * map_size;

    r32 texel_x1 = local_u[1] * map_size;
    r32 texel_y1 = local_v[1] * map_size;

    r32 texel_x2 = local_u[2] * map_size;
    r32 texel_y2 = local_v[2] * map_size;

    r32 texel_x3 = local_u[3] * map_size;
    r32 texel_y3 = local_v[3] * map_size;

    u32 x0 = (u32) (texel_x0);
    u32 y0 = (u32) (texel_y0);

    u32 x1 = (u32) (texel_x1);
    u32 y1 = (u32) (texel_y1);

    u32 x2 = (u32) (texel_x2);
    u32 y2 = (u32) (texel_y2);

    u32 x3 = (u32) (texel_x3);
    u32 y3 = (u32) (texel_y3);

    TEX_OFFSET(offset0, map_pitch, x0, y0);
    TEX_OFFSET(offset1, map_pitch, x1, y1);
    TEX_OFFSET(offset2, map_pitch, x2, y2);
    TEX_OFFSET(offset3, map_pitch, x3, y3);

    texpixel_rgf_t *sample0 = texture_pointer + offset0;
    texpixel_rgf_t *sample1 = texture_pointer + offset1;
    texpixel_rgf_t *sample2 = texture_pointer + offset2;
    texpixel_rgf_t *sample3 = texture_pointer + offset3;

    output[0] = compare[0] > sample0->r ? 0 : 1;
    output[1] = compare[1] > sample1->r ? 0 : 1;
    output[2] = compare[2] > sample2->r ? 0 : 1;
    output[3] = compare[3] > sample3->r ? 0 : 1;
}

void shadow_map_sun_sample_linear(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("shadow_map_sun_sample_linear");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    u32 map_size = texture->header->width - 1;
    u32 map_pitch = texture->header->width;

    r32 tmp_u[GC_FRAG_SIZE];
    r32 tmp_v[GC_FRAG_SIZE];

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    _tex_clamp(u, v, tmp_u, tmp_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        local_u[0] = tmp_u[i];
        local_v[0] = tmp_v[i];

        local_u[1] = tmp_u[i] + texture->header->tex_du;
        local_v[1] = tmp_v[i];

        local_u[2] = tmp_u[i];
        local_v[2] = tmp_v[i] + texture->header->tex_dv;

        local_u[3] = local_u[1];
        local_v[3] = local_v[2];

        _tex_clamp(local_u, local_v, local_u, local_v);

        r32 texel_x0 = local_u[0] * map_size;
        r32 texel_y0 = local_v[0] * map_size;

        r32 texel_x1 = local_u[1] * map_size;
        r32 texel_y1 = local_v[1] * map_size;

        r32 texel_x2 = local_u[2] * map_size;
        r32 texel_y2 = local_v[2] * map_size;

        r32 texel_x3 = local_u[3] * map_size;
        r32 texel_y3 = local_v[3] * map_size;

        u32 x0 = (u32) (texel_x0);
        u32 y0 = (u32) (texel_y0);

        u32 x1 = (u32) (texel_x1);
        u32 y1 = (u32) (texel_y1);

        u32 x2 = (u32) (texel_x2);
        u32 y2 = (u32) (texel_y2);

        u32 x3 = (u32) (texel_x3);
        u32 y3 = (u32) (texel_y3);

        r32 s = texel_x0 - x0;
        r32 t = texel_y0 - y0;

        TEX_OFFSET(offset_0, map_pitch, x0, y0);
        TEX_OFFSET(offset_1, map_pitch, x1, y1);
        TEX_OFFSET(offset_2, map_pitch, x2, y2);
        TEX_OFFSET(offset_3, map_pitch, x3, y3);

        texpixel_rgf_t *sample_0 = texture_pointer + offset_0;
        texpixel_rgf_t *sample_1 = texture_pointer + offset_1;
        texpixel_rgf_t *sample_2 = texture_pointer + offset_2;
        texpixel_rgf_t *sample_3 = texture_pointer + offset_3;

        r32 c0 = compare[i] > sample_0->r ? 0 : 1;
        r32 c1 = compare[i] > sample_1->r ? 0 : 1;
        r32 c2 = compare[i] > sample_2->r ? 0 : 1;
        r32 c3 = compare[i] > sample_3->r ? 0 : 1;

        output[i] = (c0 + (c1 - c0) * s) * (1 - t) + (c2 + (c3 - c2) * s) * t;
    }
}

void shadow_map_sun_sample_pcf_3x3(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("shadow_map_sun_sample_pcf_3x3");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    u32 map_size = texture->header->width - 1;
    u32 map_pitch = texture->header->width;

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    r32 pcf_u[3][GC_FRAG_SIZE];
    r32 pcf_v[3][GC_FRAG_SIZE];

    _tex_clamp(u, v, local_u, local_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 t_u1 = local_u[i] - texture->header->tex_du;
        r32 t_u2 = local_u[i] + texture->header->tex_du;

        r32 t_v1 = local_v[i] - texture->header->tex_dv;
        r32 t_v2 = local_v[i] + texture->header->tex_dv;

        pcf_u[0][0] = t_u1; pcf_u[0][1] = local_u[i]; pcf_u[0][2] = t_u2; pcf_u[0][3] = t_u1;
        pcf_u[1][0] = local_u[i]; pcf_u[1][1] = t_u2; pcf_u[1][2] = t_u1; pcf_u[1][3] = local_u[i];
        pcf_u[2][0] = t_u2; pcf_u[2][1] = 0; pcf_u[2][2] = 0; pcf_u[2][3] = 0;

        pcf_v[0][0] = t_v1; pcf_v[0][1] = t_v1; pcf_v[0][2] = t_v1; pcf_v[0][3] = local_v[i];
        pcf_v[1][0] = local_v[i]; pcf_v[1][1] = local_v[i]; pcf_v[1][2] = t_v2; pcf_v[1][3] = t_v2;
        pcf_v[2][0] = t_v2; pcf_v[2][1] = 0; pcf_v[2][2] = 0; pcf_v[2][3] = 0;

        output[i] = 0;

        for (u8 j = 0; j < 3; ++j)
        {
            _tex_clamp(pcf_u[j], pcf_v[j], pcf_u[j], pcf_v[j]);

            if (j < 2)
            {
                u32 x0 = (u32) (pcf_u[j][0] * map_size);
                u32 y0 = (u32) (pcf_v[j][0] * map_size);

                u32 x1 = (u32) (pcf_u[j][1] * map_size);
                u32 y1 = (u32) (pcf_v[j][1] * map_size);

                u32 x2 = (u32) (pcf_u[j][2] * map_size);
                u32 y2 = (u32) (pcf_v[j][2] * map_size);

                u32 x3 = (u32) (pcf_u[j][3] * map_size);
                u32 y3 = (u32) (pcf_v[j][3] * map_size);

                TEX_OFFSET(offset0, map_pitch, x0, y0);
                TEX_OFFSET(offset1, map_pitch, x1, y1);
                TEX_OFFSET(offset2, map_pitch, x2, y2);
                TEX_OFFSET(offset3, map_pitch, x3, y3);

                texpixel_rgf_t *sample0 = texture_pointer + offset0;
                texpixel_rgf_t *sample1 = texture_pointer + offset1;
                texpixel_rgf_t *sample2 = texture_pointer + offset2;
                texpixel_rgf_t *sample3 = texture_pointer + offset3;

                output[i] += compare[i] > sample0->r ? 0 : 1;
                output[i] += compare[i] > sample1->r ? 0 : 1;
                output[i] += compare[i] > sample2->r ? 0 : 1;
                output[i] += compare[i] > sample3->r ? 0 : 1;
            }
            else
            {
                u32 x0 = (u32) (pcf_u[j][0] * map_size);
                u32 y0 = (u32) (pcf_v[j][0] * map_size);

                TEX_OFFSET(offset0, map_pitch, x0, y0);
                texpixel_rgf_t *sample0 = texture_pointer + offset0;
                output[i] += compare[i] > sample0->r ? 0 : 1;
            }
        }

        output[i] /= 9;
    }
}

// TODO(gabic): simd
void shadow_map_sun_sample_pcf_3x3_linear(texture2d_t *shadow_map, r32 *u, r32 *v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("shadow_map_sun_sample_pcf_3x3_linear");

    texture_mip_t *texture = shadow_map->mips;

    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    r32 pcf_u[3][GC_FRAG_SIZE];
    r32 pcf_v[3][GC_FRAG_SIZE];

    r32 local_output[GC_FRAG_SIZE];

    _tex_clamp(u, v, local_u, local_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 t_u1 = local_u[i] - texture->header->tex_du;
        r32 t_u2 = local_u[i] + texture->header->tex_du;

        r32 t_v1 = local_v[i] - texture->header->tex_dv;
        r32 t_v2 = local_v[i] + texture->header->tex_dv;

        pcf_u[0][0] = t_u1; pcf_u[0][1] = local_u[i]; pcf_u[0][2] = t_u2; pcf_u[0][3] = t_u1;
        pcf_u[1][0] = local_u[i]; pcf_u[1][1] = t_u2; pcf_u[1][2] = t_u1; pcf_u[1][3] = local_u[i];
        pcf_u[2][0] = t_u2; pcf_u[2][1] = 0; pcf_u[2][2] = 0; pcf_u[2][3] = 0;

        pcf_v[0][0] = t_v1; pcf_v[0][1] = t_v1; pcf_v[0][2] = t_v1; pcf_v[0][3] = local_v[i];
        pcf_v[1][0] = local_v[i]; pcf_v[1][1] = local_v[i]; pcf_v[1][2] = t_v2; pcf_v[1][3] = t_v2;
        pcf_v[2][0] = t_v2; pcf_v[2][1] = 0; pcf_v[2][2] = 0; pcf_v[2][3] = 0;

        output[i] = 0;

        for (u8 j = 0; j < 3; ++j)
        {
            _tex_clamp(pcf_u[j], pcf_v[j], pcf_u[j], pcf_v[j]);
            shadow_map_sun_sample_linear(shadow_map, pcf_u[j], pcf_v[j], compare, local_output);

            if (j < 2)
                output[i] += local_output[0] + local_output[1] + local_output[2] + local_output[3];
            else
                output[i] += local_output[0];
        }

        output[i] /= 9;
    }

#if 0
    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 base_u = _tex_clamp(texcoord->x[i]);
        r32 base_v = _tex_clamp(texcoord->y[i]);

        output[i] = 0;

        r32 tu[GC_FRAG_SIZE] = {
            base_u - texture->header->tex_du,
            base_u,
            base_u + texture->header->tex_du,
            0
        };

        r32 tv = base_v - texture->header->tex_dv;

        for (u8 ty = 0; ty < 3; ++ty)
        {
            for (u8 tx = 0; tx < 3; ++tx) {
                output[i] += shadow_map_sample_filter_single(shadow_map, tu[tx], tv, compare->data[i]);
            }

            tv += texture->header->tex_dv;
        }

        output[i] /= 9;
    }
#endif
}

void shadow_map_sun_sample_pcf_5x5(texture2d_t *shadow_map, r32 *in_u, r32 *in_v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("shadow_map_sun_sample_pcf_5x5");

    texture_mip_t *texture = shadow_map->mips;
    texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

    u32 map_size = texture->header->width - 1;
    u32 map_pitch = texture->header->width;

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    r32 pcf_u[7][GC_FRAG_SIZE];
    r32 pcf_v[7][GC_FRAG_SIZE];

    r32 u_d2 = 2 * texture->header->tex_du;
    r32 u_d1 = texture->header->tex_du;
    r32 v_d2 = 2 * texture->header->tex_dv;
    r32 v_d1 = texture->header->tex_dv;

    _tex_clamp(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 t_u1 = u[i] - u_d2;
        r32 t_u2 = u[i] - u_d1;
        r32 t_u3 = u[i] + u_d1;
        r32 t_u4 = u[i] + u_d2;

        r32 t_v1 = v[i] - v_d2;
        r32 t_v2 = v[i] - v_d1;
        r32 t_v3 = v[i] + v_d1;
        r32 t_v4 = v[i] + v_d2;

        pcf_u[0][0] = t_u1; pcf_u[0][1] = t_u2; pcf_u[0][2] = u[i]; pcf_u[0][3] = t_u3;
        pcf_u[1][0] = t_u4; pcf_u[1][1] = t_u1; pcf_u[1][2] = t_u2; pcf_u[1][3] = u[i];
        pcf_u[2][0] = t_u3; pcf_u[2][1] = t_u4; pcf_u[2][2] = t_u1; pcf_u[2][3] = t_u2;
        pcf_u[3][0] = u[i]; pcf_u[3][1] = t_u3; pcf_u[3][2] = t_u4; pcf_u[3][3] = t_u1;
        pcf_u[4][0] = t_u2; pcf_u[4][1] = u[i]; pcf_u[4][2] = t_u3; pcf_u[4][3] = t_u4;
        pcf_u[5][0] = t_u1; pcf_u[5][1] = t_u2; pcf_u[5][2] = u[i]; pcf_u[5][3] = t_u3;
        pcf_u[6][0] = t_u4; pcf_u[6][1] = 0; pcf_u[6][2] = 0; pcf_u[6][3] = 0;

        pcf_v[0][0] = t_v1; pcf_v[0][1] = t_v1; pcf_v[0][2] = t_v1; pcf_v[0][3] = t_v1;
        pcf_v[1][0] = t_v1; pcf_v[1][1] = t_v2; pcf_v[1][2] = t_v2; pcf_v[1][3] = t_v2;
        pcf_v[2][0] = t_v2; pcf_v[2][1] = t_v2; pcf_v[2][2] = v[i]; pcf_v[2][3] = v[i];
        pcf_v[3][0] = v[i]; pcf_v[3][1] = v[i]; pcf_v[3][2] = v[i]; pcf_v[3][3] = t_v3;
        pcf_v[4][0] = t_v3; pcf_v[4][1] = t_v3; pcf_v[4][2] = t_v3; pcf_v[4][3] = t_v3;
        pcf_v[5][0] = t_v4; pcf_v[5][1] = t_v4; pcf_v[5][2] = t_v4; pcf_v[5][3] = t_v4;
        pcf_v[6][0] = t_v4; pcf_v[6][1] = 0; pcf_v[6][2] = 0; pcf_v[6][3] = 0;

        output[i] = 0;

        for (u8 j = 0; j < 7; ++j)
        {
            _tex_clamp(pcf_u[j], pcf_v[j], pcf_u[j], pcf_v[j]);

            if (j < 6)
            {
                u32 x0 = (u32) (pcf_u[j][0] * map_size);
                u32 y0 = (u32) (pcf_v[j][0] * map_size);

                u32 x1 = (u32) (pcf_u[j][1] * map_size);
                u32 y1 = (u32) (pcf_v[j][1] * map_size);

                u32 x2 = (u32) (pcf_u[j][2] * map_size);
                u32 y2 = (u32) (pcf_v[j][2] * map_size);

                u32 x3 = (u32) (pcf_u[j][3] * map_size);
                u32 y3 = (u32) (pcf_v[j][3] * map_size);

                TEX_OFFSET(offset0, map_pitch, x0, y0);
                TEX_OFFSET(offset1, map_pitch, x1, y1);
                TEX_OFFSET(offset2, map_pitch, x2, y2);
                TEX_OFFSET(offset3, map_pitch, x3, y3);

                texpixel_rgf_t *sample0 = texture_pointer + offset0;
                texpixel_rgf_t *sample1 = texture_pointer + offset1;
                texpixel_rgf_t *sample2 = texture_pointer + offset2;
                texpixel_rgf_t *sample3 = texture_pointer + offset3;

                output[i] += compare[i] > sample0->r ? 0 : 1;
                output[i] += compare[i] > sample1->r ? 0 : 1;
                output[i] += compare[i] > sample2->r ? 0 : 1;
                output[i] += compare[i] > sample3->r ? 0 : 1;
            }
            else
            {
                u32 x0 = (u32) (pcf_u[j][0] * map_size);
                u32 y0 = (u32) (pcf_v[j][0] * map_size);

                TEX_OFFSET(offset0, map_pitch, x0, y0);
                texpixel_rgf_t *sample0 = texture_pointer + offset0;
                output[i] += compare[i] > sample0->r ? 0 : 1;
            }
        }

        output[i] /= 25;
    }

#if 0
    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 base_u = _tex_clamp(texcoord->x[i]);
        r32 base_v = _tex_clamp(texcoord->y[i]);

        output[i] = 0;

        r32 tu[5] = {
            base_u - 2 * texture->header->tex_du,
            base_u - texture->header->tex_du,
            base_u,
            base_u + texture->header->tex_du,
            base_u + 2 * texture->header->tex_du,
        };

        r32 tv = base_v - 2 * texture->header->tex_dv;

        for (u8 ty = 0; ty < 5; ++ty)
        {
            for (u8 tx = 0; tx < 5; ++tx)
            {
                r32 texel_u = _tex_clamp(tu[tx]);
                r32 texel_v = _tex_clamp(tv);

                u32 x = (u32) (texel_u * map_size);
                u32 y = (u32) (texel_v * map_size);

                TEX_OFFSET(offset, map_pitch, x, y);

                texpixel_rgf_t *sample = texture_pointer + offset;
                output[i] += compare->data[i] > sample->r ? 0 : 1;
            }

            tv += texture->header->tex_dv;
        }

        output[i] /= 25;
    }
#endif
}

void shadow_map_sun_sample_pcf_5x5_linear(texture2d_t *shadow_map, r32 *in_u, r32 *in_v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("shadow_map_sun_sample_pcf_5x5_linear");

    texture_mip_t *texture = shadow_map->mips;

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    r32 pcf_u[7][GC_FRAG_SIZE];
    r32 pcf_v[7][GC_FRAG_SIZE];

    r32 u_d2 = 2 * texture->header->tex_du;
    r32 u_d1 = texture->header->tex_du;
    r32 v_d2 = 2 * texture->header->tex_dv;
    r32 v_d1 = texture->header->tex_dv;

    r32 local_output[GC_FRAG_SIZE];

    _tex_clamp(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 t_u1 = u[i] - u_d2;
        r32 t_u2 = u[i] - u_d1;
        r32 t_u3 = u[i] + u_d1;
        r32 t_u4 = u[i] + u_d2;

        r32 t_v1 = v[i] - v_d2;
        r32 t_v2 = v[i] - v_d1;
        r32 t_v3 = v[i] + v_d1;
        r32 t_v4 = v[i] + v_d2;

        pcf_u[0][0] = t_u1; pcf_u[0][1] = t_u2; pcf_u[0][2] = u[i]; pcf_u[0][3] = t_u3;
        pcf_u[1][0] = t_u4; pcf_u[1][1] = t_u1; pcf_u[1][2] = t_u2; pcf_u[1][3] = u[i];
        pcf_u[2][0] = t_u3; pcf_u[2][1] = t_u4; pcf_u[2][2] = t_u1; pcf_u[2][3] = t_u2;
        pcf_u[3][0] = u[i]; pcf_u[3][1] = t_u3; pcf_u[3][2] = t_u4; pcf_u[3][3] = t_u1;
        pcf_u[4][0] = t_u2; pcf_u[4][1] = u[i]; pcf_u[4][2] = t_u3; pcf_u[4][3] = t_u4;
        pcf_u[5][0] = t_u1; pcf_u[5][1] = t_u2; pcf_u[5][2] = u[i]; pcf_u[5][3] = t_u3;
        pcf_u[6][0] = t_u4; pcf_u[6][1] = 0; pcf_u[6][2] = 0; pcf_u[6][3] = 0;

        pcf_v[0][0] = t_v1; pcf_v[0][1] = t_v1; pcf_v[0][2] = t_v1; pcf_v[0][3] = t_v1;
        pcf_v[1][0] = t_v1; pcf_v[1][1] = t_v2; pcf_v[1][2] = t_v2; pcf_v[1][3] = t_v2;
        pcf_v[2][0] = t_v2; pcf_v[2][1] = t_v2; pcf_v[2][2] = v[i]; pcf_v[2][3] = v[i];
        pcf_v[3][0] = v[i]; pcf_v[3][1] = v[i]; pcf_v[3][2] = v[i]; pcf_v[3][3] = t_v3;
        pcf_v[4][0] = t_v3; pcf_v[4][1] = t_v3; pcf_v[4][2] = t_v3; pcf_v[4][3] = t_v3;
        pcf_v[5][0] = t_v4; pcf_v[5][1] = t_v4; pcf_v[5][2] = t_v4; pcf_v[5][3] = t_v4;
        pcf_v[6][0] = t_v4; pcf_v[6][1] = 0; pcf_v[6][2] = 0; pcf_v[6][3] = 0;

        for (u8 j = 0; j < 7; ++j)
        {
            _tex_clamp(pcf_u[j], pcf_v[j], pcf_u[j], pcf_v[j]);
            shadow_map_sun_sample_linear(shadow_map, pcf_u[j], pcf_v[j], compare, local_output);

            if (j < 6)
                output[i] += local_output[0] + local_output[1] + local_output[2] + local_output[3];
            else
                output[i] += local_output[0];
        }

        output[i] /= 25;
    }

#if 0
    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 base_u = _tex_clamp(texcoord->x[i]);
        r32 base_v = _tex_clamp(texcoord->y[i]);

        output[i] = 0;

        r32 tu[5] = {
            base_u - 2 * texture->header->tex_du,
            base_u - texture->header->tex_du,
            base_u,
            base_u + texture->header->tex_du,
            base_u + 2 * texture->header->tex_du,
        };

        r32 tv = base_v - 2 * texture->header->tex_dv;

        for (u8 ty = 0; ty < 5; ++ty)
        {
            for (u8 tx = 0; tx < 5; ++tx)
            {
                r32 texel_u = _tex_clamp(tu[tx]);
                r32 texel_v = _tex_clamp(tv);

                output[i] += shadow_map_sample_filter_single(shadow_map, tu[tx], tv, compare->data[i]);
            }

            tv += texture->header->tex_dv;
        }

        output[i] /= 25;
    }
#endif
}

void shadow_map_sun_sample_vms(texture2d_t *shadow_map, r32 *in_u, r32 *in_v, r32 *compare, r32 *output)
{
    OPTICK_EVENT("shadow_map_sun_sample_vms");

    lod_t lod;
    LOD_CLEAR(lod);

    shader_color_t moments;

    texture_sample(shadow_map, in_u, in_v, &lod, &moments);

    r32 min_variance = 0.0010f * 0.01f;
    r32 low = 0.7f;
    r32 high = 1.0f;

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        r32 d = compare[i] - moments.r[i];
        r32 p = moments.r[i] < compare[i] ? 0 : 1;
        r32 variance = moments.g[i] - moments.r[i] * moments.r[i];

        if (variance < min_variance)
            variance = min_variance;

        r32 p_max = variance / (variance + d * d);

        p_max = LINSTEP(low, high, p_max);
        p_max = clamp(low, high, p_max);

        r32 shadow = p > p_max ? p : p_max;
        shadow = LINSTEP(low, high, shadow);
        // shadow = clamp(light->shadow.min_shadow, 1.0f, shadow);

        // if (shadow < ln_dot->data[i])
        //     ln_dot->data[i] = shadow;

        output[i] = shadow;
    }
}

void shadow_map_point_sample(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("shadow_map_point_sample");

    fv2_t texcoord;
    u32 face_index[GC_FRAG_SIZE];
    r32 local_u[GC_FRAG_SIZE];
    r32 local_v[GC_FRAG_SIZE];

    cube_uv_from_vec(direction, &texcoord, face_index, false);
    _tex_clamp(texcoord.x, texcoord.y, local_u, local_v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        texture2d_t *face = shadow_map->faces[face_index[i]];
        texture_mip_t *texture = face->mips;
        texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

        u32 map_size = texture->header->width - 1;
        u32 map_pitch = texture->header->width;

        u32 x = (u32) (local_u[i] * map_size);
        u32 y = (u32) (local_v[i] * map_size);

        TEX_OFFSET(offset, map_pitch, x, y);
        texpixel_rgf_t *sample = texture_pointer + offset;
        output[i] = compare[i] > sample->r ? 0 : 1;
    }
}

void shadow_map_point_sample_linear(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("shadow_map_point_sample_linear");

    u32 face_index[GC_FRAG_SIZE];
    fv2_t texcoord;

    cube_uv_from_vec(direction, &texcoord, face_index, false);
    shadow_map_sample_filter_single(shadow_map, face_index, texcoord.x, texcoord.y, compare, output);
}

void shadow_map_point_sample_pcf(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("shadow_map_point_sample_pcf");

    r32 inv = 1.0f / 20;

    u32 map_pitch = shadow_map->faces[0]->mips->header->width;
    u32 map_size = map_pitch - 1;

    fv3_t current_direction;
    fv2_t texcoord;
    u32 face_index[GC_FRAG_SIZE];
    gc_vec_t base_dir;

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        output[i] = 0;
        base_dir.v3.x = direction->x[i];
        base_dir.v3.y = direction->y[i];
        base_dir.v3.z = direction->z[i];

        for (u8 s = 0; s < 5; ++s)
        {
            u32 current_index = s * 4;

            current_direction.x[0] = base_dir.v3.x + cube_direction_offset[current_index + 0].v3.x * radius;
            current_direction.y[0] = base_dir.v3.y + cube_direction_offset[current_index + 0].v3.y * radius;
            current_direction.z[0] = base_dir.v3.z + cube_direction_offset[current_index + 0].v3.z * radius;

            current_direction.x[1] = base_dir.v3.x + cube_direction_offset[current_index + 1].v3.x * radius;
            current_direction.y[1] = base_dir.v3.y + cube_direction_offset[current_index + 1].v3.y * radius;
            current_direction.z[1] = base_dir.v3.z + cube_direction_offset[current_index + 1].v3.z * radius;

            current_direction.x[2] = base_dir.v3.x + cube_direction_offset[current_index + 2].v3.x * radius;
            current_direction.y[2] = base_dir.v3.y + cube_direction_offset[current_index + 2].v3.y * radius;
            current_direction.z[2] = base_dir.v3.z + cube_direction_offset[current_index + 2].v3.z * radius;

            current_direction.x[3] = base_dir.v3.x + cube_direction_offset[current_index + 3].v3.x * radius;
            current_direction.y[3] = base_dir.v3.y + cube_direction_offset[current_index + 3].v3.y * radius;
            current_direction.z[3] = base_dir.v3.z + cube_direction_offset[current_index + 3].v3.z * radius;

            cube_uv_from_vec(&current_direction, &texcoord, face_index, false);
            _tex_clamp(texcoord.x, texcoord.y, texcoord.x, texcoord.y);

            for (u8 k = 0; k < GC_FRAG_SIZE; ++k)
            {
                texture2d_t *face = shadow_map->faces[face_index[k]];
                texture_mip_t *texture = face->mips;
                texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

                u32 x = (u32) (texcoord.x[k] * map_size);
                u32 y = (u32) (texcoord.y[k] * map_size);

                TEX_OFFSET(offset, map_pitch, x, y);
                texpixel_rgf_t *sample = texture_pointer + offset;
                output[i] += compare[i] > sample->r ? 0 : 1;
            }
        }

        output[i] *= inv;
    }
}

void shadow_map_point_sample_pcf_linear(cube_texture_t *shadow_map, fv3_t *direction, r32 *compare, r32 radius, r32 *output)
{
    OPTICK_EVENT("shadow_map_point_sample_pcf_linear");

    r32 inv = 1.0f / 20;

    // u32 map_pitch = shadow_map->faces[0]->mips->header->width;
    // u32 map_size = map_pitch - 1;

    fv3_t current_direction;
    fv2_t texcoord;
    u32 face_index[GC_FRAG_SIZE];
    r32 local_output[GC_FRAG_SIZE];

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        output[i] = 0;
        VINIT3(base_dir, direction->x[i], direction->y[i], direction->z[i]);

        for (u8 s = 0; s < 5; ++s)
        {
            u32 current_index = s * 4;

            current_direction.x[0] = base_dir.v3.x + cube_direction_offset[current_index + 0].v3.x * radius;
            current_direction.y[0] = base_dir.v3.y + cube_direction_offset[current_index + 0].v3.y * radius;
            current_direction.z[0] = base_dir.v3.z + cube_direction_offset[current_index + 0].v3.z * radius;

            current_direction.x[1] = base_dir.v3.x + cube_direction_offset[current_index + 1].v3.x * radius;
            current_direction.y[1] = base_dir.v3.y + cube_direction_offset[current_index + 1].v3.y * radius;
            current_direction.z[1] = base_dir.v3.z + cube_direction_offset[current_index + 1].v3.z * radius;

            current_direction.x[2] = base_dir.v3.x + cube_direction_offset[current_index + 2].v3.x * radius;
            current_direction.y[2] = base_dir.v3.y + cube_direction_offset[current_index + 2].v3.y * radius;
            current_direction.z[2] = base_dir.v3.z + cube_direction_offset[current_index + 2].v3.z * radius;

            current_direction.x[3] = base_dir.v3.x + cube_direction_offset[current_index + 3].v3.x * radius;
            current_direction.y[3] = base_dir.v3.y + cube_direction_offset[current_index + 3].v3.y * radius;
            current_direction.z[3] = base_dir.v3.z + cube_direction_offset[current_index + 3].v3.z * radius;

            cube_uv_from_vec(&current_direction, &texcoord, face_index, false);
            _tex_clamp(texcoord.x, texcoord.y, texcoord.x, texcoord.y);
            shadow_map_sample_filter_single(shadow_map, face_index, texcoord.x, texcoord.y, compare, local_output);
            output[i] += local_output[0] + local_output[1] + local_output[2] + local_output[3];
        }

        output[i] *= inv;
    }
}

// ----------------------------------------------------------------------------------
// -- Experimental.
// ----------------------------------------------------------------------------------

// void shadow_map_pcf_5x5_nofilter(texture2d_t *shadow_map, fv2_t *texcoord, gc_vec_t *compare, r32 *output)
// {
//     OPTICK_EVENT();

//     texture_mip_t *texture = shadow_map->mips;
//     texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

//     u32 map_size = texture->header->width - 1;
//     u32 map_pitch = texture->header->width;

//     r32 mult = 1.0f / 25;

//     for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
//     {
//         r32 base_u = _tex_clamp(texcoord->x[i]);
//         r32 base_v = _tex_clamp(texcoord->y[i]);

//         base_u -= -texture->header->tex_du + texture->header->tex_du;
//         base_v -= -texture->header->tex_dv + texture->header->tex_dv;

//         output[i] = 0;

//         for (u8 row = 0; row < 5; ++row)
//         {
//             r32 tu = base_u;
//             r32 tv = base_v;

//             for (u8 col = 0; col < 5; ++col)
//             {
//                 tu += texture->header->tex_du;
//                 r32 cmp = shadow_map_sample_single(shadow_map, tu, tv, compare->data[i]);
//                 output[i] += cmp;
//             }

//             base_v += texture->header->tex_dv;
//         }

//         // for (s32 ov = -2; ov <= 2; ++ov)
//         // {
//         //     for (s32 ou = -2; ou <= 2; ++ou)
//         //     {
//         //         r32 tu = _tex_clamp(base_u + ou * texture->header->tex_du);
//         //         r32 tv = _tex_clamp(base_v + ov * texture->header->tex_dv);

//         //         r32 cmp = shadow_map_sample_single(shadow_map, tu, tv, compare->data[i]);
//         //         // output[i] += cmp * weights_3x3.data[ov + 2][ou + 2];
//         //         output[i] += cmp;
//         //     }
//         // }

//         output[i] *= mult;
//     }
// }

// void shadow_map_pcf_3x3_optimized(texture2d_t *shadow_map, fv2_t *texcoord, gc_vec_t *compare, r32 *output)
// {
//     OPTICK_EVENT();

//     texture_mip_t *texture = shadow_map->mips;
//     texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

//     u32 map_size = texture->header->width - 1;
//     u32 map_pitch = texture->header->width;
//     r32 map_size_inv = 1.0f / map_size;

//     for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
//     {
//         r32 base_u = _tex_clamp(texcoord->x[i]);
//         r32 base_v = _tex_clamp(texcoord->y[i]);

//         r32 texel_x = base_u * map_size;
//         r32 texel_y = base_v * map_size;

//         u32 x = (u32) texel_x;
//         u32 y = (u32) texel_y;

//         r32 s = texel_x - x;
//         r32 t = texel_y - y;

//         // base_u = (x - 0.5f) * map_size_inv;
//         // base_v = (y - 0.5f) * map_size_inv;

//         output[i] = 0;

//         r32 uw0 = (3 - 2 * s);
//         r32 uw1 = (1 + 2 * s);

//         r32 u0 = (2 - s) / uw0 - 1;
//         r32 u1 = s / uw1 + 1;

//         r32 vw0 = (3 - 2 * t);
//         r32 vw1 = (1 + 2 * t);

//         r32 v0 = (2 - t) / vw0 - 1;
//         r32 v1 = t / vw1 + 1;

//         u0 *= map_size_inv;
//         u1 *= map_size_inv;
//         v0 *= map_size_inv;
//         v1 *= map_size_inv;

//         output[i] += uw0 * vw0 * shadow_map_sample_single(shadow_map, base_u + u0, base_v + v0, compare->data[i]);
//         output[i] += uw1 * vw0 * shadow_map_sample_single(shadow_map, base_u + u1, base_v + v0, compare->data[i]);
//         output[i] += uw0 * vw1 * shadow_map_sample_single(shadow_map, base_u + u0, base_v + v1, compare->data[i]);
//         output[i] += uw1 * vw1 * shadow_map_sample_single(shadow_map, base_u + u1, base_v + v1, compare->data[i]);

//         output[i] /= 16.0f;
//     }
// }

// void shadow_map_pcf_5x5_optimized(texture2d_t *shadow_map, fv2_t *texcoord, gc_vec_t *compare, r32 *output)
// {
//     OPTICK_EVENT();

//     texture_mip_t *texture = shadow_map->mips;
//     texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) texture->data;

//     u32 map_size = texture->header->width;
//     u32 map_pitch = texture->header->width;
//     r32 map_size_inv = 1.0f / map_size;

//     for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
//     {
//         r32 base_u = _tex_clamp(texcoord->x[i]);
//         r32 base_v = _tex_clamp(texcoord->y[i]);

//         r32 texel_x = base_u * map_size;
//         r32 texel_y = base_v * map_size;

//         u32 x = (u32) (texel_x + 0.5f);
//         u32 y = (u32) (texel_y + 0.5f);

//         r32 s = texel_x + 0.5f - x;
//         r32 t = texel_y + 0.5f - y;

//         base_u = (x - 0.5f) * map_size_inv;
//         base_v = (y - 0.5f) * map_size_inv;

//         r32 uw0 = (4 - 3 * s);
//         r32 uw1 = 7;
//         r32 uw2 = (1 + 3 * s);

//         r32 u0 = (3 - 2 * s) / uw0 - 2;
//         r32 u1 = (3 + s) / uw1;
//         r32 u2 = s / uw2 + 2;

//         r32 vw0 = (4 - 3 * t);
//         r32 vw1 = 7;
//         r32 vw2 = (1 + 3 * t);

//         r32 v0 = (3 - 2 * t) / vw0 - 2;
//         r32 v1 = (3 + t) / vw1;
//         r32 v2 = t / vw2 + 2;

//         u0 *= map_size_inv;
//         u1 *= map_size_inv;
//         u2 *= map_size_inv;
//         v0 *= map_size_inv;
//         v1 *= map_size_inv;
//         v2 *= map_size_inv;

//         output[i] = 0;

//         output[i] += uw0 * vw0 * shadow_map_sample_single(shadow_map, base_u + u0, base_v + v0, compare->data[i]);
//         output[i] += uw1 * vw0 * shadow_map_sample_single(shadow_map, base_u + u1, base_v + v0, compare->data[i]);
//         output[i] += uw2 * vw0 * shadow_map_sample_single(shadow_map, base_u + u2, base_v + v0, compare->data[i]);

//         output[i] += uw0 * vw1 * shadow_map_sample_single(shadow_map, base_u + u0, base_v + v1, compare->data[i]);
//         output[i] += uw1 * vw1 * shadow_map_sample_single(shadow_map, base_u + u1, base_v + v1, compare->data[i]);
//         output[i] += uw2 * vw1 * shadow_map_sample_single(shadow_map, base_u + u2, base_v + v1, compare->data[i]);

//         output[i] += uw0 * vw2 * shadow_map_sample_single(shadow_map, base_u + u0, base_v + v2, compare->data[i]);
//         output[i] += uw1 * vw2 * shadow_map_sample_single(shadow_map, base_u + u1, base_v + v2, compare->data[i]);
//         output[i] += uw2 * vw2 * shadow_map_sample_single(shadow_map, base_u + u2, base_v + v2, compare->data[i]);

//         output[i] /= 144.0f;
//     }
// }

// ----------------------------------------------------------------------------------
// -- Texture sample.
// ----------------------------------------------------------------------------------

void _sample_rgbau8(texture2d_t *texture, r32 *in_u, r32 *in_v, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_sample_rgbau8");

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        texture_mip_t *selected_texture = texture->mips + lod[i];
        u32 *texture_pointer = (u32 *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        u32 sample0 = texture_pointer[offset0];

        texpixel_rgbaf_t sample0v;
        gl_unpack(sample0v, sample0);

        if (texture->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - x;
            r32 ty = tex_y - y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            filter_u[0] = u[i];
            filter_u[1] = u[i] + selected_texture->header->tex_du;
            filter_u[2] = u[i];
            filter_u[3] = u[i] + selected_texture->header->tex_du;

            filter_v[0] = v[i];
            filter_v[1] = v[i];
            filter_v[2] = v[i] + selected_texture->header->tex_dv;
            filter_v[3] = v[i] + selected_texture->header->tex_dv;

            texture->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            u32 sample1 = texture_pointer[offset1];
            u32 sample2 = texture_pointer[offset2];
            u32 sample3 = texture_pointer[offset3];

            texpixel_rgbaf_t sample1v;
            gl_unpack(sample1v, sample1);

            texpixel_rgbaf_t sample2v;
            gl_unpack(sample2v, sample2);

            texpixel_rgbaf_t sample3v;
            gl_unpack(sample3v, sample3);

            texpixel_rgbaf_t t1;
            texpixel_rgbaf_t t2;

            t1.r = sample0v.r + (sample1v.r - sample0v.r) * tx;
            t1.g = sample0v.g + (sample1v.g - sample0v.g) * tx;
            t1.b = sample0v.b + (sample1v.b - sample0v.b) * tx;
            t1.a = sample0v.a + (sample1v.a - sample0v.a) * tx;

            t2.r = sample2v.r + (sample3v.r - sample2v.r) * tx;
            t2.g = sample2v.g + (sample3v.g - sample2v.g) * tx;
            t2.b = sample2v.b + (sample3v.b - sample2v.b) * tx;
            t2.a = sample2v.a + (sample3v.a - sample2v.a) * tx;

            sample0v.r = t1.r + (t2.r - t1.r) * ty;
            sample0v.g = t1.g + (t2.g - t1.g) * ty;
            sample0v.b = t1.b + (t2.b - t1.b) * ty;
            sample0v.a = t1.a + (t2.a - t1.a) * ty;
        }

        output->r[i] = sample0v.r;
        output->g[i] = sample0v.g;
        output->b[i] = sample0v.b;
        output->a[i] = sample0v.a;
    }
}

void _sample_rgbaf(texture2d_t *texture, r32 *in_u, r32 *in_v, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_sample_rgbaf");

    // ----------------------------------------------------------------------------------
    // -- Min lod sample
    // ----------------------------------------------------------------------------------

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        texture_mip_t *selected_texture = texture->mips + lod[i];
        texpixel_rgbaf_t *texture_pointer = (texpixel_rgbaf_t *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        texpixel_rgbaf_t *sample0v = texture_pointer + offset0;

        if (texture->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - x;
            r32 ty = tex_y - y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            filter_u[0] = u[i];
            filter_u[1] = u[i] + selected_texture->header->tex_du;
            filter_u[2] = u[i];
            filter_u[3] = u[i] + selected_texture->header->tex_du;

            filter_v[0] = v[i];
            filter_v[1] = v[i];
            filter_v[2] = v[i] + selected_texture->header->tex_dv;
            filter_v[3] = v[i] + selected_texture->header->tex_dv;

            texture->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            texpixel_rgbaf_t *sample1v = texture_pointer + offset1;
            texpixel_rgbaf_t *sample2v = texture_pointer + offset2;
            texpixel_rgbaf_t *sample3v = texture_pointer + offset3;

            texpixel_rgbaf_t t1;
            texpixel_rgbaf_t t2;

            t1.r = sample0v->r + (sample1v->r - sample0v->r) * tx;
            t1.g = sample0v->g + (sample1v->g - sample0v->g) * tx;
            t1.b = sample0v->b + (sample1v->b - sample0v->b) * tx;
            t1.a = sample0v->a + (sample1v->a - sample0v->a) * tx;

            t2.r = sample2v->r + (sample3v->r - sample2v->r) * tx;
            t2.g = sample2v->g + (sample3v->g - sample2v->g) * tx;
            t2.b = sample2v->b + (sample3v->b - sample2v->b) * tx;
            t2.a = sample2v->a + (sample3v->a - sample2v->a) * tx;

            output->r[i] = t1.r + (t2.r - t1.r) * ty;
            output->g[i] = t1.g + (t2.g - t1.g) * ty;
            output->b[i] = t1.b + (t2.b - t1.b) * ty;
            output->a[i] = t1.a + (t2.a - t1.a) * ty;
        }
        else
        {
            output->r[i] = sample0v->r;
            output->g[i] = sample0v->g;
            output->b[i] = sample0v->b;
            output->a[i] = sample0v->a;
        }
    }
}

void _sample_rgbf(texture2d_t *texture, r32 *in_u, r32 *in_v, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_sample_rgbf");

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        texture_mip_t *selected_texture = texture->mips + lod[i];
        texpixel_rgbf_t *texture_pointer = (texpixel_rgbf_t *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        texpixel_rgbf_t *sample0v = texture_pointer + offset0;

        if (texture->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - x;
            r32 ty = tex_y - y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            filter_u[0] = u[i];
            filter_u[1] = u[i] + selected_texture->header->tex_du;
            filter_u[2] = u[i];
            filter_u[3] = u[i] + selected_texture->header->tex_du;

            filter_v[0] = v[i];
            filter_v[1] = v[i];
            filter_v[2] = v[i] + selected_texture->header->tex_dv;
            filter_v[3] = v[i] + selected_texture->header->tex_dv;

            texture->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            texpixel_rgbf_t *sample1v = texture_pointer + offset1;
            texpixel_rgbf_t *sample2v = texture_pointer + offset2;
            texpixel_rgbf_t *sample3v = texture_pointer + offset3;

            texpixel_rgbf_t t1;
            texpixel_rgbf_t t2;

            t1.r = sample0v->r + (sample1v->r - sample0v->r) * tx;
            t1.g = sample0v->g + (sample1v->g - sample0v->g) * tx;
            t1.b = sample0v->b + (sample1v->b - sample0v->b) * tx;

            t2.r = sample2v->r + (sample3v->r - sample2v->r) * tx;
            t2.g = sample2v->g + (sample3v->g - sample2v->g) * tx;
            t2.b = sample2v->b + (sample3v->b - sample2v->b) * tx;

            output->r[i] = t1.r + (t2.r - t1.r) * ty;
            output->g[i] = t1.g + (t2.g - t1.g) * ty;
            output->b[i] = t1.b + (t2.b - t1.b) * ty;
            output->a[i] = 0;
        }
        else
        {
            output->r[i] = sample0v->r;
            output->g[i] = sample0v->g;
            output->b[i] = sample0v->b;
            output->a[i] = 0;
        }
    }
}

void _sample_rgf(texture2d_t *texture, r32 *in_u, r32 *in_v, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_sample_rgf");

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        texture_mip_t *selected_texture = texture->mips + lod[i];
        texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        texpixel_rgf_t *sample0v = texture_pointer + offset0;

        if (texture->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - x;
            r32 ty = tex_y - y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            A4SET(filter_u, u[i], u[i] + selected_texture->header->tex_du, u[i], u[i] + selected_texture->header->tex_du);
            A4SET(filter_v, v[i], v[i], v[i] + selected_texture->header->tex_dv, v[i] + selected_texture->header->tex_dv);

            texture->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            texpixel_rgf_t *sample1v = texture_pointer + offset1;
            texpixel_rgf_t *sample2v = texture_pointer + offset2;
            texpixel_rgf_t *sample3v = texture_pointer + offset3;

            texpixel_rgf_t t1;
            texpixel_rgf_t t2;

            t1.r = sample0v->r + (sample1v->r - sample0v->r) * tx;
            t1.g = sample0v->g + (sample1v->g - sample0v->g) * tx;

            t2.r = sample2v->r + (sample3v->r - sample2v->r) * tx;
            t2.g = sample2v->g + (sample3v->g - sample2v->g) * tx;

            output->r[i] = t1.r + (t2.r - t1.r) * ty;
            output->g[i] = t1.g + (t2.g - t1.g) * ty;
            output->b[i] = 0;
            output->a[i] = 0;
        }
        else
        {
            output->r[i] = sample0v->r;
            output->g[i] = sample0v->g;
            output->b[i] = 0;
            output->a[i] = 0;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Cube sample.
// ----------------------------------------------------------------------------------

void _cube_sample_rgbau8(cube_texture_t *cubemap, r32 *in_u, r32 *in_v, u32 *face_index, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_cube_sample_rgbau8");

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture2d_t *face = cubemap->faces[0];
    face->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        face = cubemap->faces[face_index[i]];
        texture_mip_t *selected_texture = face->mips + lod[i];
        u32 *texture_pointer = (u32 *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        u32 sample0 = texture_pointer[offset0];

        texpixel_rgbaf_t sample0v;
        gl_unpack(sample0v, sample0);

        if (face->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - (u32) tex_x;
            r32 ty = tex_y - (u32) tex_y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            A4SET(filter_u, u[i], u[i] + selected_texture->header->tex_du, u[i], u[i] + selected_texture->header->tex_du);
            A4SET(filter_v, v[i], v[i], v[i] + selected_texture->header->tex_dv, v[i] + selected_texture->header->tex_dv);

            face->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            u32 sample1 = texture_pointer[offset1];
            u32 sample2 = texture_pointer[offset2];
            u32 sample3 = texture_pointer[offset3];

            texpixel_rgbaf_t sample1v;
            texpixel_rgbaf_t sample2v;
            texpixel_rgbaf_t sample3v;

            gl_unpack(sample1v, sample1);
            gl_unpack(sample2v, sample2);
            gl_unpack(sample3v, sample3);

            texpixel_rgbaf_t t1;
            texpixel_rgbaf_t t2;

            t1.r = sample0v.r + (sample1v.r - sample0v.r) * tx;
            t1.g = sample0v.g + (sample1v.g - sample0v.g) * tx;
            t1.b = sample0v.b + (sample1v.b - sample0v.b) * tx;
            t1.a = sample0v.a + (sample1v.a - sample0v.a) * tx;

            t2.r = sample2v.r + (sample3v.r - sample2v.r) * tx;
            t2.g = sample2v.g + (sample3v.g - sample2v.g) * tx;
            t2.b = sample2v.b + (sample3v.b - sample2v.b) * tx;
            t2.a = sample2v.a + (sample3v.a - sample2v.a) * tx;

            output->r[i] = t1.r + (t2.r - t1.r) * ty;
            output->g[i] = t1.g + (t2.g - t1.g) * ty;
            output->b[i] = t1.b + (t2.b - t1.b) * ty;
            output->a[i] = t1.a + (t2.a - t1.a) * ty;
        }
        else
        {
            output->r[i] = sample0v.r;
            output->g[i] = sample0v.g;
            output->b[i] = sample0v.b;
            output->a[i] = sample0v.a;
        }
    }
}

void _cube_sample_rgbaf(cube_texture_t *cubemap, r32 *in_u, r32 *in_v, u32 *face_index, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_cube_sample_rgbaf");

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture2d_t *face = cubemap->faces[0];
    face->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        face = cubemap->faces[face_index[i]];
        texture_mip_t *selected_texture = face->mips + lod[i];
        texpixel_rgbaf_t *texture_pointer = (texpixel_rgbaf_t *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        texpixel_rgbaf_t *sample0v = texture_pointer + offset0;

        if (cubemap->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - (u32) tex_x;
            r32 ty = tex_y - (u32) tex_y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            A4SET(filter_u, u[i], u[i] + selected_texture->header->tex_du, u[i], u[i] + selected_texture->header->tex_du);
            A4SET(filter_v, v[i], v[i], v[i] + selected_texture->header->tex_dv, v[i] + selected_texture->header->tex_dv);

            face->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            texpixel_rgbaf_t *sample1v = texture_pointer + offset1;
            texpixel_rgbaf_t *sample2v = texture_pointer + offset2;
            texpixel_rgbaf_t *sample3v = texture_pointer + offset3;

            texpixel_rgbaf_t t1;
            texpixel_rgbaf_t t2;

            t1.r = sample0v->r + (sample1v->r - sample0v->r) * tx;
            t1.g = sample0v->g + (sample1v->g - sample0v->g) * tx;
            t1.b = sample0v->b + (sample1v->b - sample0v->b) * tx;
            t1.a = sample0v->a + (sample1v->a - sample0v->a) * tx;

            t2.r = sample2v->r + (sample3v->r - sample2v->r) * tx;
            t2.g = sample2v->g + (sample3v->g - sample2v->g) * tx;
            t2.b = sample2v->b + (sample3v->b - sample2v->b) * tx;
            t2.a = sample2v->a + (sample3v->a - sample2v->a) * tx;

            output->r[i] = t1.r + (t2.r - t1.r) * ty;
            output->g[i] = t1.g + (t2.g - t1.g) * ty;
            output->b[i] = t1.b + (t2.b - t1.b) * ty;
            output->a[i] = t1.a + (t2.a - t1.a) * ty;
        }
        else
        {
            output->r[i] = sample0v->r;
            output->g[i] = sample0v->g;
            output->b[i] = sample0v->b;
            output->a[i] = sample0v->a;
        }
    }
}

void _cube_sample_rgbf(cube_texture_t *cubemap, r32 *in_u, r32 *in_v, u32 *face_index, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_cube_sample_rgbf");

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture2d_t *face = cubemap->faces[0];
    face->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        face = cubemap->faces[face_index[i]];
        texture_mip_t *selected_texture = face->mips + lod[i];
        texpixel_rgbf_t *texture_pointer = (texpixel_rgbf_t *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        texpixel_rgbf_t *sample0v = texture_pointer + offset0;

        if (cubemap->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - (u32) tex_x;
            r32 ty = tex_y - (u32) tex_y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            A4SET(filter_u, u[i], u[i] + selected_texture->header->tex_du, u[i], u[i] + selected_texture->header->tex_du);
            A4SET(filter_v, v[i], v[i], v[i] + selected_texture->header->tex_dv, v[i] + selected_texture->header->tex_dv);

            face->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            texpixel_rgbf_t *sample1v = texture_pointer + offset1;
            texpixel_rgbf_t *sample2v = texture_pointer + offset2;
            texpixel_rgbf_t *sample3v = texture_pointer + offset3;

            texpixel_rgbf_t t1;
            texpixel_rgbf_t t2;

            t1.r = sample0v->r + (sample1v->r - sample0v->r) * tx;
            t1.g = sample0v->g + (sample1v->g - sample0v->g) * tx;
            t1.b = sample0v->b + (sample1v->b - sample0v->b) * tx;

            t2.r = sample2v->r + (sample3v->r - sample2v->r) * tx;
            t2.g = sample2v->g + (sample3v->g - sample2v->g) * tx;
            t2.b = sample2v->b + (sample3v->b - sample2v->b) * tx;

            output->r[i] = t1.r + (t2.r - t1.r) * ty;
            output->g[i] = t1.g + (t2.g - t1.g) * ty;
            output->b[i] = t1.b + (t2.b - t1.b) * ty;
            output->a[i] = 0;
        }
        else
        {
            output->r[i] = sample0v->r;
            output->g[i] = sample0v->g;
            output->b[i] = sample0v->b;
            output->a[i] = 0;
        }
    }
}

void _cube_sample_rgf(cube_texture_t *cubemap, r32 *in_u, r32 *in_v, u32 *face_index, u32 *lod, shader_color_t *output)
{
    OPTICK_EVENT("_cube_sample_rgf");

    r32 u[GC_FRAG_SIZE];
    r32 v[GC_FRAG_SIZE];

    texture2d_t *face = cubemap->faces[0];
    face->settings.tex_wrap(in_u, in_v, u, v);

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        face = cubemap->faces[face_index[i]];
        texture_mip_t *selected_texture = face->mips + lod[i];
        texpixel_rgf_t *texture_pointer = (texpixel_rgf_t *) selected_texture->data;

        u32 tex_width = selected_texture->header->width - 1;
        u32 tex_height = selected_texture->header->height - 1;

        r32 tex_x = u[i] * tex_width;
        r32 tex_y = v[i] * tex_height;

        u32 x = (u32) (tex_x);
        u32 y = (u32) (tex_y);

        TEX_OFFSET(offset0, selected_texture->header->width, x, y);
        texpixel_rgf_t *sample0v = texture_pointer + offset0;

        if (cubemap->settings.flags & TEXTURE_FILTER)
        {
            r32 tx = tex_x - (u32) tex_x;
            r32 ty = tex_y - (u32) tex_y;

            r32 filter_u[GC_FRAG_SIZE];
            r32 filter_v[GC_FRAG_SIZE];

            A4SET(filter_u, u[i], u[i] + selected_texture->header->tex_du, u[i], u[i] + selected_texture->header->tex_du);
            A4SET(filter_v, v[i], v[i], v[i] + selected_texture->header->tex_dv, v[i] + selected_texture->header->tex_dv);

            face->settings.tex_wrap(filter_u, filter_v, filter_u, filter_v);

            u32 x1 = (u32) (filter_u[1] * tex_width);
            u32 y1 = (u32) (filter_v[1] * tex_height);

            u32 x2 = (u32) (filter_u[2] * tex_width);
            u32 y2 = (u32) (filter_v[2] * tex_height);

            u32 x3 = (u32) (filter_u[3] * tex_width);
            u32 y3 = (u32) (filter_v[3] * tex_height);

            TEX_OFFSET(offset1, selected_texture->header->width, x1, y1);
            TEX_OFFSET(offset2, selected_texture->header->width, x2, y2);
            TEX_OFFSET(offset3, selected_texture->header->width, x3, y3);

            texpixel_rgf_t *sample1v = texture_pointer + offset1;
            texpixel_rgf_t *sample2v = texture_pointer + offset2;
            texpixel_rgf_t *sample3v = texture_pointer + offset3;

            texpixel_rgf_t t1;
            texpixel_rgf_t t2;

            t1.r = sample0v->r + (sample1v->r - sample0v->r) * tx;
            t1.g = sample0v->g + (sample1v->g - sample0v->g) * tx;

            t2.r = sample2v->r + (sample3v->r - sample2v->r) * tx;
            t2.g = sample2v->g + (sample3v->g - sample2v->g) * tx;

            output->r[i] = t1.r + (t2.r - t1.r) * ty;
            output->g[i] = t1.g + (t2.g - t1.g) * ty;
            output->b[i] = 0;
            output->a[i] = 0;
        }
        else
        {
            output->r[i] = sample0v->r;
            output->g[i] = sample0v->g;
            output->b[i] = 0;
            output->a[i] = 0;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Sampling.
// ----------------------------------------------------------------------------------

void texture_sample(texture2d_t *texture, r32 *in_u, r32 *in_v, lod_t *lod, shader_color_t *output)
{
    OPTICK_EVENT("texture_sample");

    if (!(texture->settings.flags & TEXTURE_MIPS))
    {
        lod->low[0] = 0;
        lod->low[1] = 0;
        lod->low[2] = 0;
        lod->low[3] = 0;

        lod->high[0] = 0;
        lod->high[1] = 0;
        lod->high[2] = 0;
        lod->high[3] = 0;

        lod->interp[0] = 0;
        lod->interp[1] = 0;
        lod->interp[2] = 0;
        lod->interp[3] = 0;
    }
    else
    {
        if (lod->high[0] < lod->low[0]) lod->high[0] = lod->low[0];
        if (lod->high[1] < lod->low[1]) lod->high[1] = lod->low[1];
        if (lod->high[2] < lod->low[2]) lod->high[2] = lod->low[2];
        if (lod->high[3] < lod->low[3]) lod->high[3] = lod->low[3];
    }

    texture->settings.tex_sample(texture, in_u, in_v, lod->low, output);

    // NOTE(gabic): For now all the channels from the output are computed, ignoring the
    // texture pixel format.

    if (((texture->settings.flags & (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)) == (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)))
    {
        shader_color_t output_max;
        texture->settings.tex_sample(texture, in_u, in_v, lod->high, &output_max);

        output->r[0] += (output_max.r[0] - output->r[0]) * lod->interp[0];
        output->r[1] += (output_max.r[1] - output->r[1]) * lod->interp[1];
        output->r[2] += (output_max.r[2] - output->r[2]) * lod->interp[2];
        output->r[3] += (output_max.r[3] - output->r[3]) * lod->interp[3];

        output->g[0] += (output_max.g[0] - output->g[0]) * lod->interp[0];
        output->g[1] += (output_max.g[1] - output->g[1]) * lod->interp[1];
        output->g[2] += (output_max.g[2] - output->g[2]) * lod->interp[2];
        output->g[3] += (output_max.g[3] - output->g[3]) * lod->interp[3];

        output->b[0] += (output_max.b[0] - output->b[0]) * lod->interp[0];
        output->b[1] += (output_max.b[1] - output->b[1]) * lod->interp[1];
        output->b[2] += (output_max.b[2] - output->b[2]) * lod->interp[2];
        output->b[3] += (output_max.b[3] - output->b[3]) * lod->interp[3];

        output->a[0] += (output_max.a[0] - output->a[0]) * lod->interp[0];
        output->a[1] += (output_max.a[1] - output->a[1]) * lod->interp[1];
        output->a[2] += (output_max.a[2] - output->a[2]) * lod->interp[2];
        output->a[3] += (output_max.a[3] - output->a[3]) * lod->interp[3];
    }
}

void brdf_lut_sample(texture2d_t *lut, gc_vec_t *cos_theta, gc_vec_t *roughness, fv2_t *output)
{
    brdf_lut_pixel_t *pointer = (brdf_lut_pixel_t *) lut->mips->data;

    for (u8 i = 0; i < GC_FRAG_SIZE; ++i)
    {
        u32 texel_x = (u32) (cos_theta->data[i] * (lut->mips->header->width - 1));
        u32 texel_y = (u32) (roughness->data[i] * (lut->mips->header->height - 1));
        u32 offset = texel_y * lut->mips->header->width + texel_x;

        brdf_lut_pixel_t *pixel = pointer + offset;

        output->x[i] = pixel->a;
        output->y[i] = pixel->b;
    }
}

void cube_texture_sample(cube_texture_t *cubemap, fv3_t *position, lod_t *lod, b8 warp, shader_color_t *output)
{
    OPTICK_EVENT("cube_texture_sample");

    u32 face_index[GC_FRAG_SIZE];
    fv2_t texcoord;

    if (!(cubemap->settings.flags & TEXTURE_MIPS))
    {
        lod->low[0] = 0;
        lod->low[1] = 0;
        lod->low[2] = 0;
        lod->low[3] = 0;

        lod->high[0] = 0;
        lod->high[1] = 0;
        lod->high[2] = 0;
        lod->high[3] = 0;

        lod->interp[0] = 0;
        lod->interp[1] = 0;
        lod->interp[2] = 0;
        lod->interp[3] = 0;
    }
    else
    {
        if (lod->high[0] < lod->low[0]) lod->high[0] = lod->low[0];
        if (lod->high[1] < lod->low[1]) lod->high[1] = lod->low[1];
        if (lod->high[2] < lod->low[2]) lod->high[2] = lod->low[2];
        if (lod->high[3] < lod->low[3]) lod->high[3] = lod->low[3];
    }

    cube_uv_from_vec(position, &texcoord, face_index, warp);
    cubemap->settings.tex_cube_sample(cubemap, texcoord.x, texcoord.y, face_index, lod->low, output);

    // NOTE(gabic): For now only 3 channels from the output are computed, ignoring the
    // texture pixel format.

    if (((cubemap->settings.flags & (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)) == (TEXTURE_MIPS | TEXTURE_MIPS_FILTER)))
    {
        shader_color_t output_max;
        cubemap->settings.tex_cube_sample(cubemap, texcoord.x, texcoord.y, face_index, lod->high, &output_max);

        output->r[0] += (output_max.r[0] - output->r[0]) * lod->interp[0];
        output->r[1] += (output_max.r[1] - output->r[1]) * lod->interp[1];
        output->r[2] += (output_max.r[2] - output->r[2]) * lod->interp[2];
        output->r[3] += (output_max.r[3] - output->r[3]) * lod->interp[3];

        output->g[0] += (output_max.g[0] - output->g[0]) * lod->interp[0];
        output->g[1] += (output_max.g[1] - output->g[1]) * lod->interp[1];
        output->g[2] += (output_max.g[2] - output->g[2]) * lod->interp[2];
        output->g[3] += (output_max.g[3] - output->g[3]) * lod->interp[3];

        output->b[0] += (output_max.b[0] - output->b[0]) * lod->interp[0];
        output->b[1] += (output_max.b[1] - output->b[1]) * lod->interp[1];
        output->b[2] += (output_max.b[2] - output->b[2]) * lod->interp[2];
        output->b[3] += (output_max.b[3] - output->b[3]) * lod->interp[3];
    }
}

__INLINE__ void gc_texture2d_setup(texture2d_t *texture)
{
#if defined(GC_PIPE_AVX)
#elif defined(GC_PIPE_SSE)

    if (texture->settings.flags & TEXTURE_WRAP_CLAMP)
        texture->settings.tex_wrap = _sse_tex_clamp;
    else if (texture->settings.flags & TEXTURE_WRAP_REPEAT)
        texture->settings.tex_wrap = _sse_tex_repeat;
    else if (texture->settings.flags & TEXTURE_WRAP_MIRROR)
        texture->settings.tex_wrap = _sse_tex_mirror;

    if (texture->settings.format == TEXTURE_FORMAT_RGBAF)
    {
        texture->settings.tex_clear = _sse_clear_rgbaf;
        texture->settings.tex_sample = _sse_sample_rgbaf;
    }
    else if (texture->settings.format == TEXTURE_FORMAT_RGBF)
    {
        texture->settings.tex_clear = _sse_clear_rgbf;
        texture->settings.tex_sample = _sse_sample_rgbf;
    }
    else if (texture->settings.format == TEXTURE_FORMAT_RGF)
    {
        texture->settings.tex_clear = _sse_clear_rgf;
        texture->settings.tex_sample = _sse_sample_rgf;
    }
    else
    {
        texture->settings.tex_clear = _sse_clear_rgbau8;
        texture->settings.tex_sample = _sse_sample_rgbau8;
        // texture->settings.tex_sample = _sample_rgbau8;
    }

#else

    if (texture->settings.flags & TEXTURE_WRAP_CLAMP)
        texture->settings.tex_wrap = _tex_clamp;
    else if (texture->settings.flags & TEXTURE_WRAP_REPEAT)
        texture->settings.tex_wrap = _tex_repeat;
    else if (texture->settings.flags & TEXTURE_WRAP_MIRROR)
        texture->settings.tex_wrap = _tex_mirror;

    if (texture->settings.format == TEXTURE_FORMAT_RGBAF)
    {
        texture->settings.tex_clear = _clear_rgbaf;
        texture->settings.tex_sample = _sample_rgbaf;
    }
    else if (texture->settings.format == TEXTURE_FORMAT_RGBF)
    {
        texture->settings.tex_clear = _clear_rgbf;
        texture->settings.tex_sample = _sample_rgbf;
    }
    else if (texture->settings.format == TEXTURE_FORMAT_RGF)
    {
        texture->settings.tex_clear = _clear_rgf;
        texture->settings.tex_sample = _sample_rgf;
    }
    else
    {
        texture->settings.tex_clear = _clear_rgbau8;
        texture->settings.tex_sample = _sample_rgbau8;
    }

#endif
}

__INLINE__ void gc_cube_texture_setup(cube_texture_t *texture)
{
    #if defined(GC_PIPE_SSE)

    if (texture->settings.format == TEXTURE_FORMAT_RGBAF)
        texture->settings.tex_cube_sample = _sse_cube_sample_rgbaf;
    else if (texture->settings.format == TEXTURE_FORMAT_RGF)
        texture->settings.tex_cube_sample = _sse_cube_sample_rgf;
    else if (texture->settings.format == TEXTURE_FORMAT_RGBF)
        texture->settings.tex_cube_sample = _sse_cube_sample_rgbf;
    else
        texture->settings.tex_cube_sample = _sse_cube_sample_rgbau8;

    #else

    if (texture->settings.format == TEXTURE_FORMAT_RGBAF)
        texture->settings.tex_cube_sample = _cube_sample_rgbaf;
    else if (texture->settings.format == TEXTURE_FORMAT_RGF)
        texture->settings.tex_cube_sample = _cube_sample_rgf;
    else if (texture->settings.format == TEXTURE_FORMAT_RGBF)
        texture->settings.tex_cube_sample = _cube_sample_rgbf;
    else
        texture->settings.tex_cube_sample = _cube_sample_rgbau8;

    #endif
}

__INLINE__ size_t _texture_format_data_block_bytes(u32 width, u32 height, u32 format)
{
    size_t bytes = 0;

    if (format == TEXTURE_FORMAT_RGBAU8) {
        bytes = MEM_SIZE_ALIGN(TEX_DATA_BYTES_RGBAU8(width, height));
    }
    else if (format == TEXTURE_FORMAT_RGBAF) {
        bytes = MEM_SIZE_ALIGN(TEX_DATA_BYTES_RGBAF(width, height));
    }
    else if (format == TEXTURE_FORMAT_RGBF) {
        bytes = MEM_SIZE_ALIGN(TEX_DATA_BYTES_RGBF(width, height));
    }
    else if (format == TEXTURE_FORMAT_RGF) {
        bytes = MEM_SIZE_ALIGN(TEX_DATA_BYTES_RGF(width, height));
    }

    return bytes;
}

__INLINE__ size_t texture_total_data_bytes(u32 width, u32 height, u32 mips, u32 format)
{
    size_t bytes = _texture_format_data_block_bytes(width, height, format);
    u32 mip_size = width >> 1;

    for (u8 i = 1; i < mips; ++i)
    {
        bytes += _texture_format_data_block_bytes(mip_size, mip_size, format);
        mip_size = mip_size >> 1;
    }

    return bytes;
}

// ----------------------------------------------------------------------------------
// -- [texture2d_t][texture_mip_t...][texture_data_header_t...][data...]
// ----------------------------------------------------------------------------------

texture2d_t *gc_create_texture2d(u32 width, u32 height, u32 mips, u32 format, u32 flags)
{
    // Mips work only for square textures.
    if (width != height || !mips)
        mips = 1;

    u32 mip_size = width;
    size_t data_offset = 0;

    size_t header_bytes = MEM_SIZE_ALIGN(TEX_HEADER_BYTES(mips));
    size_t data_bytes = texture_total_data_bytes(width, height, mips, format);
    size_t total_bytes = header_bytes + data_bytes;

    mem_set_chunk(MEMORY_ASSETS);
    MEM_LABEL("texture2d");
    texture2d_t *texture = (texture2d_t *) gc_mem_allocate(total_bytes);
    mem_restore_chunk();

    #ifdef GCSR_ASSET_BUILDER_H
    memset(texture, 0, total_bytes);
    #endif

    texture->data_bytes = data_bytes;
    texture->mips = (texture_mip_t *) (texture + 1);
    texture->mip_count = mips;
    void *data_pointer = ADDR_OFFSET(texture, header_bytes);

    texture->type = ASSET_TEXTURE2D;
    texture->settings.flags = flags;
    texture->settings.format = format;

    #ifndef GCSR_ASSET_BUILDER_H
    gc_texture2d_setup(texture);
    #endif

    texture_mip_t *base_level = texture->mips;
    base_level->header = (texture_data_header_t *) ADDR_OFFSET(data_pointer, data_offset);
    base_level->data = base_level->header + 1;
    base_level->header->width = width;
    base_level->header->height = height;
    base_level->header->tex_du = 1.0f / width;
    base_level->header->tex_dv = 1.0f / height;
    base_level->header->bytes = _texture_format_data_block_bytes(width, height, format);

    data_offset += base_level->header->bytes;
    mip_size = mip_size >> 1;

    for (u8 i = 1; i < mips; ++i)
    {
        texture_mip_t *mip_level = texture->mips + i;
        mip_level->header = (texture_data_header_t *) ADDR_OFFSET(data_pointer, data_offset);
        mip_level->data = mip_level->header + 1;
        mip_level->header->width = mip_size;
        mip_level->header->height = mip_size;
        mip_level->header->tex_du = 1.0f / mip_size;
        mip_level->header->tex_dv = 1.0f / mip_size;
        mip_level->header->bytes = _texture_format_data_block_bytes(mip_size, mip_size, format);

        data_offset += mip_level->header->bytes;
        mip_size = mip_size >> 1;
    }

    return texture;
}

cube_texture_t *gc_create_cubemap_texture(u32 width, u32 height, u32 mips, u32 format, u32 flags)
{
    // Mips only work for square textures.
    if (width != height || !mips)
        mips = 1;

    size_t header_bytes = MEM_SIZE_ALIGN(sizeof(cube_texture_t) + 6 * TEX_HEADER_BYTES(mips));
    size_t data_bytes = texture_total_data_bytes(width, height, mips, format);
    size_t total_bytes = header_bytes + 6 * data_bytes;

    mem_set_chunk(MEMORY_ASSETS);
    MEM_LABEL("cube_texture");
    cube_texture_t *texture = (cube_texture_t *) gc_mem_allocate(total_bytes);
    mem_restore_chunk();

    #ifdef GCSR_ASSET_BUILDER_H
    memset(texture, 0, total_bytes);
    #endif

    texture2d_t *face_pointer = (texture2d_t *) (texture + 1);
    texture_mip_t *mip_pointer = (texture_mip_t *) (face_pointer + 6);
    void *data_pointer = ADDR_OFFSET(texture, header_bytes);

    texture->type = ASSET_TEXTURE_CUBEMAP;
    texture->mip_count = mips;
    texture->data_bytes = 6 * data_bytes;
    texture->settings.flags = flags;
    texture->settings.format = format;

    #ifndef GCSR_ASSET_BUILDER_H
    gc_cube_texture_setup(texture);
    #endif

    size_t data_offset = 0;

    for (u8 i = 0; i < 6; ++i)
    {
        u32 mip_size = width;

        texture->faces[i] = face_pointer++;
        texture2d_t *current_face = texture->faces[i];

        current_face->type = ASSET_TEXTURE2D;

        current_face->settings.flags = flags;
        current_face->settings.format = format;
        current_face->mip_count = mips;
        current_face->data_bytes = data_bytes;

        #ifndef GCSR_ASSET_BUILDER_H
        gc_texture2d_setup(current_face);
        #endif

        current_face->mips = mip_pointer++;
        texture_mip_t *base_mip = current_face->mips;

        base_mip->header = (texture_data_header_t *) ADDR_OFFSET(data_pointer, data_offset);
        base_mip->data = base_mip->header + 1;

        base_mip->header->width = width;
        base_mip->header->height = height;
        base_mip->header->tex_du = 1.0f / width;
        base_mip->header->tex_dv = 1.0f / height;
        base_mip->header->bytes = _texture_format_data_block_bytes(width, height, format);

        data_offset += base_mip->header->bytes;
        mip_size = mip_size >> 1;

        for (u8 j = 1; j < mips; ++j)
        {
            texture_mip_t *mip_level = mip_pointer++;

            mip_level->header = (texture_data_header_t *) ADDR_OFFSET(data_pointer, data_offset);
            mip_level->data = mip_level->header + 1;

            mip_level->header->width = mip_size;
            mip_level->header->height = mip_size;
            mip_level->header->tex_du = 1.0f / mip_size;
            mip_level->header->tex_dv = 1.0f / mip_size;
            mip_level->header->bytes = _texture_format_data_block_bytes(mip_size, mip_size, format);

            data_offset += mip_level->header->bytes;
            mip_size = mip_size >> 1;
        }
    }

    return texture;
}

// NOTE(gabic): Uses certain constants which make this kind of texture fixed.
pbr_ambient_texture_t *gc_create_pbr_ambient_texture(u32 flags)
{
    mem_set_chunk(MEMORY_ASSETS);
    MEM_LABEL("pbr_ambient_texture");
    pbr_ambient_texture_t *pbr_ambient_texture = (pbr_ambient_texture_t *) gc_mem_allocate(sizeof(pbr_ambient_texture_t));
    mem_restore_chunk();

    u32 env_mip_count = TEX_MIP_COUNT(PBR_ENVIRONMENT_SIZE);

    pbr_ambient_texture->type = ASSET_TEXTURE_PBR_AMBIENT;
    pbr_ambient_texture->environment = gc_create_cubemap_texture(PBR_ENVIRONMENT_SIZE, PBR_ENVIRONMENT_SIZE, env_mip_count, TEXTURE_FORMAT_RGBAF, flags);
    pbr_ambient_texture->irradiance = gc_create_cubemap_texture(PBR_IRRADIANCE_SIZE, PBR_IRRADIANCE_SIZE, 1, TEXTURE_FORMAT_RGBAF, flags);
    pbr_ambient_texture->prefiltered = gc_create_cubemap_texture(PBR_PREFILTERED_SIZE, PBR_PREFILTERED_SIZE, PBR_PREFILTERED_MIP_LEVELS, TEXTURE_FORMAT_RGBAF, flags);
    pbr_ambient_texture->brdf_lut = gc_create_texture2d(PBR_BRDF_LUT_SIZE, PBR_BRDF_LUT_SIZE, 1, TEXTURE_FORMAT_RGF, flags);

    return pbr_ambient_texture;
}

// NOTE(gabic): For now assume that the input/output image format is RGBAU8.
void gc_texture_copy_image_to_level0(texture2d_t *texture, u32 *src_data)
{
    texture_mip_t *base_mip_level = texture->mips;
    u32 *dest_pixel = (u32 *) base_mip_level->data;

    for (u32 y = 0; y < base_mip_level->header->height; ++y)
    {
        for (u32 x = 0; x < base_mip_level->header->width; ++x) {
            *dest_pixel++ = *src_data++;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Texture filtering.
// ----------------------------------------------------------------------------------

void _gaussian_blur_7x1_rgbau8(texture2d_t *in, texture2d_t *out, b8 horizontal)
{}

void _gaussian_blur_7x1_rgbaf(texture2d_t *in, texture2d_t *out, b8 horizontal)
{}

void _gaussian_blur_7x1_rgbf(texture2d_t *in, texture2d_t *out, b8 horizontal)
{}

void _gaussian_blur_7x1_rgf(texture2d_t *in, texture2d_t *out, b8 horizontal)
{
    texpixel_rgf_t *in_pixels = (texpixel_rgf_t *) in->mips->data;
    texpixel_rgf_t *out_pixels = (texpixel_rgf_t *) out->mips->data;

    r32 weights[4] = {
        0.3125f,
        0.234375f,
        0.09375f,
        0.015625f,
    };

    if (horizontal)
    {
        for (s32 y = 0; y < (s32) out->mips->header->height; ++y)
        {
            for (s32 x = 0; x < (s32) out->mips->header->width; ++x)
            {
                s32 offset = y * in->mips->header->width + x;
                texpixel_rgf_t *sample = in_pixels + offset;

                out_pixels->r = sample->r * weights[0];
                out_pixels->g = sample->g * weights[0];

                for (s32 k = 1; k < 4; ++k)
                {
                    if (x - k < 0 || x + k >= (s32) in->mips->header->width)
                        continue;
                    else
                    {
                        offset = y * in->mips->header->width + (x - k);
                        sample = in_pixels + offset;

                        out_pixels->r += sample->r * weights[k];
                        out_pixels->g += sample->g * weights[k];

                        offset = y * in->mips->header->width + (x + k);
                        sample = in_pixels + offset;

                        out_pixels->r += sample->r * weights[k];
                        out_pixels->g += sample->g * weights[k];
                    }
                }

                out_pixels++;
            }
        }
    }
    else
    {
        for (s32 y = 0; y < (s32) out->mips->header->height; ++y)
        {
            for (s32 x = 0; x < (s32) out->mips->header->width; ++x)
            {
                s32 offset = y * in->mips->header->width + x;
                texpixel_rgf_t *sample = in_pixels + offset;

                out_pixels->r = sample->r * weights[0];
                out_pixels->g = sample->g * weights[0];

                for (s32 k = 1; k < 4; ++k)
                {
                    if (y - k < 0 || y + k >= (s32) in->mips->header->height)
                        continue;
                    else
                    {
                        offset = (y - k) * in->mips->header->width + x;
                        sample = in_pixels + offset;

                        out_pixels->r += sample->r * weights[k];
                        out_pixels->g += sample->g * weights[k];

                        offset = (y + k) * in->mips->header->width + x;
                        sample = in_pixels + offset;

                        out_pixels->r += sample->r * weights[k];
                        out_pixels->g += sample->g * weights[k];
                    }
                }

                out_pixels++;
            }
        }
    }
}

// ----------------------------------------------------------------------------------
// -- 7x1 gaussian blur.
// ----------------------------------------------------------------------------------

texture2d_t *gc_texture2d_gaussian_blur(texture2d_t *in, texture2d_t *out, b8 horizontal)
{
    if (in->settings.format == TEXTURE_FORMAT_RGBAU8) {
        _gaussian_blur_7x1_rgbau8(in, out, horizontal);
    }
    else if (in->settings.format == TEXTURE_FORMAT_RGBAF) {
        _gaussian_blur_7x1_rgbaf(in, out, horizontal);
    }
    else if (in->settings.format == TEXTURE_FORMAT_RGBF) {
        _gaussian_blur_7x1_rgbf(in, out, horizontal);
    }
    else if (in->settings.format == TEXTURE_FORMAT_RGF) {
        _gaussian_blur_7x1_rgf(in, out, horizontal);
    }

    return out;
}

void gc_texture2d_blur_pcf(texture2d_t *in, texture2d_t *out)
{
    s32 kernel_size = 9;
    texpixel_rgf_t *in_pixels = (texpixel_rgf_t *) in->mips->data;
    texpixel_rgf_t *out_pixels = (texpixel_rgf_t *) out->mips->data;

    for (s32 y = 0; y < (s32) out->mips->header->height; ++y)
    {
        for (s32 x = 0; x < (s32) out->mips->header->width; ++x)
        {
            u32 count = 0;

            // Kernel.
            for (s32 oy = -kernel_size; oy <= kernel_size; ++oy)
            {
                s32 sy = y + oy;

                for (s32 ox = -kernel_size; ox <= kernel_size; ++ox)
                {
                    s32 sx = x + ox;

                    if (sx < 0 || sy < 0 || sx >= (s32) out->mips->header->width || sy >= (s32) out->mips->header->height)
                        continue;

                    s32 offset = sy * in->mips->header->width + sx;
                    texpixel_rgf_t *sample = in_pixels + offset;

                    out_pixels->r += sample->r;
                    out_pixels->g += sample->g;

                    count++;
                }
            }

            out_pixels->r /= count;
            out_pixels->g /= count;

            out_pixels++;
        }
    }
}

// ----------------------------------------------------------------------------------
// -- Mipmap generation.
// ----------------------------------------------------------------------------------

void _generate_mipmap_rgbau8(texture_mip_t *parent_level, texture_mip_t *current_level)
{
    texpixel_rgbaf_t color[4];
    texpixel_rgbaf_t mip_color;

    u32 *src_pointer = (u32 *) parent_level->data;
    u32 *dest_pointer = (u32 *) current_level->data;

    for (u32 row = 0; row < current_level->header->height; ++row)
    {
        u32 parent_row = row << 1;

        u32 *src_row_1 = src_pointer + parent_row * parent_level->header->width;
        u32 *src_row_2 = src_pointer + (parent_row + 1) * parent_level->header->width;

        u32 *dest_pixel = dest_pointer;

        for (u32 col = 0; col < current_level->header->width; ++col)
        {
            u32 prev_col = col << 1;

            u32 *parent_pixel_0 = src_row_1 + prev_col;
            u32 *parent_pixel_1 = src_row_1 + (prev_col + 1);
            u32 *parent_pixel_2 = src_row_2 + prev_col;
            u32 *parent_pixel_3 = src_row_2 + (prev_col + 1);

            gl_unpack(color[0], *parent_pixel_0);
            gl_unpack(color[1], *parent_pixel_1);
            gl_unpack(color[2], *parent_pixel_2);
            gl_unpack(color[3], *parent_pixel_3);

            // Average the color.
            mip_color.r = (color[0].r + color[1].r + color[2].r + color[3].r) / 4;
            mip_color.g = (color[0].g + color[1].g + color[2].g + color[3].g) / 4;
            mip_color.b = (color[0].b + color[1].b + color[2].b + color[3].b) / 4;
            mip_color.a = (color[0].a + color[1].a + color[2].a + color[3].a) / 4;

            *dest_pixel++ = gl_pack_p4(mip_color);
        }

        dest_pointer += current_level->header->width;
    }
}

void _generate_mipmap_rgbaf(texture_mip_t *parent_level, texture_mip_t *current_level)
{
    texpixel_rgbaf_t *src_pointer = (texpixel_rgbaf_t *) parent_level->data;
    texpixel_rgbaf_t *dest_pointer = (texpixel_rgbaf_t *) current_level->data;

    for (u32 row = 0; row < current_level->header->height; ++row)
    {
        u32 parent_row = row << 1;

        texpixel_rgbaf_t *src_row_1 = src_pointer + parent_row * parent_level->header->width;
        texpixel_rgbaf_t *src_row_2 = src_pointer + (parent_row + 1) * parent_level->header->width;

        texpixel_rgbaf_t *dest_pixel = dest_pointer;

        for (u32 col = 0; col < current_level->header->width; ++col)
        {
            u32 prev_col = col << 1;

            texpixel_rgbaf_t *parent_pixel_0 = src_row_1 + prev_col;
            texpixel_rgbaf_t *parent_pixel_1 = src_row_1 + (prev_col + 1);
            texpixel_rgbaf_t *parent_pixel_2 = src_row_2 + prev_col;
            texpixel_rgbaf_t *parent_pixel_3 = src_row_2 + (prev_col + 1);

            // Average the color.
            dest_pixel->r = (parent_pixel_0->r + parent_pixel_1->r + parent_pixel_2->r + parent_pixel_3->r) / 4;
            dest_pixel->g = (parent_pixel_0->g + parent_pixel_1->g + parent_pixel_2->g + parent_pixel_3->g) / 4;
            dest_pixel->b = (parent_pixel_0->b + parent_pixel_1->b + parent_pixel_2->b + parent_pixel_3->b) / 4;
            dest_pixel->a = (parent_pixel_0->a + parent_pixel_1->a + parent_pixel_2->a + parent_pixel_3->a) / 4;

            dest_pixel++;
        }

        dest_pointer += current_level->header->width;
    }
}

void _generate_mipmap_rgbf(texture_mip_t *parent_level, texture_mip_t *current_level)
{
    texpixel_rgbf_t *src_pointer = (texpixel_rgbf_t *) parent_level->data;
    texpixel_rgbf_t *dest_pointer = (texpixel_rgbf_t *) current_level->data;

    for (u32 row = 0; row < current_level->header->height; ++row)
    {
        u32 parent_row = row << 1;

        texpixel_rgbf_t *src_row_1 = src_pointer + parent_row * parent_level->header->width;
        texpixel_rgbf_t *src_row_2 = src_pointer + (parent_row + 1) * parent_level->header->width;

        texpixel_rgbf_t *dest_pixel = dest_pointer;

        for (u32 col = 0; col < current_level->header->width; ++col)
        {
            u32 prev_col = col << 1;

            texpixel_rgbf_t *parent_pixel_0 = src_row_1 + prev_col;
            texpixel_rgbf_t *parent_pixel_1 = src_row_1 + (prev_col + 1);
            texpixel_rgbf_t *parent_pixel_2 = src_row_2 + prev_col;
            texpixel_rgbf_t *parent_pixel_3 = src_row_2 + (prev_col + 1);

            // Average the color.
            dest_pixel->r = (parent_pixel_0->r + parent_pixel_1->r + parent_pixel_2->r + parent_pixel_3->r) / 4;
            dest_pixel->g = (parent_pixel_0->g + parent_pixel_1->g + parent_pixel_2->g + parent_pixel_3->g) / 4;
            dest_pixel->b = (parent_pixel_0->b + parent_pixel_1->b + parent_pixel_2->b + parent_pixel_3->b) / 4;

            dest_pixel++;
        }

        dest_pointer += current_level->header->width;
    }
}

void _generate_mipmap_rgf(texture_mip_t *parent_level, texture_mip_t *current_level)
{
    texpixel_rgf_t *src_pointer = (texpixel_rgf_t *) parent_level->data;
    texpixel_rgf_t *dest_pointer = (texpixel_rgf_t *) current_level->data;

    for (u32 row = 0; row < current_level->header->height; ++row)
    {
        u32 parent_row = row << 1;

        texpixel_rgf_t *src_row_1 = src_pointer + parent_row * parent_level->header->width;
        texpixel_rgf_t *src_row_2 = src_pointer + (parent_row + 1) * parent_level->header->width;

        texpixel_rgf_t *dest_pixel = dest_pointer;

        for (u32 col = 0; col < current_level->header->width; ++col)
        {
            u32 prev_col = col << 1;

            texpixel_rgf_t *parent_pixel_0 = src_row_1 + prev_col;
            texpixel_rgf_t *parent_pixel_1 = src_row_1 + (prev_col + 1);
            texpixel_rgf_t *parent_pixel_2 = src_row_2 + prev_col;
            texpixel_rgf_t *parent_pixel_3 = src_row_2 + (prev_col + 1);

            // Average the color.
            dest_pixel->r = (parent_pixel_0->r + parent_pixel_1->r + parent_pixel_2->r + parent_pixel_3->r) / 4;
            dest_pixel->g = (parent_pixel_0->g + parent_pixel_1->g + parent_pixel_2->g + parent_pixel_3->g) / 4;

            dest_pixel++;
        }

        dest_pointer += current_level->header->width;
    }
}

__INLINE__ void gc_texture_generate_mipmaps(texture2d_t *texture)
{
    texture_mip_t *parent_mip_level = texture->mips;

    for (u8 i = 1; i < texture->mip_count; ++i)
    {
        texture_mip_t *current_mip_level = texture->mips + i;

        if (texture->settings.format == TEXTURE_FORMAT_RGBAU8)
            _generate_mipmap_rgbau8(parent_mip_level, current_mip_level);
        else if (texture->settings.format == TEXTURE_FORMAT_RGBAF)
            _generate_mipmap_rgbaf(parent_mip_level, current_mip_level);
        else if (texture->settings.format == TEXTURE_FORMAT_RGBF)
            _generate_mipmap_rgbf(parent_mip_level, current_mip_level);
        else if (texture->settings.format == TEXTURE_FORMAT_RGF)
            _generate_mipmap_rgf(parent_mip_level, current_mip_level);

        parent_mip_level = current_mip_level;
    }
}

__INLINE__ void gc_cube_texture_generate_mipmaps(cube_texture_t *cubemap)
{
    for (u8 i = 0; i < 6; ++i)
    {
        texture2d_t *face = cubemap->faces[i];
        gc_texture_generate_mipmaps(face);
    }
}

__INLINE__ void gc_destroy_texture2d(texture2d_t *texture)
{
    if (texture)
        gc_mem_free(texture);
}

__INLINE__ void gc_destroy_cube_texture(cube_texture_t *cubemap)
{
    if (cubemap)
        gc_mem_free(cubemap);
}

void gc_destroy_pbr_ambient_texture(pbr_ambient_texture_t *pbr_ambient)
{
    if (pbr_ambient)
    {
        gc_destroy_cube_texture(pbr_ambient->environment);
        gc_destroy_cube_texture(pbr_ambient->irradiance);
        gc_destroy_cube_texture(pbr_ambient->prefiltered);
        gc_destroy_texture2d(pbr_ambient->brdf_lut);
        gc_mem_free(pbr_ambient);
    }
}