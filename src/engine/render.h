#pragma once

#include <SDL2/SDL.h>
#include <linmath.h>

#include "types.h"
#include "array_list.h"

typedef struct sprite_sheet {
	f32 width;
	f32 height;
	f32 cell_width;
	f32 cell_height;
	u32 texture_id;
} Sprite_Sheet;

typedef struct batch_vertex {
	vec2 position;
	vec2 uvs;
	vec4 color;
} Batch_Vertex;

enum {
	MAX_BATCH_QUADS = 10000,
	MAX_BATCH_VERTICES = MAX_BATCH_QUADS * 4,
	MAX_BATCH_ELEMENTS = MAX_BATCH_QUADS * 6,
};

SDL_Window *render_init(void);
void render_begin(void);
void render_end(SDL_Window *window);
void render_quad(vec2 pos, vec2 size, vec4 color);
void render_quad_line(vec2 pos, vec2 size, vec4 color);
void render_line_segment(vec2 start, vec2 end, vec4 color);
void render_aabb(f32 *aabb, vec4 color);
void render_sprite_sheet_init(Sprite_Sheet *sprite_sheet, const char *path, f32 cell_width, f32 cell_height);
void render_sprite_sheet_frame(Sprite_Sheet *sprite_sheet, f32 row, f32 column, vec2 position);
void render_set_batch_texture(u32 texture_id);
f32 render_get_scale();
