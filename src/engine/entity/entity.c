#include "../array_list.h"
#include "../util.h"
#include "../entity.h"

static Array_List *entity_list;

void entity_init(void) {
	entity_list = array_list_create(sizeof(Entity), 0);
}

usize entity_create(vec2 position, vec2 size, vec2 sprite_offset, vec2 velocity, u8 collision_layer, u8 collision_mask, bool is_kinematic, usize animation_id, On_Hit on_hit, On_Hit_Static on_hit_static) {
	usize id = entity_list->len;

	// Find inactive Entity.
	for (usize i = 0; i < entity_list->len; ++i) {
		Entity *entity = array_list_get(entity_list, i);
		if (!entity->is_active) {
			id = i;
			break;
		}
	}

	if (id == entity_list->len) {
		if (array_list_append(entity_list, &(Entity){0}) == (usize)-1) {
			ERROR_EXIT("Could not append entity to list\n");
		}
	}

	Entity *entity = entity_get(id);

	*entity = (Entity){
		.is_active = true,
		.animation_id = animation_id,
		.body_id = physics_body_create(position, size, velocity, collision_layer, collision_mask, is_kinematic, on_hit, on_hit_static),
        .sprite_offset = { sprite_offset[0], sprite_offset[1] },
	};

	return id;
}

Entity *entity_get(usize id) {
	return array_list_get(entity_list, id);
}
usize entity_count() {
	return entity_list->len;
}
