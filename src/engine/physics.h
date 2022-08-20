#pragma once

#include <stdbool.h>
#include <linmath.h>
#include "types.h"

typedef enum body_flags {
	BODY_FLAGS_COLLISION_LEFT = 1 << 0,
	BODY_FLAGS_COLLISION_RIGHT = 1 << 1,
	BODY_FLAGS_COLLISION_UP = 1 << 2,
	BODY_FLAGS_COLLISION_DOWN = 1 << 3,
	BODY_FLAGS_COLLISION_STATIC = 1 << 4,
} Body_Flags;

typedef struct aabb {
	vec2 position;
	vec2 half_size;
} AABB;

typedef struct body {
	AABB aabb;
	vec2 velocity;
	vec2 acceleration;
	u8 collision_direction_flags;
	u8 collision_layer_flags;
	u8 collision_layer;
	u8 collision_mask;
} Body;

typedef struct static_body {
	AABB aabb;
	u8 collision_layer;
	u8 collision_mask;
} Static_Body;

typedef struct hit {
	bool is_hit;
	f32 time;
	vec2 position;
	vec2 normal;
	u8 hit_layer_mask;
} Hit;

void physics_init(void);
void physics_update(void);
usize physics_body_create(vec2 position, vec2 size, vec2 velocity, u8 collision_layer, u8 collision_mask);
Body *physics_body_get(usize index);
Static_Body *physics_static_body_get(usize index);
usize physics_static_body_create(vec2 position, vec2 size, u8 collision_layer, u8 collision_mask);
bool physics_point_intersect_aabb(vec2 point, AABB aabb);
bool physics_aabb_intersect_aabb(AABB a, AABB b);
AABB aabb_minkowski_difference(AABB a, AABB b);
void aabb_penetration_vector(vec2 r, AABB aabb);
void aabb_min_max(vec2 min, vec2 max, AABB aabb);
Hit ray_intersect_aabb(vec2 position, vec2 magnitude, AABB aabb);


