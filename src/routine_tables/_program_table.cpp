// ----------------------------------------------------------------------------------
// -- File: _program_table.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-10 21:12:01
// -- Modified: 2022-12-10 21:12:01
// ----------------------------------------------------------------------------------

#include "program/rotation_around_z.cpp"
#include "program/wave.cpp"
#include "program/text_panel.cpp"
#include "program/text_panel_extrude.cpp"
#include "program/bezier.cpp"
#include "program/camera_rotation.cpp"

void init_program_table()
{
    gc_level_program_t *prg_1 = GET_PROGRAM(PRG_ROTATION_AROUND_Z);

    prg_1->name = "Rotation around z";
    prg_1->setup = _setup_rotation_around_z;
    prg_1->update = _update_rotation_around_z;
    prg_1->clear = 0;

    gc_level_program_t *prg_2 = GET_PROGRAM(PRG_WAVE);

    prg_2->name = "Waves";
    prg_2->setup = _setup_wave;
    prg_2->update = _update_wave;
    prg_2->clear = 0;

    gc_level_program_t *prg_3 = GET_PROGRAM(PRG_TEXT_PANEL);

    prg_3->name = "Text panel";
    prg_3->setup = _setup_text_panel;
    prg_3->update = _update_text_panel;
    prg_3->clear = 0;

    gc_level_program_t *prg_4 = GET_PROGRAM(PRG_TEXT_PANEL_EXTRUDE);

    prg_4->name = "Text panel extrude";
    prg_4->setup = _setup_text_panel_extrude;
    prg_4->update = _update_text_panel_extrude;
    prg_4->clear = 0;

    gc_level_program_t *prg_5 = GET_PROGRAM(PRG_BEZIER_CURVE);

    prg_5->name = "Bezier curve";
    prg_5->setup = _setup_bezier;
    prg_5->update = _update_bezier;
    prg_5->clear = 0;

    gc_level_program_t *prg_6 = GET_PROGRAM(PRG_CAMERA_ROTATION);

    prg_6->name = "Camera rotation";
    prg_6->setup = _setup_camera_rotation;
    prg_6->update = _update_camera_rotation;
    prg_6->clear = 0;
}