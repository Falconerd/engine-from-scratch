#include <glad/glad.h>
#include <SDL2/SDL.h>

#include "../util.h"
#include "../global.h"

#include "../render.h"
#include "render_internal.h"

SDL_Window *render_init_window(u32 width, u32 height) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		ERROR_EXIT("Could not init SDL: %s\n", SDL_GetError());
	}

	SDL_Window *window = SDL_CreateWindow(
		"MyGame",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_OPENGL
	);

	if (!window) {
		ERROR_EXIT("Failed to init window: %s\n", SDL_GetError());
	}

	SDL_GL_CreateContext(window);
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		ERROR_EXIT("Failed to load GL: %s\n", SDL_GetError());
	}

	puts("OpenGL Loaded");
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	return window;
}

void render_init_shaders(u32 *shader_default, u32 *shader_batch, f32 render_width, f32 render_height) {
	mat4x4 projection;
	*shader_default = render_shader_create("./shaders/default.vert", "./shaders/default.frag");
	*shader_batch = render_shader_create("./shaders/batch_quad.vert", "./shaders/batch_quad.frag");

	mat4x4_ortho(projection, 0, render_width, 0, render_height, -2, 2);

	glUseProgram(*shader_default);
	glUniformMatrix4fv(
		glGetUniformLocation(*shader_default, "projection"),
		1,
		GL_FALSE,
		&projection[0][0]
	);

	glUseProgram(*shader_batch);
	glUniformMatrix4fv(
		glGetUniformLocation(*shader_batch, "projection"),
		1,
		GL_FALSE,
		&projection[0][0]
	);

    for (u32 i = 0; i < 8; ++i) {
        char name[] = "texture_slot_N";
        sprintf(name, "texture_slot_%u", i);
        glUniform1i(glGetUniformLocation(*shader_batch, name), i);
    }
}

void render_init_color_texture(u32 *texture) {
	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);

	u8 solid_white[4] = {255, 255, 255, 255};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, solid_white);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void render_init_quad(u32 *vao, u32 *vbo, u32 *ebo) {
	//	 x,	y, z, u, v
	f32 vertices[] = {
		 0.5,  0.5, 0, 0, 0,
		 0.5, -0.5, 0, 0, 1,
		-0.5, -0.5, 0, 1, 1,
		-0.5,  0.5, 0, 1, 0
	};

	u32 indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	glGenVertexArrays(1, vao);
	glGenBuffers(1, vbo);
	glGenBuffers(1, ebo);

	glBindVertexArray(*vao);

	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// xyz
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), NULL);
	glEnableVertexAttribArray(0);

	// uv
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void*)(3 * sizeof(f32)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void render_init_line(u32 *vao, u32 *vbo) {
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(f32), NULL, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), NULL);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

void render_init_batch_quads(u32 *vao, u32 *vbo, u32 *ebo) {
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	u32 indices[MAX_BATCH_ELEMENTS];
	for (u32 i = 0, offset = 0; i < MAX_BATCH_ELEMENTS; i += 6, offset += 4) {
		indices[i + 0] = offset + 0;
		indices[i + 1] = offset + 1;
		indices[i + 2] = offset + 2;
		indices[i + 3] = offset + 2;
		indices[i + 4] = offset + 3;
		indices[i + 5] = offset + 0;
	}

	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, MAX_BATCH_VERTICES * sizeof(Batch_Vertex), NULL, GL_DYNAMIC_DRAW);

	// [x, y], [u, v], [r, g, b, a], [texture_slot]
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Batch_Vertex), (void*)offsetof(Batch_Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Batch_Vertex), (void*)offsetof(Batch_Vertex, uvs));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Batch_Vertex), (void*)offsetof(Batch_Vertex, color));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Batch_Vertex), (void*)offsetof(Batch_Vertex, texture_slot));

	glGenBuffers(1, ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_BATCH_ELEMENTS * sizeof(u32), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
