#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "../global.h"
#include "../render.h"
#include "../array_list.h"
#include "../util.h"
#include "render_internal.h"

static f32 window_width = 1920;
static f32 window_height = 1080;
static f32 render_width = 640;
static f32 render_height = 360;
static f32 scale = 3;

static u32 vao_quad;
static u32 vbo_quad;
static u32 ebo_quad;
static u32 vao_line;
static u32 vbo_line;
static u32 shader_default;
static u32 texture_color;
static u32 vao_batch;
static u32 vbo_batch;
static u32 ebo_batch;
static u32 shader_batch;
static Array_List *list_batch;

SDL_Window *render_init(void) {
	SDL_Window *window = render_init_window(window_width, window_height);

	render_init_quad(&vao_quad, &vbo_quad, &ebo_quad);
	render_init_batch_quads(&vao_batch, &vbo_batch, &ebo_batch);
	render_init_line(&vao_line, &vbo_line);
	render_init_shaders(&shader_default, &shader_batch, render_width, render_height);
	render_init_color_texture(&texture_color);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	list_batch = array_list_create(sizeof(Batch_Vertex), 8);

	stbi_set_flip_vertically_on_load(1);

	return window;
}

void render_begin(void) {
	glClearColor(0.08, 0.1, 0.1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	list_batch->len = 0;
}

static void render_batch(Batch_Vertex *vertices, usize count, u32 texture_id) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo_batch);
	glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(Batch_Vertex), vertices);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glUseProgram(shader_batch);
	glBindVertexArray(vao_batch);

	glDrawElements(GL_TRIANGLES, (count >> 2) * 6, GL_UNSIGNED_INT, NULL);
}

static void append_quad(vec2 position, vec2 size, vec4 texture_coordinates, vec4 color) {
	vec4 uvs = {0, 0, 1, 1};

	if (texture_coordinates != NULL) {
		memcpy(uvs, texture_coordinates, sizeof(vec4));
	}

	array_list_append(list_batch, &(Batch_Vertex){
		.position = {position[0], position[1]},
		.uvs = {uvs[0], uvs[1]},
		.color = {color[0], color[1], color[2], color[3]},
	});

	array_list_append(list_batch, &(Batch_Vertex){
		.position = {position[0] + size[0], position[1]},
		.uvs = {uvs[2], uvs[1]},
		.color = {color[0], color[1], color[2], color[3]},
	});

	array_list_append(list_batch, &(Batch_Vertex){
		.position = {position[0] + size[0], position[1] + size[1]},
		.uvs = {uvs[2], uvs[3]},
		.color = {color[0], color[1], color[2], color[3]},
	});

	array_list_append(list_batch, &(Batch_Vertex){
		.position = {position[0], position[1] + size[1]},
		.uvs = {uvs[0], uvs[3]},
		.color = {color[0], color[1], color[2], color[3]},
	});
}

void render_end(SDL_Window *window, u32 batch_texture_id) {
	render_batch(list_batch->items, list_batch->len, batch_texture_id);

	SDL_GL_SwapWindow(window);
}

void render_quad(vec2 pos, vec2 size, vec4 color) {
	glUseProgram(shader_default);

	mat4x4 model;
	mat4x4_identity(model);

	mat4x4_translate(model, pos[0], pos[1], 0);
	mat4x4_scale_aniso(model, model, size[0], size[1], 1);

	glUniformMatrix4fv(glGetUniformLocation(shader_default, "model"), 1, GL_FALSE, &model[0][0]);
	glUniform4fv(glad_glGetUniformLocation(shader_default, "color"), 1, color);

	glBindVertexArray(vao_quad);

	glBindTexture(GL_TEXTURE_2D, texture_color);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

	glBindVertexArray(0);
}

void render_line_segment(vec2 start, vec2 end, vec4 color) {
	glUseProgram(shader_default);
	glLineWidth(3);

	f32 x = end[0] - start[0];
	f32 y = end[1] - start[1];
	f32 line[6] = {0, 0, 0, x, y, 0};

	mat4x4 model;
	mat4x4_translate(model, start[0], start[1], 0);

	glUniformMatrix4fv(glGetUniformLocation(shader_default, "model"), 1, GL_FALSE, &model[0][0]);
	glUniform4fv(glGetUniformLocation(shader_default, "color"), 1, color);

	glBindTexture(GL_TEXTURE_2D, texture_color);
	glBindVertexArray(vao_line);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_line);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line);
	glDrawArrays(GL_LINES, 0, 2);

	glBindVertexArray(0);
}

void render_quad_line(vec2 pos, vec2 size, vec4 color) {
	vec2 points[4] = {
		{pos[0] - size[0] * 0.5, pos[1] - size[1] * 0.5},
		{pos[0] + size[0] * 0.5, pos[1] - size[1] * 0.5},
		{pos[0] + size[0] * 0.5, pos[1] + size[1] * 0.5},
		{pos[0] - size[0] * 0.5, pos[1] + size[1] * 0.5},
	};

	render_line_segment(points[0], points[1], color);
	render_line_segment(points[1], points[2], color);
	render_line_segment(points[2], points[3], color);
	render_line_segment(points[3], points[0], color);
}

void render_aabb(f32 *aabb, vec4 color) {
	vec2 size;
	vec2_scale(size, &aabb[2], 2);
	render_quad_line(&aabb[0], size, color);
}

f32 render_get_scale() {
	return scale;
}

void render_sprite_sheet_init(Sprite_Sheet *sprite_sheet, const char *path, f32 cell_width, f32 cell_height) {
	glGenTextures(1, &sprite_sheet->texture_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sprite_sheet->texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, channel_count;
	u8 *image_data = stbi_load(path, &width, &height, &channel_count, 0);
	if (!image_data) {
		ERROR_EXIT("Failed to load image: %s\n", path);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	sprite_sheet->width = (f32)width;
	sprite_sheet->height = (f32)height;
	sprite_sheet->cell_width = cell_width;
	sprite_sheet->cell_height = cell_height;
}

static void calculate_sprite_texture_coordinates(vec4 result, f32 row, f32 column, f32 texture_width, f32 texture_height, f32 cell_width, f32 cell_height) {
	f32 w = 1.0 / (texture_width / cell_width);
	f32 h = 1.0 / (texture_height / cell_height);
	f32 x = column * w;
	f32 y = row * h;
	result[0] = x;
	result[1] = y;
	result[2] = x + w;
	result[3] = y + h;
}

void render_sprite_sheet_frame(Sprite_Sheet *sprite_sheet, f32 row, f32 column, vec2 position) {
	vec4 uvs;
	calculate_sprite_texture_coordinates(uvs, row, column, sprite_sheet->width, sprite_sheet->height, sprite_sheet->cell_width, sprite_sheet->cell_height);

	vec2 size = {sprite_sheet->cell_width, sprite_sheet->cell_height};
	vec2 bottom_left = {position[0] - size[0] * 0.5, position[1] - size[1] * 0.5};
	append_quad(bottom_left, size, uvs, WHITE);
}
