#pragma once

#include <stdbool.h>
#include <linmath.h>
#include "types.h"

typedef struct entity {
	usize body_id;
	bool is_active;
} Entity;

void entity_init(void);
usize entity_create(vec2 position, vec2 size, vec2 velocity);
Entity *entity_get(usize id);
usize entity_count();
