#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

void audio_init(void);
void audio_sound_load(Mix_Chunk **chunk, const char *path);
void audio_music_load(Mix_Music **music, const char *path);
void audio_sound_play(Mix_Chunk *sound);
void audio_music_play(Mix_Music *music);

