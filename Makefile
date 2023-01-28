render=src/engine/render/render.c src/engine/render/render_init.c src/engine/render/render_util.c
io=src/engine/io/io.c
config=src/engine/config/config.c
input=src/engine/input/input.c
time=src/engine/time/time.c
physics=src/engine/physics/physics.c
array_list=src/engine/array_list/array_list.c
entity=src/engine/entity/entity.c
animation=src/engine/animation/animation.c
audio=src/engine/audio/audio.c
files=deps/src/glad.c src/main.c src/engine/global.c $(render) $(io) $(config) $(input) $(time) $(physics) $(array_list) $(entity) $(animation) $(audio)

libs=-lm `sdl2-config --cflags --libs` -lSDL2_mixer `pkg-config --libs glfw3` -ldl

build:
	gcc -g3 -O0 -I./deps/include $(files) $(libs) -o mygame.out
