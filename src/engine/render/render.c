#include <glad/glad.h>

#include "../global.h"
#include "render.h"
#include "render_internal.h"

static Render_State_Internal state = {0};

void render_init() {
    global.render.width = 800;
	global.render.height = 600;

    global.render.window = render_init_window(global.render.width, global.render.height);

    render_init_quad(&state.vao_quad, &state.vbo_quad, &state.ebo_quad);
}

void render_begin() {
    glClearColor(0.08, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void render_end() {
    SDL_GL_SwapWindow(global.render.window);
}

void render_quad(vec2 pos, vec2 size, vec4 color) {
    glBindVertexArray(state.vao_quad);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

    glBindVertexArray(0);
}

