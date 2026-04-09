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
	state.event_list = array_list_create(sizeof(Physics_Event), 0);

	state.gravity = -79;
	state.terminal_velocity = -7000;

	tick_rate = 1.f / iterations;
}

static void log_hit(Physics_Event_Kind kind, Body_Handle a, Hit hit) {
	Physics_Event event = {.kind = kind};
	if (kind == PHYSICS_EVENT_HIT_BODY) {
		event.hit_body.a = a;
		event.hit_body.hit = hit;
		event.hit_body.b = (Body_Handle)hit.other;
	} else if (kind == PHYSICS_EVENT_HIT_STATIC) {
		event.hit_static.a = a;
		event.hit_static.hit = hit;
		event.hit_static.b = (Static_Body_Handle)hit.other;
	} else {
		ERROR_EXIT("Invalid physics event kind in log_hit.\n");
	}

	array_list_append(state.event_list, &event);
}

static bool update_sweep_result(Hit *result, Body* body, Body *other, vec2 velocity) {
	if ((body->collision_mask & other->collision_layer) == 0) {
		return false;
	}

	AABB sum_aabb = other->aabb;
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

		return true;
	}

	return false;
}

static bool update_sweep_result_static(Hit *result, Body *body, Static_Body *static_body, vec2 velocity) {
	if ((body->collision_mask & static_body->collision_layer) == 0) {
		return false;
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

		return true;
	}

	return false;
}

static Hit sweep_static_bodies(Body *body, vec2 velocity) {
	Hit result = {.time = 0xBEEF};

	for (u32 i = 0; i < state.static_body_list->len; ++i) {
		Static_Body *static_body = array_list_get(state.static_body_list, (usize)i);
		if (update_sweep_result_static(&result, body, static_body, velocity)) {
			result.other = (Handle){i, static_body->generation};
		}
	}

	return result;
}

static Hit sweep_bodies(Body *body, u32 index, vec2 velocity) {
	Hit result = {.time = 0xBEEF};

	for (u32 i = 0; i < state.body_list->len; ++i) {
		if (i == index) {
			continue;
		}
		
		Body *other = array_list_get(state.body_list, (usize)i);
		if (!other->is_active) {
			continue;
		}

		if (update_sweep_result(&result, body, other, velocity)) {
			result.other = (Handle){i, other->generation};
		}
	}

	return result;
}

static void sweep_response(Body_Handle handle, vec2 velocity) {
	Body *body = physics_body_get(handle);
	if (!body) return;

	Hit hit_static = sweep_static_bodies(body, velocity);
	Hit hit_moving = sweep_bodies(body, handle.index, velocity);

	if (hit_moving.is_hit) {
		// NOTE: We don't do anything in physics engine when moving bodies collide
		log_hit(PHYSICS_EVENT_HIT_BODY, handle, hit_moving);
	}

	if (hit_static.is_hit) {
		body->aabb.position[0] = hit_static.position[0];
		body->aabb.position[1] = hit_static.position[1];

		if (hit_static.normal[0] != 0) {
			body->aabb.position[1] += velocity[1];
			body->velocity[0] = 0;
		} else if (hit_static.normal[1] != 0) {
			body->aabb.position[0] += velocity[0];
			body->velocity[1] = 0;
		}

		log_hit(PHYSICS_EVENT_HIT_STATIC, handle, hit_static);
	} else {
		vec2_add(body->aabb.position, body->aabb.position, velocity);
	}
}

static void stationary_response(Body_Handle handle) {
	Body *body = physics_body_get(handle);

	for (u32 i = 0; i < state.static_body_list->len; ++i) {
		Static_Body *static_body = array_list_get(state.static_body_list, i);

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
	for (usize i = 0; i < state.body_list->len; ++i) {
		Body *other = array_list_get(state.body_list, i);

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
			Hit hit = {
				.is_hit = true,
				.normal = {0}, // TODO?
				.position = {0}, // TODO?
				.time = 0, // Not relevant
				.other = {i, other->generation},
			};
			log_hit(PHYSICS_EVENT_HIT_BODY, handle, hit);
		}
	}
}

// Return struct as caller doesn't need to write
Array_List physics_update(void) {
	state.event_list->len = 0;

	for (u32 i = 0; i < state.body_list->len; ++i) {
		Body_Handle handle = {.index = i, .generation = ((Body *)array_list_get(state.body_list, i))->generation};

		Body *body = physics_body_get(handle);
		if (!body || !body->is_active) {
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
			sweep_response(handle, scaled_velocity);
			stationary_response(handle);
		}
	}

	return *state.event_list;
}

Body_Handle physics_body_create(vec2 position, vec2 size, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, On_Hit on_hit, On_Hit_Static on_hit_static, Entity_Handle entity_handle) {
	u32 index = state.body_list->len;
	for (u32 i = 0; i < state.body_list->len; i += 1) {
		Body *body = array_list_get(state.body_list, i);
		if (!body->is_active) {
			index = i;
			break;
		}
	}

	if (index == state.body_list->len) {
		if (array_list_append(state.body_list, &(Body){0}) == (usize)-1) {
			ERROR_EXIT("Could not append body to list\n");
		}
	}

	Body *body = array_list_get(state.body_list, index);
	u32 generation = body->generation;

	body->aabb = (AABB){
		.position = { position[0], position[1] },
		.half_size = { size[0] * 0.5, size[1] * 0.5 },
	};
	body->velocity[0] = velocity[0];
	body->velocity[1] = velocity[1];
	body->collision_layer = collision_layer;
	body->collision_mask = collision_mask;
	body->on_hit = on_hit;
	body->on_hit_static = on_hit_static;
	body->is_kinematic = is_kinematic;
	body->is_active = true;
	body->entity = entity_handle;

	return (Body_Handle){
		.index = index,
		.generation = generation,
	};
}

Body *physics_body_get(Body_Handle handle) {
	if (handle.index >= state.body_list->len) return NULL;
	Body *body = array_list_get(state.body_list, handle.index);
	if (body->generation != handle.generation) return NULL;
	if (!body->is_active) return NULL;
	return body;
}

usize physics_static_body_create(vec2 position, vec2 size, u8 collision_layer) {
	Static_Body static_body = {
		.aabb = {
			.position = { position[0], position[1] },
			.half_size = { size[0] * 0.5, size[1] * 0.5 },
		},
		.collision_layer = collision_layer,
	};

	if (array_list_append(state.static_body_list, &static_body) == (usize)-1)
		ERROR_EXIT("Could not append static body to list\n");

	return state.static_body_list->len - 1;
}

Trigger_Handle physics_trigger_create(vec2 position, vec2 size, u8 collision_layer, u8 collision_mask, On_Hit on_hit) {
    return (Handle)physics_body_create(position, size, (vec2){0, 0}, collision_layer, collision_mask, true, on_hit, NULL, (Entity_Handle){(u32)-1, 0});
}

Static_Body *physics_static_body_get(Static_Body_Handle handle) {
	if (handle.index >= state.static_body_list->len) return NULL;
	Static_Body *static_body = array_list_get(state.static_body_list, handle.index);
	if (static_body->generation != handle.generation) return NULL;
	return static_body;

}

usize physics_static_body_count() {
    return state.static_body_list->len;
}

void physics_reset(void) {
    state.static_body_list->len = 0;
    state.body_list->len = 0;
}

void physics_body_destroy(Body_Handle handle) {
	Body *body = physics_body_get(handle);
	if (!body) return;
	body->is_active = false;
	// Invalidate outsanding handles
	body->generation += 1;
}

Static_Body *physics_static_body_get_by_index(usize i) {
	return (Static_Body *)array_list_get(state.static_body_list, i);
}
