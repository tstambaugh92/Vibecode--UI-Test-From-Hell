#include "sort_mode.h"
#include "background.h"
#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************************
 * FILE: sort_mode.c
 *
 * Sorting visualizer core. This module owns algorithm op generation,
 * per-step execution, celebration/autorestart logic, HUD/help rendering,
 * and sort-scene drawing behavior.
 **************************************************************************/

/**************************************************************************
 * sort_algorithm_name
 *
 * Purpose:
 *   Convert algorithm enum to user-facing short name.
 *
 * Input:
 *   alg - algorithm enum value
 *
 * Output:
 *   const char* - static name string
 **************************************************************************/
const char *sort_algorithm_name(Algorithm alg) {
    switch (alg) {
        case ALG_BUBBLE: return "Bubble";
        case ALG_INSERTION: return "Insertion";
        case ALG_SELECTION: return "Selection";
        case ALG_QUICK: return "Quick";
        case ALG_MERGE: return "Merge";
        case ALG_HEAP: return "Heap";
        case ALG_STALIN: return "Stalin";
        default: return "Unknown";
    }
}

typedef struct {
    Uint8 normal_r, normal_g, normal_b;
    Uint8 active_r, active_g, active_b;
    Uint8 sorted_r, sorted_g, sorted_b;
    Uint8 celeb_r, celeb_g, celeb_b;
} Palette;

static const Palette PALETTES[9] = {
    /* Palette 1: Rubik's-inspired */
    {0,155,72, 196,30,58, 0,70,173, 255,120,120},
    {135,215,255, 255,170,70, 140,235,155, 255,120,90},
    {255,160,120, 255,230,120, 150,220,170, 255,90,160},
    {120,235,190, 255,205,95, 145,245,170, 240,110,255},
    {215,170,255, 255,220,110, 155,225,165, 255,120,120},
    {255,210,130, 255,140,90, 175,235,145, 210,100,255},
    {120,180,255, 255,220,120, 125,230,175, 255,110,170},
    {160,245,150, 255,190,110, 140,245,160, 255,110,90},
    {220,190,255, 255,215,125, 160,230,165, 255,95,235}
};

/**************************************************************************
 * emit_op
 *
 * Purpose:
 *   Append one abstract operation to the precomputed operation stream.
 *
 * Input:
 *   state       - sort state
 *   type,a,b    - operation metadata
 *   value       - payload for write operations
 *
 * Output:
 *   bool - false if MAX_OPS capacity has been reached
 **************************************************************************/
static bool emit_op(SortState *state, OpType type, int a, int b, int value) {
    if (state->ops_count >= MAX_OPS) return false;
    state->ops[state->ops_count++] = (Operation){type, a, b, value};
    return true;
}

/**************************************************************************
 * sort_init_state
 *
 * Purpose:
 *   Initialize runtime sort state defaults.
 *
 * Input:
 *   state - sort state to initialize
 *
 * Output:
 *   None
 **************************************************************************/
void sort_init_state(SortState *state) {
    memset(state, 0, sizeof(*state));
    state->sound_on = true;
    state->step_interval = 0.0018;
    state->highlight_a = -1;
    state->highlight_b = -1;
    state->palette_index = 0;
    state->animated_bg_on = false;
    state->run_started_ms = SDL_GetTicks();
}

/**************************************************************************
 * sort_shuffle_values
 *
 * Purpose:
 *   Fill bars with ascending values, then Fisher-Yates shuffle and reset
 *   all execution/celebration cursors.
 *
 * Input:
 *   state - sort state to mutate
 *
 * Output:
 *   None
 **************************************************************************/
void sort_shuffle_values(SortState *state) {
    for (int i = 0; i < BAR_COUNT; i++) state->values[i] = i + 1;
    for (int i = BAR_COUNT - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = state->values[i];
        state->values[i] = state->values[j];
        state->values[j] = t;
    }
    state->running = false;
    state->sorted = false;
    state->highlight_a = -1;
    state->highlight_b = -1;
    state->ops_index = 0;
    state->ops_count = 0;
    state->celebration_pending = false;
    state->celebration_active = false;
    state->auto_restart_pending = false;
    state->celebration_index = 0;
    state->auto_restart_at_ms = 0;
    state->celebration_accumulator = 0.0;
}

/**************************************************************************
 * sort_set_algorithm
 *
 * Purpose:
 *   Switch active algorithm and immediately reshuffle dataset.
 *
 * Input:
 *   state - sort state
 *   alg   - new algorithm selection
 *
 * Output:
 *   None
 **************************************************************************/
void sort_set_algorithm(SortState *state, Algorithm alg) {
    state->algorithm = alg;
    sort_shuffle_values(state);
}

/**************************************************************************
 * sort_set_palette
 *
 * Purpose:
 *   Set active palette index with range clamping.
 *
 * Input:
 *   state         - sort state
 *   palette_index - requested palette slot
 *
 * Output:
 *   None
 **************************************************************************/
void sort_set_palette(SortState *state, int palette_index) {
    if (palette_index < 0) palette_index = 0;
    if (palette_index > 8) palette_index = 8;
    state->palette_index = palette_index;
}

/**************************************************************************
 * build_bubble_ops / build_insertion_ops / build_selection_ops
 * build_quick_ops* / build_merge_ops* / build_heap_ops*
 *
 * Purpose:
 *   Simulate each sorting algorithm against a local array copy while
 *   recording abstract OP_COMPARE / OP_SWAP / OP_WRITE instructions.
 *
 * Input:
 *   state - sort state for op output
 *   arr   - mutable local working array
 *
 * Output:
 *   bool - false when op buffer capacity is exhausted
 **************************************************************************/
static bool build_bubble_ops(SortState *state, int arr[]) {
    for (int i = 0; i < BAR_COUNT; i++) {
        bool swapped = false;
        for (int j = 0; j < BAR_COUNT - i - 1; j++) {
            if (!emit_op(state, OP_COMPARE, j, j + 1, 0)) return false;
            if (arr[j] > arr[j + 1]) {
                int t = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = t;
                swapped = true;
                if (!emit_op(state, OP_SWAP, j, j + 1, 0)) return false;
            }
        }
        if (!swapped) break;
    }
    return true;
}

static bool build_insertion_ops(SortState *state, int arr[]) {
    for (int i = 1; i < BAR_COUNT; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0) {
            if (!emit_op(state, OP_COMPARE, j, i, 0)) return false;
            if (arr[j] <= key) break;
            arr[j + 1] = arr[j];
            if (!emit_op(state, OP_WRITE, j + 1, i, arr[j + 1])) return false;
            j--;
        }
        arr[j + 1] = key;
        if (!emit_op(state, OP_WRITE, j + 1, i, key)) return false;
    }
    return true;
}

static bool build_selection_ops(SortState *state, int arr[]) {
    for (int i = 0; i < BAR_COUNT - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < BAR_COUNT; j++) {
            if (!emit_op(state, OP_COMPARE, min_idx, j, 0)) return false;
            if (arr[j] < arr[min_idx]) min_idx = j;
        }
        if (min_idx != i) {
            int t = arr[i];
            arr[i] = arr[min_idx];
            arr[min_idx] = t;
            if (!emit_op(state, OP_SWAP, i, min_idx, 0)) return false;
        }
    }
    return true;
}

static bool quick_partition(SortState *state, int arr[], int low, int high, int *pivot_out) {
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (!emit_op(state, OP_COMPARE, j, high, 0)) return false;
        if (arr[j] <= pivot) {
            i++;
            int t = arr[i];
            arr[i] = arr[j];
            arr[j] = t;
            if (!emit_op(state, OP_SWAP, i, j, 0)) return false;
        }
    }
    int t = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = t;
    if (!emit_op(state, OP_SWAP, i + 1, high, 0)) return false;
    *pivot_out = i + 1;
    return true;
}

static bool build_quick_ops_rec(SortState *state, int arr[], int low, int high) {
    if (low >= high) return true;
    int p = low;
    if (!quick_partition(state, arr, low, high, &p)) return false;
    if (!build_quick_ops_rec(state, arr, low, p - 1)) return false;
    if (!build_quick_ops_rec(state, arr, p + 1, high)) return false;
    return true;
}

static bool build_quick_ops(SortState *state, int arr[]) {
    return build_quick_ops_rec(state, arr, 0, BAR_COUNT - 1);
}

static bool build_merge_ops_rec(SortState *state, int arr[], int tmp[], int left, int right) {
    if (left >= right) return true;
    int mid = (left + right) / 2;
    if (!build_merge_ops_rec(state, arr, tmp, left, mid)) return false;
    if (!build_merge_ops_rec(state, arr, tmp, mid + 1, right)) return false;

    int i = left;
    int j = mid + 1;
    int k = left;
    while (i <= mid && j <= right) {
        if (!emit_op(state, OP_COMPARE, i, j, 0)) return false;
        tmp[k++] = (arr[i] <= arr[j]) ? arr[i++] : arr[j++];
    }
    while (i <= mid) tmp[k++] = arr[i++];
    while (j <= right) tmp[k++] = arr[j++];
    for (int p = left; p <= right; p++) {
        arr[p] = tmp[p];
        if (!emit_op(state, OP_WRITE, p, right, arr[p])) return false;
    }
    return true;
}

static bool build_merge_ops(SortState *state, int arr[]) {
    int tmp[BAR_COUNT];
    return build_merge_ops_rec(state, arr, tmp, 0, BAR_COUNT - 1);
}

static bool heap_sift_down(SortState *state, int arr[], int n, int root) {
    int r = root;
    while (true) {
        int largest = r;
        int l = 2 * r + 1;
        int m = 2 * r + 2;
        if (l < n) {
            if (!emit_op(state, OP_COMPARE, largest, l, 0)) return false;
            if (arr[l] > arr[largest]) largest = l;
        }
        if (m < n) {
            if (!emit_op(state, OP_COMPARE, largest, m, 0)) return false;
            if (arr[m] > arr[largest]) largest = m;
        }
        if (largest == r) break;
        int t = arr[r];
        arr[r] = arr[largest];
        arr[largest] = t;
        if (!emit_op(state, OP_SWAP, r, largest, 0)) return false;
        r = largest;
    }
    return true;
}

static bool build_heap_ops(SortState *state, int arr[]) {
    for (int i = BAR_COUNT / 2 - 1; i >= 0; i--) {
        if (!heap_sift_down(state, arr, BAR_COUNT, i)) return false;
    }
    for (int end = BAR_COUNT - 1; end > 0; end--) {
        int t = arr[0];
        arr[0] = arr[end];
        arr[end] = t;
        if (!emit_op(state, OP_SWAP, 0, end, 0)) return false;
        if (!heap_sift_down(state, arr, end, 0)) return false;
    }
    return true;
}

static bool build_stalin_ops(SortState *state, int arr[]) {
    int leader_idx = 0;
    for (int i = 1; i < BAR_COUNT; i++) {
        if (!emit_op(state, OP_COMPARE, leader_idx, i, 0)) return false;
        if (arr[i] >= arr[leader_idx]) {
            leader_idx = i;
        } else {
            arr[i] = 0;
            if (!emit_op(state, OP_WRITE, i, leader_idx, 0)) return false;
        }
    }
    return true;
}

/**************************************************************************
 * sort_build_ops
 *
 * Purpose:
 *   Rebuild the operation stream for the currently selected algorithm.
 *
 * Input:
 *   state - sort state with current values/algorithm
 *
 * Output:
 *   bool - true on success, false if op buffer overflow occurs
 **************************************************************************/
bool sort_build_ops(SortState *state) {
    int arr[BAR_COUNT];
    memcpy(arr, state->values, sizeof(arr));
    state->ops_count = 0;
    state->ops_index = 0;
    state->compare_ops = 0;
    state->swap_ops = 0;
    state->write_ops = 0;

    bool ok = false;
    switch (state->algorithm) {
        case ALG_BUBBLE: ok = build_bubble_ops(state, arr); break;
        case ALG_INSERTION: ok = build_insertion_ops(state, arr); break;
        case ALG_SELECTION: ok = build_selection_ops(state, arr); break;
        case ALG_QUICK: ok = build_quick_ops(state, arr); break;
        case ALG_MERGE: ok = build_merge_ops(state, arr); break;
        case ALG_HEAP: ok = build_heap_ops(state, arr); break;
        case ALG_STALIN: ok = build_stalin_ops(state, arr); break;
        default: break;
    }
    if (!ok) return false;
    /* Derive aggregate op counters for HUD stats. */
    for (int i = 0; i < state->ops_count; i++) {
        if (state->ops[i].type == OP_COMPARE) state->compare_ops++;
        else if (state->ops[i].type == OP_SWAP) state->swap_ops++;
        else if (state->ops[i].type == OP_WRITE) state->write_ops++;
    }
    return true;
}

/**************************************************************************
 * play_event_sound / play_success_chime
 *
 * Purpose:
 *   Emit lightweight sound cues for active operations and end-of-sort.
 *
 * Input:
 *   audio - audio subsystem
 *   state - sort state (sound enabled/disabled + value context)
 *
 * Output:
 *   None
 **************************************************************************/
static void play_event_sound(AudioState *audio, const SortState *state, OpType type, int value) {
    if (!state->sound_on) return;
    float pct = (float)value / (float)BAR_COUNT;
    float base = 280.0f + 640.0f * pct;
    if (type == OP_SWAP) audio_enqueue_tone(audio, base + 70.0f, 0.13f);
    else if (type == OP_WRITE) audio_enqueue_tone(audio, base + 120.0f, 0.10f);
}

static void play_success_chime(AudioState *audio, const SortState *state) {
    if (!state->sound_on) return;
    audio_clear(audio);
    audio_enqueue_tone(audio, 523.25f, 0.12f);
    audio_enqueue_tone(audio, 659.25f, 0.12f);
    audio_enqueue_tone(audio, 783.99f, 0.14f);
    audio_enqueue_tone(audio, 1046.50f, 0.16f);
}

/**************************************************************************
 * sort_execute_step
 *
 * Purpose:
 *   Execute one precomputed operation from the operation stream.
 *
 * Input:
 *   state - sort state
 *   audio - audio subsystem
 *
 * Output:
 *   None
 **************************************************************************/
void sort_execute_step(SortState *state, AudioState *audio) {
    if (!state->running || state->sorted) return;
    if (state->ops_index >= state->ops_count) {
        state->running = false;
        state->sorted = true;
        state->sort_finished_ms = SDL_GetTicks();
        state->cycle_count++;
        state->highlight_a = -1;
        state->highlight_b = -1;
        state->celebration_pending = false;
        state->celebration_active = false;
        state->auto_restart_pending = false;
        state->celebration_index = 0;
        state->auto_restart_at_ms = 0;
        state->celebration_accumulator = 0.0;
        if (state->algorithm == ALG_STALIN) {
            Uint32 motif_ms = audio_play_midi_excerpt(audio, "audio/ussranthem.mid", 9.9, state->sound_on);
            if (motif_ms == 0) {
                motif_ms = audio_play_stalin_anthem(audio, state->sound_on);
            }
            state->auto_restart_pending = true;
            state->auto_restart_at_ms = SDL_GetTicks() + motif_ms;
        } else {
            audio_clear(audio);
            state->celebration_pending = true;
        }
        return;
    }

    Operation op = state->ops[state->ops_index++];
    state->highlight_a = op.a;
    state->highlight_b = op.b;
    if (op.type == OP_COMPARE) return;

    if (op.type == OP_SWAP && op.a >= 0 && op.a < BAR_COUNT && op.b >= 0 && op.b < BAR_COUNT) {
        int t = state->values[op.a];
        state->values[op.a] = state->values[op.b];
        state->values[op.b] = t;
        play_event_sound(audio, state, OP_SWAP, state->values[op.a]);
        return;
    }
    if (op.type == OP_WRITE && op.a >= 0 && op.a < BAR_COUNT) {
        state->values[op.a] = op.value;
        play_event_sound(audio, state, OP_WRITE, op.value);
    }
}

/**************************************************************************
 * sort_run_celebration_step
 *
 * Purpose:
 *   Run post-sort "victory lap" compare pass and schedule auto-restart.
 *
 * Input:
 *   state - sort state
 *   audio - audio subsystem
 *   dt    - frame delta time in seconds
 *
 * Output:
 *   None
 **************************************************************************/
void sort_run_celebration_step(SortState *state, AudioState *audio, double dt) {
    if (!state->sorted || !state->celebration_active) return;
    state->celebration_accumulator += dt;
    const double verify_interval = 0.010;
    /* Fixed-step cadence keeps end-of-sort sound tempo stable. */
    while (state->celebration_accumulator >= verify_interval) {
        state->celebration_accumulator -= verify_interval;
        if (state->celebration_index >= BAR_COUNT - 1) {
            state->celebration_active = false;
            state->highlight_a = -1;
            state->highlight_b = -1;
            play_success_chime(audio, state);
            state->auto_restart_pending = true;
            state->auto_restart_at_ms = SDL_GetTicks() + 500;
            return;
        }
        int a = state->celebration_index;
        int b = a + 1;
        state->highlight_a = a;
        state->highlight_b = b;
        int vb = state->values[b];
        if (state->sound_on) {
            float pct = (float)vb / (float)BAR_COUNT;
            audio_enqueue_tone(audio, 500.0f + 420.0f * pct, 0.08f);
        }
        state->celebration_index++;
    }
}

/**************************************************************************
 * draw_hud
 *
 * Purpose:
 *   Draw top HUD stats and optional controls/help panel.
 *
 * Input:
 *   renderer             - SDL renderer
 *   state                - sort state
 *   width,height         - viewport dimensions
 *   snake_overlay_active - whether snake PiP is visible
 *   snake_overlay_bottom - Y edge where help panel must avoid overlap
 *
 * Output:
 *   None
 **************************************************************************/
static void draw_hud(
    SDL_Renderer *renderer,
    const SortState *state,
    int width,
    int height,
    bool snake_overlay_active,
    float snake_overlay_bottom
) {
    /* Scale text and panel geometry from a stable reference viewport. */
    float sx = (float)width / 1200.0f;
    float sy = (float)height / 720.0f;
    float ui_scale = sx < sy ? sx : sy;
    if (ui_scale < UI_SCALE_MIN) ui_scale = UI_SCALE_MIN;
    if (ui_scale > UI_SCALE_MAX) ui_scale = UI_SCALE_MAX;

    float text_scale = ui_scale * HUD_TEXT_SCALE_FACTOR;
    if (text_scale < HUD_TEXT_SCALE_MIN) text_scale = HUD_TEXT_SCALE_MIN;
    if (text_scale > HUD_TEXT_SCALE_MAX) text_scale = HUD_TEXT_SCALE_MAX;

    Uint64 now_ms = SDL_GetTicks();
    double elapsed = (double)(now_ms - state->run_started_ms) / 1000.0;

    float text_x = HUD_TEXT_X * ui_scale;
    float line1_y = HUD_LINE1_Y * ui_scale;
    float line2_y = HUD_LINE2_Y * ui_scale;
    float line3_y = HUD_LINE3_Y * ui_scale;

    SDL_FRect hud_bg = {
        (HUD_BG_X * ui_scale),
        (HUD_BG_Y * ui_scale),
        (HUD_BG_W * ui_scale),
        (HUD_BG_H * ui_scale)
    };
    SDL_SetRenderDrawColor(renderer, 8, 12, 20, 150);
    SDL_RenderFillRect(renderer, &hud_bg);
    SDL_SetRenderDrawColor(renderer, 80, 110, 150, 110);
    SDL_RenderRect(renderer, &hud_bg);

    SDL_SetRenderDrawColor(renderer, 230, 235, 245, 255);
    SDL_SetRenderScale(renderer, text_scale, text_scale);
    SDL_RenderDebugTextFormat(
        renderer, text_x / text_scale, line1_y / text_scale,
        "Alg:%s  Cycle:%d  Ops:%d/%d  Palette:%d",
        sort_algorithm_name(state->algorithm),
        state->cycle_count,
        state->ops_index,
        state->ops_count,
        state->palette_index + 1
    );
    SDL_RenderDebugTextFormat(
        renderer, text_x / text_scale, line2_y / text_scale,
        "C:%d  S:%d  W:%d  Speed:%.4fs  Sound:%s",
        state->compare_ops,
        state->swap_ops,
        state->write_ops,
        state->step_interval,
        state->sound_on ? "on" : "off"
    );
    SDL_RenderDebugTextFormat(
        renderer, text_x / text_scale, line3_y / text_scale,
        "Elapsed: %.2fs  State:%s%s  (H for help)",
        elapsed,
        state->running ? "running" : (state->celebration_active ? "victory-lap" : "idle"),
        state->sorted ? " sorted" : ""
    );
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    if (!state->show_help) return;

    const char *help_lines[] = {
        "Controls",
        "1..7 algorithm  |  S start/pause  |  R reshuffle",
        "F/F11 fullscreen | +/- speed | M mute | B animated bg | H help",
        "Numpad 1-9 palette | Esc quit",
        "Secret code: S N A K E"
    };
    const int help_line_count = (int)(sizeof(help_lines) / sizeof(help_lines[0]));

    float line_step = HELP_PANEL_LINE_SPACING * ui_scale;
    float panel_h = (HELP_PANEL_PAD_TOP * ui_scale)
        + ((float)(help_line_count - 1) * line_step)
        + (8.0f * ui_scale)
        + (HELP_PANEL_PAD_BOTTOM * ui_scale);

    float panel_y = HELP_PANEL_DEFAULT_Y * ui_scale;
    float min_from_hud = (HUD_BG_Y + HUD_BG_H + HELP_PANEL_GAP_FROM_HUD) * ui_scale;
    if (panel_y < min_from_hud) panel_y = min_from_hud;
    if (snake_overlay_active) {
        float min_y = snake_overlay_bottom + (HELP_PANEL_GAP_FROM_SNAKE * ui_scale);
        if (panel_y < min_y) panel_y = min_y;
    }
    if (panel_y + panel_h > (float)height - (HELP_PANEL_MARGIN_SCREEN * ui_scale)) {
        panel_y = (float)height - panel_h - (HELP_PANEL_MARGIN_SCREEN * ui_scale);
    }
    if (panel_y < HELP_PANEL_MARGIN_SCREEN * ui_scale) panel_y = HELP_PANEL_MARGIN_SCREEN * ui_scale;

    /* Compute width from the longest line so future lines auto-fit. */
    size_t longest_len = 0;
    for (int i = 0; i < help_line_count; i++) {
        size_t n = strlen(help_lines[i]);
        if (n > longest_len) longest_len = n;
    }

    float panel_x = HELP_PANEL_X * ui_scale;
    float inner_pad_l = HELP_PANEL_PAD_LEFT * ui_scale;
    float inner_pad_r = HELP_PANEL_PAD_RIGHT * ui_scale;
    float glyph_w = 8.0f * text_scale;
    float needed_text_w = ((float)longest_len * glyph_w);
    float panel_w = inner_pad_l + needed_text_w + inner_pad_r;
    float max_panel_w = (float)width - panel_x - (HELP_PANEL_X * ui_scale);
    if (panel_w < (float)width * HELP_PANEL_MIN_WIDTH_FACTOR) panel_w = (float)width * HELP_PANEL_MIN_WIDTH_FACTOR;
    if (panel_w > max_panel_w) panel_w = max_panel_w;

    SDL_FRect panel = {panel_x, panel_y, panel_w, panel_h};
    SDL_SetRenderDrawColor(renderer, 18, 26, 40, 225);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 110, 140, 190, 255);
    SDL_RenderRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 235, 240, 250, 255);
    SDL_SetRenderScale(renderer, text_scale, text_scale);
    float ty = panel_y + (HELP_PANEL_PAD_TOP * ui_scale);
    float panel_text_x = (panel_x + inner_pad_l) / text_scale;
    for (int i = 0; i < help_line_count; i++) {
        float y = ty + ((float)i * line_step);
        SDL_RenderDebugText(renderer, panel_text_x, y / text_scale, help_lines[i]);
    }
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

/**************************************************************************
 * sort_draw_scene
 *
 * Purpose:
 *   Draw background, bars, highlights, and HUD for sort mode.
 *
 * Input:
 *   renderer             - SDL renderer
 *   state                - sort state
 *   width,height         - viewport dimensions
 *   snake_overlay_active - whether snake PiP is visible
 *   snake_overlay_bottom - Y edge where help panel must avoid overlap
 *
 * Output:
 *   None
 **************************************************************************/
void sort_draw_scene(
    SDL_Renderer *renderer,
    const SortState *state,
    int width,
    int height,
    bool snake_overlay_active,
    float snake_overlay_bottom
) {
    const Palette *palette = &PALETTES[state->palette_index % 9];
    if (state->animated_bg_on) {
        BackgroundPalette bg_palette = {
            palette->normal_r, palette->normal_g, palette->normal_b,
            palette->active_r, palette->active_g, palette->active_b
        };
        background_draw_animated(renderer, width, height, state->palette_index % 9, &bg_palette);
    } else {
        SDL_SetRenderDrawColor(renderer, 9, 12, 22, 255);
        SDL_RenderClear(renderer);
    }

    float content_w = (float)width - (SIDE_MARGIN * 2.0f);
    if (content_w < 1.0f) content_w = 1.0f;
    float bar_w = content_w / (float)BAR_COUNT;
    if (bar_w < 1.0f) bar_w = 1.0f;
    int max_h = height - 40;
    if (max_h < 20) max_h = 20;

    for (int i = 0; i < BAR_COUNT; i++) {
        int value = state->values[i];
        int h = (value * max_h) / BAR_COUNT;
        float x = SIDE_MARGIN + ((float)i * bar_w);
        int y = height - h;
        Uint8 r = palette->normal_r, g = palette->normal_g, b = palette->normal_b;
        if (state->celebration_active && (i == state->highlight_a || i == state->highlight_b)) {
            r = palette->celeb_r; g = palette->celeb_g; b = palette->celeb_b;
        } else if (i == state->highlight_a || i == state->highlight_b) {
            r = palette->active_r; g = palette->active_g; b = palette->active_b;
        }
        if (state->sorted && !state->celebration_active) {
            r = palette->sorted_r; g = palette->sorted_g; b = palette->sorted_b;
        }
        float rect_w = bar_w - BAR_GAP;
        if (rect_w < 1.0f) rect_w = 1.0f;
        SDL_FRect rect = {x, (float)y, rect_w, (float)h};
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    draw_hud(renderer, state, width, height, snake_overlay_active, snake_overlay_bottom);
}

/**************************************************************************
 * sort_update_title
 *
 * Purpose:
 *   Update OS window title with current run status.
 *
 * Input:
 *   window - SDL window handle
 *   state  - sort state
 *
 * Output:
 *   None
 **************************************************************************/
void sort_update_title(SDL_Window *window, const SortState *state) {
    char title[512];
    snprintf(
        title,
        sizeof(title),
        "Vibecode: UI Test From Hell | 1..7 Algorithm | S Start/Pause | R Shuffle | M Mute | +/- Speed | Alg=%s | Ops %d/%d | Sound=%s | Speed=%.4fs",
        sort_algorithm_name(state->algorithm),
        state->ops_index,
        state->ops_count,
        state->sound_on ? "on" : "off",
        state->step_interval
    );
    SDL_SetWindowTitle(window, title);
}

/**************************************************************************
 * sort_handle_start_pause
 *
 * Purpose:
 *   Toggle run state, with sorted-run reset behavior.
 *
 * Input:
 *   state - sort state
 *   audio - audio subsystem
 *
 * Output:
 *   None
 **************************************************************************/
void sort_handle_start_pause(SortState *state, AudioState *audio) {
    if (state->sorted) {
        sort_shuffle_values(state);
        sort_build_ops(state);
    }
    state->running = !state->running;
    if (state->running && state->ops_index == 0) {
        state->run_started_ms = SDL_GetTicks();
    }
    state->celebration_pending = false;
    state->celebration_active = false;
    state->auto_restart_pending = false;
    state->celebration_index = 0;
    if (!state->running) audio_clear(audio);
}

/**************************************************************************
 * sort_handle_speed_up
 *
 * Purpose:
 *   Increase simulation speed by reducing per-step interval.
 *
 * Input:
 *   state - sort state
 *
 * Output:
 *   None
 **************************************************************************/
void sort_handle_speed_up(SortState *state) {
    state->step_interval -= 0.00025;
    if (state->step_interval < 0.0001) state->step_interval = 0.0001;
}

/**************************************************************************
 * sort_handle_speed_down
 *
 * Purpose:
 *   Decrease simulation speed by increasing per-step interval.
 *
 * Input:
 *   state - sort state
 *
 * Output:
 *   None
 **************************************************************************/
void sort_handle_speed_down(SortState *state) {
    state->step_interval += 0.00025;
    if (state->step_interval > 0.02) state->step_interval = 0.02;
}
