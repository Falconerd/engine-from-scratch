# Finishing the Game: Videos 19-21

Comparison reference: `git@github.com:Falconerd/games-from-scratch/arcade-platform-shooter`

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
- [ ] Load sounds (copy wav files from origin/19 or finished version):
  ```c
  static Mix_Chunk *SOUND_SHOOT;
  static Mix_Chunk *SOUND_BULLET_HIT_WALL;
  static Mix_Chunk *SOUND_HURT;
  static Mix_Chunk *SOUND_ENEMY_DEATH;
  static Mix_Chunk *SOUND_PLAYER_DEATH;
  ```
- [ ] Set `.sfx = SOUND_SHOOT` on pistol, play in `spawn_projectile()`

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

### Game: compute_velocities + angle-based spawning

- [ ] Add `#define PI 3.14159265358979323846` and `#include <math.h>`
- [ ] Implement:
  ```c
  static void compute_velocities(vec2 v, f32 speed, f32 degrees) {
      f32 radians = degrees * (PI / 180.0);
      v[0] = speed * cos(radians);
      v[1] = speed * sin(radians);
  }
  ```
- [ ] Update `spawn_projectile()` to accept `f32 angle, f32 lifetime`, use
  `player->is_flipped` instead of `animation->is_flipped`, pass lifetime to entity_create

### Game: All projectile animations

- [ ] Add `anim_projectile_large_id`, `anim_projectile_rocket_id`, `anim_projectile_crate_id`
- [ ] Create animations (columns 1, 2, 3 in props sprite sheet)

### Game: Weapon struct additions + all 5 weapons

- [ ] Add to Weapon struct: `f32 sprite_offset_flipped_x`, `u32 sprite_coords[2]`
- [ ] Reorder enum: PISTOL, REVOLVER, SMG, SHOTGUN, ROCKET_LAUNCHER
- [ ] Define all 5 weapons with tuned values (see branch origin/19 for reference)
- [ ] Tune pistol: speed 200 → 700, proper sprite coords/offsets

### Game: Shotgun spread + recoil

- [ ] Weapon-specific shooting with switch:
  - Shotgun: 7 bullets with angle spread, 0.2s lifetime
  - Default: single projectile, no angle, no lifetime
- [ ] Apply recoil to player velocity after firing:
  ```c
  velx = player->is_flipped ? weapon.recoil : -weapon.recoil;
  ```

### Game: Weapon switching (number keys)

- [ ] Handle SDLK_1 through SDLK_5 in SDL event loop

### Game: Weapon sprite rendering

- [ ] Load weapons sprite sheet (`assets/weapons.png` — copy from origin/19)
- [ ] After entity render loop, render current weapon on player using sprite_coords and
  sprite_offset / sprite_offset_flipped_x

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

- [ ] Create `shaders/circle.vert` and `shaders/circle.frag` (distance field circle)
- [ ] Load shader in render_init, add `render_circle()` function
  - Note: position/radius must be multiplied by scale (3) for gl_FragCoord

### Engine: Screen shake

- [ ] `render_screen_shake_add(f32 duration, f32 magnitude)` — accumulate
- [ ] `render_screen_shake(f32 delta_time)` — offset projection matrix with random jitter
- [ ] Add `frandr(f32 min, f32 max)` utility

### Game: Rocket explosion + area damage

- [ ] Track rocket_explosion_position, rocket_explosion_timer, rocket_entity_id
- [ ] Detect rocket in collision callbacks, trigger explosion
- [ ] `rocket_damage()`: kill all enemies within EXPLOSION_RADIUS
- [ ] Render expanding circle, screen shake on explosion

### Game: Rocket smoke trail

- [ ] Smoke sprite sheet + animation (copy `assets/smoke.png` from finished version)
- [ ] Spawn kinematic smoke particles behind rockets with random rotation + fade

### Game: Weapon-specific sounds + screen shake per weapon

- [ ] Per-weapon sounds: revolver.wav, shotgun.wav, machine_gun.wav, rocket_launched.wav
- [ ] Per-weapon screen shake values

### Game: Rocket behavior

- [ ] Make rockets kinematic (no gravity)

---

## Video 21: Text, Score, Polish, Wrap-up

**Title angle**: "Finishing My C Game From Scratch" or "Score, Text Rendering & Polish
(C Game Dev)"
**New viewer context**: We've built a complete 2D arcade shooter from scratch — player,
enemies, 5 weapons, explosions. Today we add text rendering, a score system, and the final
game feel polish to ship it.

### Engine: Text rendering (stb_truetype)

- [ ] Add `stb_truetype.h` to deps, text shaders, `render_text()` function

### Game: Score system

- [ ] Track score, increment on kills, render centered at top

### Game: Sprite color transitions

- [ ] `sprite_color` / `desired_sprite_color` / `sprite_color_delta` on Entity
- [ ] Hit flash on enemies, alpha fade on smoke

### Game: Game feel polish

- [ ] Weapon kick (persistent recoil decay)
- [ ] Variable jump height (cut velocity on key release)
- [ ] Enemy death animation (pop up, rotate, fade)
- [ ] Large enemy landing screen shake
- [ ] Debug rendering toggle

### Game: Weapon box pickups (optional)

- [ ] Replace number-key switching with in-world box entities

---

## Assets needed

From branch origin/19 (`git checkout origin/19 -- <path>`):
```
assets/shoot.wav
assets/bullet_hit_wall.wav
assets/hurt.wav
assets/enemy_death.wav
assets/player_death.wav
assets/weapons.png
assets/revolver.wav
```

From finished version (`../games-from-scratch/arcade-platform-shooter/assets/`):
```
explosion.wav
shotgun.wav
machine_gun.wav
rocket_launched.wav
smoke.png
8-BIT_WONDER.TTF
box.wav (if doing weapon boxes)
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
- `frandr(f32 min, f32 max)` (Video 20)
