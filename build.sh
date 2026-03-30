#!/bin/sh
RENDER=src/engine/render/*.c
IO=src/engine/io/*.c
CONFIG=src/engine/config/*.c
INPUT=src/engine/input/*.c
TIME=src/engine/time/*.c
PHYSICS=src/engine/physics/*.c
ARRAY_LIST=src/engine/array_list/*.c
ENTITY=src/engine/entity/*.c
AUDIO=src/engine/audio/*.c
ANIMATION=src/engine/animation/*.c
C_FILES="$RENDER $IO $CONFIG $INPUT $TIME $PHYSICS $ARRAY_LIST $ENTITY $AUDIO $ANIMATION"

clang -g3 deps/src/glad.c src/main.c src/engine/global.c $C_FILES -I./deps/include -L./deps/lib $(sdl2-config --cflags --libs) -lm -lSDL2_mixer
