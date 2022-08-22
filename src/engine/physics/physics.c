#include <linmath.h>
#include "../global.h"
#include "../array_list.h"
#include "../util.h"
#include "../physics.h"
#include "physics_internal.h"

static Physics_State_Internal state;

static u32 iterations = 4;
static f32 tick_rate;

void aabb_min_max(vec2 min, vec2 max, AABB aabb) {
	vec2_sub(min, aabb.position, aabb.half_size);
	vec2_add(max, aabb.position, aabb.half_size);
}

AABB aabb_minkowski_difference(AABB a, AABB b) {
	AABB result;
	vec2_sub(result.position, a.position, b.position);
	vec2_add(result.half_size, a.half_size, b.half_size);

	return result;
}

Hit ray_intersect_aabb(vec2 pos, vec2 magnitude, AABB aabb) {
	Hit hit = {0};
	vec2 min, max;
	aabb_min_max(min, max, aabb);

	f32 last_entry = -INFINITY;
	f32 first_exit = INFINITY;

	for (u8 i = 0; i < 2; ++i) {
		if (magnitude[i] != 0) {
			f32 t1 = (min[i] - pos[i]) / magnitude[i];
			f32 t2 = (max[i] - pos[i]) / magnitude[i];

			last_entry = fmaxf(last_entry, fminf(t1, t2));
			first_exit = fminf(first_exit, fmaxf(t1, t2));
		} else if (pos[i] <= min[i] || pos[i] >= max[i]) {
			return hit;
		}
	}

	if (first_exit > last_entry && first_exit > 0 && last_entry < 1) {
		hit.position[0] = pos[0] + magnitude[0] * last_entry;
		hit.position[1] = pos[1] + magnitude[1] * last_entry;

		hit.is_hit = true;
		hit.time = last_entry;

		f32 dx = hit.position[0] - aabb.position[0];
		f32 dy = hit.position[1] - aabb.position[1];
		f32 px = aabb.half_size[0] - fabsf(dx);
		f32 py = aabb.half_size[1] - fabsf(dy);

		if (px < py) {
			hit.normal[0] = (dx > 0) - (dx < 0);
		} else {
			hit.normal[1] = (dy > 0) - (dy < 0);
		}
	}

	return hit;
}

bool physics_aabb_intersect_aabb(AABB a, AABB b) {
	vec2 min, max;
	aabb_min_max(min, max, aabb_minkowski_difference(a, b));

	return (min[0] <= 0 && max[0] >= 0 && min[1] <= 0 && max[1] >= 0);
}

void aabb_penetration_vector(vec2 r, AABB aabb) {
	vec2 min, max;
	aabb_min_max(min, max, aabb);

	f32 min_dist = fabsf(min[0]);
	r[0] = min[0];
	r[1] = 0;

	if (fabsf(max[0]) < min_dist) {
		min_dist = fabsf(max[0]);
		r[0] = max[0];
	}

	if (fabsf(min[1]) < min_dist) {
		min_dist = fabsf(min[1]);
		r[0] = 0;
		r[1] = min[1];
	}

	if (fabsf(max[1]) < min_dist) {
		r[0] = 0;
		r[1] = max[1];
	}
}

bool physics_point_intersect_aabb(vec2 point, AABB aabb) {
	vec2 min, max;
	aabb_min_max(min, max, aabb);
	return  point[0] >= min[0] &&
		point[0] <= max[0] &&
		point[1] >= min[1] &&
		point[1] <= max[1];
}

void physics_init(void) {
	state.body_list = array_list_create(sizeof(Body), 0);
	state.static_body_list = array_list_create(sizeof(Static_Body), 0);

	state.gravity = -200;
	state.terminal_velocity = -10000;

	tick_rate = 1.f / iterations;
}

static Hit aabb_sweep_aabb(AABB a, AABB b, vec2 velocity) {
	AABB sum_aabb = b;
	vec2_add(sum_aabb.half_size, sum_aabb.half_size, a.half_size);

	return ray_intersect_aabb(a.position, velocity, sum_aabb);
}

static void update_sweep_result(Hit *result, Hit hit, u8 other_collision_layer, vec2 velocity) {
	result->hit_layer_mask |= other_collision_layer;

	if (hit.time < result->time) {
		*result = hit;
	} else if (hit.time == result->time) {
		// Solve highest velocity axis first.
		if (fabsf(velocity[0]) > fabsf(velocity[1]) && hit.normal[0] != 0) {
			*result = hit;
		} else if (fabsf(velocity[1]) > fabsf(velocity[0]) && hit.normal[1] != 0) {
			*result = hit;
		}
	}
}

static Hit sweep_static_bodies(Body *body, vec2 velocity) {
	Hit result = {.time = 0xBEEF};

	for (u32 i = 0; i < state.static_body_list->len; ++i) {
		Static_Body *static_body = physics_static_body_get(i);

		if ((body->collision_mask & static_body->collision_layer) == 0) {
			continue;
		}

		Hit hit = aabb_sweep_aabb(body->aabb, static_body->aabb, velocity);
		if (!hit.is_hit) {
			continue;
		}

		update_sweep_result(&result, hit, static_body->collision_layer, velocity);
	}

	return result;
}

static Hit sweep_bodies(Body *body, vec2 velocity) {
	Hit result = {.time = 0xBEEF};

	for (u32 i = 0; i < state.body_list->len; ++i) {
		Body *other = physics_body_get(i);

		if (body == other) {
			continue;
		}

		if ((body->collision_mask & other->collision_mask) == 0) {
			continue;
		}

		Hit hit = aabb_sweep_aabb(body->aabb, other->aabb, velocity);
		if (!hit.is_hit) {
			continue;
		}

		update_sweep_result(&result, hit, other->collision_layer, velocity);
	}

	return result;
}

static void sweep_response(Body *body, vec2 velocity) {
	Hit hit = sweep_static_bodies(body, velocity);
	Hit hit_moving = sweep_bodies(body, velocity);

	if (hit_moving.is_hit) {
		body->collision_layer_flags |= hit_moving.hit_layer_mask;
	}

	if (hit.is_hit) {
		body->aabb.position[0] = hit.position[0];
		body->aabb.position[1] = hit.position[1];

		body->collision_layer_flags |= hit.hit_layer_mask;

		if (hit.normal[0] != 0) {
			body->aabb.position[1] += velocity[1];
			body->velocity[0] = 0;

			if (hit.normal[0] < 0) {
				body->collision_direction_flags |= BODY_FLAGS_COLLISION_RIGHT;
			} else {
				body->collision_direction_flags |= BODY_FLAGS_COLLISION_LEFT;
			}
		} else if (hit.normal[1] != 0) {
			body->aabb.position[0] += velocity[0];
			body->velocity[1] = 0;

			if (hit.normal[1] < 0) {
				body->collision_direction_flags |= BODY_FLAGS_COLLISION_UP;
			} else {
				body->collision_direction_flags |= BODY_FLAGS_COLLISION_DOWN;
			}
		}
	} else {
		vec2_add(body->aabb.position, body->aabb.position, velocity);
	}
}

static bool aabb_intersect_aabb(AABB a, AABB b, vec2 penetration_vector) {
	AABB aabb = aabb_minkowski_difference(b, a);
	vec2 min, max;
	aabb_min_max(min, max, aabb);

	if (min[0] <= 0 && max[0] >= 0 && min[1] <= 0 && max[1] >= 0) {
		if (penetration_vector) {
			aabb_penetration_vector(penetration_vector, aabb);
		}

		return true;
	}

	return false;
}

static void stationary_response(Body *body) {
	for (u32 i = 0; i < state.static_body_list->len; ++i) {
		Static_Body *static_body = physics_static_body_get(i);

		if ((body->collision_mask & static_body->collision_layer) == 0) {
			continue;
		}

		vec2 penetration_vector;
		if (aabb_intersect_aabb(body->aabb, static_body->aabb, penetration_vector)) {
			body->collision_layer_flags |= static_body->collision_layer;
			vec2_add(body->aabb.position, body->aabb.position, penetration_vector);
		}
	}

	for (u32 i = 0; i < state.body_list->len; ++i) {
		Body *other = physics_body_get(i);
		if (other == body) {
			continue;
		}

		if (aabb_intersect_aabb(body->aabb, other->aabb, NULL)) {
			body->collision_layer_flags |= other->collision_layer;
		}
	}
}

void physics_update(void) {
	Body *body;

	for (u32 i = 0; i < state.body_list->len; ++i) {
		body = array_list_get(state.body_list, i);

		body->velocity[1] += state.gravity;
		if (state.terminal_velocity > body->velocity[1]) {
			body->velocity[1] = state.terminal_velocity;
		}

		body->velocity[0] += body->acceleration[0];
		body->velocity[1] += body->acceleration[1];

		vec2 scaled_velocity;
		vec2_scale(scaled_velocity, body->velocity, global.time.delta * tick_rate);

		body->collision_direction_flags = 0;
		body->collision_layer_flags = 0;

		// Static collisions.
		for (u32 j = 0; j < iterations; ++j) {
			sweep_response(body, scaled_velocity);
			stationary_response(body);
		}
	}
}

usize physics_body_create(vec2 position, vec2 size, vec2 velocity, u8 collision_layer, u8 collision_mask) {
	Body body = {
		.aabb = {
			.position = { position[0], position[1] },
			.half_size = { size[0] * 0.5, size[1] * 0.5 },
		},
		.velocity = { velocity[0], velocity[1] },
		.collision_layer = collision_layer,
		.collision_mask = collision_mask,
	};

	if (array_list_append(state.body_list, &body) == (usize)-1)
		ERROR_EXIT("Could not append body to list\n");

	return state.body_list->len - 1;
}

Body *physics_body_get(usize index) {
	return array_list_get(state.body_list, index);
}

usize physics_static_body_create(vec2 position, vec2 size, u8 collision_layer, u8 collision_mask) {
	Static_Body static_body = {
		.aabb = {
			.position = { position[0], position[1] },
			.half_size = { size[0] * 0.5, size[1] * 0.5 },
		},
		.collision_layer = collision_layer,
		.collision_mask = collision_mask,
	};

	if (array_list_append(state.static_body_list, &static_body) == (usize)-1)
		ERROR_EXIT("Could not append static body to list\n");

	return state.static_body_list->len - 1;
}

Static_Body *physics_static_body_get(usize index) {
	return array_list_get(state.static_body_list, index);
}

