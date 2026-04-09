#pragma once

#include <stdbool.h>
#include <linmath.h>
#include "physics.h"
#include "types.h"

typedef struct entity {
	Body_Handle body;
	usize animation_id;
	vec2 sprite_offset;
	u32 generation;
	f32 lifetime;
	bool is_active;
	bool is_enraged;
	bool is_flipped;
	u8 health;
} Entity;

void entity_init(void);
Entity_Handle entity_create(vec2 position, vec2 size, vec2 sprite_offset, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, usize animation_id, On_Hit on_hit, On_Hit_Static on_hit_static);
Entity *entity_get(Entity_Handle handle);
usize entity_count(void);
void entity_reset(void);
Entity *entity_by_body(Body_Handle handle);
usize entity_id_by_body(Body_Handle handle);

bool entity_damage(Entity_Handle handle, u8 amount);
void entity_destroy(Entity_Handle handle);

Entity *entity_get_by_index(usize i);
void entity_destroy_by_index(usize i);
