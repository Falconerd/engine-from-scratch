#pragma once

#include "render.h"

#define MAX_FRAMES 16

typedef struct animation_frame {
	f32 duration;
	u8 row;
	u8 column;
} Animation_Frame;

typedef struct animation_definition {
	Sprite_Sheet *sprite_sheet;
	Animation_Frame frames[MAX_FRAMES];
	u8 frame_count;
} Animation_Definition;

typedef struct animation {
	usize animation_definition_id;
	f32 current_frame_time;
	u8 current_frame_index;
	bool does_loop;
	bool is_active;
	bool is_flipped;
} Animation;

void animation_init(void);
usize animation_definition_create(Sprite_Sheet *sprite_sheet, f32 duration, u8 row, u8 *columns, u8 frame_count);
usize animation_create(usize animation_definition_id, bool does_loop);
void animation_destroy(usize id);
Animation *animation_get(usize id);
void animation_update(f32 dt);
void animation_render(Animation *animation, vec2 position, vec4 color, u32 texture_slots[8]);
