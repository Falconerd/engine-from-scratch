#pragma once

#include "../array_list.h"
#include "../types.h"

typedef struct physics_state_internal {
	f32 gravity;
	Array_List *body_list;
} Physics_State_Internal;

