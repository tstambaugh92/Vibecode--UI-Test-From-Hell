#ifndef SORT_MODE_H
#define SORT_MODE_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "audio.h"

/**************************************************************************
 * STRUCTS
 **************************************************************************/
typedef enum {
    ALG_BUBBLE = 0,
    ALG_INSERTION = 1,
    ALG_SELECTION = 2,
    ALG_QUICK = 3,
    ALG_MERGE = 4,
    ALG_HEAP = 5
} Algorithm;

typedef enum {
    OP_COMPARE = 0,
    OP_SWAP = 1,
    OP_WRITE = 2
} OpType;

typedef struct {
    OpType type;
    int a;
    int b;
    int value;
} Operation;

typedef struct {
    int values[180];
    Algorithm algorithm;
    bool running;
    bool sorted;
    bool sound_on;
    int highlight_a;
    int highlight_b;
    int ops_index;
    int ops_count;
    Operation ops[500000];
    double step_interval;
    bool celebration_pending;
    bool celebration_active;
    bool auto_restart_pending;
    bool show_help;
    int celebration_index;
    Uint64 auto_restart_at_ms;
    double celebration_accumulator;
    Uint64 run_started_ms;
    Uint64 sort_finished_ms;
    int cycle_count;
    int compare_ops;
    int swap_ops;
    int write_ops;
    int palette_index;
    bool animated_bg_on;
} SortState;

/**************************************************************************
 * PROTOTYPES
 **************************************************************************/
const char *sort_algorithm_name(Algorithm alg);
void sort_init_state(SortState *state);
void sort_set_algorithm(SortState *state, Algorithm alg);
void sort_shuffle_values(SortState *state);
bool sort_build_ops(SortState *state);
void sort_execute_step(SortState *state, AudioState *audio);
void sort_run_celebration_step(SortState *state, AudioState *audio, double dt);
void sort_draw_scene(
    SDL_Renderer *renderer,
    const SortState *state,
    int width,
    int height,
    bool snake_overlay_active,
    float snake_overlay_bottom
);
void sort_update_title(SDL_Window *window, const SortState *state);
void sort_handle_start_pause(SortState *state, AudioState *audio);
void sort_handle_speed_up(SortState *state);
void sort_handle_speed_down(SortState *state);
void sort_set_palette(SortState *state, int palette_index);

#endif
