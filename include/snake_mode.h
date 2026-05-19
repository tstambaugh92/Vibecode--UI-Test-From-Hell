#ifndef SNAKE_MODE_H
#define SNAKE_MODE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct {
    int x;
    int y;
} Cell;

typedef struct {
    Cell body[512];
    int length;
    int dx;
    int dy;
    int pending_dx;
    int pending_dy;
    bool has_pending_turn;
    int food_x;
    int food_y;
    bool alive;
    bool started;
    int score;
    double move_interval;
    double accumulator;
} SnakeState;

void snake_reset(SnakeState *snake, int cols, int rows);
void snake_set_direction(SnakeState *snake, int dx, int dy);
void snake_step(SnakeState *snake, int cols, int rows);
void snake_draw_scene(SDL_Renderer *renderer, const SnakeState *snake, int width, int height);
void snake_draw_overlay(SDL_Renderer *renderer, const SnakeState *snake, int width, int height, float top_y);

#endif
