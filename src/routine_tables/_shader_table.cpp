// ----------------------------------------------------------------------------------
// -- File: _shader_table.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description: A group of test shader programs to play around with.
// -- Created: 2020-10-25 15:28:52
// -- Modified: 2022-04-28 20:49:26
// ----------------------------------------------------------------------------------

#include "shaders/shader_point.cpp"
#include "shaders/shader_wireframe.cpp"
#include "shaders/shader_flat_color.cpp"
#include "shaders/shader_flat_texture.cpp"
// #include "shaders/shader_flat_color_gouraud.cpp"
// #include "shaders/shader_flat_color_blinn_phong.cpp"
// #include "shaders/shader_gouraud.cpp"
#include "shaders/shader_blinn_phong.cpp"
#include "shaders/shader_pbr.cpp"
#include "shaders/shader_cartoon.cpp"
#include "shaders/shader_depth_buffer.cpp"
#include "shaders/shader_shadow.cpp"
#include "shaders/shader_point_shadow.cpp"
#include "shaders/shader_skybox.cpp"
#include "shaders/shader_pbr_skybox.cpp"
#include "shaders/shader_reflection_refraction.cpp"
#include "shaders/shader_debug_light.cpp"
#include "shaders/shader_debug_grid.cpp"

// ----------------------------------------------------------------------------------
// -- NOTE(gabic): Each program has to specify the number of buffers it expects and
// -- the number of components that will be read from the buffers. The number of
// -- components can be increased if extra attributes will be added in the vertex
// -- shader.
// ----------------------------------------------------------------------------------

void init_shader_table()
{
    gc_shader_t *wireframe_shader = GET_SHADER(SHADER_WIREFRAME);

    wireframe_shader->name = "Wireframe shader";
    wireframe_shader->id = SHADER_WIREFRAME;
    wireframe_shader->varying_count = 0;

    wireframe_shader->read = (vstream_reader_callback_t) gl_vstream_line;
    wireframe_shader->vs = shader_wireframe_vs;
    wireframe_shader->fs = shader_wireframe_fs;

    // ----------------------------------------------------------------------------------

    gc_shader_t *point_shader = GET_SHADER(SHADER_POINT);

    point_shader->name = "Point shader";
    point_shader->id = SHADER_POINT;
    point_shader->varying_count = 0;

    point_shader->read = (vstream_reader_callback_t) gl_vstream_0;
    point_shader->vs = shader_point_vs;
    point_shader->fs = shader_point_fs;

    // ----------------------------------------------------------------------------------

    gc_shader_t *MeshFlatProgram = GET_SHADER(SHADER_FLAT_COLOR);

    MeshFlatProgram->name = "Mesh flat color shader";
    MeshFlatProgram->id = SHADER_FLAT_COLOR;
    // MeshFlatProgram->attributes = 1;
    // MeshFlatProgram->interpolation_components = 0;
    // MeshFlatProgram->vs = shader_flat_color_vs;
    // MeshFlatProgram->fs = shader_flat_color_fs;

    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_flat_texture = GET_SHADER(SHADER_TEXTURE);

    shader_flat_texture->name = "Flat texture shader";
    shader_flat_texture->id = SHADER_TEXTURE;
    shader_flat_texture->varying_count = 2;
    shader_flat_texture->read = (vstream_reader_callback_t) gl_vstream_0;
    shader_flat_texture->setup = shader_flat_texture_setup;
    shader_flat_texture->vs = shader_flat_texture_vs;
    shader_flat_texture->fs = shader_flat_texture_fs;

    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_blinn_phong = GET_SHADER(SHADER_BLINN_PHONG);

    shader_blinn_phong->name = "Blinn-Phong shader";
    shader_blinn_phong->id = SHADER_BLINN_PHONG;
    shader_blinn_phong->varying_count = 15;
    shader_blinn_phong->read = (vstream_reader_callback_t) gl_vstream_1;
    shader_blinn_phong->setup = shader_blinn_phong_setup;
    shader_blinn_phong->vs = shader_blinn_phong_vs;
    shader_blinn_phong->fs = shader_blinn_phong_fs;

    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_pbr = GET_SHADER(SHADER_PBR);

    shader_pbr->name = "PBR shader";
    shader_pbr->id = SHADER_PBR;
    shader_pbr->varying_count = 15;
    shader_pbr->read = (vstream_reader_callback_t) gl_vstream_1;
    shader_pbr->setup = shader_pbr_setup;
    shader_pbr->vs = shader_pbr_vs;
    shader_pbr->fs = shader_pbr_fs;

    // ----------------------------------------------------------------------------------

    gc_shader_t *CartoonProgram = GET_SHADER(SHADER_CARTOON);

    CartoonProgram->name = "Cartoon shader";
    CartoonProgram->id = SHADER_CARTOON;
    // pos, uv, normal
    // CartoonProgram->attributes = 3;
    // intensity
    // CartoonProgram->interpolation_components = 1 + MAX_SCENE_LIGHTS;
    // CartoonProgram->vs = shader_cartoon_vs;
    // CartoonProgram->fs = shader_cartoon_fs;

    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_depth_buffer = GET_SHADER(SHADER_DEPTH_BUFFER);

    shader_depth_buffer->name = "Depth buffer shader";
    shader_depth_buffer->id = SHADER_DEPTH_BUFFER;
    shader_depth_buffer->read = (vstream_reader_callback_t) gl_vstream_1;
    shader_depth_buffer->vs = shader_depth_buffer_vs;
    shader_depth_buffer->fs = shader_depth_buffer_fs;

    // ----------------------------------------------------------------------------------
    // -- SHADER SHADOW.
    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_shadow = GET_SHADER(SHADER_SHADOW);

    shader_shadow->name = "Shadow shader";
    shader_shadow->id = SHADER_SHADOW;
    shader_shadow->read = (vstream_reader_callback_t) gl_vstream_1;
    // shader_shadow->varying_count = 5;
    shader_shadow->vs = shader_shadow_vs;
    shader_shadow->fs = shader_shadow_fs;

    // ----------------------------------------------------------------------------------
    // -- SHADER POINT SHADOW.
    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_point_shadow = GET_SHADER(SHADER_POINT_SHADOW);

    shader_point_shadow->name = "Point shadow shader";
    shader_point_shadow->id = SHADER_POINT_SHADOW;
    shader_point_shadow->varying_count = 5;
    shader_point_shadow->read = (vstream_reader_callback_t) gl_vstream_2;
    shader_point_shadow->vs = shader_point_shadow_vs;
    shader_point_shadow->fs = shader_point_shadow_fs;

    // ----------------------------------------------------------------------------------
    // -- SHADER SKYBOX.
    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_skybox = GET_SHADER(SHADER_SKYBOX);

    shader_skybox->name = "Skybox shader";
    shader_skybox->id = SHADER_SKYBOX;
    // vertex position.
    shader_skybox->varying_count = 5;
    shader_skybox->read = (vstream_reader_callback_t) gl_vstream_2;
    shader_skybox->setup = shader_skybox_setup;
    shader_skybox->vs = shader_skybox_vs;
    shader_skybox->fs = shader_skybox_fs;

    gc_shader_t *shader_skybox_pbr = GET_SHADER(SHADER_SKYBOX_PBR);

    shader_skybox_pbr->name = "Skybox pbr shader";
    shader_skybox_pbr->id = SHADER_SKYBOX_PBR;
    // vertex position.
    shader_skybox_pbr->varying_count = 5;
    shader_skybox_pbr->read = (vstream_reader_callback_t) gl_vstream_2;
    shader_skybox_pbr->setup = shader_pbr_skybox_setup;
    shader_skybox_pbr->vs = shader_pbr_skybox_vs;
    shader_skybox_pbr->fs = shader_pbr_skybox_fs;

    // ----------------------------------------------------------------------------------
    // -- SHADER REFLECTION REFRACTION.
    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_reflection_refraction = GET_SHADER(SHADER_REFLECTION_REFRACTION);

    shader_reflection_refraction->name = "Reflection/refraction shader";
    shader_reflection_refraction->id = SHADER_REFLECTION_REFRACTION;
    shader_reflection_refraction->varying_count = 9;
    shader_reflection_refraction->read = (vstream_reader_callback_t) gl_vstream_1;
    shader_reflection_refraction->setup = shader_reflection_refraction_setup;
    shader_reflection_refraction->vs = shader_reflection_refraction_vs;
    shader_reflection_refraction->fs = shader_reflection_refraction_fs;

    // ----------------------------------------------------------------------------------
    // -- SHADER DEBUG LIGHT.
    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_debug_light = GET_SHADER(SHADER_DEBUG_LIGHT);

    shader_debug_light->name = "Debug light shader";
    shader_debug_light->id = SHADER_DEBUG_LIGHT;
    shader_debug_light->varying_count = 0;
    shader_debug_light->read = (vstream_reader_callback_t) gl_vstream_0;
    shader_debug_light->setup = 0;
    shader_debug_light->vs = shader_debug_light_vs;
    shader_debug_light->fs = shader_debug_light_fs;

    // ----------------------------------------------------------------------------------
    // -- SHADER DEBUG GRID.
    // ----------------------------------------------------------------------------------

    gc_shader_t *shader_debug_grid = GET_SHADER(SHADER_DEBUG_GRID);

    shader_debug_grid->name = "Debug light shader";
    shader_debug_grid->id = SHADER_DEBUG_GRID;
    shader_debug_grid->varying_count = 0;
    shader_debug_grid->read = (vstream_reader_callback_t) gl_vstream_line_color;
    shader_debug_grid->setup = 0;
    shader_debug_grid->vs = shader_debug_grid_vs;
    shader_debug_grid->fs = shader_debug_grid_fs;
}
