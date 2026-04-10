#pragma once

#include <stdbool.h>
#include <linmath.h>
#include "types.h"
#include "array_list.h"

typedef struct hit Hit;
typedef struct body Body;
typedef struct static_body Static_Body;

typedef void (*On_Hit)(Body *self, Body *other, Hit hit);
typedef void (*On_Hit_Static)(Body *self, Static_Body *other, Hit hit);

typedef struct aabb {
	vec2 position;
	vec2 half_size;
} AABB;

struct body {
	AABB aabb;
	vec2 velocity;
	vec2 acceleration;
	On_Hit on_hit;
	On_Hit_Static on_hit_static;
	Entity_Handle entity;
	u32 generation;
	u8 collision_layer;
	u8 collision_mask;
	bool is_kinematic;
	bool is_active;
};

struct static_body {
	AABB aabb;
	u32 generation;
	u8 collision_layer;
};

struct hit {
	Handle other;
	f32 time;
	vec2 position;
	vec2 normal;
	bool is_hit;
};

typedef enum Physics_Event_Kind {
	PHYSICS_EVENT_HIT_BODY,
	PHYSICS_EVENT_HIT_STATIC,
} Physics_Event_Kind;

typedef struct Physics_Event {
	Physics_Event_Kind kind;
	union {
		struct {Body_Handle a; Body_Handle b; Hit hit;} hit_body;
		struct {Body_Handle a; Static_Body_Handle b; Hit hit;} hit_static;
	};
} Physics_Event;

void physics_init(void);
Array_List physics_update(void);
Body_Handle physics_body_create(vec2 position, vec2 size, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, On_Hit on_hit, On_Hit_Static on_hit_static, Entity_Handle entity_handle);
void physics_body_destroy(Body_Handle body_handle);
Handle physics_trigger_create(vec2 position, vec2 size, u8 collision_layer, u8 collision_mask, On_Hit on_hit);
Body *physics_body_get(Body_Handle handle);
Static_Body *physics_static_body_get(Static_Body_Handle handle);
usize physics_static_body_count();
usize physics_static_body_create(vec2 position, vec2 size, u8 collision_layer);
bool physics_point_intersect_aabb(vec2 point, AABB aabb);
bool physics_aabb_intersect_aabb(AABB a, AABB b);
AABB aabb_minkowski_difference(AABB a, AABB b);
void aabb_penetration_vector(vec2 r, AABB aabb);
void aabb_min_max(vec2 min, vec2 max, AABB aabb);
Hit ray_intersect_aabb(vec2 position, vec2 magnitude, AABB aabb);
void physics_reset(void);

Body *physics_body_get_by_index(usize i);
Static_Body *physics_static_body_get_by_index(usize i);
void physics_events_clear(void);
