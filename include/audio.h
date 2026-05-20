#ifndef AUDIO_H
#define AUDIO_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/**************************************************************************
 * STRUCTS
 **************************************************************************/
typedef struct {
    SDL_AudioStream *stream;
    bool ready;
} AudioState;

/**************************************************************************
 * PROTOTYPES
 **************************************************************************/
bool audio_init(AudioState *audio);
void audio_shutdown(AudioState *audio);
void audio_clear(AudioState *audio);
void audio_enqueue_tone(AudioState *audio, float freq_hz, float gain);
Uint32 audio_play_stalin_anthem(AudioState *audio, bool sound_enabled);
Uint32 audio_play_midi_excerpt(AudioState *audio, const char *midi_path, double max_seconds, bool sound_enabled);

#endif
