#include "audio.h"
#include "constants.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool audio_init(AudioState *audio) {
    SDL_AudioSpec spec = {0};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq = AUDIO_SAMPLE_RATE;

    audio->stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        NULL,
        NULL
    );
    if (!audio->stream) {
        audio->ready = false;
        return false;
    }

    SDL_ResumeAudioStreamDevice(audio->stream);
    audio->ready = true;
    return true;
}

void audio_shutdown(AudioState *audio) {
    if (audio->stream) {
        SDL_DestroyAudioStream(audio->stream);
        audio->stream = NULL;
    }
    audio->ready = false;
}

void audio_clear(AudioState *audio) {
    if (audio->stream) {
        SDL_ClearAudioStream(audio->stream);
    }
}

void audio_enqueue_tone(AudioState *audio, float freq_hz, float gain) {
    if (!audio->ready || !audio->stream) return;
    if (SDL_GetAudioStreamQueued(audio->stream) > AUDIO_MAX_QUEUED_BYTES) return;

    int samples = (AUDIO_SAMPLE_RATE * AUDIO_TONE_MS) / 1000;
    float buffer[1024];
    if (samples > (int)(sizeof(buffer) / sizeof(buffer[0]))) {
        samples = (int)(sizeof(buffer) / sizeof(buffer[0]));
    }

    static float phase = 0.0f;
    float step = (2.0f * (float)M_PI * freq_hz) / (float)AUDIO_SAMPLE_RATE;
    int fade = samples / 6;
    if (fade < 8) fade = 8;
    if (fade > samples / 2) fade = samples / 2;
    for (int i = 0; i < samples; i++) {
        float env = 1.0f;
        if (i < fade) env = (float)i / (float)fade;
        else if (i > samples - fade) env = (float)(samples - i) / (float)fade;
        if (env < 0.0f) env = 0.0f;
        buffer[i] = sinf(phase) * gain * env;
        phase += step;
        if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
    }
    SDL_PutAudioStreamData(audio->stream, buffer, samples * (int)sizeof(float));
}
