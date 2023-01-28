#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "../types.h"
#include "../util.h"

void audio_init(void) {
	SDL_Init(SDL_INIT_AUDIO);

	i32 audio_rate = 44100;
	u16 audio_format = MIX_DEFAULT_FORMAT;
	i32 audio_channels = 2;
	i32 audio_buffers = 4096;

	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		ERROR_EXIT("SDL_Mixer error: OpenAudio: %s\n", Mix_GetError());
	}

    Mix_Volume(-1, 6);
    Mix_VolumeMusic(2);
}

void audio_sound_load(Mix_Chunk **chunk, const char *path) {
	*chunk = Mix_LoadWAV(path);
	if (!*chunk) {
		ERROR_EXIT("Failed to load WAV: %s\n", Mix_GetError());
	}
}

void audio_music_load(Mix_Music **music, const char *path) {
	*music = Mix_LoadMUS(path);
	if (!*music) {
		ERROR_EXIT("Failed to load music file %s: %s\n", path, Mix_GetError());
	}
}

void audio_sound_play(Mix_Chunk *sound) {
	Mix_PlayChannel(-1, sound, 0);
}

void audio_music_play(Mix_Music *music) {
	Mix_PlayMusic(music, -1);
}

