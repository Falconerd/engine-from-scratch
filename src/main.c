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
#include "engine/entity.h"
#include "engine/render.h"
#include "engine/animation.h"

static const f32 SPEED_ENEMY_LARGE = 200;
static const f32 SPEED_ENEMY_SMALL = 400;
static const f32 HEALTH_ENEMY_LARGE = 7;
static const f32 HEALTH_ENEMY_SMALL = 3;

typedef enum collision_layer {
	COLLISION_LAYER_PLAYER = 1,
	COLLISION_LAYER_ENEMY = 1 << 1,
	COLLISION_LAYER_TERRAIN = 1 << 2,
} Collision_Layer;

static bool should_quit = false;
vec4 player_color = {0, 1, 1, 1};
bool player_is_grounded = false;

static void input_handle(Body *body_player) {
	if (global.input.escape) {
		should_quit = true;
	}

	f32 velx = 0;
	f32 vely = body_player->velocity[1];

	if (global.input.right) {
		velx += 600;
	}

	if (global.input.left) {
		velx -= 600;
	}

	if (global.input.up && player_is_grounded) {
		player_is_grounded = false;
		vely = 2000;
	}

	body_player->velocity[0] = velx;
	body_player->velocity[1] = vely;
}

void player_on_hit(Body *self, Body *other, Hit hit) {
	if (other->collision_layer == COLLISION_LAYER_ENEMY) {
		player_color[0] = 1;
		player_color[2] = 0;
	}
}

void player_on_hit_static(Body *self, Static_Body *other, Hit hit) {
	if (hit.normal[1] > 0) {
		player_is_grounded = true;
	}
}

void enemy_small_on_hit_static(Body *self, Static_Body *other, Hit hit) {
	if (hit.normal[0] > 0) {
		self->velocity[0] = SPEED_ENEMY_SMALL;
	}

	if (hit.normal[0] < 0) {
		self->velocity[0] = -SPEED_ENEMY_SMALL;
	}
}

void enemy_large_on_hit_static(Body *self, Static_Body *other, Hit hit) {
	if (hit.normal[0] > 0) {
		self->velocity[0] = SPEED_ENEMY_LARGE;
	}

	if (hit.normal[0] < 0) {
		self->velocity[0] = -SPEED_ENEMY_LARGE;
	}
}

void fire_on_hit(Body *self, Body *other, Hit hit) {
	if (other->collision_layer == COLLISION_LAYER_ENEMY) {
		for (usize i = 0; i < entity_count(); ++i) {
			Entity *entity = entity_get(i);

			if (entity->body_id == hit.other_id) {
				Body *body = physics_body_get(entity->body_id);
				body->is_active = false;
				entity->is_active = false;
				break;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	time_init(60);
	SDL_Window *window = render_init();
	config_init();
	physics_init();
	entity_init();
	animation_init();

	SDL_ShowCursor(false);

	u8 enemy_mask = COLLISION_LAYER_PLAYER | COLLISION_LAYER_TERRAIN;
	u8 player_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_TERRAIN;
	u8 fire_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_PLAYER;

	usize player_id = entity_create((vec2){100, 200}, (vec2){24, 24}, (vec2){0, 0}, COLLISION_LAYER_PLAYER, player_mask, false, player_on_hit, player_on_hit_static);

	i32 window_width, window_height;
	SDL_GetWindowSize(window, &window_width, &window_height);
	f32 width = window_width / render_get_scale();
	f32 height = window_width / render_get_scale();

	u32 static_body_a_id = physics_static_body_create((vec2){width * 0.5 - 12.5, height - 12.5}, (vec2){width - 25, 25}, COLLISION_LAYER_TERRAIN);
	u32 static_body_b_id = physics_static_body_create((vec2){width - 12.5, height * 0.5 + 12.5}, (vec2){25, height - 25}, COLLISION_LAYER_TERRAIN);
	u32 static_body_c_id = physics_static_body_create((vec2){width * 0.5 + 12.5, 12.5}, (vec2){width - 25, 25}, COLLISION_LAYER_TERRAIN);
	u32 static_body_d_id = physics_static_body_create((vec2){12.5, height * 0.5 - 12.5}, (vec2){25, height - 25}, COLLISION_LAYER_TERRAIN);
	u32 static_body_e_id = physics_static_body_create((vec2){width * 0.5, height * 0.5}, (vec2){62.5, 62.5}, COLLISION_LAYER_TERRAIN);

	usize entity_fire = entity_create((vec2){370, 50}, (vec2){25, 25}, (vec2){0}, 0, fire_mask, true, fire_on_hit, NULL);

	Sprite_Sheet sprite_sheet_player;
	render_sprite_sheet_init(&sprite_sheet_player, "assets/player.png", 24, 24);

	usize adef_player_walk_id = animation_definition_create(
			&sprite_sheet_player,
			(f32[]){0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1},
			(u8[]){0, 0, 0, 0, 0, 0, 0},
			(u8[]){1, 2, 3, 4, 5, 6, 7},
			7);
	usize adef_player_idle_id = animation_definition_create(&sprite_sheet_player, (f32[]){0}, (u8[]){0}, (u8[]){0}, 1);
	usize anim_player_walk_id = animation_create(adef_player_walk_id, true);
	usize anim_player_idle_id = animation_create(adef_player_idle_id, false);

	Entity *player = entity_get(player_id);
	player->animation_id = anim_player_idle_id;

	f32 spawn_timer = 0;

	while (!should_quit) {
		time_update();

		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				should_quit = true;
				break;
			default:
				break;
			}
		}

		Entity *player = entity_get(player_id);
		Body *body_player = physics_body_get(player->body_id);

		if (body_player->velocity[0] != 0) {
			player->animation_id = anim_player_walk_id;
		} else {
			player->animation_id = anim_player_idle_id;
		}

		Static_Body *static_body_a = physics_static_body_get(static_body_a_id);
		Static_Body *static_body_b = physics_static_body_get(static_body_b_id);
		Static_Body *static_body_c = physics_static_body_get(static_body_c_id);
		Static_Body *static_body_d = physics_static_body_get(static_body_d_id);
		Static_Body *static_body_e = physics_static_body_get(static_body_e_id);

		input_update();
		input_handle(body_player);
		physics_update();

		animation_update(global.time.delta);

		// Spawn enemies.
		{
			spawn_timer -= global.time.delta;
			if (spawn_timer <= 0) {
				spawn_timer = (f32)((rand() % 200) + 200) / 100.f;

				spawn_timer *= 0.2;

				bool is_flipped = rand() % 100 >= 50;
				bool is_small_entity = rand() % 100 > 18;

				f32 spawn_x = is_flipped ? 540 : 100;

				usize entity_id = entity_create((vec2){spawn_x, 200}, (vec2){20, 20}, (vec2){0, 0}, COLLISION_LAYER_ENEMY, enemy_mask, false, NULL, enemy_small_on_hit_static);
				Entity *entity = entity_get(entity_id);
				Body *body = physics_body_get(entity->body_id);
				body->velocity[0] = is_flipped ? -400 : 400;
			}
		}

		render_begin();

		for (usize i = 0; i < entity_count(); ++i) {
			Entity *entity = entity_get(i);
			Body *body = physics_body_get(entity->body_id);

			if (body->is_active) {
				render_aabb((f32*)body, TURQUOISE);
			} else {
				render_aabb((f32*)body, RED);
			}
		}

		render_aabb((f32*)static_body_a, WHITE);
		render_aabb((f32*)static_body_b, WHITE);
		render_aabb((f32*)static_body_c, WHITE);
		render_aabb((f32*)static_body_d, WHITE);
		render_aabb((f32*)static_body_e, WHITE);

		// Render animated entities...
		for (usize i = 0; i < entity_count(); ++i) {
			Entity *entity = entity_get(i);
			if (entity->animation_id == (usize)-1) {
				continue;
			}

			Body *body = physics_body_get(entity->body_id);
			Animation *anim = animation_get(entity->animation_id);
			Animation_Definition *adef = anim->definition;
			Animation_Frame *aframe = &adef->frames[anim->current_frame_index];

			if (body->velocity[0] < 0) {
				anim->is_flipped = true;
			} else if (body->velocity[0] > 0) {
				anim->is_flipped = false;
			}

			render_sprite_sheet_frame(adef->sprite_sheet, aframe->row, aframe->column, body->aabb.position, anim->is_flipped);
		}

		render_sprite_sheet_frame(&sprite_sheet_player, 1, 2, (vec2){100, 100}, false);
		render_sprite_sheet_frame(&sprite_sheet_player, 0, 4, (vec2){200, 200}, false);

		render_end(window, sprite_sheet_player.texture_id);

		player_color[0] = 0;
		player_color[2] = 1;

		time_update_late();
	}

	return 0;
}

