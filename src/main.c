#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "audio.h"
#include "constants.h"
#include "snake_mode.h"
#include "sort_mode.h"

typedef enum {
    MODE_SORT = 0,
    MODE_SNAKE = 1
} AppMode;

static void handle_sort_key(
    SDL_Keycode key,
    SortState *sort,
    AudioState *audio,
    bool *is_fullscreen,
    SDL_Window *window
) {
    bool should_autostart = false;
    if (key == SDLK_F || key == SDLK_F11) {
        *is_fullscreen = !(*is_fullscreen);
        SDL_SetWindowFullscreen(window, *is_fullscreen);
    } else if (key == SDLK_1) {
        sort_set_algorithm(sort, ALG_BUBBLE);
        sort_build_ops(sort);
        audio_clear(audio);
        should_autostart = true;
    } else if (key == SDLK_2) {
        sort_set_algorithm(sort, ALG_INSERTION);
        sort_build_ops(sort);
        audio_clear(audio);
        should_autostart = true;
    } else if (key == SDLK_3) {
        sort_set_algorithm(sort, ALG_SELECTION);
        sort_build_ops(sort);
        audio_clear(audio);
        should_autostart = true;
    } else if (key == SDLK_4) {
        sort_set_algorithm(sort, ALG_QUICK);
        sort_build_ops(sort);
        audio_clear(audio);
        should_autostart = true;
    } else if (key == SDLK_5) {
        sort_set_algorithm(sort, ALG_MERGE);
        sort_build_ops(sort);
        audio_clear(audio);
        should_autostart = true;
    } else if (key == SDLK_6) {
        sort_set_algorithm(sort, ALG_HEAP);
        sort_build_ops(sort);
        audio_clear(audio);
        should_autostart = true;
    } else if (key == SDLK_R) {
        sort_shuffle_values(sort);
        sort_build_ops(sort);
        audio_clear(audio);
        should_autostart = true;
    } else if (key == SDLK_H) {
        sort->show_help = !sort->show_help;
    } else if (key == SDLK_B) {
        sort->animated_bg_on = !sort->animated_bg_on;
    } else if (key == SDLK_KP_1) {
        sort_set_palette(sort, 0);
    } else if (key == SDLK_KP_2) {
        sort_set_palette(sort, 1);
    } else if (key == SDLK_KP_3) {
        sort_set_palette(sort, 2);
    } else if (key == SDLK_KP_4) {
        sort_set_palette(sort, 3);
    } else if (key == SDLK_KP_5) {
        sort_set_palette(sort, 4);
    } else if (key == SDLK_KP_6) {
        sort_set_palette(sort, 5);
    } else if (key == SDLK_KP_7) {
        sort_set_palette(sort, 6);
    } else if (key == SDLK_KP_8) {
        sort_set_palette(sort, 7);
    } else if (key == SDLK_KP_9) {
        sort_set_palette(sort, 8);
    } else if (key == SDLK_M) {
        sort->sound_on = !sort->sound_on;
        if (!sort->sound_on) audio_clear(audio);
    } else if (key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS) {
        sort_handle_speed_up(sort);
    } else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
        sort_handle_speed_down(sort);
    } else if (key == SDLK_S) {
        sort_handle_start_pause(sort, audio);
    }

    if (should_autostart) {
        sort->running = true;
        sort->run_started_ms = SDL_GetTicks();
    }
}

int main(void) {
    srand((unsigned int)time(NULL));

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
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
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    AudioState audio = {0};
    if (!audio_init(&audio)) {
        fprintf(stderr, "Audio disabled: %s\n", SDL_GetError());
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
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode key = event.key.key;
                if (key == SDLK_ESCAPE) {
                    quit = true;
                    continue;
                }

                if (mode == MODE_SNAKE) {
                    if (key == SDLK_TAB) {
                        mode = MODE_SORT;
                        snake_overlay = true;
                        sort.running = true;
                        sort_update_title(window, &sort);
                    } else if (key == SDLK_X) {
                        mode = MODE_SORT;
                        snake_overlay = false;
                        snake.started = false;
                        sort.running = true;
                        sort_update_title(window, &sort);
                    } else if (key == SDLK_SPACE) {
                        snake_reset(&snake, snake_cols, snake_rows);
                    } else if (key == SDLK_UP || key == SDLK_W) {
                        snake_set_direction(&snake, 0, -1);
                    } else if (key == SDLK_DOWN || key == SDLK_S) {
                        snake_set_direction(&snake, 0, 1);
                    } else if (key == SDLK_LEFT || key == SDLK_A) {
                        snake_set_direction(&snake, -1, 0);
                    } else if (key == SDLK_RIGHT || key == SDLK_D) {
                        snake_set_direction(&snake, 1, 0);
                    }
                    continue;
                }

                if (key == SDLK_TAB && snake_overlay) {
                    mode = MODE_SNAKE;
                    snake_overlay = false;
                    continue;
                }

                if (snake_overlay) {
                    if (key == SDLK_X) {
                        snake_overlay = false;
                        snake.started = false;
                        continue;
                    } else if (key == SDLK_SPACE) {
                        snake_reset(&snake, snake_cols, snake_rows);
                        continue;
                    } else if (key == SDLK_UP) {
                        snake_set_direction(&snake, 0, -1);
                        continue;
                    } else if (key == SDLK_DOWN) {
                        snake_set_direction(&snake, 0, 1);
                        continue;
                    } else if (key == SDLK_LEFT) {
                        snake_set_direction(&snake, -1, 0);
                        continue;
                    } else if (key == SDLK_RIGHT) {
                        snake_set_direction(&snake, 1, 0);
                        continue;
                    }
                }

                handle_sort_key(key, &sort, &audio, &is_fullscreen, window);

                char c = 0;
                if (key >= SDLK_A && key <= SDLK_Z) c = (char)('A' + (key - SDLK_A));
                if (c != 0) {
                    if (c == cheat[cheat_index]) {
                        cheat_index++;
                        if (cheat[cheat_index] == '\0') {
                            mode = MODE_SNAKE;
                            snake_overlay = false;
                            snake_reset(&snake, snake_cols, snake_rows);
                            audio_clear(&audio);
                            cheat_index = 0;
                        }
                    } else {
                        cheat_index = (c == cheat[0]) ? 1 : 0;
                    }
                } else {
                    cheat_index = 0;
                }

                sort_update_title(window, &sort);
            }
        }

        Uint64 now = SDL_GetTicks();
        double dt = (double)(now - last_ticks) / 1000.0;
        last_ticks = now;

        if (mode == MODE_SNAKE) {
            snake.accumulator += dt;
            while (snake.accumulator >= snake.move_interval) {
                snake.accumulator -= snake.move_interval;
                snake_step(&snake, snake_cols, snake_rows);
            }
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
            snake.accumulator += dt;
            while (snake.accumulator >= snake.move_interval) {
                snake.accumulator -= snake.move_interval;
                snake_step(&snake, snake_cols, snake_rows);
            }
            snake_draw_overlay(renderer, &snake, w, h, snake_overlay_top);
        }
        SDL_RenderPresent(renderer);
        if (sort.running) sort_update_title(window, &sort);
        SDL_Delay(1);
    }

    audio_shutdown(&audio);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
