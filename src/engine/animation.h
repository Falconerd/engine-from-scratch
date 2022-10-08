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
	Animation_Definition *definition;
	f32 current_frame_time;
	u8 current_frame_index;
	bool does_loop;
} Animation;

usize animation_definition_create(Sprite_Sheet *sprite_sheet, f32 *durations, u8 *rows, u8 *columns, u8 frame_count);

