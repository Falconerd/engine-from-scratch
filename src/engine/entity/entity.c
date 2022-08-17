#include "../array_list.h"
#include "../entity.h"
#include "../physics.h"

static Array_List *entity_list;

void entity_init(void)
{
	entity_list = array_list_create(sizeof(Entity), 0);
}

usize entity_create(vec2 position, vec2 size, vec2 velocity)
{
	vec2_scale(size, size, 0.5);

	Entity entity = {
		.body_id = physics_body_create(position, size, velocity),
		.is_active = true,
	};

	return array_list_append(entity_list, &entity);
}

Entity *entity_get(usize id)
{
	return array_list_get(entity_list, id);
}

usize entity_count()
{
	return entity_list->len;
}

