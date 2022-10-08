#include <assert.h>

#include "../array_list.h"
#include "../animation.h"

static Array_List *animation_definition_list;

void animation_init() {
	animation_definition_list = array_list_create(sizeof(Animation_Definition), 0);
}

usize animation_definition_create(Sprite_Sheet *sprite_sheet, f32 *durations, u8 *rows, u8 *columns, u8 frame_count) {
	assert(frame_count <= MAX_FRAMES);

	Animation_Definition def = {0};

	def.sprite_sheet = sprite_sheet;
	def.frame_count = frame_count;

	for (u8 i = 0; i < frame_count; ++i) {
		def.frames[i] = (Animation_Frame){
			.column = columns[i],
			.row = rows[i],
			.duration = durations[i],
		};
	}

	return array_list_append(animation_definition_list, &def);
}

