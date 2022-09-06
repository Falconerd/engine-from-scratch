render=src/engine/render/render.c src/engine/render/render_init.c src/engine/render/render_util.c
io=src/engine/io/io.c
config=src/engine/config/config.c
input=src/engine/input/input.c
time=src/engine/time/time.c
physics=src/engine/physics/physics.c
array_list=src/engine/array_list/array_list.c
entity=src/engine/entity/entity.c
files=src/glad.c src/main.c src/engine/global.c $(render) $(io) $(config) $(input) $(time) $(physics) $(array_list) $(entity)

libs=-lm `sdl2-config --cflags --libs`

#CL /Zi /I W:/include $(files) /link $(libs) /OUT:mygame.exe

build:
	gcc -g3 $(files) $(libs) -o mygame.out
