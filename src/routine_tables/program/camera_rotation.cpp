// ----------------------------------------------------------------------------------
// -- File: camera_rotation.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2023-03-20 11:54:38
// -- Modified: 2023-03-20 11:54:39
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

typedef struct
{
    gc_level_t *level;

    r32 min_radius;
    r32 max_radius;
    r32 min_height;
    r32 max_height;
    r32 rotation_speed_ms;
    r32 radius_speed_ms;
    r32 height_speed_ms;

    r32 radius_interp;
    r32 angle_interp;
    r32 height_interp;

    r32 angle_offset;
} camera_rotation_data_t;

void *_setup_camera_rotation(gc_level_t *level)
{
    memory_manager_t *manager = &GCSR.state->manager;
    camera_rotation_data_t *data = (camera_rotation_data_t *) stack_push(manager->stack, sizeof(camera_rotation_data_t));
    gc_camera_t *camera = &GCSR.gl->camera;

    data->min_radius = 4;
    data->max_radius = 7;
    data->min_height = 1;
    data->max_height = 4;
    data->rotation_speed_ms = 360 / 40000.0f;
    data->radius_speed_ms = 360 / 30000.0f;
    data->height_speed_ms = 2.0f / 10000.0f;

    data->radius_interp = 0;
    data->angle_offset = 0;

    data->level = level;

    JSON_VALUE_OBJECT_LOOP(level->program_settings->value, si)
    {
        struct _json_object_entry *prop = JSON_OBJECT_PROPERTY(level->program_settings->value, si);

        if (JSON_PROPERTY_COMPARE(prop, "min_radius", json_double))
            data->min_radius = JSON_PROPERTY_VALUE_DOUBLE(prop);
        else if (JSON_PROPERTY_COMPARE(prop, "max_radius", json_double))
            data->max_radius = JSON_PROPERTY_VALUE_DOUBLE(prop);
        else if (JSON_PROPERTY_COMPARE(prop, "min_height", json_double))
            data->min_height = JSON_PROPERTY_VALUE_DOUBLE(prop);
        else if (JSON_PROPERTY_COMPARE(prop, "max_height", json_double))
            data->max_height = JSON_PROPERTY_VALUE_DOUBLE(prop);
        else if (JSON_PROPERTY_COMPARE(prop, "rotation_speed", json_double))
        {
            r32 speed = JSON_PROPERTY_VALUE_DOUBLE(prop);
            data->rotation_speed_ms = speed / 1000.0f;
        }
        else if (JSON_PROPERTY_COMPARE(prop, "radius_speed", json_double))
        {
            r32 speed = JSON_PROPERTY_VALUE_DOUBLE(prop);
            data->radius_speed_ms = speed / 1000.0f;
        }
        else if (JSON_PROPERTY_COMPARE(prop, "height_speed", json_double))
        {
            r32 speed = JSON_PROPERTY_VALUE_DOUBLE(prop);
            data->height_speed_ms = speed / 1000.0f;
        }
        else if (JSON_PROPERTY_COMPARE(prop, "angle_offset", json_double))
            data->angle_offset = JSON_PROPERTY_VALUE_DOUBLE(prop);;
    }

    data->angle_interp = data->angle_offset + camera->rotation.yaw;

    return data;
}

void _update_camera_rotation(r32 delta, void *data)
{
    camera_rotation_data_t *params = (camera_rotation_data_t *) data;
    gc_camera_t *camera = &GCSR.gl->camera;

    camera->manual_orientation = true;

    r32 radius_t = 0.5f * (sinf(DEG2RAD(params->radius_interp)) + 1);
    r32 height_t = 0.5f * (sinf(DEG2RAD(params->height_interp)) + 1);

    r32 radius = params->min_radius + (params->max_radius - params->min_radius) * radius_t;
    r32 height = params->min_height + (params->max_height - params->min_height) * height_t;

    camera->eye.v3.x = camera->target.v3.x + radius * cosf(DEG2RAD(params->angle_interp) - HALF_PI);
    camera->eye.v3.y = camera->target.v3.y + radius * sinf(DEG2RAD(params->angle_interp) - HALF_PI);
    camera->eye.v3.z = camera->target.v3.z + height;

    params->angle_interp += params->rotation_speed_ms * delta;
    params->radius_interp += params->radius_speed_ms * delta;
    params->height_interp += params->height_speed_ms * delta;
}