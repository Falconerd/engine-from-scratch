#pragma once

#include <stdbool.h>
#include <linmath.h>
#include "physics.h"
#include "types.h"
#include "render.h"

typedef struct entity {
	usize body_id;
	usize animation_id;
    vec2 sprite_offset;
	bool is_active;
    bool is_enraged;
} Entity;

void entity_init(void);
usize entity_create(vec2 position, vec2 size, vec2 sprite_offset, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, usize animation_id, On_Hit on_hit, On_Hit_Static on_hit_static);
Entity *entity_get(usize id);
usize entity_count(void);
void entity_reset(void);
Entity *entity_by_body_id(usize body_id);
usize entity_id_by_body_id(usize body_id);

void entity_damage(usize entity_id, u8 amount);
void entity_destroy(usize entity_id);

