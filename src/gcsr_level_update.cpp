// ----------------------------------------------------------------------------------
// -- File: gcsr_level_update.cpp
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2021-12-22 20:31:54
// -- Modified:
// ----------------------------------------------------------------------------------

#include "level/level_void.cpp"
#include "level/african_head_level.cpp"

void init_level_callback_table(engine_state_t *state)
{
    state->level_update_table[LEVEL_VOID - 1] = level_void_update;
    state->level_update_table[LEVEL_AFRICAN_HEAD - 1] = african_head_level_update;
}