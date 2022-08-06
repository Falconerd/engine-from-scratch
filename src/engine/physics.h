#pragma once

#include <stdbool.h>
#include <linmath.h>
#include "types.h"

typedef struct aabb {
	vec2 position;
	vec2 half_size;
} AABB;

typedef struct body {
	AABB aabb;
	vec2 velocity;
	vec2 acceleration;
} Body;

typedef struct static_body {
	AABB aabb;
} Static_Body;

typedef struct hit {
	bool is_hit;
	f32 time;
	vec2 position;
	vec2 normal;
} Hit;

void physics_init(void);
void physics_update(void);

usize physics_body_create(vec2 position, vec2 size);
Body *physics_body_get(usize index);
usize physics_static_body_create(vec2 position, vec2 size);
Static_Body *physics_static_body_get(usize index);

bool physics_point_intersect_aabb(vec2 point, AABB aabb);
bool physics_aabb_intersect_aabb(AABB a, AABB b);
AABB aabb_minkowski_difference(AABB a, AABB b);
void aabb_penetration_vector(vec2 r, AABB aabb);
void aabb_min_max(vec2 min, vec2 max, AABB aabb);
Hit ray_intersect_aabb(vec2 position, vec2 magnitude, AABB aabb);

