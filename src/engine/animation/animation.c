#include <assert.h>

#include "../util.h"
#include "../array_list.h"
#include "../animation.h"

Array_List *animation_definition_storage;
Array_List *animation_storage;

void animation_init(void) {
    // TODO: BUG WITH ARRAY_LIST IMPLEMENTATION CAUSES CREATED WITH 0 LENGTH TO NOT WORK
	animation_definition_storage = array_list_create(sizeof(Animation_Definition), 0);
	animation_storage = array_list_create(sizeof(Animation), 0);
}

usize animation_definition_create(Sprite_Sheet *sprite_sheet, f32 duration, u8 row, u8 *columns, u8 frame_count) {
	assert(frame_count <= MAX_FRAMES);

	Animation_Definition def = {0};

	def.sprite_sheet = sprite_sheet;
	def.frame_count = frame_count;

	for (u8 i = 0; i < frame_count; ++i) {
		def.frames[i] = (Animation_Frame){
			.column = columns[i],
			.row = row,
			.duration = duration,
		};
	}

	return array_list_append(animation_definition_storage, &def);
}

usize animation_create(usize animation_definition_id, bool does_loop) {
	usize id = animation_storage->len;
	Animation_Definition *adef = array_list_get(animation_definition_storage, animation_definition_id);
	if (adef == NULL) {
		ERROR_EXIT("Animation Definition with id %zu not found.", animation_definition_id);
	}

	// Try to find free slot first.
	for (usize i = 0; i < animation_storage->len; ++i) {
		Animation *animation = array_list_get(animation_storage, i);
		if (!animation->is_active) {
			id = i;
			break;
		}
	}

	if (id == animation_storage->len) {
		array_list_append(animation_storage, &(Animation){0});
	}

	Animation *animation = array_list_get(animation_storage, id);

	// Other fields default to 0 when using field dot syntax.
	*animation = (Animation){
		.animation_definition_id = animation_definition_id,
		.does_loop = does_loop,
		.is_active = true,
	};

	return id;
}

void animation_destroy(usize id) {
	Animation *animation = array_list_get(animation_storage, id);
	animation->is_active = false;
}

Animation *animation_get(usize id) {
	return array_list_get(animation_storage, id);
}

void animation_update(f32 dt) {
	for (usize i = 0; i < animation_storage->len; ++i) {
		Animation *animation = array_list_get(animation_storage, i);
		Animation_Definition *adef = array_list_get(animation_definition_storage, animation->animation_definition_id);
		animation->current_frame_time -= dt;

		if (animation->current_frame_time <= 0) {
			animation->current_frame_index += 1;

			// Loop or stay on last frame.
			if (animation->current_frame_index == adef->frame_count) {
				if (animation->does_loop) {
					animation->current_frame_index = 0;
				} else {
					animation->current_frame_index -= 1;
				}
			}

			animation->current_frame_time = adef->frames[animation->current_frame_index].duration;
		}
	}
}

void animation_render(Animation *animation, vec2 position, vec4 color, u32 texture_slots[8]) {
    Animation_Definition *adef = array_list_get(animation_definition_storage, animation->animation_definition_id);
    Animation_Frame *aframe = &adef->frames[animation->current_frame_index];
    render_sprite_sheet_frame(adef->sprite_sheet, aframe->row, aframe->column, position, animation->is_flipped, WHITE, texture_slots);
}

