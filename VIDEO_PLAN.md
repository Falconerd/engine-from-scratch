# Finishing the Game: Videos 19-21

Comparison reference: `../games-from-scratch/arcade-platform-shooter` (local clone)

## Current state (branch 18, commit b16297c — synced with Episode 18)

What exists:
- Fire trigger: enemies fall in, respawn enraged. Player death resets.
- `reset()`, `physics_reset()`, `entity_reset()`
- Audio: jump sound, stage 1 music
- Input: left, right, up, shoot, escape
- Entity struct: body_id, animation_id, sprite_offset, is_active, is_enraged. **No health,
  no is_flipped, no lifetime.**
- Animation struct: has `is_flipped` (will need to move to Entity)
- `entity_damage()` / `entity_destroy()` declared but **not implemented**
- `physics_body_destroy()` **not declared or implemented**
- Weapon type enum, projectile type enum, weapon struct (no sfx, no sprite_offset_flipped_x,
  no sprite_coords)
- `spawn_projectile()` exists but passes 0/0 for layer/mask, no callbacks, no angle, no
  lifetime
- Pistol defined (speed 200, basic values)
- Small projectile animation
- Shoot timer works, timer decrements in main loop
- Debug AABB rendering always on

---

## Video 19: Complete Weapon System

**Title angle**: "5 Weapons From Scratch in C (No Engine)" or similar
**New viewer context** (30s at start): We're building a 2D arcade shooter from scratch in C
with SDL2/OpenGL. We have a player, enemies, physics, and a basic pistol that shoots
projectiles that don't do anything yet. Today we make weapons actually work.

Starting from: branch 18 (basic pistol shoots, projectiles fly forever with no collisions)

Structure:
1. Engine housekeeping (damage, destroy, health, is_flipped move, lifetime)
2. Projectile collisions + sounds
3. All weapons, switching, rendering

### Engine: entity_damage, entity_destroy, physics_body_destroy

- [ ] Add `u8 health` field to Entity struct in `src/engine/entity.h`
- [ ] Change `entity_damage` return type from `void` to `bool` in `entity.h`:
  ```c
  bool entity_damage(usize entity_id, u8 amount);
  ```
- [ ] Implement in `src/engine/entity/entity.c`:
  ```c
  bool entity_damage(usize entity_id, u8 amount) {
      Entity *entity = entity_get(entity_id);
      if (amount >= entity->health) {
          entity_destroy(entity_id);
          return true;
      }
      entity->health -= amount;
      return false;
  }

  void entity_destroy(usize entity_id) {
      Entity *entity = entity_get(entity_id);
      physics_body_destroy(entity->body_id);
      entity->is_active = false;
  }
  ```
- [ ] Add `physics_body_destroy` declaration to `src/engine/physics.h`
- [ ] Implement in `src/engine/physics/physics.c`:
  ```c
  void physics_body_destroy(usize body_id) {
      Body *body = physics_body_get(body_id);
      body->is_active = false;
  }
  ```

### Engine: Move is_flipped from Animation to Entity

- [ ] Add `bool is_flipped` to Entity struct in `entity.h`
- [ ] Remove `bool is_flipped` from Animation struct in `animation.h`
- [ ] Update `animation_render()` signature to take `bool is_flipped`:
  ```c
  void animation_render(Animation *animation, bool is_flipped, vec2 position,
                        vec4 color, u32 texture_slots[8]);
  ```
- [ ] Update implementation in `animation.c`
- [ ] In `input_handle()`: set `player->is_flipped` instead of anim->is_flipped. Change
  signature to `input_handle(Entity *player, Body *body_player)`
- [ ] In entity render loop: pass `entity->is_flipped` to `animation_render()`, remove the
  velocity-based is_flipped setting on Animation
- [ ] In `enemy_small_on_hit_static()`: set `entity->is_flipped` when changing direction
- [ ] In `spawn_enemy()`: set `entity->is_flipped = is_flipped`

### Engine: Add lifetime to Entity

- [ ] Add `f32 lifetime` to Entity struct in `entity.h`
- [ ] Add `f32 lifetime` parameter to `entity_create()` (both .h and .c)
- [ ] Update ALL entity_create call sites to pass `0` for lifetime
- [ ] In entity render loop, before rendering:
  ```c
  if (entity->lifetime > 0) {
      entity->lifetime -= global.time.delta;
      if (entity->lifetime <= 0) {
          entity_destroy(i);
          continue;
      }
  }
  ```

### Game: Sound effects

- [ ] Add `Mix_Chunk *sfx` field to Weapon struct
- [ ] Load sounds (copy wav files from `../games-from-scratch/arcade-platform-shooter/assets/`):
  ```c
  static Mix_Chunk *SOUND_SHOOT;
  static Mix_Chunk *SOUND_BULLET_HIT_WALL;
  static Mix_Chunk *SOUND_HURT;
  static Mix_Chunk *SOUND_ENEMY_DEATH;
  static Mix_Chunk *SOUND_PLAYER_DEATH;

  // In main():
  audio_sound_load(&SOUND_SHOOT, "assets/shoot.wav");
  audio_sound_load(&SOUND_BULLET_HIT_WALL, "assets/bullet_hit_wall.wav");
  audio_sound_load(&SOUND_HURT, "assets/hurt.wav");
  audio_sound_load(&SOUND_ENEMY_DEATH, "assets/enemy_death.wav");
  audio_sound_load(&SOUND_PLAYER_DEATH, "assets/player_death.wav");
  ```
- [ ] Set `.sfx = SOUND_SHOOT` on pistol, play in `spawn_projectile()` via `audio_sound_play(weapon.sfx)`

### Game: Projectile collision callbacks

- [ ] Update `spawn_projectile()` to pass `COLLISION_LAYER_PROJECTILE`, `projectile_mask`,
  and callbacks
- [ ] Implement `projectile_on_hit()`:
  ```c
  void projectile_on_hit(Body *self, Body *other, Hit hit) {
      if (other->collision_layer == COLLISION_LAYER_ENEMY) {
          if (entity_damage(other->entity_id, 1)) {
              audio_sound_play(SOUND_ENEMY_DEATH);
          }
          audio_sound_play(SOUND_HURT);
      }
  }
  ```
- [ ] Implement `projectile_on_hit_static()`:
  ```c
  void projectile_on_hit_static(Body *self, Static_Body *other, Hit hit) {
      audio_sound_play(SOUND_BULLET_HIT_WALL);
      entity_destroy(self->entity_id);
  }
  ```

### Game: Enemy health + fire_on_hit fix

- [ ] In `spawn_enemy()`: `entity->health = is_small ? HEALTH_ENEMY_SMALL : HEALTH_ENEMY_LARGE`
- [ ] In `fire_on_hit()`: replace `enemy->is_active = false; other->is_active = false;`
  with `entity_destroy(other->entity_id);`

### Engine: Add frandr utility

- [ ] Add `frandr(f32 min, f32 max)` declaration to `src/engine/util.h`
- [ ] Implement (inline or in util.c):
  ```c
  f32 frandr(f32 min, f32 max) {
      return min + (f32)rand() / (f32)(RAND_MAX / (max - min));
  }
  ```

### Game: Update spawn_projectile

- [ ] Change signature to accept explicit velocity components and lifetime:
  ```c
  static void spawn_projectile(Projectile_Type type, f32 vx, f32 vy,
                                f32 lifetime, On_Hit on_hit,
                                On_Hit_Static on_hit_static)
  ```
  Reference uses `spawn_projectile(PT_BULLET, x, y, vx, vy, ttl, on_collide, on_collide_static)`
  but our engine passes position via entity_create from the player body, so we just need
  `vx, vy, lifetime` plus callbacks.
- [ ] Use `player->is_flipped` to flip vx: `player->is_flipped ? -vx : vx`
- [ ] Pass `lifetime` to `entity_create()`
- [ ] Set projectile as kinematic (no gravity)
- [ ] Play weapon sound: `audio_sound_play(weapon.sfx)`

### Game: All projectile animations

- [ ] Add `anim_projectile_large_id`, `anim_projectile_rocket_id`, `anim_projectile_crate_id`
- [ ] Create animations (columns 1, 2, 3 in props sprite sheet)

### Game: Weapon struct additions + all 5 weapons

- [ ] Add to Weapon struct: `f32 sprite_offset_flipped_x`, `u32 sprite_coords[2]`
- [ ] Reorder enum to match reference: `WT_MACHINE_GUN, WT_SHOTGUN, WT_ROCKET_LAUNCHER, WT_PISTOL, WT_REVOLVER`
- [ ] Each weapon needs per-weapon sound, per-weapon screen shake, per-weapon fire rate.
  Values from the reference implementation:

  | Weapon         | Projectile     | Speed | Fire Rate | Screen Shake (dur, mag) | Sound              |
  |----------------|----------------|-------|-----------|-------------------------|--------------------|
  | Pistol         | PT_BULLET      | 300   | 0.25      | 0.05, 0.03              | SOUND_SHOOT        |
  | Revolver       | PT_BULLET_LARGE| 300   | 0.55      | 0.1, 0.75               | SOUND_REVOLVER     |
  | Machine Gun    | PT_BULLET      | 400   | 0.05      | 0.05, 0.15              | SOUND_MACHINE_GUN  |
  | Shotgun        | PT_BULLET      | 280-350 (random) | 0.75 | 0.1, 0.75         | SOUND_SHOTGUN      |
  | Rocket         | PT_ROCKET      | 200   | 1.25      | (on explosion only)     | SOUND_ROCKET_LAUNCHED |

- [ ] Weapon sprite coords from reference (row, column in weapons.png 32x32 sheet):
  - Pistol: row 0, col 3. Offset: x=-8, y=-12, flipped_x=-24
  - Revolver: row 1, col 4. Offset: x=-8, y=-12, flipped_x=-24
  - Machine Gun: row 0, col 2. Offset: x=-10, y=-12, flipped_x=-22
  - Shotgun: row 0, col 1. Offset: x=-10, y=-14, flipped_x=-22
  - Rocket Launcher: row 1, col 0. Offset: x=-8, y=-12, flipped_x=-24

- [ ] Projectile collision callbacks differ by type in reference:
  - `on_bullet_collide`: `entity_destroy(self)`, `--enemy->health`, call `on_enemy_hit`
  - `on_bullet_large_collide`: same but `enemy->health -= 2` (revolver does double damage)
  - `on_bullet_collide_static`: `entity_destroy(self)`, play BULLET_HIT_WALL_SOUND
  - `on_rocket_collide`: `entity_destroy(self)`, set explosion position/timer,
    `render_screen_shake_add(EXPLOSION_TIME, 1.5)`, play EXPLOSION_SOUND

### Game: Weapon-specific shooting logic

- [ ] Weapon-specific shooting with switch (from reference `input_handle` / keyboard E):
  ```c
  case WT_MACHINE_GUN:
      // Single bullet with random vertical spread, fast fire rate
      spawn_projectile(PT_BULLET, 400, frandr(-15, 15), 9,
                       on_bullet_collide, on_bullet_collide_static);
      state.weapon_kick = 100;
      render_screen_shake_add(0.05, 0.15);
      break;

  case WT_SHOTGUN:
      // 15 pellets with random velocity spread, short lifetime (0.25s)
      for (u32 i = 0; i < 15; ++i) {
          f32 vy = frandr(-35, 35);
          f32 vx = frandr(280, 350);
          spawn_projectile(PT_BULLET, vx, vy, 0.25,
                           on_bullet_collide, on_bullet_collide_static);
      }
      render_screen_shake_add(0.1, 0.75);
      break;

  case WT_ROCKET_LAUNCHER:
      // Single rocket, slow acceleration, kinematic, 9s lifetime
      spawn_projectile(PT_ROCKET, 200, 0, 9,
                       on_rocket_collide, on_rocket_collide);
      break;

  case WT_PISTOL:
      // Single bullet, offset y+5
      spawn_projectile(PT_BULLET, 300, 0, 9,
                       on_bullet_collide, on_bullet_collide_static);
      render_screen_shake_add(0.05, 0.03);
      break;

  case WT_REVOLVER:
      // Large bullet (2x damage), offset y+5
      spawn_projectile(PT_BULLET_LARGE, 300, 0, 9,
                       on_bullet_large_collide, on_bullet_collide_static);
      render_screen_shake_add(0.1, 0.75);
      break;
  ```

### Game: Weapon kick (persistent recoil decay)

- [ ] Track `f32 weapon_kick` in game state
- [ ] On shoot: set `weapon_kick = 100` (machine gun) or similar per weapon
- [ ] Each frame: `weapon_kick -= 1000 * delta`
- [ ] When `weapon_kick >= 0`: override horizontal velocity
  ```c
  if (weapon_kick >= 0) {
      velx = player->is_flipped ? weapon_kick : -weapon_kick;
  }
  ```

### Game: Weapon switching (number keys)

- [ ] Handle SDLK_1 through SDLK_5 in SDL event loop

### Game: Weapon sprite rendering

- [ ] Load weapons sprite sheet (`assets/weapons.png` — copy from reference):
  ```c
  Sprite_Sheet sprite_sheet_weapons;
  render_sprite_sheet_init(&sprite_sheet_weapons, "assets/weapons.png", 32, 32);
  ```
- [ ] After entity render loop, render current weapon on player:
  ```c
  Entity *player = entity_get(player_id);
  Body *body = physics_body_get(player->body_id);
  Weapon weapon = weapons[weapon_type];
  vec2 offset = {
      player->is_flipped ? weapon.sprite_offset_flipped_x : weapon.sprite_offset[0],
      weapon.sprite_offset[1]
  };
  vec2 p;
  vec2_add(p, body->aabb.position, offset);
  render_sprite_sheet_frame(&sprite_sheet_weapons,
      weapon.sprite_coords[0], weapon.sprite_coords[1],
      p, player->is_flipped, WHITE, texture_slots);
  ```

### Testing checklist

- [ ] Bullets collide with enemies and terrain, sounds play
- [ ] Enemies take damage, die at 0 health
- [ ] All 5 weapons fire with correct projectile types
- [ ] Number keys 1-5 switch weapons
- [ ] Weapon sprite renders on player, flips correctly
- [ ] Shotgun fires 7 spread bullets with short lifetime
- [ ] Recoil pushes player
- [ ] Projectiles with lifetime expire
- [ ] Fire trigger still works (entity_destroy path)

---

## Video 20: Rocket Launcher, Circle Shader, Screen Shake, Smoke

**Title angle**: "Explosions & Screen Shake From Scratch in C" or "Making a Rocket Launcher
Feel Good (C Game Dev)"
**New viewer context**: Quick recap — we have a 2D shooter with 5 weapons. The rocket launcher
fires but doesn't explode. Today we add a circle shader for explosions, screen shake, and
smoke particles.

### Engine: Add rotation to Entity + render path

- [ ] Add `f32 rotation` to Entity struct
- [ ] Update `animation_render()` and `render_sprite_sheet_frame()` to support rotation
  (model matrix path when rotation != 0, batch path otherwise)

### Engine: Circle shader

- [ ] Create `shaders/circle.vert`:
  ```glsl
  #version 330 core
  layout (location = 0) in vec3 a_pos;
  uniform mat4 projection;
  uniform mat4 model;
  void main() {
      gl_Position = projection * model * vec4(a_pos, 1.0);
  }
  ```
- [ ] Create `shaders/circle.frag`:
  ```glsl
  #version 330 core
  out vec4 FragColor;
  uniform vec4 color;
  uniform vec2 position;
  uniform float radius;
  void main() {
      vec2 st = vec2(gl_FragCoord.x - position.x + radius,
                     gl_FragCoord.y - position.y + radius) / vec2(radius * 2);
      float pct = distance(st, vec2(0.5));
      if (pct > 0.5) {
          FragColor = vec4(0);
      } else {
          FragColor = color;
      }
  }
  ```
- [ ] Load circle shader in render_init, store as `circle_shader`
- [ ] Implement `render_circle(f32 x, f32 y, f32 radius, vec4 color)`:
  - Model matrix: translate to screen center, scale to screen size
  - Position/radius uniforms must be multiplied by SCALE (reference uses `SCALE=4`)
    for gl_FragCoord alignment:
    ```c
    float r[] = {radius * SCALE};
    glUniform2fv(..., (vec2){x * SCALE, y * SCALE});
    glUniform1fv(..., &r[0]);
    ```

### Engine: Screen shake

- [ ] Add `f32 screen_shake_timer` and `f32 screen_shake_magnitude` to render state
- [ ] `render_screen_shake_add(f32 duration, f32 magnitude)` — accumulate both values:
  ```c
  void render_screen_shake_add(f32 duration, f32 magnitude) {
      state->screen_shake_timer += duration;
      state->screen_shake_magnitude += magnitude;
  }
  ```
- [ ] `render_screen_shake(f32 delta_time)` — offset projection matrix with random jitter:
  ```c
  void render_screen_shake(f32 delta_time) {
      if (state->screen_shake_timer <= 0) {
          state->screen_shake_magnitude = 0;
          // Reset to normal ortho projection
          mat4x4_ortho(state->projection, 0, WIDTH, 0, HEIGHT, -2.0f, 2.0f);
          glUniformMatrix4fv(..., &state->projection[0][0]);
          return;
      }
      state->screen_shake_timer -= delta_time;
      f32 x = frandr(-state->screen_shake_magnitude, state->screen_shake_magnitude);
      f32 y = frandr(-state->screen_shake_magnitude, state->screen_shake_magnitude);
      mat4x4_ortho(state->projection, 0 + x, WIDTH + x, 0 + y, HEIGHT + y, -2.0f, 2.0f);
      glUniformMatrix4fv(..., &state->projection[0][0]);
  }
  ```
- [ ] Call `render_screen_shake(delta)` each frame after physics update

### Game: Rocket explosion + area damage

- [ ] Constants: `EXPLOSION_RADIUS = 60`, `EXPLOSION_TIME = 0.25`
- [ ] Track state: `vec2 rocket_explosion_position`, `f32 rocket_explosion_timer`,
  `u32 rocket_id`
- [ ] `on_rocket_collide` callback:
  ```c
  void on_rocket_collide(Body *self, Static_Body *other, Hit hit) {
      entity_destroy(self->entity_id);
      rocket_explosion_position = hit.position;
      rocket_explosion_timer = EXPLOSION_TIME;
      render_screen_shake_add(EXPLOSION_TIME, 1.5);
      rocket_id = 0;
      audio_sound_play(SOUND_EXPLOSION);
  }
  ```
- [ ] `rocket_damage(f32 pct)` — kill all enemies within expanding radius:
  ```c
  static void rocket_damage(f32 pct) {
      for (usize i = 0; i < entity_count(); ++i) {
          Entity *entity = entity_get(i);
          Body *body = physics_body_get(entity->body_id);
          if (body->collision_layer == COLLISION_LAYER_ENEMY && !body->is_kinematic) {
              f32 dist = vec2_sqr_dist(body->aabb.position, rocket_explosion_position);
              if (dist <= EXPLOSION_RADIUS * EXPLOSION_RADIUS * pct) {
                  kill_enemy(i);
              }
          }
      }
  }
  ```
- [ ] In render loop, when `rocket_explosion_timer > 0`:
  ```c
  glUseProgram(circle_shader);
  f32 pct = 1 - rocket_explosion_timer / EXPLOSION_TIME;
  render_circle(rocket_explosion_position[0],
                rocket_explosion_position[1],
                EXPLOSION_RADIUS * pct, WHITE);
  rocket_explosion_timer -= delta;
  rocket_damage(pct);
  ```

### Game: Rocket smoke trail

- [ ] Copy `assets/smoke.png` from reference, create sprite sheet (24x24) + idle animation
- [ ] Track `f32 rocket_smoke_timer`, init to `0.01` when rocket spawns
- [ ] Each frame when `rocket_id > 0` and timer expires, spawn a smoke entity:
  ```c
  if (rocket_id > 0) {
      Entity *rocket = entity_get(rocket_id);
      rocket_smoke_timer -= delta;
      if (rocket->is_active && rocket_smoke_timer < 0) {
          usize smoke_id = entity_create(rocket_pos, (vec2){0,0}, (vec2){0,0},
              (vec2){frandr(-10,10), frandr(-10,10)}, 0, 0, true,
              anim_smoke_id, NULL, NULL, frandr(0.15, 0.6));
          Entity *smoke = entity_get(smoke_id);
          smoke->rotation = frandr(0, 2 * PI);
          // Alpha fade: sprite_color_delta[3] = -0.05, desired = 0
          rocket_smoke_timer = 0.05;
      }
  }
  ```

### Game: Weapon-specific sounds + screen shake per weapon

- [ ] Load additional weapon sounds:
  ```c
  static Mix_Chunk *SOUND_REVOLVER;
  static Mix_Chunk *SOUND_SHOTGUN;
  static Mix_Chunk *SOUND_MACHINE_GUN;
  static Mix_Chunk *SOUND_ROCKET_LAUNCHED;
  static Mix_Chunk *SOUND_EXPLOSION;

  audio_sound_load(&SOUND_REVOLVER, "assets/revolver.wav");
  audio_sound_load(&SOUND_SHOTGUN, "assets/shotgun.wav");
  audio_sound_load(&SOUND_MACHINE_GUN, "assets/machine_gun.wav");
  audio_sound_load(&SOUND_ROCKET_LAUNCHED, "assets/rocket_launched.wav");
  audio_sound_load(&SOUND_EXPLOSION, "assets/explosion.wav");
  ```
- [ ] Play per-weapon sound in the shoot switch (not in spawn_projectile).
  Screen shake values per weapon listed in Video 19 weapon table above.

### Game: Rocket behavior

- [ ] Make rockets kinematic (no gravity)
- [ ] Rockets use acceleration instead of instant velocity in reference:
  ```c
  // In spawn_projectile for PT_ROCKET:
  projectile->acceleration[0] = player->is_flipped ? -speed * 0.05 : speed * 0.05;
  projectile->desired_velocity[0] = player->is_flipped ? -speed : speed;
  projectile->velocity[0] = 0;  // starts slow, accelerates
  ```
  Note: this requires `acceleration` and `desired_velocity` fields on Entity or Body,
  which the reference has but our engine does not. Either add them or simplify to
  instant velocity (less satisfying feel).

---

## Video 21: Text, Score, Polish, Wrap-up

**Title angle**: "Finishing My C Game From Scratch" or "Score, Text Rendering & Polish
(C Game Dev)"
**New viewer context**: We've built a complete 2D arcade shooter from scratch — player,
enemies, 5 weapons, explosions. Today we add text rendering, a score system, and the final
game feel polish to ship it.

### Engine: Text rendering (stb_truetype)

- [ ] Add `stb_truetype.h` to `deps/include/`
- [ ] Copy font `assets/8-BIT_WONDER.TTF` from reference
- [ ] Create text shaders:
  `shaders/text.vert`:
  ```glsl
  #version 330 core
  layout (location = 0) in vec4 vertex;
  out vec2 uvs;
  uniform mat4 projection;
  void main() {
      uvs = vertex.zw;
      gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
  }
  ```
  `shaders/text.frag`:
  ```glsl
  #version 330 core
  in vec2 uvs;
  out vec4 frag_color;
  uniform sampler2D tex;
  uniform vec4 color;
  void main() {
      frag_color = vec4(1.0, 1.0, 1.0, texture(tex, uvs).r) * color;
  }
  ```
- [ ] Add to render state: `u32 text_vao, text_vbo, text_shader, text_texture`
- [ ] In render_init: load font with stb_truetype, bake bitmap (512x512 atlas),
  create GL texture, setup text VAO/VBO
- [ ] Implement `render_text(const char *text, f32 x, f32 y, vec4 color, u8 is_centered)`:
  - Multiply x, y by SCALE for screen coordinates
  - If centered, measure total width first and offset x by -width/2
  - Use `stbtt_GetBakedQuad` per character, upload quads to text VBO

### Game: Score system

- [ ] Add `u32 score` and `char score_string[10]` to game state
- [ ] Increment score on enemy kills: `++score; sprintf(score_string, "%d", score);`
- [ ] Reset score to 0 in `reset()`
- [ ] Render centered at top of screen:
  ```c
  glUseProgram(text_shader);
  render_text(score_string, WIDTH / 2, HEIGHT - 20, WHITE, 1);
  ```

### Game: Sprite color transitions

- [ ] Add to Entity struct: `vec4 sprite_color`, `vec4 desired_sprite_color`,
  `vec4 sprite_color_delta` — init sprite_color to `{1,1,1,1}` on create
- [ ] Each frame, per entity, lerp each channel:
  ```c
  for (u32 j = 0; j < 4; ++j) {
      entity->sprite_color[j] += entity->sprite_color_delta[j];
      if (entity->sprite_color_delta[j] != 0 &&
          fabs(entity->sprite_color[j]) < fabs(entity->desired_sprite_color[j])) {
          entity->sprite_color[j] = entity->desired_sprite_color[j];
      }
  }
  ```
- [ ] Pass `entity->sprite_color` instead of `WHITE` to `animation_render()`
- [ ] Smoke alpha fade: on smoke spawn, set `sprite_color_delta[3] = -0.05`,
  `desired_sprite_color[3] = 0`

### Game: Game feel polish

- [ ] Variable jump height — cut velocity on key release:
  ```c
  if (!keyboard[jump] && jump_was_pressed) {
      vely *= 0.5;
      jump_was_pressed = false;
  }
  ```
  Set `jump_was_pressed = true` when jump triggers.

- [ ] Enemy death animation — instead of instant destroy, set time_to_live:
  ```c
  static void kill_enemy(usize id) {
      Entity *enemy = entity_get(id);
      enemy->lifetime = 3;
      // Change collision layer so it stops colliding
      Body *body = physics_body_get(enemy->body_id);
      body->collision_layer = COLLISION_LAYER_MISC;
      body->velocity[1] = 100;  // pop up
      audio_sound_play(SOUND_ENEMY_DEATH);
  }
  ```
  In render loop: when `entity->lifetime > 0 && !is_kinematic`, rotate the entity
  (`entity->rotation += delta * 10`).

- [ ] Large enemy landing screen shake — in `enemy_large_on_hit_static`, when
  landing (normal[1] > 0 and had vertical velocity):
  ```c
  if (last_velocity_y != 0) {
      render_screen_shake_add(EXPLOSION_TIME, 0.2);
  }
  ```

- [ ] Debug rendering toggle — wrap AABB rendering in `#if DEBUG` or a runtime flag

- [ ] Enemy on-hit knockback from reference:
  ```c
  // On bullet hit, zero enemy velocity and set acceleration to recover
  enemy->desired_velocity[0] = SPEED * fsign(enemy->velocity[0]);
  enemy->acceleration[0] = SPEED * fsign(enemy->velocity[0]) * 0.1;
  enemy->velocity[0] = 0;
  ```
  (Requires acceleration/desired_velocity support in engine)

### Game: Weapon box pickups (optional)

- [ ] Replace number-key switching with in-world box entities
- [ ] Define spawn regions:
  ```c
  static const f32 BOX_SPAWN_REGIONS[][4] = {
      {32, HEIGHT - 7 * 32, 6 * 32, 6 * 32},
      {8 * 32, HEIGHT - 7 * 32, 6 * 32, 6 * 32}
  };
  ```
- [ ] `spawn_box()`: pick random region, random position within it via `frandr`,
  create entity with `CL_BOX` layer, `BOX_IDLE_ANIM` (props sheet column 3)
- [ ] `on_box_collide`: if collides with player, pick random weapon type
  (avoid same weapon), destroy box, spawn new box, play `BOX_SOUND`,
  increment score
- [ ] Copy `assets/box.wav` from reference

---

## Assets needed

All from reference (`../games-from-scratch/arcade-platform-shooter/assets/`):

Video 19:
```
assets/shoot.wav          (shoot1.wav in reference — rename)
assets/bullet_hit_wall.wav
assets/hurt.wav
assets/enemy_death.wav
assets/player_death.wav
assets/weapons.png
```

Video 20:
```
assets/revolver.wav
assets/shotgun.wav
assets/machine_gun.wav
assets/rocket_launched.wav
assets/explosion.wav
assets/smoke.png
```

Video 21:
```
assets/8-BIT_WONDER.TTF
assets/box.wav            (if doing weapon boxes)
```

## Summary of engine changes across all 3 videos

### entity.h
- `u8 health`, `bool is_flipped`, `f32 lifetime` (Video 19)
- `f32 rotation` (Video 20)
- `vec4 sprite_color`, `vec4 desired_sprite_color`, `vec4 sprite_color_delta` (Video 21)

### entity.c
- `entity_damage()`, `entity_destroy()` (Video 19)

### physics.h / physics.c
- `physics_body_destroy()` (Video 19)

### render.h / render.c
- `render_sprite_sheet_frame()` gains rotation parameter (Video 20)
- `render_circle()` (Video 20)
- `render_screen_shake_add()`, `render_screen_shake()` (Video 20)
- `render_text()` (Video 21)
- New shaders: circle.vert/frag (Video 20), text.vert/frag (Video 21)
- New dep: stb_truetype.h (Video 21)

### animation.h / animation.c
- Remove `is_flipped`, add `is_flipped` param to `animation_render()` (Video 19)

### util.h
- `frandr(f32 min, f32 max)` (Video 19, used for shotgun spread)
