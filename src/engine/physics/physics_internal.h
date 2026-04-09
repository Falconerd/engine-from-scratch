#pragma once

#include "../types.h"
#include "../mem/arena.h"

typedef struct physics_state_internal {
	f32 gravity;
	f32 terminal_velocity;
	Arena bodies;
	Arena static_bodies;
} Physics_State_Internal;

