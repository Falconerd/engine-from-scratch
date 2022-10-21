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

#define DEBUG 0

static Mix_Music *MUSIC_STAGE_1;
static Mix_Chunk *SOUND_JUMP;

static Sprite_Sheet sprite_sheet_map;
static Sprite_Sheet sprite_sheet_player;
static Sprite_Sheet sprite_sheet_enemy_small;
static Sprite_Sheet sprite_sheet_enemy_large;
static Sprite_Sheet sprite_sheet_props;

static const f32 SPEED_PLAYER = 250;
static const f32 JUMP_VELOCITY = 1350;
static const f32 GROUNDED_TIME = 0.1;
static const f32 SPEED_ENEMY_LARGE = 80;
static const f32 SPEED_ENEMY_SMALL = 100;
static const f32 HEALTH_ENEMY_LARGE = 7;
static const f32 HEALTH_ENEMY_SMALL = 3;

typedef enum collision_layer {
	COLLISION_LAYER_PLAYER = 1,
	COLLISION_LAYER_ENEMY = 1 << 1,
	COLLISION_LAYER_TERRAIN = 1 << 2,
	COLLISION_LAYER_ENEMY_PASSTHROUGH = 1 << 3,
	COLLISION_LAYER_PROJECTILE = 1 << 4,
} Collision_Layer;

static const u8 enemy_mask = COLLISION_LAYER_PLAYER | COLLISION_LAYER_TERRAIN;
static const u8 player_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_TERRAIN | COLLISION_LAYER_ENEMY_PASSTHROUGH;
static const u8 fire_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_PLAYER;
static const u8 projectile_mask = COLLISION_LAYER_ENEMY | COLLISION_LAYER_TERRAIN;

static SDL_Window *window;
static i32 window_width, window_height;
static f32 render_width, render_height;
static usize anim_player_walk_id;
static usize anim_player_idle_id;
static usize anim_enemy_small_id;
static usize anim_enemy_large_id;
static usize anim_box_id;

typedef enum weapon_type {
	WEAPON_TYPE_PISTOL,
	WEAPON_TYPE_REVOLVER,
	WEAPON_TYPE_MACHINE_GUN,
	WEAPON_TYPE_ROCKET_LAUNCHER,
	WEAPON_TYPE_COUNT,
} Weapon_Type;

typedef enum projectile_type {
	PROJECTILE_TYPE_SMALL,
	PROJECTILE_TYPE_LARGE,
	PROJECTILE_TYPE_ROCKET,
	PROJECTILE_TYPE_COUNT,
} Projectile_Type;

typedef struct weapon {
	Weapon_Type type;
	Projectile_Type projectile_type;
	f32 velocity;
	f32 cooldown;
	vec2 sprite_offset;
	vec2 spawn_offset;
} Weapon;

static Weapon weapons[WEAPON_TYPE_COUNT] = {
	{WEAPON_TYPE_PISTOL, PROJECTILE_TYPE_SMALL, 600, 0.2},
	{WEAPON_TYPE_REVOLVER, PROJECTILE_TYPE_LARGE, 700, 0.4},
};

typedef struct projectile {
	usize animation_id;
	vec2 size;
} Projectile;

static Projectile projectiles[PROJECTILE_TYPE_COUNT] = {
	{0, {4, 3}},
	{0, {7, 3}},
	{0, {11, 5}},
};

// State
static bool should_quit = false;
static f32 spawn_timer = 0;
static f32 shoot_timer = 0;
static f32 ground_timer = 0;
static Weapon_Type current_weapon_type;

void reset(void);

void player_on_hit(Body *self, Body *other, Hit hit) {
	if (other->collision_layer == COLLISION_LAYER_ENEMY) {
	}
}

void player_on_hit_static(Body *self, Static_Body *other, Hit hit) {
	if (hit.normal[1] > 0) {
		ground_timer = GROUNDED_TIME;
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
		Entity *entity = entity_by_body_id(hit.other_id);
		Body *body = physics_body_get(entity->body_id);

		entity->is_active = false;
		body->is_active = false;
	} else if (other->collision_layer == COLLISION_LAYER_PLAYER) {
		reset();
	}
}

void projectile_on_hit(Body *self, Body *other, Hit hit) {
	if (other->collision_layer == COLLISION_LAYER_ENEMY) {
		Entity *entity = entity_by_body_id(hit.other_id);
	}
}

void projectile_on_hit_static(Body *self, Static_Body *other, Hit hit) {
	Entity *entity = entity_by_body_id(hit.self_id);
	entity->is_active = false;
	self->is_active = false;
}

static void input_handle(Entity *player) {
	if (global.input.escape) {
		should_quit = true;
	}

	Body *body = physics_body_get(player->body_id);
	Animation *walk_anim = animation_get(anim_player_walk_id);
	Animation *idle_anim = animation_get(anim_player_idle_id);

	f32 velx = 0;
	f32 vely = body->velocity[1];

	if (global.input.right) {
		velx += SPEED_PLAYER;
		walk_anim->is_flipped = false;
		idle_anim->is_flipped = false;
	}

	if (global.input.left) {
		velx -= SPEED_PLAYER;
		walk_anim->is_flipped = true;
		idle_anim->is_flipped = true;
	}

	if (global.input.up && ground_timer >= 0) {
		ground_timer = -1;
		vely = JUMP_VELOCITY;
		audio_sound_play(SOUND_JUMP);
	}

	if (global.input.shoot) {
		if (shoot_timer <= 0) {
			Weapon *weapon = &weapons[current_weapon_type];
			Projectile *projectile = &projectiles[weapon->projectile_type];

			shoot_timer = weapon->cooldown;

			vec2 pos;
			vec2_add(pos, body->aabb.position, weapon->spawn_offset);

			f32 velocity = walk_anim->is_flipped ? -weapon->velocity : weapon->velocity;

			Entity *e = entity_get(entity_create(pos, projectile->size, (vec2){0, 0}, (vec2){velocity, 0}, COLLISION_LAYER_PROJECTILE, projectile_mask, true, projectile_on_hit, projectile_on_hit_static));
			e->animation_id = projectile->animation_id;
		}
	}

	body->velocity[0] = velx;
	body->velocity[1] = vely;
}

void spawn_enemy(bool is_small, bool is_enraged, bool is_flipped) {
	f32 spawn_x = is_flipped ? render_width : 0;
	f32 speed = is_small ? SPEED_ENEMY_SMALL : SPEED_ENEMY_LARGE;

	if (is_enraged) {
		speed *= 1.5;
	}

	usize entity_id;
	Entity *entity;

	if (is_small) {
		entity_id = entity_create(
			(vec2){spawn_x, render_height - 64},
			(vec2){12, 12},
			(vec2){0, 6},
			(vec2){0, 0},
			COLLISION_LAYER_ENEMY,
			enemy_mask,
			false,
			NULL,
			enemy_small_on_hit_static);
		entity = entity_get(entity_id);
		entity->animation_id = anim_enemy_small_id;
	} else {
		entity_id = entity_create(
			(vec2){spawn_x, render_height - 64},
			(vec2){20, 20},
			(vec2){0, 10},
			(vec2){0, 0},
			COLLISION_LAYER_ENEMY,
			enemy_mask,
			false,
			NULL,
			enemy_large_on_hit_static);
		entity = entity_get(entity_id);
		entity->animation_id = anim_enemy_large_id;
	}
	Body *body = physics_body_get(entity->body_id);
	body->velocity[0] = is_flipped ? -speed : speed;
}

void reset(void) {
	audio_music_play(MUSIC_STAGE_1);

	entity_reset();
	physics_reset();

	SDL_GetWindowSize(window, &window_width, &window_height);
	render_width = window_width / render_get_scale();
	render_height = window_height / render_get_scale();

	entity_create((vec2){100, 200}, (vec2){12, 12}, (vec2){0, 6}, (vec2){0, 0}, COLLISION_LAYER_PLAYER, player_mask, false, player_on_hit, player_on_hit_static);

	// Init level.
	{

		physics_static_body_create((vec2){render_width * 0.5, render_height - 16}, (vec2){render_width, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.25 - 16, 16}, (vec2){render_width * 0.5 - 32, 48}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.75 + 16, 16}, (vec2){render_width * 0.5 - 32, 48}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){16, render_height * 0.5 - 3 * 32}, (vec2){32, render_height}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width - 16, render_height * 0.5 - 3 * 32}, (vec2){32, render_height}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){32 + 64, render_height - 32 * 3 - 16}, (vec2){128, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width - 32 - 64, render_height - 32 * 3 - 16}, (vec2){128, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.5, render_height - 32 * 3 - 16}, (vec2){192, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){render_width * 0.5, 32 * 3 + 24}, (vec2){448, 32}, COLLISION_LAYER_TERRAIN);
		physics_static_body_create((vec2){16, render_height - 64}, (vec2){32, 64}, COLLISION_LAYER_ENEMY_PASSTHROUGH);
		physics_static_body_create((vec2){render_width - 16, render_height - 64}, (vec2){32, 64}, COLLISION_LAYER_ENEMY_PASSTHROUGH);
	}

	Entity *player = entity_get(0);
	player->animation_id = anim_player_idle_id;

	entity_create((vec2){render_width * 0.5, -4}, (vec2){0, 0}, (vec2){64, 8}, (vec2){0}, 0, fire_mask, true, fire_on_hit, NULL);

	spawn_timer = 0;
}

int main(int argc, char *argv[]) {
	time_init(60);
	window = render_init();
	config_init();
	physics_init();
	entity_init();
	animation_init();
	audio_init();

	SDL_ShowCursor(false);

	// Init audio.
	{
		audio_sound_load(&SOUND_JUMP, "assets/jump.wav");
		audio_music_load(&MUSIC_STAGE_1, "assets/breezys_mega_quest_2_stage_1.mp3");
	}

	// Init sprites.
	{
		render_sprite_sheet_init(&sprite_sheet_player, "assets/player.png", 24, 24);
		render_sprite_sheet_init(&sprite_sheet_map, "assets/map.png", 640, 360);
		render_sprite_sheet_init(&sprite_sheet_enemy_small, "assets/enemy_small.png", 24, 24);
		render_sprite_sheet_init(&sprite_sheet_enemy_large, "assets/enemy_large.png", 40, 40);
		render_sprite_sheet_init(&sprite_sheet_props, "assets/props_16x16.png", 16, 16);
	}

	// Init animations.
	{
		usize adef_player_walk_id = animation_definition_create(&sprite_sheet_player, (f32[7]){0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1}, (u8[7]){0, 0, 0, 0, 0, 0, 0}, (u8[7]){1, 2, 3, 4, 5, 6, 7}, 7);
		usize adef_player_idle_id = animation_definition_create(&sprite_sheet_player, (f32[1]){0}, (u8[1]){0}, (u8[1]){0}, 1);
		usize adef_enemy_small_id = animation_definition_create(&sprite_sheet_enemy_small, (f32[8]){0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1}, (u8[8]){1, 1, 1, 1, 1, 1, 1, 1}, (u8[8]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
		usize adef_enemy_large_id = animation_definition_create(&sprite_sheet_enemy_large, (f32[8]){0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1}, (u8[8]){1, 1, 1, 1, 1, 1, 1, 1}, (u8[8]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
		usize adef_projectile_small = animation_definition_create(&sprite_sheet_props, (f32[1]){0}, (u8[1]){0}, (u8[1]){0}, 1);
		usize adef_projectile_large = animation_definition_create(&sprite_sheet_props, (f32[1]){0}, (u8[1]){0}, (u8[1]){1}, 1);
		usize adef_projectile_rocket = animation_definition_create(&sprite_sheet_props, (f32[1]){0}, (u8[1]){0}, (u8[1]){2}, 1);
		usize adef_box = animation_definition_create(&sprite_sheet_props, (f32[1]){0}, (u8[1]){0}, (u8[1]){3}, 1);

		anim_player_walk_id = animation_create(adef_player_walk_id, true);
		anim_player_idle_id = animation_create(adef_player_idle_id, false);
		anim_enemy_small_id = animation_create(adef_enemy_small_id, true);
		anim_enemy_large_id = animation_create(adef_enemy_large_id, true);

		projectiles[PROJECTILE_TYPE_SMALL].animation_id = animation_create(adef_projectile_small, false);
		projectiles[PROJECTILE_TYPE_LARGE].animation_id = animation_create(adef_projectile_large, false);
		projectiles[PROJECTILE_TYPE_ROCKET].animation_id = animation_create(adef_projectile_rocket, false);

		anim_box_id = animation_create(adef_box, false);
	}

	reset();

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

		Entity *player = entity_get(0);
		Body *body_player = physics_body_get(player->body_id);

		if (body_player->velocity[0] != 0 || (global.input.left || global.input.right)) {
			player->animation_id = anim_player_walk_id;
		} else {
			player->animation_id = anim_player_idle_id;
		}

		input_update();
		input_handle(player);
		physics_update();

		animation_update(global.time.delta);

		// Update timers.
		{
			shoot_timer -= global.time.delta;
			spawn_timer -= global.time.delta;
			ground_timer -= global.time.delta;
		}

		// Spawn enemies.
		{
			if (spawn_timer <= 0) {
				spawn_timer = (f32)((rand() % 200) + 200) / 100.f;

				spawn_timer *= 0.2;

				bool is_flipped = rand() % 100 >= 50;
				bool is_small_enemy = rand() % 100 > 18;

				spawn_enemy(is_small_enemy, false, is_flipped);
			}
		}

		render_begin();

		render_sprite_sheet_frame(&sprite_sheet_map, 0, 0, (vec2){320, 180}, false, WHITE, 2);

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

		// Handle animated entities.
		for (usize i = 0; i < entity_count(); ++i) {
			Entity *entity = entity_get(i);
			if (!entity->is_active) {
				continue;
			}

			if (entity->animation_id == (usize)-1) {
				continue;
			}

			Body *body = physics_body_get(entity->body_id);
			Animation *anim = animation_get(entity->animation_id);

			if (body->velocity[0] < 0) {
				anim->is_flipped = true;
			}

			if (body->velocity[0] > 0) {
				anim->is_flipped = false;
			}

			vec2 pos;
			vec2_add(pos, body->aabb.position, entity->sprite_offset);
			animation_render(anim, pos, WHITE);
		}

		render_end(window, (u32[8]){
			0,
			sprite_sheet_player.texture_id,
			sprite_sheet_map.texture_id,
			sprite_sheet_enemy_small.texture_id,
			sprite_sheet_enemy_large.texture_id,
			sprite_sheet_props.texture_id,
		});

		time_update_late();
	}

	return 0;
}

