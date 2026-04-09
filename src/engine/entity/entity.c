#include "../array_list.h"
#include "../util.h"
#include "../entity.h"

static Array_List *entity_list;

void entity_init(void) {
	entity_list = array_list_create(sizeof(Entity), 0);
}

Entity_Handle entity_create(vec2 position, vec2 size, vec2 sprite_offset, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, usize animation_id, On_Hit on_hit, On_Hit_Static on_hit_static) {
	u32 index = entity_list->len;
	for (u32 i = 0; i < entity_list->len; i += 1) {
		Entity *entity = array_list_get(entity_list, i);
		if (!entity->is_active) {
			index = i;
			break;
		}
	}

	if (index == entity_list->len) {
		if (array_list_append(entity_list, &(Entity){0}) == (usize)-1) {
			ERROR_EXIT("Could not append entity to list\n");
		}
	}

	Entity *entity = array_list_get(entity_list, index);
	u32 generation = entity->generation;
	Entity_Handle handle = {index, generation};

	entity->is_active = true;
	entity->animation_id = animation_id;
	entity->body = physics_body_create(position, size, velocity, collision_layer, collision_mask, is_kinematic, on_hit, on_hit_static, handle);
	entity->sprite_offset[0] = sprite_offset[0];
	entity->sprite_offset[1] = sprite_offset[1];

	return handle;
}

Entity *entity_get(Body_Handle handle) {
	if (handle.index >= entity_list->len) return NULL;
	Entity *entity = array_list_get(entity_list, handle.index);
	if (entity->generation != handle.generation) return NULL;
	return entity;
}

usize entity_count() {
	return entity_list->len;
}

void entity_reset(void) {
    entity_list->len = 0;
}

bool entity_damage(Entity_Handle handle, u8 amount) {
	Entity *entity = entity_get(handle);
	entity->health -= amount;
	if (entity->health <= 0) {
		entity_destroy(handle);
		return true;
	}
	return false;
}

void entity_destroy(Entity_Handle handle) {
	Entity *entity = entity_get(handle);
	if (!entity) return;
	physics_body_destroy(entity->body);
	entity->is_active = false;
}

Entity *entity_get_by_index(usize i) {
	return (Entity *)array_list_get(entity_list, i);
}

void entity_destroy_by_index(usize i) {
	Entity *entity = entity_get_by_index(i);
	if (!entity) return;
	entity_destroy((Entity_Handle){i, entity->generation});
}
