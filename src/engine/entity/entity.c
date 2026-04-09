#include "../util.h"
#include "../entity.h"
#include "../mem/arena.h"

static Arena entities;

#define MAX_ENTITIES 512

void entity_init(void) {
	usize capacity = MAX_ENTITIES * sizeof(Entity);
	void *buffer = malloc(capacity);
	if (buffer == NULL) {
		ERROR_EXIT("Failed to allocate entity buffer.\n");
	}
	entities = arena_create(buffer, capacity);
}

usize entity_create(vec2 position, vec2 size, vec2 sprite_offset, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, usize animation_id, On_Hit on_hit, On_Hit_Static on_hit_static) {
	usize id = ARENA_LEN(entities, Entity);
	usize len = id;

	// Find inactive Entity.
	for (usize i = 0; i < len; ++i) {
		Entity *entity = (Entity *)entities.data + i;
		if (!entity->is_active) {
			id = i;
			break;
		}
	}

	Entity *entity = NULL;

	if (id == len) {
		entity = ARENA_PUSH(&entities, Entity);
		if (entity == NULL) {
			ERROR_EXIT("Could not append entity to list\n");
		}
	} else {
		entity = (Entity *)entities.data + id;
	}

	entity->is_active = true;
	entity->animation_id = animation_id;
	entity->body_id = physics_body_create(position, size, velocity, collision_layer, collision_mask, is_kinematic, on_hit, on_hit_static, id);
    entity->sprite_offset[0] = sprite_offset[0];
    entity->sprite_offset[1] = sprite_offset[1];

	return id;
}

Entity *entity_get(usize id) {
	Entity *entity = (Entity *)entities.data;
	return entity + id;
}

usize entity_count() {
	return ARENA_LEN(entities, Entity);
}

void entity_reset(void) {
	arena_clear(&entities);
}

bool entity_damage(usize entity_id, u8 amount) {
	Entity *entity = entity_get(entity_id);
	entity->health -= amount;
	if (entity->health <= 0) {
		entity_destroy(entity_id);
		return true;
	}
	return false;
}

void entity_destroy(usize entity_id) {
	Entity *entity = entity_get(entity_id);
	physics_body_destroy(entity->body_id);
	entity->is_active = false;
}
