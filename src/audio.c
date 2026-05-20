#include "audio.h"
#include "constants.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * Oscillator phase is kept between calls so back-to-back queued tones do not
 * all restart at zero crossing. That makes rapid sort sounds less clicky and
 * gives the little synth a continuous "instrument" feel.
 */
static float g_phase = 0.0f;
static float g_sub_phase = 0.0f;

/**************************************************************************
 * FILE: audio.c
 *
 * Audio helper module for simple synthesized tone playback. This module
 * wraps SDL audio stream setup/teardown and provides short-envelope
 * synthesized tone queueing used by sort/snake feedback and MIDI excerpts.
 **************************************************************************/

/**************************************************************************
 * audio_init
 *
 * Purpose:
 *   Create and start an SDL playback stream for mono float audio.
 *
 * Input:
 *   audio - pointer to audio state to initialize
 *
 * Output:
 *   bool - true when stream is ready, false on failure
 **************************************************************************/
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

/**************************************************************************
 * audio_shutdown
 *
 * Purpose:
 *   Destroy the SDL audio stream and reset readiness state.
 *
 * Input:
 *   audio - pointer to audio state
 *
 * Output:
 *   None
 **************************************************************************/
void audio_shutdown(AudioState *audio) {
    if (audio->stream) {
        SDL_DestroyAudioStream(audio->stream);
        audio->stream = NULL;
    }
    audio->ready = false;
}

/**************************************************************************
 * audio_clear
 *
 * Purpose:
 *   Drop queued audio to stop pending tones immediately.
 *
 * Input:
 *   audio - pointer to audio state
 *
 * Output:
 *   None
 **************************************************************************/
void audio_clear(AudioState *audio) {
    if (audio->stream) {
        SDL_ClearAudioStream(audio->stream);
    }
}

/**************************************************************************
 * audio_enqueue_tone
 *
 * Purpose:
 *   Synthesize and enqueue a short blended waveform tone with an ADSR-style
 *   envelope. A frequency of 0.0 queues silence for rest timing.
 *
 * Input:
 *   audio   - pointer to audio state
 *   freq_hz - tone frequency in Hz
 *   gain    - linear amplitude multiplier
 *
 * Output:
 *   None
 **************************************************************************/
void audio_enqueue_tone(AudioState *audio, float freq_hz, float gain) {
    if (!audio->ready || !audio->stream) return;
    if (SDL_GetAudioStreamQueued(audio->stream) > AUDIO_MAX_QUEUED_BYTES) return;

    int samples = (AUDIO_SAMPLE_RATE * AUDIO_TONE_MS) / 1000;
    float buffer[1024];
    if (samples > (int)(sizeof(buffer) / sizeof(buffer[0]))) {
        samples = (int)(sizeof(buffer) / sizeof(buffer[0]));
    }

    /*
     * Convert frequency into radians advanced per sample. The envelope is
     * attack/decay/sustain/release shaped to avoid hard edges that would pop.
     */
    float step = (2.0f * (float)M_PI * freq_hz) / (float)AUDIO_SAMPLE_RATE;
    int attack = samples / 10;
    int decay = samples / 5;
    int release = samples / 4;
    if (attack < 4) attack = 4;
    if (decay < 6) decay = 6;
    if (release < 8) release = 8;
    if (attack + decay + release >= samples) {
        attack = samples / 6;
        decay = samples / 6;
        release = samples / 5;
    }
    const float sustain_level = 0.74f;

    /*
     * The tone is not a pure sine: sine is clean, triangle adds bite, and a
     * half-frequency sub oscillator gives it some body on small speakers.
     */
    float sub_step = step * 0.5f;
    for (int i = 0; i < samples; i++) {
        float env = sustain_level;
        if (i < attack) {
            env = (float)i / (float)attack;
        } else if (i < attack + decay) {
            float t = (float)(i - attack) / (float)decay;
            env = 1.0f + (sustain_level - 1.0f) * t;
        } else if (i > samples - release) {
            float t = (float)(i - (samples - release)) / (float)release;
            env = sustain_level * (1.0f - t);
        }
        if (env < 0.0f) env = 0.0f;

        float sample = 0.0f;
        if (freq_hz > 0.0f) {
            float sine = sinf(g_phase);
            float tri = (2.0f / (float)M_PI) * asinf(sinf(g_phase));
            float sub = sinf(g_sub_phase);
            sample = (0.70f * sine) + (0.22f * tri) + (0.08f * sub);
        }
        buffer[i] = sample * gain * env;

        if (freq_hz > 0.0f) {
            g_phase += step;
            if (g_phase > 2.0f * (float)M_PI) g_phase -= 2.0f * (float)M_PI;
            g_sub_phase += sub_step;
            if (g_sub_phase > 2.0f * (float)M_PI) g_sub_phase -= 2.0f * (float)M_PI;
        }
    }
    SDL_PutAudioStreamData(audio->stream, buffer, samples * (int)sizeof(float));
}

/**************************************************************************
 * audio_enqueue_tone_duration
 *
 * Purpose:
 *   Synthesize one continuous tone/rest for an exact duration in seconds.
 *   This is used for MIDI playback where note lengths come from event timing
 *   instead of the fixed AUDIO_TONE_MS sort sound chunk size.
 *
 * Input:
 *   audio   - pointer to audio state
 *   freq_hz - tone frequency in Hz; 0.0 creates silence/rest with same timing
 *   gain    - linear amplitude multiplier
 *   seconds - requested duration
 *
 * Output:
 *   None
 **************************************************************************/
static void audio_enqueue_tone_duration(AudioState *audio, float freq_hz, float gain, double seconds) {
    if (!audio->ready || !audio->stream) return;
    if (seconds <= 0.0) return;
    if (SDL_GetAudioStreamQueued(audio->stream) > AUDIO_MAX_QUEUED_BYTES) return;

    int total_samples = (int)(seconds * (double)AUDIO_SAMPLE_RATE + 0.5);
    if (total_samples < 1) total_samples = 1;
    const int chunk_samples = 2048;
    float buffer[chunk_samples];

    /*
     * This mirrors audio_enqueue_tone, but writes in chunks so longer MIDI
     * notes do not need a huge stack buffer.
     */
    float step = (2.0f * (float)M_PI * freq_hz) / (float)AUDIO_SAMPLE_RATE;
    float sub_step = step * 0.5f;
    int attack = total_samples / 12;
    int decay = total_samples / 8;
    int release = total_samples / 6;
    if (attack < 8) attack = 8;
    if (decay < 10) decay = 10;
    if (release < 12) release = 12;
    if (attack + decay + release >= total_samples) {
        attack = total_samples / 6;
        decay = total_samples / 6;
        release = total_samples / 5;
    }
    const float sustain_level = 0.68f;

    int written = 0;
    while (written < total_samples) {
        int n = total_samples - written;
        if (n > chunk_samples) n = chunk_samples;
        for (int i = 0; i < n; i++) {
            int idx = written + i;
            float env = sustain_level;
            if (idx < attack) {
                env = (float)idx / (float)attack;
            } else if (idx < attack + decay) {
                float t = (float)(idx - attack) / (float)decay;
                env = 1.0f + (sustain_level - 1.0f) * t;
            } else if (idx > total_samples - release) {
                float t = (float)(idx - (total_samples - release)) / (float)release;
                env = sustain_level * (1.0f - t);
            }
            if (env < 0.0f) env = 0.0f;

            float sample = 0.0f;
            if (freq_hz > 0.0f) {
                float sine = sinf(g_phase);
                float tri = (2.0f / (float)M_PI) * asinf(sinf(g_phase));
                float sub = sinf(g_sub_phase);
                sample = (0.60f * sine) + (0.28f * tri) + (0.12f * sub);
            }
            buffer[i] = sample * gain * env;

            if (freq_hz > 0.0f) {
                g_phase += step;
                if (g_phase > 2.0f * (float)M_PI) g_phase -= 2.0f * (float)M_PI;
                g_sub_phase += sub_step;
                if (g_sub_phase > 2.0f * (float)M_PI) g_sub_phase -= 2.0f * (float)M_PI;
            }
        }
        SDL_PutAudioStreamData(audio->stream, buffer, n * (int)sizeof(float));
        written += n;
    }
}

/**************************************************************************
 * audio_enqueue_repeated_tone
 *
 * Purpose:
 *   Helper to enqueue the same tone chunk multiple times.
 *
 * Input:
 *   audio   - audio state
 *   freq_hz - frequency in Hz
 *   gain    - amplitude multiplier
 *   repeats - chunk repeat count
 *
 * Output:
 *   None
 **************************************************************************/
static void audio_enqueue_repeated_tone(AudioState *audio, float freq_hz, float gain, int repeats) {
    for (int i = 0; i < repeats; i++) {
        audio_enqueue_tone(audio, freq_hz, gain);
    }
}

/**************************************************************************
 * audio_enqueue_note_beats
 *
 * Purpose:
 *   Enqueue one note for a beat-based duration at a fixed tempo.
 *
 * Input:
 *   audio   - audio state
 *   freq_hz - frequency in Hz
 *   gain    - amplitude multiplier
 *   beats   - length in quarter-note beats
 *   bpm     - tempo
 *
 * Output:
 *   int - number of AUDIO_TONE_MS chunks enqueued
 **************************************************************************/
static int audio_enqueue_note_beats(AudioState *audio, float freq_hz, float gain, float beats, float bpm) {
    float seconds_per_beat = 60.0f / bpm;
    float seconds_per_chunk = (float)AUDIO_TONE_MS / 1000.0f;
    int repeats = (int)((beats * seconds_per_beat) / seconds_per_chunk + 0.5f);
    if (repeats < 1) repeats = 1;
    audio_enqueue_repeated_tone(audio, freq_hz, gain, repeats);
    return repeats;
}

/**************************************************************************
 * audio_enqueue_seconds
 *
 * Purpose:
 *   Enqueue a precise seconds-based tone/rest and return the approximate
 *   number of fixed-size AUDIO_TONE_MS chunks that duration represents.
 *
 * Input:
 *   audio   - audio state
 *   freq_hz - frequency in Hz; 0.0 means rest/silence
 *   gain    - amplitude multiplier
 *   seconds - requested duration
 *
 * Output:
 *   int - approximate fixed chunk count for callers that reason in chunks
 **************************************************************************/
static int audio_enqueue_seconds(AudioState *audio, float freq_hz, float gain, double seconds) {
    if (seconds <= 0.0) return 0;
    const double chunk = (double)AUDIO_TONE_MS / 1000.0;
    int repeats = (int)(seconds / chunk + 0.5);
    if (repeats < 1) repeats = 1;
    audio_enqueue_tone_duration(audio, freq_hz, gain, seconds);
    return repeats;
}

/**************************************************************************
 * read_be_u16
 *
 * Purpose:
 *   Read a big-endian 16-bit integer from MIDI file bytes.
 *
 * Input:
 *   p - pointer to at least two bytes
 *
 * Output:
 *   uint16_t - decoded value
 **************************************************************************/
static uint16_t read_be_u16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

/**************************************************************************
 * read_be_u32
 *
 * Purpose:
 *   Read a big-endian 32-bit integer from MIDI file bytes.
 *
 * Input:
 *   p - pointer to at least four bytes
 *
 * Output:
 *   uint32_t - decoded value
 **************************************************************************/
static uint32_t read_be_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/**************************************************************************
 * read_varlen
 *
 * Purpose:
 *   Read a Standard MIDI variable-length quantity. MIDI delta times and
 *   many MIDI event payload lengths use this 7-bit continuation encoding.
 *
 * Input:
 *   data - byte buffer
 *   size - total buffer size
 *   pos  - in/out cursor within data
 *   out  - receives decoded value
 *
 * Output:
 *   int - 1 on success, 0 on malformed/truncated input
 **************************************************************************/
static int read_varlen(const uint8_t *data, size_t size, size_t *pos, uint32_t *out) {
    uint32_t value = 0;
    int count = 0;
    while (*pos < size && count < 4) {
        uint8_t b = data[(*pos)++];
        value = (value << 7) | (uint32_t)(b & 0x7F);
        count++;
        if ((b & 0x80) == 0) {
            *out = value;
            return 1;
        }
    }
    return 0;
}

/**************************************************************************
 * midi_note_to_hz
 *
 * Purpose:
 *   Convert MIDI note number to equal-tempered frequency using A4=440 Hz.
 *
 * Input:
 *   note - MIDI note number, where 69 is A4
 *
 * Output:
 *   float - frequency in Hz
 **************************************************************************/
static float midi_note_to_hz(uint8_t note) {
    return 440.0f * powf(2.0f, ((float)note - 69.0f) / 12.0f);
}

/**************************************************************************
 * NoteEvent
 *
 * Purpose:
 *   Compact beat-based note event for scripted monophonic melodies.
 **************************************************************************/
typedef struct {
    float freq_hz;
    float beats;
} NoteEvent;

/**************************************************************************
 * audio_play_stalin_anthem
 *
 * Purpose:
 *   Queue a short monophonic anthem motif for Stalin sort completion.
 *
 * Input:
 *   audio         - audio state
 *   sound_enabled - whether motif playback is allowed
 *
 * Output:
 *   Uint32 - approximate motif duration in milliseconds
 **************************************************************************/
Uint32 audio_play_stalin_anthem(AudioState *audio, bool sound_enabled) {
    if (!sound_enabled || !audio->ready || !audio->stream) return 0;
    audio_clear(audio);

    /*
     * Based on the provided opening phrase reference (4/4, quarter=~80).
     * We keep it monophonic and short (~5s) for the celebration window.
     */
    const float bpm = 80.0f;
    const float g5 = 783.99f;
    const float c6 = 1046.50f;
    const float b5 = 987.77f;
    const float a5 = 880.00f;
    const float e5 = 659.25f;
    const float fs5 = 739.99f;

    const NoteEvent phrase[] = {
        {0.0f, 0.5f},    /* pickup rest */
        {g5, 0.5f},
        {c6, 1.0f},
        {g5, 0.75f},
        {a5, 0.25f},
        {b5, 1.0f},
        {e5, 0.5f},
        {e5, 0.5f},

        {a5, 1.0f},
        {g5, 0.75f},
        {fs5, 0.25f},
        {g5, 1.0f},
        {e5, 0.5f},
        {e5, 0.5f},

        // {fs5, 1.0f},
        // {g5, 0.75f},
        // {a5, 0.25f},
        // {b5, 1.0f},
        // {a5, 0.75f},
        // {g5, 0.25f},
        // {a5, 1.0f}
    };

    for (int i = 0; i < (int)(sizeof(phrase) / sizeof(phrase[0])); i++) {
        audio_enqueue_note_beats(audio, phrase[i].freq_hz, 0.11f, phrase[i].beats, bpm);
    }

    /* Use actual queued duration so auto-restart timing matches playback. */
    int queued_bytes = SDL_GetAudioStreamQueued(audio->stream);
    if (queued_bytes <= 0) return 0;
    return (Uint32)((1000.0 * (double)queued_bytes) / ((double)AUDIO_SAMPLE_RATE * sizeof(float)));
}

/**************************************************************************
 * audio_play_midi_excerpt
 *
 * Purpose:
 *   Parse a simple Standard MIDI file and queue up to max_seconds of a
 *   monophonic excerpt using the internal tone synth.
 *
 * Input:
 *   audio         - audio state
 *   midi_path     - path to .mid file
 *   max_seconds   - playback cap in seconds
 *   sound_enabled - whether playback is allowed
 *
 * Output:
 *   Uint32 - approximate queued playback duration in milliseconds
 **************************************************************************/
Uint32 audio_play_midi_excerpt(AudioState *audio, const char *midi_path, double max_seconds, bool sound_enabled) {
    if (!sound_enabled || !audio->ready || !audio->stream || !midi_path || max_seconds <= 0.0) return 0;


    float gain = .22f;
    FILE *fp = fopen(midi_path, "rb");
    if (!fp) return 0;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }
    long sz = ftell(fp);
    if (sz <= 0) {
        fclose(fp);
        return 0;
    }
    rewind(fp);

    uint8_t *bytes = (uint8_t *)malloc((size_t)sz);
    if (!bytes) {
        fclose(fp);
        return 0;
    }
    size_t got = fread(bytes, 1, (size_t)sz, fp);
    fclose(fp);
    if (got != (size_t)sz) {
        free(bytes);
        return 0;
    }

    if ((size_t)sz < 14 || bytes[0] != 'M' || bytes[1] != 'T' || bytes[2] != 'h' || bytes[3] != 'd') {
        free(bytes);
        return 0;
    }

    uint32_t hdr_len = read_be_u32(bytes + 4);
    if (hdr_len < 6 || (size_t)sz < 8 + hdr_len) {
        free(bytes);
        return 0;
    }
    uint16_t format = read_be_u16(bytes + 8);
    uint16_t ntrks = read_be_u16(bytes + 10);
    uint16_t division = read_be_u16(bytes + 12);
    if (ntrks == 0 || division == 0) {
        free(bytes);
        return 0;
    }

    /*
     * SMPTE time division is uncommon for this tiny use case; fall back to a
     * plain 480 PPQ grid instead of rejecting the file outright.
     */
    int ticks_per_quarter = (division & 0x8000) ? 480 : (int)division;
    size_t pos = 8 + hdr_len;
    /*
     * Format 1 MIDI usually stores tempo/control data in track 0 and notes in
     * later tracks. This player is deliberately monophonic, so it picks one
     * likely melody track rather than mixing the whole file.
     */
    uint16_t target_track = 0;
    if (format == 1 && ntrks > 1) target_track = 1;

    const uint8_t *track = NULL;
    size_t track_len = 0;
    for (uint16_t t = 0; t < ntrks; t++) {
        if (pos + 8 > (size_t)sz) break;
        if (bytes[pos] != 'M' || bytes[pos + 1] != 'T' || bytes[pos + 2] != 'r' || bytes[pos + 3] != 'k') break;
        uint32_t len = read_be_u32(bytes + pos + 4);
        pos += 8;
        if (pos + len > (size_t)sz) break;
        if (t == target_track) {
            track = bytes + pos;
            track_len = len;
            break;
        }
        pos += len;
    }

    if (!track || track_len == 0) {
        free(bytes);
        return 0;
    }

    audio_clear(audio);
    uint8_t running_status = 0;
    uint32_t tempo_us_per_qn = 500000;
    double now_s = 0.0;
    double emit_from_s = 0.0;
    int active_note = -1;
    float active_hz = 0.0f;

    size_t p = 0;
    while (p < track_len) {
        uint32_t delta_ticks = 0;
        if (!read_varlen(track, track_len, &p, &delta_ticks)) break;
        double delta_s = ((double)delta_ticks * (double)tempo_us_per_qn) / (1000000.0 * (double)ticks_per_quarter);
        double event_time_s = now_s + delta_s;

        if (event_time_s > max_seconds) {
            audio_enqueue_seconds(audio, active_hz, gain, max_seconds - emit_from_s);
            now_s = max_seconds;
            break;
        }

        /*
         * Emit audio for the time span before this event using whichever note
         * was active. If active_hz is 0.0, this writes silence for a rest.
         */
        if (event_time_s > emit_from_s) {
            audio_enqueue_seconds(audio, active_hz, gain, event_time_s - emit_from_s);
            emit_from_s = event_time_s;
        }
        now_s = event_time_s;

        if (p >= track_len) break;
        uint8_t status = track[p++];
        if (status < 0x80) {
            if (running_status == 0) break;
            p--;
            status = running_status;
        } else if (status >= 0x80 && status <= 0xEF) {
            /* Running status applies only to channel voice messages. */
            running_status = status;
        } else {
            running_status = 0;
        }

        if (status == 0xFF) {
            if (p >= track_len) break;
            uint8_t meta_type = track[p++];
            uint32_t meta_len = 0;
            if (!read_varlen(track, track_len, &p, &meta_len)) break;
            if (p + meta_len > track_len) break;
            if (meta_type == 0x51 && meta_len == 3) {
                tempo_us_per_qn = ((uint32_t)track[p] << 16) | ((uint32_t)track[p + 1] << 8) | track[p + 2];
            } else if (meta_type == 0x2F) {
                p += meta_len;
                break;
            }
            p += meta_len;
            continue;
        }

        if (status == 0xF0 || status == 0xF7) {
            uint32_t syx_len = 0;
            if (!read_varlen(track, track_len, &p, &syx_len)) break;
            if (p + syx_len > track_len) break;
            p += syx_len;
            continue;
        }

        uint8_t event_type = status & 0xF0;
        if (event_type == 0xC0 || event_type == 0xD0) {
            if (p >= track_len) break;
            p += 1;
            continue;
        }
        if (p + 1 >= track_len) break;
        uint8_t d1 = track[p++];
        uint8_t d2 = track[p++];

        /*
         * This intentionally tracks only one note at a time. New note-on wins,
         * note-off only stops playback if it matches the active note.
         */
        if (event_type == 0x90) {
            if (d2 > 0) {
                active_note = (int)d1;
                active_hz = midi_note_to_hz(d1);
            } else if (active_note == (int)d1) {
                active_note = -1;
                active_hz = 0.0f;
            }
        } else if (event_type == 0x80) {
            if (active_note == (int)d1) {
                active_note = -1;
                active_hz = 0.0f;
            }
        }
    }

    if (now_s < max_seconds && emit_from_s < max_seconds) {
        audio_enqueue_seconds(audio, active_hz, gain, max_seconds - emit_from_s);
    }

    int queued_bytes = SDL_GetAudioStreamQueued(audio->stream);
    free(bytes);
    if (queued_bytes <= 0) return 0;
    return (Uint32)((1000.0 * (double)queued_bytes) / ((double)AUDIO_SAMPLE_RATE * sizeof(float)));
}
