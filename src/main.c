#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/glad.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "engine/global.h"
#include "engine/config.h"
#include "engine/input.h"
#include "engine/time.h"
#include "engine/physics.h"
#include "engine/util.h"
#include "engine/entity.h"
#include "engine/render.h"
#include "engine/animation.h"
#include "engine/audio.h"

#define DEBUG 1

static Mix_Music *MUSIC_STAGE_1;
static Mix_Chunk *SOUND_JUMP;

static Sprite_Sheet sprite_sheet_map;
static Sprite_Sheet sprite_sheet_player;

static const f32 SPEED_ENEMY_LARGE = 200;
static const f32 SPEED_ENEMY_SMALL = 400;
static const f32 HEALTH_ENEMY_LARGE = 7;
static const f32 HEALTH_ENEMY_SMALL = 3;

typedef enum collision_layer {
	COLLISION_LAYER_PLAYER = 1,
	COLLISION_LAYER_ENEMY = 1 << 1,
	COLLISION_LAYER_TERRAIN = 1 << 2,
	COLLISION_LAYER_ENEMY_PASSTHROUGH = 1 << 3,
} Collision_Layer;

static bool should_quit = false;
vec4 player_color = {0, 1, 1, 1};
bool player_is_grounded = false;

void reset(void);

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
		vely = 1500;
		audio_sound_play(SOUND_JUMP);
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
	} else if (other->collision_layer == COLLISION_LAYER_PLAYER) {
		reset();
	}
}

void reset(void) {
	audio_music_play(MUSIC_STAGE_1);
}

int main(int argc, char *argv[]) {
	time_init(60);
	SDL_Window *window = render_init();
	config_init();
	physics_init();
	entity_init();
	animation_init();
	audio_init();

	audio_sound_load(&SOUND_JUMP, "assets/jump.wav");
	audio_music_load(&MUSIC_STAGE_1, "assets/breezys_mega_quest_2_stage_1.mp3");

	SDL_ShowCursor(false);

	u8 enemy_mask = COLLISION_LAYER_PLAYER | COLLISION_LAYER_TERRAIN;
	u8 player_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_TERRAIN | COLLISION_LAYER_ENEMY_PASSTHROUGH;
	u8 fire_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_PLAYER;

	usize player_id = entity_create((vec2){100, 200}, (vec2){24, 24}, (vec2){0, 0}, COLLISION_LAYER_PLAYER, player_mask, false, player_on_hit, player_on_hit_static);

	i32 window_width, window_height;
	SDL_GetWindowSize(window, &window_width, &window_height);
	f32 width = window_width / render_get_scale();
	f32 height = window_height / render_get_scale();

	usize static_body_ceiling_id = physics_static_body_create((vec2){width * 0.5, height - 16}, (vec2){width, 32}, COLLISION_LAYER_TERRAIN);
	usize static_body_floor_left_id = physics_static_body_create((vec2){width * 0.25 - 16, 16}, (vec2){width * 0.5 - 32, 48}, COLLISION_LAYER_TERRAIN);
	usize static_body_floor_right_id = physics_static_body_create((vec2){width * 0.75 + 16, 16}, (vec2){width * 0.5 - 32, 48}, COLLISION_LAYER_TERRAIN);
	usize static_body_wall_left_id = physics_static_body_create((vec2){16, height * 0.5 - 3 * 32}, (vec2){32, height}, COLLISION_LAYER_TERRAIN);
	usize static_body_wall_right_id = physics_static_body_create((vec2){width - 16, height * 0.5 - 3 * 32}, (vec2){32, height}, COLLISION_LAYER_TERRAIN);
	usize static_body_platform_top_left_id = physics_static_body_create((vec2){32 + 64, height - 32 * 3 - 16}, (vec2){128, 32}, COLLISION_LAYER_TERRAIN);
	usize static_body_platform_top_right_id = physics_static_body_create((vec2){width - 32 - 64, height - 32 * 3 - 16}, (vec2){128, 32}, COLLISION_LAYER_TERRAIN);
	usize static_body_platform_top_id = physics_static_body_create((vec2){width * 0.5, height - 32 * 3 - 16}, (vec2){192, 32}, COLLISION_LAYER_TERRAIN);
	usize static_body_platform_bottom_id = physics_static_body_create((vec2){width * 0.5, 32 * 3 + 24}, (vec2){448, 32}, COLLISION_LAYER_TERRAIN);
	usize static_body_spawner_left_id = physics_static_body_create((vec2){16, height - 64}, (vec2){32, 64}, COLLISION_LAYER_ENEMY_PASSTHROUGH);
	usize static_body_spawner_right_id = physics_static_body_create((vec2){width - 16, height - 64}, (vec2){32, 64}, COLLISION_LAYER_ENEMY_PASSTHROUGH);

	usize entity_fire_id = entity_create((vec2){width * 0.5, -4}, (vec2){64, 8}, (vec2){0}, 0, fire_mask, true, fire_on_hit, NULL);

	render_sprite_sheet_init(&sprite_sheet_player, "assets/player.png", 24, 24);
	render_sprite_sheet_init(&sprite_sheet_map, "assets/map.png", 640, 360);

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

				f32 spawn_x = is_flipped ? width : 0;

				usize entity_id = entity_create((vec2){spawn_x, height - 64}, (vec2){20, 20}, (vec2){0, 0}, COLLISION_LAYER_ENEMY, enemy_mask, false, NULL, enemy_small_on_hit_static);
				Entity *entity = entity_get(entity_id);
				Body *body = physics_body_get(entity->body_id);
				body->velocity[0] = is_flipped ? -SPEED_ENEMY_SMALL : SPEED_ENEMY_SMALL;
			}
		}

		render_begin();

		render_sprite_sheet_frame(&sprite_sheet_map, 0, 0, (vec2){320, 180}, false, (vec4){1, 1, 1, 0.5}, 2);

#if DEBUG
		for (usize i = 0; i < physics_body_count(); ++i) {
			Body *body = physics_body_get(i);

			if (body->is_active) {
				render_aabb((f32*)body, TURQUOISE);
			} else {
				render_aabb((f32*)body, RED);
			}
		}

		for (usize i = 0; i < physics_static_body_count(); ++i) {
			render_aabb((f32*)physics_static_body_get(i), WHITE);
		}
#endif

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

			render_sprite_sheet_frame(adef->sprite_sheet, aframe->row, aframe->column, body->aabb.position, anim->is_flipped, WHITE, 1);
		}

		render_end(window, (u32[8]){0, sprite_sheet_player.texture_id, sprite_sheet_map.texture_id, 0, 0, 0, 0, 0});

		player_color[0] = 0;
		player_color[2] = 1;

		time_update_late();
	}

	return 0;
}

