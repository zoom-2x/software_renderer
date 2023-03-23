// ----------------------------------------------------------------------------------
// -- File: gcsr_gl_avs.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-08-15 21:59:19
// -- Modified:
// ----------------------------------------------------------------------------------

#ifndef GCSR_STRUCT_GlFragment
#define GCSR_STRUCT_GlFragment

struct gc_primitive_t;

__ALIGN__ struct GlFragment
{
    r32 l1_8x[8];
    r32 l2_8x[8];
    r32 l3_8x[8];

    r32 colorr_8x[8];
    r32 colorg_8x[8];
    r32 colorb_8x[8];
    r32 colora_8x[8];

    u32 mask_8x[8];

    r32 z[8];
    r32 pt[8];

    gc_primitive_t *primitive;

    u16 sx;
    u16 sy;
    u8 mask;
    u32 framebufferIndex;
};

#endif

#ifndef GCSR_STRUCT_GlFragmentData
#define GCSR_STRUCT_GlFragmentData

__ALIGN__ struct GlFragmentShaderPack
{
    u32 pos_x_8x[8];
    u32 pos_y_8x[8];

    r32 uv_u_8x[8];
    r32 uv_v_8x[8];

    r32 norm_x_8x[8];
    r32 norm_y_8x[8];
    r32 norm_z_8x[8];

    r32 channelr_8x[8];
    r32 channelg_8x[8];
    r32 channelb_8x[8];
    r32 channela_8x[8];

    r32 *outputr_8x;
    r32 *outputg_8x;
    r32 *outputb_8x;
    r32 *outputa_8x;
};

#endif