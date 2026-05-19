#ifndef AUDIO_H
#define AUDIO_H

#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct {
    SDL_AudioStream *stream;
    bool ready;
} AudioState;

bool audio_init(AudioState *audio);
void audio_shutdown(AudioState *audio);
void audio_clear(AudioState *audio);
void audio_enqueue_tone(AudioState *audio, float freq_hz, float gain);

#endif
