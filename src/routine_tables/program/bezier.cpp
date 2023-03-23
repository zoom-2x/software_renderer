// ----------------------------------------------------------------------------------
// -- File: bezier.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-09 11:43:42
// -- Modified: 2022-12-09 11:43:43
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

typedef struct
{
    mesh_t *locomotive_meshes[3];
    mesh_t *wagon_meshes[3];

    gc_model_t *models;

    gc_material_t *locomotive_material;
    gc_material_t *wagon_material;

    // gc_vec_t locomotive_color;
    // gc_vec_t wagon_color;

    transform_t transform;

    b8 valid;
    u8 size;

    s32 base_frame_offset;
    s32 locomotive_frame_offset;
    s32 wagon_frame_offset;
} train_t;

typedef struct
{
    b8 is_spacestation;
    gc_level_t *level;

    u32 train_count;
    train_t *trains;

    u32 curve_count;
    bezier_curve_t *curves;

    u32 model_count;
    gc_model_t *models;

    u32 animation_count;
    animation_t *animations;
} bezier_program_t;

void _json_read_flags(struct _json_value *flag_data, bezier_curve_t *curve)
{
    if (flag_data->type == json_object)
    {
        JSON_VALUE_OBJECT_LOOP(flag_data, i)
        {
            struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(flag_data, i);

            if (JSON_PROPERTY_COMPARE(prop, "closed_path", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop))
                curve->flags |= BEZIER_CLOSED_PATH;

            else if (JSON_PROPERTY_COMPARE(prop, "synchronize", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop))
                curve->flags |= BEZIER_SYNCHRONIZE;

            else if (JSON_PROPERTY_COMPARE(prop, "debug", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop))
                curve->flags |= BEZIER_DEBUG;

            else if (JSON_PROPERTY_COMPARE(prop, "debug_lut", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop))
                curve->flags |= BEZIER_DEBUG_LUT;

            else if (JSON_PROPERTY_COMPARE(prop, "debug_tangents", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop))
                curve->flags |= BEZIER_DEBUG_TANGENTS;

            else if (JSON_PROPERTY_COMPARE(prop, "debug_points", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop))
                curve->flags |= BEZIER_DEBUG_POINTS;

            else if (JSON_PROPERTY_COMPARE(prop, "reverse", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop))
                curve->flags |= BEZIER_REVERSE;
        }
    }
}

void _json_read_points(struct _json_value *point_data, bezier_point_t *curve_points, r32 scale)
{
    // Points.
    JSON_VALUE_ARRAY_LOOP (point_data, i)
    {
        struct _json_value *json_point = JSON_ARRAY_VALUE(point_data, i);
        bezier_point_t *point = curve_points + i;

        // Only two points are specified, control point and one handle point,
        // the second handle point will be generated.

        if (json_point->type == json_array && JSON_ARRAY_LENGTH(json_point) == 6)
        {
            gc_vec_t *point_0 = point->p + 0;
            gc_vec_t *point_1 = point->p + 1;
            gc_vec_t *point_2 = point->p + 2;

            point_0->data[0] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 0);
            point_0->data[1] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 1);
            point_0->data[2] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 2);

            point_1->data[0] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 3);
            point_1->data[1] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 4);
            point_1->data[2] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 5);

            // Generate the second handle point.

            point_2->data[0] = scale * (2 * point_0->data[0] - point_1->data[0]);
            point_2->data[1] = scale * (2 * point_0->data[1] - point_1->data[1]);
            point_2->data[2] = scale * (2 * point_0->data[2] - point_1->data[2]);
        }

        else if (json_point->type == json_array && JSON_ARRAY_LENGTH(json_point) == 9)
        {
            gc_vec_t *point_0 = point->p + 0;
            gc_vec_t *point_1 = point->p + 1;
            gc_vec_t *point_2 = point->p + 2;

            point_0->data[0] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 0);
            point_0->data[1] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 1);
            point_0->data[2] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 2);

            point_1->data[0] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 3);
            point_1->data[1] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 4);
            point_1->data[2] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 5);

            point_2->data[0] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 6);
            point_2->data[1] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 7);
            point_2->data[2] = scale * JSON_ARRAY_VALUE_FLOAT(json_point, 8);
        }
    }
}

void _generate_sections(bezier_curve_t *curve)
{
    if (!curve->section_count)
        return;

    memory_manager_t *manager = &GCSR.state->manager;
    curve->sections = (bezier_section_t *) stack_push(manager->stack, sizeof(bezier_section_t) * curve->section_count);

    for (u32 i = 0, si = 0; i < curve->point_count - 1; ++i, ++si)
    {
        bezier_section_t *section = curve->sections + si;

        bezier_point_t *current_point = curve->points + i;
        bezier_point_t *next_point = curve->points + (i + 1);

        if (i == 0)
        {
            section->p0 = current_point->p + 0;
            section->p1 = current_point->p + 1;
        }
        else
        {
            section->p0 = current_point->p + 0;
            section->p1 = current_point->p + 2;
        }

        section->p2 = next_point->p + 1;
        section->p3 = next_point->p + 0;
    }

    // Last section.
    if (curve->flags & BEZIER_CLOSED_PATH)
    {
        bezier_section_t *section = curve->sections + curve->section_count - 1;

        bezier_point_t *current_point = curve->points + curve->point_count - 1;
        bezier_point_t *next_point = curve->points + 0;

        section->p0 = current_point->p + 0;
        section->p1 = current_point->p + 2;
        section->p2 = next_point->p + 2;
        section->p3 = next_point->p + 0;
    }
}

void _read_keyframe_data(keyframe_t *keyframes, struct _json_object_entry *data, r32 scale)
{
    u32 count = JSON_ARRAY_LENGTH(data->value);

    JSON_VALUE_ARRAY_LOOP(data->value, i)
    {
        struct _json_value *keyframe = JSON_ARRAY_VALUE(data->value, i);
        keyframe_t *current_keyframe = keyframes + i;

        current_keyframe->frame = 0;
        current_keyframe->value = 0;

        JSON_VALUE_OBJECT_LOOP(keyframe, j)
        {
            struct _json_object_entry *key = JSON_OBJECT_PROPERTY(keyframe, j);

            if (JSON_PROPERTY_COMPARE(key, "frame", json_integer))
            {
                current_keyframe->frame = JSON_PROPERTY_VALUE_INTEGER(key);

                if (current_keyframe->frame > 1)
                    current_keyframe->frame *= scale;
            }
            else if (JSON_PROPERTY_COMPARE(key, "value", json_double)) {
                current_keyframe->value = JSON_PROPERTY_VALUE_DOUBLE(key);
            }
            else if (JSON_PROPERTY_COMPARE(key, "value", json_integer)) {
                current_keyframe->value = JSON_PROPERTY_VALUE_INTEGER(key);
            }
        }
    }
}

void _json_read_animation(struct _json_value *animation_data, animation_t *animation, bezier_program_t *data)
{
    memory_manager_t *manager = &GCSR.state->manager;

    animation->type = ANIM_BEZIER_CURVE;
    animation->curve = 0;

    if (!animation->target_fps)
        animation->target_fps = 30;

    animation->timescale = 1.0f;
    animation->frame_ms = 1000.0f / animation->target_fps;
    animation->one_over_frame_ms = 1.0f / animation->frame_ms;
    animation->current_ms = 0;

    animation->frame_offset = 0;
    animation->flags = ANIMATION_FOLLOW_PATH;

    animation->target_count = 0;
    animation->targets = 0;

    animation->frames = 0;
    animation->frame = 1;

    animation->offset_keyframes = 0;
    animation->roll_keyframes = 0;
    animation->pitch_keyframes = 0;
    animation->heading_keyframes = 0;
    animation->horizontal_keyframes = 0;
    animation->vertical_keyframes = 0;
    animation->scaling_x_keyframes = 0;
    animation->scaling_y_keyframes = 0;

    animation->offset = 0;
    animation->roll = 0;
    animation->pitch = 0;
    animation->heading = 0;
    animation->horizontal = 0;
    animation->vertical = 0;
    animation->scaling_x = 0;
    animation->scaling_y = 0;

    struct _json_object_entry *targets = 0;
    struct _json_object_entry *offset = 0;
    struct _json_object_entry *roll = 0;
    struct _json_object_entry *pitch = 0;
    struct _json_object_entry *heading = 0;
    struct _json_object_entry *horizontal = 0;
    struct _json_object_entry *vertical = 0;
    struct _json_object_entry *scaling_x = 0;
    struct _json_object_entry *scaling_y = 0;

    s32 train_index = -1;

    u32 target_count = 0;
    u32 offset_count = 0;
    u32 roll_count = 0;
    u32 pitch_count = 0;
    u32 heading_count = 0;
    u32 horizontal_count = 0;
    u32 vertical_count = 0;
    u32 scaling_x_count = 0;
    u32 scaling_y_count = 0;

    train_t *follow_train = 0;
    gc_model_t *follow_object = 0;

    JSON_VALUE_OBJECT_LOOP(animation_data, ai)
    {
        struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(animation_data, ai);

        if (JSON_PROPERTY_COMPARE(prop, "curve", json_integer))
        {
            u32 curve_id = JSON_PROPERTY_VALUE_INTEGER(prop);

            if (curve_id >= 0 && curve_id < data->curve_count)
                animation->curve = data->curves + curve_id;
        }

        else if (JSON_PROPERTY_COMPARE(prop, "train", json_integer)) {
            train_index = JSON_PROPERTY_VALUE_INTEGER(prop);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "targets", json_array))
        {
            target_count = JSON_ARRAY_LENGTH(prop->value);
            targets = prop;
        }

        else if (JSON_PROPERTY_COMPARE(prop, "frames", json_integer)) {
            animation->frames = JSON_PROPERTY_VALUE_INTEGER(prop);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "disabled", json_boolean))
        {
            if (JSON_PROPERTY_VALUE_BOOL(prop))
                FLAG_ENABLE(animation->flags, ANIMATION_DISABLED);
            else
                FLAG_DISABLE(animation->flags, ANIMATION_DISABLED);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "use_camera", json_boolean))
        {
            if (JSON_PROPERTY_VALUE_BOOL(prop))
                FLAG_ENABLE(animation->flags, ANIMATION_USE_CAMERA);
            else
                FLAG_DISABLE(animation->flags, ANIMATION_USE_CAMERA);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "loop", json_boolean))
        {
            if (JSON_PROPERTY_VALUE_BOOL(prop))
                FLAG_ENABLE(animation->flags, ANIMATION_LOOP);
            else
                FLAG_DISABLE(animation->flags, ANIMATION_LOOP);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "follow_path", json_boolean))
        {
            if (JSON_PROPERTY_VALUE_BOOL(prop))
                FLAG_ENABLE(animation->flags, ANIMATION_FOLLOW_PATH);
            else
                FLAG_DISABLE(animation->flags, ANIMATION_FOLLOW_PATH);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "follow_object", json_integer))
        {
            s32 target_index = JSON_PROPERTY_VALUE_INTEGER(prop);

            if (target_index < data->level->model_count)
                follow_object = data->level->models + target_index;
        }

        else if (JSON_PROPERTY_COMPARE(prop, "follow_train", json_integer))
        {
            s32 target_index = JSON_PROPERTY_VALUE_INTEGER(prop);

            if (target_index >= 0 && target_index < (s32) data->train_count)
                follow_train = data->trains + target_index;
        }

        else if (JSON_PROPERTY_COMPARE(prop, "frame_offset", json_integer)) {
            animation->frame_offset = JSON_PROPERTY_VALUE_INTEGER(prop);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "timescale", json_double)) {
            animation->timescale = JSON_PROPERTY_VALUE_DOUBLE(prop);
        }

        else if (JSON_PROPERTY_COMPARE(prop, "keyframes", json_object))
        {
            JSON_VALUE_OBJECT_LOOP(prop->value, ki)
            {
                struct _json_object_entry *keyframe = JSON_OBJECT_PROPERTY(prop->value, ki);

                if (JSON_PROPERTY_COMPARE(keyframe, "offset", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    offset = keyframe;
                }
                else if (JSON_PROPERTY_COMPARE(keyframe, "roll", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    roll = keyframe;
                }
                else if (JSON_PROPERTY_COMPARE(keyframe, "pitch", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    pitch = keyframe;
                }
                else if (JSON_PROPERTY_COMPARE(keyframe, "heading", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    heading = keyframe;
                }
                else if (JSON_PROPERTY_COMPARE(keyframe, "horizontal", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    horizontal = keyframe;
                }
                else if (JSON_PROPERTY_COMPARE(keyframe, "vertical", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    vertical = keyframe;
                }
                else if (JSON_PROPERTY_COMPARE(keyframe, "scaling_x", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    scaling_x = keyframe;
                }
                else if (JSON_PROPERTY_COMPARE(keyframe, "scaling_y", json_array) && JSON_ARRAY_LENGTH(keyframe->value)) {
                    scaling_y = keyframe;
                }
            }
        }
    }

    animation->frames *= animation->timescale;

    if (!FLAG(animation->flags, ANIMATION_DISABLED) && FLAG(animation->flags, ANIMATION_USE_CAMERA))
    {
        GCSR.gl->camera.type = GC_CAMERA_FOCUS_POINT;

        if (follow_train && follow_train->valid)
            GCSR.gl->camera.track_target = follow_train->models + 0;
        else if (follow_object)
            GCSR.gl->camera.track_target = follow_object;
    }

    if (!FLAG(animation->flags, ANIMATION_DISABLED) && !FLAG(animation->flags, ANIMATION_USE_CAMERA))
    {
        // A train was specified, generate the targets.
        if (train_index >= 0 && train_index < (s32) data->train_count)
        {
            train_t *train = data->trains + train_index;

            animation->target_count = train->size;
            animation->targets = (animation_target_t *) stack_push(manager->stack, sizeof(animation_target_t) * train->size);

            s32 base_offset = train->base_frame_offset;

            for (u32 i = 0; i < train->size; ++i)
            {
                animation_target_t *target = animation->targets + i;

                target->model = train->models + i;
                target->offset = base_offset;

                if (i == 0)
                    base_offset += train->locomotive_frame_offset;
                else
                    base_offset += train->wagon_frame_offset;

                // Send the model to the triangle queue.
                PUSH_TRIANGLE(target->model);
            }
        }

        // Read the targets.
        else if (target_count && targets)
        {
            r32 one_over_timescale = 1.0f / animation->timescale;
            s32 old_offset = 0;
            s32 old_new_offset = 0;

            animation->target_count = target_count;
            animation->targets = (animation_target_t *) stack_push(manager->stack, sizeof(animation_target_t) * animation->target_count);

            JSON_VALUE_ARRAY_LOOP(targets->value, ti)
            {
                struct _json_value *target_prop = JSON_ARRAY_VALUE(targets->value, ti);
                animation_target_t *target = animation->targets + ti;

                target->model = 0;
                target->offset = 0;

                JSON_VALUE_OBJECT_LOOP(target_prop, tpi)
                {
                    struct _json_object_entry *tp = JSON_OBJECT_PROPERTY(target_prop, tpi);

                    if (JSON_PROPERTY_COMPARE(tp, "model", json_integer))
                    {
                        u32 model_index = JSON_PROPERTY_VALUE_INTEGER(tp);

                        if (model_index < data->level->model_count)
                            target->model = data->level->models + model_index;
                        else
                        {
                            printf("[ANIMATION]: Invalid target model index !\n");
                            FLAG_ENABLE(animation->flags, ANIMATION_DISABLED);
                        }
                    }

                    else if (JSON_PROPERTY_COMPARE(tp, "offset", json_integer)) {
                        target->offset = JSON_PROPERTY_VALUE_INTEGER(tp);
                    }
                }
            }
        }
    }

    // -- Allocate memory for the keyframes.

    if (offset) offset_count = JSON_ARRAY_LENGTH(offset->value);
    if (roll) roll_count = JSON_ARRAY_LENGTH(roll->value);
    if (pitch) pitch_count = JSON_ARRAY_LENGTH(pitch->value);
    if (heading) heading_count = JSON_ARRAY_LENGTH(heading->value);
    if (horizontal) horizontal_count = JSON_ARRAY_LENGTH(horizontal->value);
    if (vertical) vertical_count = JSON_ARRAY_LENGTH(vertical->value);
    if (scaling_x) scaling_x_count = JSON_ARRAY_LENGTH(scaling_x->value);
    if (scaling_y) scaling_y_count = JSON_ARRAY_LENGTH(scaling_y->value);

    size_t offset_bytes = sizeof(keyframe_t) * offset_count;
    size_t roll_bytes = sizeof(keyframe_t) * roll_count;
    size_t pitch_bytes = sizeof(keyframe_t) * pitch_count;
    size_t heading_bytes = sizeof(keyframe_t) * heading_count;
    size_t horizontal_bytes = sizeof(keyframe_t) * horizontal_count;
    size_t vertical_bytes = sizeof(keyframe_t) * vertical_count;
    size_t scaling_x_bytes = sizeof(keyframe_t) * scaling_x_count;
    size_t scaling_y_bytes = sizeof(keyframe_t) * scaling_y_count;

    size_t total_bytes = offset_bytes +
                         roll_bytes +
                         pitch_bytes +
                         heading_bytes +
                         horizontal_bytes +
                         vertical_bytes +
                         scaling_x_bytes +
                         scaling_y_bytes;

    keyframe_t *keyframe_pointer = (keyframe_t *) stack_push(manager->stack, total_bytes);
    size_t pointer_offset = 0;

    if (offset)
    {
        animation->offset_keyframes = offset_count;
        animation->offset = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += offset_bytes;

        _read_keyframe_data(animation->offset, offset, animation->timescale);
    }

    if (roll)
    {
        animation->roll_keyframes = roll_count;
        animation->roll = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += roll_bytes;

        _read_keyframe_data(animation->roll, roll, animation->timescale);
    }

    if (pitch)
    {
        animation->pitch_keyframes = pitch_count;
        animation->pitch = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += pitch_bytes;

        _read_keyframe_data(animation->pitch, pitch, animation->timescale);
    }

    if (heading)
    {
        animation->heading_keyframes = heading_count;
        animation->heading = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += heading_bytes;

        _read_keyframe_data(animation->heading, heading, animation->timescale);
    }

    if (horizontal)
    {
        animation->horizontal_keyframes = horizontal_count;
        animation->horizontal = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += horizontal_bytes;

        _read_keyframe_data(animation->horizontal, horizontal, animation->timescale);
    }

    if (vertical)
    {
        animation->vertical_keyframes = vertical_count;
        animation->vertical = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += vertical_bytes;

        _read_keyframe_data(animation->vertical, vertical, animation->timescale);
    }

    if (scaling_x)
    {
        animation->scaling_x_keyframes = scaling_x_count;
        animation->scaling_x = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += scaling_x_bytes;

        _read_keyframe_data(animation->scaling_x, scaling_x, animation->timescale);
    }

    if (scaling_y)
    {
        animation->scaling_y_keyframes = scaling_y_count;
        animation->scaling_y = (keyframe_t *) ADDR_OFFSET(keyframe_pointer, pointer_offset);
        pointer_offset += scaling_y_bytes;

        _read_keyframe_data(animation->scaling_y, scaling_y, animation->timescale);
    }
}

void update_curve_point_rotation(bezier_curve_t *curve)
{
    gc_camera_t *camera = &GCSR.gl->camera;
}

void _generate_lut(bezier_curve_t *curve)
{
    memory_manager_t *manager = &GCSR.state->manager;

    r32 lut_step = 1.0f / (curve->lut_samples - 1);
    curve->lut = (bezier_lut_sample_t *) stack_push(manager->stack, sizeof(bezier_lut_sample_t) * curve->lut_samples);
    bezier_lut_sample_t *lut_pointer = curve->lut;
    gc_vec_t prev_point;

    for (u32 i = 0; i < curve->lut_samples; ++i)
    {
        r32 tg = i * lut_step;
        // r32 tc = bezier_tc(curve->section_count, tg);
        gc_vec_t point = bezier_compute_point(curve, tg);

        if (i == 0)
        {
            lut_pointer->distance = 0;
            lut_pointer->t = tg;
        }
        else
        {
            gc_vec_t v;

            v.data[0] = point.data[0] - prev_point.data[0];
            v.data[1] = point.data[1] - prev_point.data[1];
            v.data[2] = point.data[2] - prev_point.data[2];

            bezier_lut_sample_t *prev_lut = lut_pointer - 1;

            lut_pointer->distance = prev_lut->distance + gl_vec3_len(&v);
            lut_pointer->t = tg;
        }

        prev_point = point;
        lut_pointer++;
    }
}

void _generate_debug_points(bezier_curve_t *curve)
{
    memory_manager_t *manager = &GCSR.state->manager;

    mesh_t *debug_point_mesh = (mesh_t *) stack_push(manager->stack, sizeof(mesh_t));
    curve->debug_points_model = (gc_model_t *) stack_push(manager->stack, sizeof(gc_model_t) * curve->dbgp_count);

    debug_point_mesh->type = GL_MESH_POINT;
    debug_point_mesh->indices_count = 1;
    debug_point_mesh->indices = (u32 *) stack_push(manager->stack, sizeof(u32));
    debug_point_mesh->vertices = (asset_vertex_t *) stack_push(manager->stack, sizeof(asset_vertex_t));

    *debug_point_mesh->indices = 0;

    debug_point_mesh->vertices->pos[0] = 0;
    debug_point_mesh->vertices->pos[1] = 0;
    debug_point_mesh->vertices->pos[2] = 0;
    debug_point_mesh->vertices->pos[3] = 1;

    r32 step = 1.0f / curve->dbgp_count;

    for (u32 i = 0; i < curve->dbgp_count; ++i)
    {
        gc_model_t *model = curve->debug_points_model + i;

        model->shader_id = SHADER_NONE;
        model->material = 0;

        model->disabled = false;
        model->meshes[0] = 0;
        model->meshes[1] = 0;
        model->meshes[2] = debug_point_mesh;

        model->object.position.v3.x = 0;
        model->object.position.v3.y = 0;
        model->object.position.v3.z = 0;

        model->object.rotation.v3.x = 0;
        model->object.rotation.v3.y = 0;
        model->object.rotation.v3.z = 0;

        model->object.scaling.v3.x = 1;
        model->object.scaling.v3.y = 1;
        model->object.scaling.v3.z = 1;

        model->object.t = i * step;

        PUSH_POINT(model);
    }
}

void _generate_curve_points(bezier_curve_t *curve)
{
    memory_manager_t *manager = &GCSR.state->manager;

    curve->points_mesh = (mesh_t *) stack_push(manager->stack, sizeof(mesh_t));
    curve->points_model = (gc_model_t *) stack_push(manager->stack, sizeof(gc_model_t));

    u32 *index_buffer = (u32 *) stack_push(manager->stack, sizeof(u32) * curve->point_count * 3);
    asset_vertex_t *vertex_buffer = (asset_vertex_t *) stack_push(manager->stack, sizeof(asset_vertex_t) * curve->point_count * 3);

    curve->points_mesh->type = GL_MESH_POINT;
    curve->points_mesh->indices_count = curve->point_count * 3;
    curve->points_mesh->indices = index_buffer;
    curve->points_mesh->vertices = vertex_buffer;

    curve->points_model->shader_id = SHADER_NONE;
    curve->points_model->material = 0;

    curve->points_model->disabled = false;
    curve->points_model->meshes[0] = 0;
    curve->points_model->meshes[1] = 0;
    curve->points_model->meshes[2] = curve->points_mesh;

    curve->points_model->object.position.v3.x = 0;
    curve->points_model->object.position.v3.y = 0;
    curve->points_model->object.position.v3.z = 0;

    curve->points_model->object.rotation.v3.x = 0;
    curve->points_model->object.rotation.v3.y = 0;
    curve->points_model->object.rotation.v3.z = 0;

    curve->points_model->object.scaling.v3.x = 1;
    curve->points_model->object.scaling.v3.y = 1;
    curve->points_model->object.scaling.v3.z = 1;

    u32 *index_pointer = index_buffer;
    asset_vertex_t *vertex_pointer = vertex_buffer;

    // Mesh data.
    for (u32 i = 0; i < curve->point_count; ++i)
    {
        u32 base_index = i * 3;
        bezier_point_t *point = curve->points + i;

        for (u32 j = 0; j < 3; ++j)
        {
            *index_pointer++ = base_index + j;

            vertex_pointer->pos[0] = point->p[j].data[0];
            vertex_pointer->pos[1] = point->p[j].data[1];
            vertex_pointer->pos[2] = point->p[j].data[2];
            vertex_pointer->pos[3] = 1;

            vertex_pointer++;
        }
    }

    PUSH_POINT(curve->points_model);
}

void _generate_curve_tangents(bezier_curve_t *curve)
{
    memory_manager_t *manager = &GCSR.state->manager;

    curve->tangents_mesh = (mesh_t *) stack_push(manager->stack, sizeof(mesh_t));
    curve->tangents_model = (gc_model_t *) stack_push(manager->stack, sizeof(gc_model_t));

    u32 *index_buffer = (u32 *) stack_push(manager->stack, sizeof(u32) * curve->point_count * 2 * 2);
    asset_vertex_t *vertex_buffer = (asset_vertex_t *) stack_push(manager->stack, sizeof(asset_vertex_t) * curve->point_count * 3);

    curve->tangents_mesh->type = GL_MESH_LINE;
    curve->tangents_mesh->indices_count = curve->point_count * 2 * 2;
    curve->tangents_mesh->indices = index_buffer;
    curve->tangents_mesh->vertices = vertex_buffer;

    curve->tangents_model->shader_id = SHADER_NONE;
    curve->tangents_model->material = 0;

    curve->tangents_model->disabled = false;
    curve->tangents_model->meshes[0] = 0;
    curve->tangents_model->meshes[1] = curve->tangents_mesh;
    curve->tangents_model->meshes[2] = 0;

    curve->tangents_model->object.position.v3.x = 0;
    curve->tangents_model->object.position.v3.y = 0;
    curve->tangents_model->object.position.v3.z = 0;

    curve->tangents_model->object.rotation.v3.x = 0;
    curve->tangents_model->object.rotation.v3.y = 0;
    curve->tangents_model->object.rotation.v3.z = 0;

    curve->tangents_model->object.scaling.v3.x = 1;
    curve->tangents_model->object.scaling.v3.y = 1;
    curve->tangents_model->object.scaling.v3.z = 1;

    MODEL_OVERWRITE_VECTOR_SET(curve->tangents_model, wireframe_color, 1, 0, 0, 1);
    PREMULT_ALPHA(curve->tangents_model->overwrites.wireframe_color.value.u_vector);

    u32 *index_pointer = index_buffer;
    asset_vertex_t *vertex_pointer = vertex_buffer;

    // Mesh data.
    for (u32 i = 0; i < curve->point_count; ++i)
    {
        u32 base_index = i * 3;
        bezier_point_t *point = curve->points + i;

        gc_vec_t *base_point = point->p + 0;
        gc_vec_t *control_point_0 = point->p + 1;
        gc_vec_t *control_point_1 = point->p + 2;

        vertex_pointer->pos[0] = base_point->data[0];
        vertex_pointer->pos[1] = base_point->data[1];
        vertex_pointer->pos[2] = base_point->data[2];
        vertex_pointer->pos[3] = 1;

        vertex_pointer++;

        vertex_pointer->pos[0] = control_point_0->data[0];
        vertex_pointer->pos[1] = control_point_0->data[1];
        vertex_pointer->pos[2] = control_point_0->data[2];
        vertex_pointer->pos[3] = 1;

        vertex_pointer++;

        vertex_pointer->pos[0] = control_point_1->data[0];
        vertex_pointer->pos[1] = control_point_1->data[1];
        vertex_pointer->pos[2] = control_point_1->data[2];
        vertex_pointer->pos[3] = 1;

        vertex_pointer++;

        *index_pointer++ = base_index;
        *index_pointer++ = base_index + 1;
        *index_pointer++ = base_index;
        *index_pointer++ = base_index + 2;
    }

    PUSH_LINE(curve->tangents_model);
}

void _generate_curve_mesh(bezier_curve_t *curve)
{
    memory_manager_t *manager = &GCSR.state->manager;

    curve->base_mesh = (mesh_t *) stack_push(manager->stack, sizeof(mesh_t));
    curve->base_model = (gc_model_t *) stack_push(manager->stack, sizeof(gc_model_t));

    if (!curve->samples)
        return;

    curve->base_mesh->type = GL_MESH_LINE;
    curve->base_mesh->indices_count = 0;
    curve->base_mesh->indices = 0;
    curve->base_mesh->vertices = 0;

    curve->base_model->disabled = false;
    curve->base_model->meshes[0] = 0;
    curve->base_model->meshes[1] = curve->base_mesh;
    curve->base_model->meshes[2] = 0;

    curve->base_model->object.transforms.count = 0;

    curve->base_model->object.position.v3.x = 0;
    curve->base_model->object.position.v3.y = 0;
    curve->base_model->object.position.v3.z = 0;

    curve->base_model->object.rotation.v3.x = 0;
    curve->base_model->object.rotation.v3.y = 0;
    curve->base_model->object.rotation.v3.z = 0;

    curve->base_model->object.scaling.v3.x = 1;
    curve->base_model->object.scaling.v3.y = 1;
    curve->base_model->object.scaling.v3.z = 1;

    r32 tg = 0;
    r32 sample_step = 1.0f / (curve->samples - 1);
    u32 line_count = curve->samples - 1;
    u32 *index_buffer = (u32 *) stack_push(manager->stack, sizeof(u32) * line_count * 2);
    asset_vertex_t *vertex_buffer = (asset_vertex_t *) stack_push(manager->stack, sizeof(asset_vertex_t) * curve->samples);

    u32 *index_pointer = index_buffer;
    asset_vertex_t *vertex_pointer = vertex_buffer;

    curve->base_mesh->type = GL_MESH_LINE;
    curve->base_mesh->indices_count = line_count * 2;
    curve->base_mesh->indices = index_buffer;
    curve->base_mesh->vertices = vertex_buffer;

    // Vertices.
    for (u32 i = 0; i < curve->samples; ++i)
    {
        // r32 tc = bezier_tc(curve->section_count, tg);
        gc_vec_t point = bezier_compute_point(curve, tg);

        asset_vertex_t *vertex = vertex_pointer + i;

        vertex->pos[0] = point.data[0];
        vertex->pos[1] = point.data[1];
        vertex->pos[2] = point.data[2];
        vertex->pos[3] = 1;

        tg += sample_step;
    }

    // Indices.
    for (u32 i = 0; i < line_count; ++i)
    {
        index_pointer[i * 2] = i;
        index_pointer[i * 2 + 1] = i + 1;
    }

    PUSH_LINE(curve->base_model);
}

void _json_read_train(struct _json_value *train_data, train_t *train, gc_level_t *level)
{
    memory_manager_t *manager = &GCSR.state->manager;

    if (train_data->type == json_object)
    {
        train->valid = true;

        train->locomotive_meshes[0] = 0;
        train->locomotive_meshes[1] = 0;
        train->locomotive_meshes[2] = 0;

        train->wagon_meshes[0] = 0;
        train->wagon_meshes[1] = 0;
        train->wagon_meshes[2] = 0;

        train->locomotive_material = 0;
        train->wagon_material = 0;

        train->transform.translation.v3.x = 0;
        train->transform.translation.v3.y = 0;
        train->transform.translation.v3.z = 0;

        train->transform.rotation.v3.x = 0;
        train->transform.rotation.v3.y = 0;
        train->transform.rotation.v3.z = 0;

        train->transform.scaling.v3.x = 1;
        train->transform.scaling.v3.y = 1;
        train->transform.scaling.v3.z = 1;

        train->size = 3;
        train->base_frame_offset = 0;
        train->locomotive_frame_offset = -4;
        train->wagon_frame_offset = -4;

        JSON_VALUE_OBJECT_LOOP(train_data, i)
        {
            struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(train_data, i);

            if (JSON_PROPERTY_COMPARE(prop, "locomotive_meshes", json_array) && JSON_ARRAY_LENGTH(prop->value) == 3)
            {
                for (u8 j = 0; j < 3; ++j)
                {
                    u32 mesh_index = JSON_ARRAY_VALUE_INTEGER(prop->value, j);

                    if (mesh_index < level->mesh_count)
                        train->locomotive_meshes[j] = (mesh_t *) level->meshes[mesh_index];
                    else
                    {
                        printf("[TRAIN] Invalid locomotive mesh index !\n");
                        train->valid = false;
                    }
                }
            }

            else if (JSON_PROPERTY_COMPARE(prop, "wagon_meshes", json_array) && JSON_ARRAY_LENGTH(prop->value) == 3)
            {
                for (u8 j = 0; j < 3; ++j)
                {
                    u32 mesh_index = JSON_ARRAY_VALUE_INTEGER(prop->value, j);

                    if (mesh_index < level->mesh_count)
                        train->wagon_meshes[j] = (mesh_t *) level->meshes[mesh_index];
                    else
                    {
                        printf("[TRAIN] Invalid wagon mesh index !\n");
                        train->valid = false;
                    }
                }
            }

            else if (JSON_PROPERTY_COMPARE(prop, "locomotive_material", json_integer))
            {
                u32 material_index = JSON_PROPERTY_VALUE_INTEGER(prop);

                if (material_index >= 0 && material_index < level->material_count)
                    train->locomotive_material = level->materials + material_index;
                else
                {
                    printf("[TRAIN] Invalid locomotive material index !\n");
                    train->valid = false;
                }
            }

            else if (JSON_PROPERTY_COMPARE(prop, "wagon_material", json_integer))
            {
                u32 material_index = JSON_PROPERTY_VALUE_INTEGER(prop);

                if (material_index >= 0 && material_index < level->material_count)
                    train->wagon_material = level->materials + material_index;
                else
                {
                    printf("[TRAIN] Invalid wagon material index !\n");
                    train->valid = false;
                }
            }

            else if (JSON_PROPERTY_COMPARE(prop, "transform", json_object)) {
                _json_read_transform(prop->value, &train->transform);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "size", json_integer)) {
                train->size = JSON_PROPERTY_VALUE_INTEGER(prop);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "base_frame_offset", json_integer)) {
                train->base_frame_offset = JSON_PROPERTY_VALUE_INTEGER(prop);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "locomotive_frame_offset", json_integer)) {
                train->locomotive_frame_offset = JSON_PROPERTY_VALUE_INTEGER(prop);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "wagon_frame_offset", json_integer)) {
                train->wagon_frame_offset = JSON_PROPERTY_VALUE_INTEGER(prop);
            }
        }
    }
}

void _json_read_curve(struct _json_value *curve_data, bezier_curve_t *curve)
{
    memory_manager_t *manager = &GCSR.state->manager;

    if (curve_data->type == json_object)
    {
        struct _json_value *point_array = 0;

        // Default values.
        curve->flags = 0;

        curve->dbgp_count = 20;
        curve->dbgp_cycle_ms = 10000;
        curve->dbgp_step = 1.0f / curve->dbgp_cycle_ms;
        curve->dbgp_current_ms = 0;

        JSON_VALUE_OBJECT_LOOP(curve_data, i)
        {
            struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(curve_data, i);

            if (JSON_PROPERTY_COMPARE(prop, "transform", json_object)) {
                _json_read_transform(prop->value, &curve->transform);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "global_scale", json_double))
            {
                curve->global_scale = JSON_PROPERTY_VALUE_DOUBLE(prop);
                curve->global_scale = curve->global_scale ? curve->global_scale : 1;
            }

            else if (JSON_PROPERTY_COMPARE(prop, "flags", json_object)) {
                _json_read_flags(prop->value, curve);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "points", json_array))
            {
                point_array = prop->value;
                curve->point_count = JSON_ARRAY_LENGTH(point_array);
            }
        }

        if (curve->point_count)
        {
            curve->section_count = curve->point_count - 1;

            if (curve->flags & BEZIER_CLOSED_PATH)
                curve->section_count++;

            curve->samples *= curve->section_count;
            curve->lut_samples *= curve->section_count;
            curve->points = (bezier_point_t *) stack_push(manager->stack, sizeof(bezier_point_t) * curve->point_count);

            _json_read_points(point_array, curve->points, curve->global_scale);
            _generate_sections(curve);
            _generate_lut(curve);

            if (curve->flags & BEZIER_DEBUG)
                _generate_curve_mesh(curve);

            if (curve->flags & BEZIER_DEBUG_TANGENTS)
            {
                _generate_curve_points(curve);
                _generate_curve_tangents(curve);
            }

            if (curve->flags & BEZIER_DEBUG_POINTS)
                _generate_debug_points(curve);
        }
    }
}

void *_setup_bezier(gc_level_t *level)
{
    memory_manager_t *manager = &GCSR.state->manager;
    bezier_program_t *bezier_data = (bezier_program_t *) stack_push(manager->stack, sizeof(bezier_program_t));

    bezier_data->is_spacestation = false;
    bezier_data->level = level;

    if (level->program_settings)
    {
        u32 train_count = 0;
        u32 curve_count = 0;
        u32 animation_count = 0;

        u32 target_fps = 30;
        u32 samples = 5;
        u32 lut_samples = 10;
        r32 global_scale = 1;

        struct _json_object_entry *trains = 0;
        struct _json_object_entry *curves = 0;
        struct _json_object_entry *animations = 0;

        JSON_VALUE_OBJECT_LOOP(level->program_settings->value, si)
        {
            struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(level->program_settings->value, si);

            if (JSON_PROPERTY_COMPARE(prop, "is_spacestation", json_boolean) && JSON_PROPERTY_VALUE_BOOL(prop)) {
                bezier_data->is_spacestation = true;
            }

            else if (JSON_PROPERTY_COMPARE(prop, "curves", json_array) && JSON_ARRAY_LENGTH(prop->value))
            {
                curves = prop;
                curve_count = JSON_ARRAY_LENGTH(prop->value);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "trains", json_array) && JSON_ARRAY_LENGTH(prop->value))
            {
                trains = prop;
                train_count = JSON_ARRAY_LENGTH(prop->value);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "animations", json_array) && JSON_ARRAY_LENGTH(prop->value))
            {
                animations = prop;
                animation_count = JSON_ARRAY_LENGTH(prop->value);
            }

            else if (JSON_PROPERTY_COMPARE(prop, "samples", json_integer))
            {
                samples = JSON_PROPERTY_VALUE_INTEGER(prop);
                samples = samples ? samples : 5;
            }

            else if (JSON_PROPERTY_COMPARE(prop, "lut_samples", json_integer))
            {
                lut_samples = JSON_PROPERTY_VALUE_INTEGER(prop);
                lut_samples = lut_samples ? lut_samples : 10;
            }

            else if (JSON_PROPERTY_COMPARE(prop, "target_fps", json_integer)) {
                target_fps = JSON_PROPERTY_VALUE_INTEGER(prop);
            }
        }

        if (bezier_data->is_spacestation && bezier_data->level->model_count > 4)
        {
            gc_model_t *set_0 = bezier_data->level->models + 1;
            gc_model_t *set_1 = bezier_data->level->models + 2;
            gc_model_t *set_2 = bezier_data->level->models + 3;

            set_0->object.rtype = EULER_ZYX;
            set_1->object.rtype = EULER_ZYX;
            set_2->object.rtype = EULER_ZYX;
        }
        else
            bezier_data->is_spacestation = false;

        // ----------------------------------------------------------------------------------
        // -- Read the trains.
        // ----------------------------------------------------------------------------------

        if (trains && train_count)
        {
            bezier_data->train_count = train_count;
            bezier_data->trains = (train_t *) stack_push(manager->stack, sizeof(train_t) * train_count);

            dynamic_array_header_t *queue = da_header(GCSR.gl->triangle_queue);

            JSON_VALUE_ARRAY_LOOP (trains->value, i)
            {
                struct _json_value *train_data = JSON_ARRAY_VALUE(trains->value, i);
                train_t *train = bezier_data->trains + i;

                _json_read_train(train_data, train, level);

                // Generate train models.

                if (train->size)
                {
                    train->models = (gc_model_t *) stack_push(manager->stack, sizeof(gc_model_t) * train->size);

                    for (u32 j = 0; j < train->size; ++j)
                    {
                        gc_model_t *model = train->models + j;

                        model->shader_id = SHADER_NONE;
                        model->material = 0;
                        model->disabled = false;

                        model->meshes[0] = 0;
                        model->meshes[1] = 0;
                        model->meshes[2] = 0;

                        model->object.position.v3.x = train->transform.translation.v3.x;
                        model->object.position.v3.y = train->transform.translation.v3.y;
                        model->object.position.v3.z = train->transform.translation.v3.z;

                        model->object.rotation.v3.x = train->transform.rotation.v3.x;
                        model->object.rotation.v3.y = train->transform.rotation.v3.y;
                        model->object.rotation.v3.z = train->transform.rotation.v3.z;

                        model->object.scaling.v3.x = train->transform.scaling.v3.x;
                        model->object.scaling.v3.y = train->transform.scaling.v3.y;
                        model->object.scaling.v3.z = train->transform.scaling.v3.z;

                        if (j == 0)
                        {
                            model->material = train->locomotive_material;

                            model->meshes[0] = train->locomotive_meshes[0];
                            model->meshes[1] = train->locomotive_meshes[1];
                            model->meshes[2] = train->locomotive_meshes[2];
                        }
                        else
                        {
                            model->material = train->wagon_material;

                            model->meshes[0] = train->wagon_meshes[0];
                            model->meshes[1] = train->wagon_meshes[1];
                            model->meshes[2] = train->wagon_meshes[2];
                        }
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // -- Read the curves.
        // ----------------------------------------------------------------------------------

        if (curves && curve_count)
        {
            bezier_data->curve_count = curve_count;
            bezier_data->curves = (bezier_curve_t *) stack_push(manager->stack, sizeof(bezier_curve_t) * curve_count);

            JSON_VALUE_ARRAY_LOOP (curves->value, i)
            {
                struct _json_value *curve_data = JSON_ARRAY_VALUE(curves->value, i);
                bezier_curve_t *curve = bezier_data->curves + i;

                curve->samples = samples;
                curve->lut_samples = lut_samples;
                curve->global_scale = global_scale;

                _json_read_curve(curve_data, curve);
            }

            // Read the animations.

            bezier_data->animation_count = animation_count;
            bezier_data->animations = (animation_t *) stack_push(manager->stack, sizeof(animation_t) * animation_count);

            JSON_VALUE_ARRAY_LOOP (animations->value, i)
            {
                struct _json_value *animation_data = JSON_ARRAY_VALUE(animations->value, i);

                animation_t *animation = bezier_data->animations + i;
                animation->target_fps = target_fps;

                _json_read_animation(animation_data, animation, bezier_data);
            }
        }
    }

    return bezier_data;
}

void _update_bezier(r32 delta, void *data)
{
    bezier_program_t *bezier_data = (bezier_program_t *) data;

    gc_level_t *level = bezier_data->level;
    gc_model_t *cube = level->models + 0;

    // Debug points for each curve.
    for (u8 k = 0; k < bezier_data->curve_count; ++k)
    {
        bezier_curve_t *curve = bezier_data->curves + k;

        if (!curve->point_count)
            continue;

        if (curve->flags & BEZIER_DEBUG)
        {
            MODEL_TRANSFORM_RESET(curve->base_model);
            model_transform_push(curve->base_model,
                                 &curve->transform.translation,
                                 &curve->transform.rotation,
                                 &curve->transform.scaling,
                                 EULER_XYZ);
            MODEL_PUSH_BASE_TRANSFORM(curve->base_model);
        }

        if (curve->flags & BEZIER_DEBUG_POINTS)
        {
            r32 t_step_incr = delta * curve->dbgp_step;

            for (u32 i = 0; i < curve->dbgp_count; ++i)
            {
                gc_model_t *model = curve->debug_points_model + i;

                r32 tg = model->object.t;

                if (curve->flags & BEZIER_SYNCHRONIZE)
                    tg = bezier_sync_to_lut_from_tg(curve, tg);

                // r32 tc = bezier_tc(curve->section_count, tg);
                gc_vec_t new_position = bezier_compute_point(curve, tg);

                model->object.position.data[0] = new_position.data[0];
                model->object.position.data[1] = new_position.data[1];
                model->object.position.data[2] = new_position.data[2];

                model->object.t += t_step_incr;

                if (model->object.t > 1)
                    model->object.t -= floorf(model->object.t);

                MODEL_TRANSFORM_RESET(model);
                model_transform_push(model,
                                     &curve->transform.translation,
                                     &curve->transform.rotation,
                                     &curve->transform.scaling,
                                     EULER_XYZ);
                MODEL_PUSH_BASE_TRANSFORM(model);
            }
        }
    }

    // Spacestation animation (rotation).
    if (bezier_data->is_spacestation)
    {
        gc_model_t *set_0 = bezier_data->level->models + 1;
        gc_model_t *set_1 = bezier_data->level->models + 2;
        gc_model_t *set_2 = bezier_data->level->models + 3;

        // Full rotation at every 15 seconds.
        r32 speed = 2 * PI / (15000);
        r32 angle_incr = speed * delta;

        set_0->object.rotation.v3.z -= RAD2DEG(angle_incr);
        set_1->object.rotation.v3.z -= RAD2DEG(angle_incr);
        set_2->object.rotation.v3.z -= RAD2DEG(angle_incr);
    }

    // Animations.
    for (u32 i = 0; i < bezier_data->animation_count; ++i)
    {
        animation_t *animation = bezier_data->animations + i;
        update_animation(animation, delta);
    }

    // Update the shadow maps.
    for (u32 i = 0; i < bezier_data->level->light_count; ++i)
    {
        gc_light_t *light = bezier_data->level->lights + i;

        if (light->type == GC_SUN_LIGHT || light->type == GC_POINT_LIGHT)
            light->updated = true;
    }
}