#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/glad.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "engine/global.h"
#include "engine/config.h"
#include "engine/input.h"
#include "engine/time.h"
#include "engine/physics.h"
#include "engine/util.h"

static bool should_quit = false;
static vec2 pos;

static void input_handle(void) {
	if (global.input.escape == KS_PRESSED || global.input.escape == KS_HELD)
		should_quit = true;

	i32 x, y;
	SDL_GetMouseState(&x, &y);
	pos[0] = (f32)x;
	pos[1] = global.render.height - y;
}

int main(int argc, char *argv[]) {
	time_init(60);
	config_init();
	render_init();
	physics_init();

	pos[0] = global.render.width * 0.5;
	pos[1] = global.render.height * 0.5;

	SDL_ShowCursor(false);

	AABB test_aabb = {
		.position = {global.render.width * 0.5, global.render.height * 0.5},
		.half_size = {50, 50}
	};

	AABB cursor_aabb = {.half_size = {75, 75}};

	AABB start_aabb = {.half_size = {75, 75}};

	AABB sum_aabb = {
		.position = {test_aabb.position[0], test_aabb.position[1]},
		.half_size = {
			test_aabb.half_size[0] + cursor_aabb.half_size[0],
			test_aabb.half_size[1] + cursor_aabb.half_size[1]}
	};

	while (!should_quit) {
		time_update();

		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				should_quit = true;
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					start_aabb.position[0] = pos[0];
					start_aabb.position[1] = pos[1];
				}
				break;
			default:
				break;
			}
		}

		input_update();
		input_handle();
		physics_update();

		render_begin();

		cursor_aabb.position[0] = pos[0];
		cursor_aabb.position[1] = pos[1];

		render_aabb((f32*)&test_aabb, WHITE);

		vec4 faded = {1, 1, 1, 0.3};

		if (physics_aabb_intersect_aabb(test_aabb, cursor_aabb)) {
			render_aabb((f32*)&cursor_aabb, RED);
		} else {
			render_aabb((f32*)&cursor_aabb, WHITE);
		}

		render_aabb((f32*)&start_aabb, faded);
		render_line_segment(start_aabb.position, pos, faded);
		
		f32 x = sum_aabb.position[0];
		f32 y = sum_aabb.position[1];
		f32 size = sum_aabb.half_size[0];

		render_line_segment((vec2){x - size, 0}, (vec2){x - size, global.render.height}, faded);
		render_line_segment((vec2){x + size, 0}, (vec2){x + size, global.render.height}, faded);
		render_line_segment((vec2){0, y - size}, (vec2){global.render.width, y - size}, faded);
		render_line_segment((vec2){0, y + size}, (vec2){global.render.width, y + size}, faded);

		vec2 min, max;
		aabb_min_max(min, max, sum_aabb);

		vec2 magnitude;
		vec2_sub(magnitude, pos, start_aabb.position);

		Hit hit = ray_intersect_aabb(start_aabb.position, magnitude, sum_aabb);

		if (hit.is_hit) {
			AABB hit_aabb = {
				.position = {hit.position[0], hit.position[1]},
				.half_size = {start_aabb.half_size[0], start_aabb.half_size[1]}
			};
			render_aabb((f32*)&hit_aabb, CYAN);
			render_quad(hit.position, (vec2){5, 5}, CYAN);
		}

		for (u8 i = 0; i < 2; ++i) {
			if (magnitude[i] != 0) {
				f32 t1 = (min[i] - pos[i]) / magnitude[i];
				f32 t2 = (max[i] - pos[i]) / magnitude[i];

				vec2 point;
				vec2_scale(point, magnitude, t1);
				vec2_add(point, point, pos);
				if (min[i] < start_aabb.position[i])
					render_quad(point, (vec2){5, 5}, ORANGE);
				else
					render_quad(point, (vec2){5, 5}, CYAN);

				vec2 start = {point[0], point[1]};

				vec2_scale(point, magnitude, t2);
				vec2_add(point, point, pos);
				if (max[i] < start_aabb.position[i])
					render_quad(point, (vec2){5, 5}, CYAN);
				else
					render_quad(point, (vec2){5, 5}, ORANGE);

				if (t1 < 0 && t2 < 0)
					render_line_segment(start, point, WHITE);
			}
		}

		render_end();
		time_update_late();
	}

	return 0;
}

