#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "audio.h"
#include "constants.h"
#include "snake_mode.h"
#include "sort_mode.h"

/**************************************************************************
 * FILE: main.c
 *
 * This is the main runtime controller for the application. It initializes
 * SDL systems, owns the app mode state machine (sort mode, snake mode,
 * and snake PiP overlay), routes keyboard input, and runs the per-frame
 * update/render loop.
 **************************************************************************/

typedef enum {
    MODE_SORT = 0,
    MODE_SNAKE = 1
} AppMode;

typedef struct {
    SortState *sort;
    SnakeState *snake;
    AudioState *audio;
    AppMode *mode;
    bool *snake_overlay;
    bool *is_fullscreen;
    SDL_Window *window;
    int snake_cols;
    int snake_rows;
    const char *cheat;
    int *cheat_index;
} InputContext;

/**************************************************************************
 * initialize_sdl_systems
 *
 * Purpose:
 *   Initialize SDL core, create the main window/renderer, and initialize
 *   the audio stream.
 *
 * Input:
 *   window_out   - receives created SDL_Window*
 *   renderer_out - receives created SDL_Renderer*
 *   audio_out    - receives initialized audio state
 *
 * Output:
 *   bool - true on successful video setup (audio may still be disabled)
 **************************************************************************/
static bool initialize_sdl_systems(
    SDL_Window **window_out,
    SDL_Renderer **renderer_out,
    AudioState *audio_out
) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Vibecode: UI Test From Hell",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    *audio_out = (AudioState){0};
    if (!audio_init(audio_out)) {
        fprintf(stderr, "Audio disabled: %s\n", SDL_GetError());
    }

    *window_out = window;
    *renderer_out = renderer;
    return true;
}

/**************************************************************************
 * shutdown_sdl_systems
 *
 * Purpose:
 *   Tear down audio and SDL rendering/window resources.
 *
 * Input:
 *   window   - SDL window
 *   renderer - SDL renderer
 *   audio    - audio state
 *
 * Output:
 *   None
 **************************************************************************/
static void shutdown_sdl_systems(SDL_Window *window, SDL_Renderer *renderer, AudioState *audio) {
    audio_shutdown(audio);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

/**************************************************************************
 * handle_sort_key
 *
 * Purpose:
 *   Handle all sort-mode hotkeys and perform immediate state updates.
 *
 * Input:
 *   key           - SDL key code for this key press
 *   sort          - active sort state
 *   audio         - audio engine state
 *   is_fullscreen - pointer to fullscreen toggle flag
 *   window        - SDL window handle
 *
 * Output:
 *   None (state is modified in-place through pointers).
 **************************************************************************/
static void handle_sort_key(
    SDL_Keycode key,
    SortState *sort,
    AudioState *audio,
    bool *is_fullscreen,
    SDL_Window *window
) {
    bool should_keep_running = sort->running || sort->auto_restart_pending;

    switch (key) {
        case SDLK_F:
        case SDLK_F11:
            *is_fullscreen = !(*is_fullscreen);
            SDL_SetWindowFullscreen(window, *is_fullscreen);
            break;
        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
        case SDLK_8:
            switch (key)
            {
                case SDLK_1: sort_set_algorithm(sort,ALG_BUBBLE); break;
                case SDLK_2: sort_set_algorithm(sort, ALG_INSERTION); break;
                case SDLK_3: sort_set_algorithm(sort, ALG_SELECTION); break;
                case SDLK_4: sort_set_algorithm(sort, ALG_QUICK); break;
                case SDLK_5: sort_set_algorithm(sort, ALG_MERGE); break;
                case SDLK_6: sort_set_algorithm(sort, ALG_HEAP); break;
                case SDLK_7: sort_set_algorithm(sort, ALG_STALIN); break;
                case SDLK_8: sort_set_algorithm(sort, ALG_SLOT8); break;
            }
            sort_build_ops(sort);
            audio_clear(audio);
            if (should_keep_running) {
                sort_handle_start_pause(sort, audio);
            }
            break;
        case SDLK_R:
            sort_shuffle_values(sort);
            sort_build_ops(sort);
            audio_clear(audio);
            break;
        case SDLK_H:
            sort->show_help = !sort->show_help;
            break;
        case SDLK_B:
            sort->animated_bg_on = !sort->animated_bg_on;
            break;
        case SDLK_KP_1:
            sort_set_palette(sort, 0);
            break;
        case SDLK_KP_2:
            sort_set_palette(sort, 1);
            break;
        case SDLK_KP_3:
            sort_set_palette(sort, 2);
            break;
        case SDLK_KP_4:
            sort_set_palette(sort, 3);
            break;
        case SDLK_KP_5:
            sort_set_palette(sort, 4);
            break;
        case SDLK_KP_6:
            sort_set_palette(sort, 5);
            break;
        case SDLK_KP_7:
            sort_set_palette(sort, 6);
            break;
        case SDLK_KP_8:
            sort_set_palette(sort, 7);
            break;
        case SDLK_KP_9:
            sort_set_palette(sort, 8);
            break;
        case SDLK_M:
            sort->sound_on = !sort->sound_on;
            if (!sort->sound_on) audio_clear(audio);
            break;
        case SDLK_EQUALS:
        case SDLK_PLUS:
        case SDLK_KP_PLUS:
            sort_handle_speed_up(sort);
            break;
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            sort_handle_speed_down(sort);
            break;
        case SDLK_S:
            sort_handle_start_pause(sort, audio);
            break;
        default:
            break;
    }

}

/**************************************************************************
 * handle_snake_key
 *
 * Purpose:
 *   Handle snake input in either full-screen mode or PiP overlay mode.
 *
 * Input:
 *   key - key pressed
 *   ctx - shared input context
 *
 * Output:
 *   bool - true if snake handled the key
 **************************************************************************/
static bool handle_snake_key(SDL_Keycode key, InputContext *ctx) {
    bool full_snake = (*ctx->mode == MODE_SNAKE);
    bool overlay_snake = (*ctx->mode == MODE_SORT && *ctx->snake_overlay);

    if (!full_snake && !overlay_snake) return false;

    switch (key) {
        case SDLK_TAB:
            if (full_snake) {
                *ctx->mode = MODE_SORT;
                *ctx->snake_overlay = true;
                ctx->sort->running = true;
            } else {
                *ctx->mode = MODE_SNAKE;
                *ctx->snake_overlay = false;
                audio_clear(ctx->audio);
            }
            sort_update_title(ctx->window, ctx->sort);
            return true;
        case SDLK_X:
            *ctx->mode = MODE_SORT;
            *ctx->snake_overlay = false;
            ctx->snake->started = false;
            ctx->sort->running = true;
            sort_update_title(ctx->window, ctx->sort);
            return true;
        case SDLK_SPACE:
            snake_reset(ctx->snake, ctx->snake_cols, ctx->snake_rows);
            return true;
        case SDLK_UP:
            snake_set_direction(ctx->snake, 0, -1);
            return true;
        case SDLK_DOWN:
            snake_set_direction(ctx->snake, 0, 1);
            return true;
        case SDLK_LEFT:
            snake_set_direction(ctx->snake, -1, 0);
            return true;
        case SDLK_RIGHT:
            snake_set_direction(ctx->snake, 1, 0);
            return true;
        case SDLK_W:
            if (!full_snake) return false;
            snake_set_direction(ctx->snake, 0, -1);
            return true;
        case SDLK_S:
            if (!full_snake) return false;
            snake_set_direction(ctx->snake, 0, 1);
            return true;
        case SDLK_A:
            if (!full_snake) return false;
            snake_set_direction(ctx->snake, -1, 0);
            return true;
        case SDLK_D:
            if (!full_snake) return false;
            snake_set_direction(ctx->snake, 1, 0);
            return true;
        default:
            return false;
    }
}

/**************************************************************************
 * update_cheat_code
 *
 * Purpose:
 *   Incrementally match S N A K E and switch to snake mode when complete.
 *
 * Input:
 *   key - key pressed
 *   ctx - shared input context
 *
 * Output:
 *   None
 **************************************************************************/
static void update_cheat_code(SDL_Keycode key, InputContext *ctx) {
    char c = 0;
    if (key >= SDLK_A && key <= SDLK_Z) c = (char)('A' + (key - SDLK_A));
    if (c != 0) {
        if (c == ctx->cheat[*ctx->cheat_index]) {
            (*ctx->cheat_index)++;
            if (ctx->cheat[*ctx->cheat_index] == '\0') {
                *ctx->mode = MODE_SNAKE;
                *ctx->snake_overlay = false;
                snake_reset(ctx->snake, ctx->snake_cols, ctx->snake_rows);
                audio_clear(ctx->audio);
                *ctx->cheat_index = 0;
            }
        } else {
            *ctx->cheat_index = (c == ctx->cheat[0]) ? 1 : 0;
        }
    } else {
        *ctx->cheat_index = 0;
    }
}

/**************************************************************************
 * process_events
 *
 * Purpose:
 *   Poll and process all pending SDL events for this frame.
 *
 * Input:
 *   ctx  - shared input context
 *   quit - set true when app should terminate
 *
 * Output:
 *   None
 **************************************************************************/
static void process_events(InputContext *ctx, bool *quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            *quit = true;
            continue;
        }
        if (event.type != SDL_EVENT_KEY_DOWN) continue;

        SDL_Keycode key = event.key.key;
        if (key == SDLK_ESCAPE) {
            *quit = true;
            continue;
        }

        if (handle_snake_key(key, ctx)) {
            continue;
        }

        handle_sort_key(key, ctx->sort, ctx->audio, ctx->is_fullscreen, ctx->window);
        update_cheat_code(key, ctx);
        sort_update_title(ctx->window, ctx->sort);
    }
}

/**************************************************************************
 * step_snake
 *
 * Purpose:
 *   Advance snake simulation using accumulator/fixed-step logic.
 *
 * Input:
 *   snake     - snake state
 *   dt        - frame delta time
 *   snake_cols, snake_rows - board dimensions
 *
 * Output:
 *   None
 **************************************************************************/
static void step_snake(SnakeState *snake, double dt, int snake_cols, int snake_rows) {
    snake->accumulator += dt;
    while (snake->accumulator >= snake->move_interval) {
        snake->accumulator -= snake->move_interval;
        snake_step(snake, snake_cols, snake_rows);
    }
}

/**************************************************************************
 * main
 *
 * Purpose:
 *   Program entry point. Initializes subsystems, runs the event/update/
 *   render loop, and performs shutdown.
 *
 * Input:
 *   None
 *
 * Output:
 *   int - process exit code (0 success, non-zero failure)
 **************************************************************************/
int main(void) {
    srand((unsigned int)time(NULL));

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    AudioState audio = {0};
    if (!initialize_sdl_systems(&window, &renderer, &audio)) {
        return 1;
    }

    SortState sort = {0};
    sort_init_state(&sort);
    sort_set_algorithm(&sort, ALG_BUBBLE);
    sort_build_ops(&sort);
    sort.running = true;
    sort_update_title(window, &sort);

    SnakeState snake = {0};
    const int snake_cols = 48;
    const int snake_rows = 28;
    snake_reset(&snake, snake_cols, snake_rows);

    AppMode mode = MODE_SORT;
    bool snake_overlay = false;
    bool is_fullscreen = false;
    const char *cheat = "SNAKE";
    int cheat_index = 0;

    bool quit = false;
    Uint64 last_ticks = SDL_GetTicks();
    double acc = 0.0;

    while (!quit) {
        InputContext input_ctx = {
            .sort = &sort,
            .snake = &snake,
            .audio = &audio,
            .mode = &mode,
            .snake_overlay = &snake_overlay,
            .is_fullscreen = &is_fullscreen,
            .window = window,
            .snake_cols = snake_cols,
            .snake_rows = snake_rows,
            .cheat = cheat,
            .cheat_index = &cheat_index
        };
        process_events(&input_ctx, &quit);

        Uint64 now = SDL_GetTicks();
        double dt = (double)(now - last_ticks) / 1000.0;
        last_ticks = now;

        if (mode == MODE_SNAKE) {
            step_snake(&snake, dt, snake_cols, snake_rows);
            int w = WINDOW_WIDTH;
            int h = WINDOW_HEIGHT;
            SDL_GetWindowSize(window, &w, &h);
            snake_draw_scene(renderer, &snake, w, h);
            SDL_RenderPresent(renderer);
            SDL_Delay(1);
            continue;
        }

        acc += dt;
        while (acc >= sort.step_interval) {
            sort_execute_step(&sort, &audio);
            acc -= sort.step_interval;
        }

        if (sort.celebration_pending && !sort.celebration_active) {
            sort.celebration_pending = false;
            sort.celebration_active = true;
            sort.celebration_index = 0;
            sort.celebration_accumulator = 0.0;
            audio_clear(&audio);
        }
        sort_run_celebration_step(&sort, &audio, dt);

        if (sort.auto_restart_pending) {
            /* Auto-loop behavior: brief delay after celebration, then reshuffle. */
            Uint64 now_ms = SDL_GetTicks();
            if (now_ms >= sort.auto_restart_at_ms) {
                sort.auto_restart_pending = false;
                sort_shuffle_values(&sort);
                sort_build_ops(&sort);
                sort.running = true;
                sort.run_started_ms = SDL_GetTicks();
                sort_update_title(window, &sort);
            }
        }

        int w = WINDOW_WIDTH;
        int h = WINDOW_HEIGHT;
        SDL_GetWindowSize(window, &w, &h);
        float sx = (float)w / 1200.0f;
        float sy = (float)h / 720.0f;
        float ui_scale = sx < sy ? sx : sy;
        if (ui_scale < UI_SCALE_MIN) ui_scale = UI_SCALE_MIN;
        if (ui_scale > UI_SCALE_MAX) ui_scale = UI_SCALE_MAX;
        float snake_overlay_top = SNAKE_OVERLAY_TOP * ui_scale;
        float snake_overlay_bottom = snake_overlay_top + ((float)h * SNAKE_OVERLAY_HEIGHT_FACTOR) + SNAKE_OVERLAY_EXTRA_BOTTOM;

        sort_draw_scene(renderer, &sort, w, h, snake_overlay, snake_overlay_bottom);
        if (snake_overlay) {
            step_snake(&snake, dt, snake_cols, snake_rows);
            snake_draw_overlay(renderer, &snake, w, h, snake_overlay_top);
        }
        SDL_RenderPresent(renderer);
        if (sort.running) sort_update_title(window, &sort);
        SDL_Delay(1);
    }

    shutdown_sdl_systems(window, renderer, &audio);
    return 0;
}
