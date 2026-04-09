#include <linmath.h>
#include "../global.h"
#include "../util.h"
#include "../physics.h"
#include "../mem/arena.h"
#include "physics_internal.h"

static Physics_State_Internal state;

static u32 iterations = 4;
static f32 tick_rate;

#define PHYSICS_MAX_BODIES 512
#define PHYSICS_MAX_STATIC_BODIES 32

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
	u64 bodies_capacity = sizeof(Body) * PHYSICS_MAX_BODIES; 
	void *bodies = malloc(bodies_capacity);
	state.bodies = arena_create(bodies, bodies_capacity);

	u64 static_bodies_capacity = sizeof(Body) * PHYSICS_MAX_STATIC_BODIES; 
	void *static_bodies = malloc(static_bodies_capacity);
	state.static_bodies = arena_create(static_bodies, static_bodies_capacity);

	state.gravity = -79;
	state.terminal_velocity = -7000;

	tick_rate = 1.f / iterations;
}

static void update_sweep_result(Hit *result, Body *body, usize other_id, vec2 velocity) {
	Body *other = physics_body_get(other_id);

	if ((body->collision_mask & other->collision_layer) == 0) {
		return;
	}

	AABB sum_aabb = other->aabb;
	vec2_add(sum_aabb.half_size, sum_aabb.half_size, body->aabb.half_size);

	Hit hit = ray_intersect_aabb(body->aabb.position, velocity, sum_aabb);
	if (hit.is_hit) {
		if (body->on_hit && (body->collision_mask & other->collision_layer) == 0) {
			body->on_hit(body, other, hit);
		}

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

		result->other_id = other_id;
	}
}

static void update_sweep_result_static(Hit *result, Body *body, usize other_id, vec2 velocity) {
	Static_Body *static_body = physics_static_body_get(other_id);

	if ((body->collision_mask & static_body->collision_layer) == 0) {
		return;
	}

	AABB sum_aabb = static_body->aabb;
	vec2_add(sum_aabb.half_size, sum_aabb.half_size, body->aabb.half_size);

	Hit hit = ray_intersect_aabb(body->aabb.position, velocity, sum_aabb);
	if (hit.is_hit) {
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

		result->other_id = other_id;
	}
}

static Hit sweep_static_bodies(Body *body, vec2 velocity) {
	Hit result = {.time = 0xBEEF};

	for (u32 i = 0; i < ARENA_LEN(state.static_bodies, Static_Body); i += 1) {
		update_sweep_result_static(&result, body, i, velocity);
	}

	return result;
}

static Hit sweep_bodies(Body *body, vec2 velocity) {
	Hit result = {.time = 0xBEEF};

	for (u32 i = 0; i < ARENA_LEN(state.bodies, Body); i += 1) {
		Body *other = physics_body_get(i);

		if (body == other) {
			continue;
		}

		update_sweep_result(&result, body, i, velocity);
	}

	return result;
}

static void sweep_response(Body *body, vec2 velocity) {
	Hit hit = sweep_static_bodies(body, velocity);
	Hit hit_moving = sweep_bodies(body, velocity);

	if (hit_moving.is_hit) {
		if (body->on_hit != NULL) {
			body->on_hit(body, physics_body_get(hit_moving.other_id), hit_moving);
		}
	}

	if (hit.is_hit) {
		body->aabb.position[0] = hit.position[0];
		body->aabb.position[1] = hit.position[1];

		if (hit.normal[0] != 0) {
			body->aabb.position[1] += velocity[1];
			body->velocity[0] = 0;
		} else if (hit.normal[1] != 0) {
			body->aabb.position[0] += velocity[0];
			body->velocity[1] = 0;
		}

		if (body->on_hit_static != NULL) {
			body->on_hit_static(body, physics_static_body_get(hit.other_id), hit);
		}
	} else {
		vec2_add(body->aabb.position, body->aabb.position, velocity);
	}
}

static void stationary_response(Body *body) {
	for (u32 i = 0; i < ARENA_LEN(state.static_bodies, Static_Body); ++i) {
		Static_Body *static_body = physics_static_body_get(i);

		if ((body->collision_mask & static_body->collision_layer) == 0) {
			continue;
		}

		AABB aabb = aabb_minkowski_difference(static_body->aabb, body->aabb);
		vec2 min, max;
		aabb_min_max(min, max, aabb);

		if (min[0] <= 0 && max[0] >= 0 && min[1] <= 0 && max[1] >= 0) {
			vec2 penetration_vector;
			aabb_penetration_vector(penetration_vector, aabb);

			vec2_add(body->aabb.position, body->aabb.position, penetration_vector);
		}
	}

	// Check for on-hit events.
	for (usize i = 0; i < ARENA_LEN(state.bodies, Body); ++i) {
		Body *other = physics_body_get(i);

		if (!body->on_hit) {
			continue;
		}

		if ((body->collision_mask & other->collision_layer) == 0) {
			continue;
		}

		AABB aabb = aabb_minkowski_difference(other->aabb, body->aabb);
		vec2 min, max;
		aabb_min_max(min, max, aabb);

		if (min[0] <= 0 && max[0] >= 0 && min[1] <= 0 && max[1] >= 0) {
			body->on_hit(body, other, (Hit){.is_hit = true, .other_id = i});
		}
	}
}

void physics_update(void) {
	for (u32 i = 0; i < ARENA_LEN(state.bodies, Body); i += 1) {
		Body *body = (Body *)state.bodies.data + i;

		if (!body->is_active) {
			continue;
		}

		if (!body->is_kinematic) {
			body->velocity[1] += state.gravity;
			if (state.terminal_velocity > body->velocity[1]) {
				body->velocity[1] = state.terminal_velocity;
			}
		}

		body->velocity[0] += body->acceleration[0];
		body->velocity[1] += body->acceleration[1];

		vec2 scaled_velocity;
		vec2_scale(scaled_velocity, body->velocity, global.time.delta * tick_rate);

		for (u32 j = 0; j < iterations; ++j) {
			sweep_response(body, scaled_velocity);
			stationary_response(body);
		}
	}
}

usize physics_body_create(vec2 position, vec2 size, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, On_Hit on_hit, On_Hit_Static on_hit_static, usize entity_id) {
	// Find inactive Body
	usize len = state.bodies.used / sizeof(Body);
	usize id = len;

	for (usize i = 0; i < len; i += 1) {
		Body *body = (Body *)state.bodies.data + i;
		if (!body->is_active) {
			id = i;
			break;
		}
	}

	Body *body = NULL;

	if (id == len) {
		body = ARENA_PUSH(&state.bodies, Body);
		if (body == NULL) {
			ERROR_EXIT("Failed to create physics body.\n");
		}
	} else {
		body = (Body *)state.bodies.data + id;
	}

	body->aabb = (AABB){
		.position = {position[0], position[1]},
		.half_size = {size[0] * 0.5f, size[1] * 0.5f}
	};
	body->velocity[0] = velocity[0];
	body->velocity[1] = velocity[1];
	body->collision_layer = collision_layer;
	body->collision_mask = collision_mask;
	body->on_hit = on_hit;
	body->on_hit_static = on_hit_static;
	body->is_kinematic = is_kinematic;
	body->is_active = true;
    body->entity_id = entity_id;

	return id;
}

Body *physics_body_get(usize index) {
	Body *body = (Body *)state.bodies.data;
	return body + index;
}

usize physics_static_body_create(vec2 position, vec2 size, u8 collision_layer) {
	usize id = ARENA_LEN(state.static_bodies, Static_Body);
	Static_Body *static_body = ARENA_PUSH(&state.static_bodies, Static_Body);
	if (static_body == NULL) {
		ERROR_EXIT("Could not append static body to list\n");
	}

	static_body->aabb = (AABB){
		.position = {position[0], position[1]},
		.half_size = {size[0] * 0.5, size[1] * 0.5},
	};
	static_body->collision_layer = collision_layer;

	return id;
}

usize physics_trigger_create(vec2 position, vec2 size, u8 collision_layer, u8 collision_mask, On_Hit on_hit) {
    return physics_body_create(position, size, (vec2){0, 0}, collision_layer, collision_mask, true, on_hit, NULL, (usize)-1);
}

Static_Body *physics_static_body_get(usize index) {
	Static_Body *static_body = (Static_Body *)state.static_bodies.data;
	return static_body + index;
}

usize physics_static_body_count() {
	return state.static_bodies.used / sizeof(Static_Body);
}

void physics_reset(void) {
	arena_clear(&state.bodies);
	arena_clear(&state.static_bodies);
}

void physics_body_destroy(usize body_id) {
	Body *body = physics_body_get(body_id);
	body->is_active = false;
}
