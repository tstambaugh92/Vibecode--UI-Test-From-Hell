#include "snake_mode.h"
#include <stdlib.h>

static bool snake_cell_occupied(const SnakeState *snake, int x, int y) {
    for (int i = 0; i < snake->length; i++) {
        if (snake->body[i].x == x && snake->body[i].y == y) return true;
    }
    return false;
}

static void snake_place_food(SnakeState *snake, int cols, int rows) {
    if (cols < 2 || rows < 2) return;
    int attempts = 0;
    do {
        snake->food_x = rand() % cols;
        snake->food_y = rand() % rows;
        attempts++;
    } while (snake_cell_occupied(snake, snake->food_x, snake->food_y) && attempts < 5000);
}

void snake_reset(SnakeState *snake, int cols, int rows) {
    snake->length = 5;
    snake->dx = 1;
    snake->dy = 0;
    snake->pending_dx = 1;
    snake->pending_dy = 0;
    snake->has_pending_turn = false;
    snake->alive = true;
    snake->started = false;
    snake->score = 0;
    snake->move_interval = 0.095;
    snake->accumulator = 0.0;

    int cx = cols / 2;
    int cy = rows / 2;
    for (int i = 0; i < snake->length; i++) {
        snake->body[i].x = cx - i;
        snake->body[i].y = cy;
    }
    snake_place_food(snake, cols, rows);
}

void snake_set_direction(SnakeState *snake, int dx, int dy) {
    if (!snake->alive) return;
    if (snake->has_pending_turn) return;
    if (snake->dx == -dx && snake->dy == -dy) return;
    snake->pending_dx = dx;
    snake->pending_dy = dy;
    snake->has_pending_turn = true;
    snake->started = true;
}

void snake_step(SnakeState *snake, int cols, int rows) {
    if (!snake->alive || !snake->started) return;

    if (snake->has_pending_turn) {
        snake->dx = snake->pending_dx;
        snake->dy = snake->pending_dy;
        snake->has_pending_turn = false;
    }

    Cell head = snake->body[0];
    int nx = head.x + snake->dx;
    int ny = head.y + snake->dy;

    if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) {
        snake->alive = false;
        return;
    }

    for (int i = 0; i < snake->length; i++) {
        if (snake->body[i].x == nx && snake->body[i].y == ny) {
            snake->alive = false;
            return;
        }
    }

    bool ate = (nx == snake->food_x && ny == snake->food_y);
    if (ate && snake->length < (int)(sizeof(snake->body) / sizeof(snake->body[0]))) {
        snake->length++;
        snake->score += 10;
        snake->move_interval *= 0.985;
        if (snake->move_interval < 0.040) snake->move_interval = 0.040;
    }

    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }
    snake->body[0].x = nx;
    snake->body[0].y = ny;

    if (ate) snake_place_food(snake, cols, rows);
}

static void draw_snake_board(
    SDL_Renderer *renderer,
    const SnakeState *snake,
    int width,
    int height,
    float origin_x,
    float origin_y,
    float board_w,
    float board_h,
    bool with_panel,
    bool show_text
) {
    const int cols = 48;
    const int rows = 28;
    float cw = board_w / (float)cols;
    float ch = board_h / (float)rows;

    /* Score increases by 10 per food; change theme every 5 foods (50 points). */
    int theme = (snake->score / 50) % 6;
    Uint8 bg_r = 16, bg_g = 28, bg_b = 20;
    Uint8 border_r = 80, border_g = 120, border_b = 94;
    Uint8 head_r = 120, head_g = 245, head_b = 140;
    Uint8 body_r = 70, body_g = 195, body_b = 110;
    Uint8 food_r = 255, food_g = 106, food_b = 106;

    if (theme == 1) { bg_r = 20; bg_g = 22; bg_b = 36; border_r = 110; border_g = 120; border_b = 200; head_r = 135; head_g = 200; head_b = 255; body_r = 90; body_g = 155; body_b = 235; food_r = 255; food_g = 180; food_b = 120; }
    if (theme == 2) { bg_r = 30; bg_g = 18; bg_b = 28; border_r = 170; border_g = 90; border_b = 170; head_r = 255; head_g = 150; head_b = 235; body_r = 210; body_g = 95; body_b = 190; food_r = 120; food_g = 235; food_b = 210; }
    if (theme == 3) { bg_r = 24; bg_g = 24; bg_b = 18; border_r = 145; border_g = 135; border_b = 90; head_r = 255; head_g = 218; head_b = 120; body_r = 220; body_g = 175; body_b = 90; food_r = 130; food_g = 220; food_b = 130; }
    if (theme == 4) { bg_r = 16; bg_g = 30; bg_b = 30; border_r = 90; border_g = 170; border_b = 170; head_r = 120; head_g = 255; head_b = 245; body_r = 85; body_g = 210; body_b = 200; food_r = 255; food_g = 135; food_b = 180; }
    if (theme == 5) { bg_r = 26; bg_g = 20; bg_b = 16; border_r = 170; border_g = 125; border_b = 90; head_r = 255; head_g = 176; head_b = 120; body_r = 230; body_g = 130; body_b = 85; food_r = 140; food_g = 210; food_b = 255; }

    if (with_panel) {
        SDL_FRect panel = {origin_x - 8.0f, origin_y - 8.0f, board_w + 16.0f, board_h + 16.0f};
        SDL_SetRenderDrawColor(renderer, 10, 20, 14, 220);
        SDL_RenderFillRect(renderer, &panel);
        SDL_SetRenderDrawColor(renderer, 90, 125, 100, 255);
        SDL_RenderRect(renderer, &panel);
    }

    SDL_FRect board = {origin_x, origin_y, board_w, board_h};
    SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255);
    SDL_RenderFillRect(renderer, &board);
    SDL_SetRenderDrawColor(renderer, border_r, border_g, border_b, 255);
    SDL_RenderRect(renderer, &board);

    SDL_FRect food = {
        origin_x + (float)snake->food_x * cw + 1.0f,
        origin_y + (float)snake->food_y * ch + 1.0f,
        cw - 2.0f,
        ch - 2.0f
    };
    SDL_SetRenderDrawColor(renderer, food_r, food_g, food_b, 255);
    SDL_RenderFillRect(renderer, &food);

    for (int i = 0; i < snake->length; i++) {
        float x = origin_x + (float)snake->body[i].x * cw + 1.0f;
        float y = origin_y + (float)snake->body[i].y * ch + 1.0f;
        SDL_FRect r = {x, y, cw - 2.0f, ch - 2.0f};
        if (i == 0) SDL_SetRenderDrawColor(renderer, head_r, head_g, head_b, 255);
        else SDL_SetRenderDrawColor(renderer, body_r, body_g, body_b, 255);
        SDL_RenderFillRect(renderer, &r);
    }

    if (show_text) {
        float sx = (float)width / 1200.0f;
        float sy = (float)height / 720.0f;
        float ui_scale = sx < sy ? sx : sy;
        if (ui_scale < 0.85f) ui_scale = 0.85f;
        if (ui_scale > 2.6f) ui_scale = 2.6f;
        float text_scale = ui_scale * 1.30f;
        if (text_scale < 1.25f) text_scale = 1.25f;
        if (text_scale > 2.8f) text_scale = 2.8f;

        SDL_SetRenderDrawColor(renderer, 228, 238, 232, 255);
        SDL_SetRenderScale(renderer, text_scale, text_scale);
        SDL_RenderDebugTextFormat(renderer, 16.0f / text_scale, 12.0f / text_scale, "Snake Mode (secret)  Score:%d", snake->score);
        SDL_RenderDebugText(renderer, 16.0f / text_scale, 28.0f / text_scale, "Arrows/WASD move | Space restart | Tab toggles PiP | X close");
        if (!snake->alive) {
            SDL_RenderDebugText(renderer, 16.0f / text_scale, 44.0f / text_scale, "Game over. Hit Space to restart.");
        }
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    }
}

void snake_draw_scene(SDL_Renderer *renderer, const SnakeState *snake, int width, int height) {
    SDL_SetRenderDrawColor(renderer, 8, 16, 12, 255);
    SDL_RenderClear(renderer);

    float board_w = (float)width * 0.86f;
    float board_h = (float)height * 0.84f;
    float origin_x = ((float)width - board_w) * 0.5f;
    float origin_y = ((float)height - board_h) * 0.55f;
    draw_snake_board(renderer, snake, width, height, origin_x, origin_y, board_w, board_h, false, true);
}

void snake_draw_overlay(SDL_Renderer *renderer, const SnakeState *snake, int width, int height, float top_y) {
    float board_w = (float)width * 0.33f;
    float board_h = (float)height * 0.30f;
    float origin_x = 14.0f;
    float origin_y = top_y;
    draw_snake_board(renderer, snake, width, height, origin_x, origin_y, board_w, board_h, true, false);
}
