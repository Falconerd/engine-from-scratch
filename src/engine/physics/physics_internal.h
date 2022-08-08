#pragma once

#include "../array_list.h"
#include "../types.h"

typedef struct physics_state_internal {
	f32 gravity;
	f32 terminal_velocity;
	Array_List *body_list;
	Array_List *static_body_list;
} Physics_State_Internal;

