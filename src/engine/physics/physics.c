#include <linmath.h>
#include "../global.h"
#include "../array_list.h"
#include "../util.h"
#include "../physics.h"
#include "physics_internal.h"

static Physics_State_Internal state;

void physics_init(void) {
	state.body_list = array_list_create(sizeof(Body), 0);
}

void physics_update(void) {
	Body *body;

	for (u32 i = 0; i < state.body_list->len; ++i) {
		body = array_list_get(state.body_list, i);
		body->velocity[0] += body->acceleration[0] * global.time.delta;
		body->velocity[1] += body->acceleration[1] * global.time.delta;
		body->aabb.position[0] += body->velocity[0] * global.time.delta;
		body->aabb.position[1] += body->velocity[1] * global.time.delta;
	}
}

usize physics_body_create(vec2 position, vec2 size) {
	Body body = {
		.aabb = {
			.position = { position[0], position[1] },
			.half_size = { size[0] * 0.5, size[1] * 0.5 },
		},
		.velocity = { 0, 0 },
	};

	if (array_list_append(state.body_list, &body) == (usize)-1)
		ERROR_EXIT("Could not append body to list\n");

	return state.body_list->len - 1;
}

Body *physics_body_get(usize index) {
	return array_list_get(state.body_list, index);
}


