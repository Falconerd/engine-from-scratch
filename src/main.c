#include <stdio.h>
#include <stdbool.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "engine/global.h"

int main(int argc, char *argv[]) {
    render_init();

    puts("Hello there!");

    bool should_quit = false;

    while (!should_quit) {
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

        render_begin();

        render_quad((vec2){100, 100}, (vec2){25, 25}, (vec4){1, 1, 1, 1});

        render_end();
    }

    return 0;
}

