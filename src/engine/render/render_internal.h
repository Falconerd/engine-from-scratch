#pragma once

#include <SDL2/SDL.h>

#include "../types.h"
#include "../render.h"

typedef struct render_state_internal {
	u32 vao_quad;
	u32 vbo_quad;
	u32 ebo_quad;
	u32 vao_line;
	u32 vbo_line;
	u32 shader_default;
	u32 texture_color;
	mat4x4 projection;
} Render_State_Internal;

SDL_Window *render_init_window(u32 width, u32 height);
void render_init_quad(u32 *vao, u32 *vbo, u32 *ebo);
void render_init_color_texture(u32 *texture);
void render_init_shaders(Render_State_Internal *state);
void render_init_line(u32 *vao, u32 *vbo);
u32 render_shader_create(const char *path_vert, const char *path_frag);

