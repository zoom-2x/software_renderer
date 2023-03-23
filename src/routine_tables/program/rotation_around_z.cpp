// ----------------------------------------------------------------------------------
// -- File: rotation_around_z.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-12-10 21:14:09
// -- Modified: 2022-12-10 21:14:10
// ----------------------------------------------------------------------------------

extern global_vars_t GCSR;

typedef struct
{
    gc_level_t *level;
} rotation_around_z_data_t;

void *_setup_rotation_around_z(gc_level_t *level)
{
    memory_manager_t *manager = &GCSR.state->manager;
    rotation_around_z_data_t *data = (rotation_around_z_data_t *) stack_push(manager->stack, sizeof(rotation_around_z_data_t));

    data->level = level;

    return data;
}

void _update_rotation_around_z(r32 delta, void *data)
{
    rotation_around_z_data_t *params = (rotation_around_z_data_t *) data;

    dynamic_array_header_t *queue_0 = da_header(GCSR.gl->triangle_queue);
    dynamic_array_header_t *queue_1 = da_header(GCSR.gl->line_queue);
    dynamic_array_header_t *queue_2 = da_header(GCSR.gl->point_queue);

    for (u32 i = 0; i < queue_0->length; ++i)
    {
        gc_model_t *model = GCSR.gl->triangle_queue[i];

        if (model->shader_id == SHADER_SKYBOX || model->shader_id == SHADER_SKYBOX_PBR)
            continue;

        model->object.rotation.v3.z += 1.0f;
    }

    // for (u32 i = 0; i < queue_1->length; ++i)
    // {
    //     gc_model_t *model = GCSR.gl->line_queue[i];
    //     model->object.rotation.v3.z += 1.0f;
    // }

    // for (u32 i = 0; i < queue_2->length; ++i)
    // {
    //     gc_model_t *model = GCSR.gl->point_queue[i];
    //     model->object.rotation.v3.z += 1.0f;
    // }

    // Update the shadow maps.
    for (u32 i = 0; i < params->level->light_count; ++i)
    {
        gc_light_t *light = params->level->lights + i;

        if (light->type == GC_SUN_LIGHT || light->type == GC_POINT_LIGHT)
            light->updated = true;
    }
}
