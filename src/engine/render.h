#pragma once

#include <SDL2/SDL.h>
#include <linmath.h>

#include "types.h"

typedef struct render_state {
	SDL_Window *window;
	f32 width;
	f32 height;
} Render_State;

SDL_Window *render_init(void);
void render_begin(void);
void render_end(SDL_Window *window);
void render_quad(vec2 pos, vec2 size, vec4 color);
void render_quad_line(vec2 pos, vec2 size, vec4 color);
void render_line_segment(vec2 start, vec2 end, vec4 color);
void render_aabb(f32 *aabb, vec4 color);

