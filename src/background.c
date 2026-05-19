#include "background.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float x, y;
    float vx, vy;
    float size;
    float angle;
    float spin;
    int type;
    int color_variant;
    float life;
} BgShape;

static void draw_gradient_shape(
    SDL_Renderer *renderer,
    int type,
    float cx,
    float cy,
    float size,
    float rot,
    Uint8 r,
    Uint8 g,
    Uint8 b,
    Uint8 a
) {
    const int n = 96;
    SDL_FPoint pts[n];
    float min_y = 1e9f;
    float max_y = -1e9f;

    for (int i = 0; i < n; i++) {
        float t = ((float)i / (float)n) * 2.0f * (float)M_PI;
        float xr = 0.0f;
        float yr = 0.0f;
        if (type == 0) {
            float rr = size * (0.55f + 0.45f * cosf(5.0f * t));
            xr = rr * cosf(t);
            yr = rr * sinf(t);
        } else if (type == 1) {
            float xh = 16.0f * powf(sinf(t), 3.0f);
            float yh = 13.0f * cosf(t) - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t);
            xr = xh * (size / 18.0f);
            yr = -yh * (size / 18.0f);
        } else {
            float rr = size * (0.36f + 0.64f * fabsf(sinf(3.0f * t)));
            xr = rr * cosf(t);
            yr = rr * sinf(t);
        }
        float rx = xr * cosf(rot) - yr * sinf(rot);
        float ry = xr * sinf(rot) + yr * cosf(rot);
        pts[i].x = cx + rx;
        pts[i].y = cy + ry;
        if (pts[i].y < min_y) min_y = pts[i].y;
        if (pts[i].y > max_y) max_y = pts[i].y;
    }

    if (max_y - min_y < 1.0f) {
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_FRect dot = {cx - 1.0f, cy - 1.0f, 2.0f, 2.0f};
        SDL_RenderFillRect(renderer, &dot);
        return;
    }

    float xints[192];
    int y0 = (int)floorf(min_y);
    int y1 = (int)ceilf(max_y);
    for (int y = y0; y <= y1; y++) {
        float yy = (float)y + 0.5f;
        int nints = 0;
        for (int i = 0; i < n; i++) {
            SDL_FPoint p1 = pts[i];
            SDL_FPoint p2 = pts[(i + 1) % n];
            bool cond = ((p1.y <= yy && p2.y > yy) || (p2.y <= yy && p1.y > yy));
            if (!cond) continue;
            float t = (yy - p1.y) / (p2.y - p1.y);
            xints[nints++] = p1.x + t * (p2.x - p1.x);
        }
        for (int i = 0; i < nints - 1; i++) {
            for (int j = i + 1; j < nints; j++) {
                if (xints[j] < xints[i]) {
                    float tmp = xints[i];
                    xints[i] = xints[j];
                    xints[j] = tmp;
                }
            }
        }

        float p = (yy - min_y) / (max_y - min_y);
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
        float fade = 1.0f - p;
        float shaped = 0.28f + 0.72f * powf(fade, 0.38f);
        if (shaped < 0.02f) shaped = 0.02f;
        Uint8 la = (Uint8)((float)a * shaped);
        SDL_SetRenderDrawColor(renderer, r, g, b, la);
        for (int i = 0; i + 1 < nints; i += 2) {
            SDL_RenderLine(renderer, xints[i], yy, xints[i + 1], yy);
        }
    }

    SDL_SetRenderDrawColor(renderer, (Uint8)(r * 0.52f), (Uint8)(g * 0.52f), (Uint8)(b * 0.52f), (Uint8)(a * 0.35f));
    SDL_FPoint outline[n + 1];
    for (int i = 0; i < n; i++) outline[i] = pts[i];
    outline[n] = pts[0];
    SDL_RenderLines(renderer, outline, n + 1);
}

static void spawn_bg_shape(BgShape *s, int width, int height) {
    float cx = (float)width * 0.5f;
    float cy = (float)height * 0.5f;
    int edge = rand() % 4;
    float margin = 120.0f;
    if (edge == 0) {
        s->x = -margin;
        s->y = (float)(rand() % (height + 1));
    } else if (edge == 1) {
        s->x = (float)width + margin;
        s->y = (float)(rand() % (height + 1));
    } else if (edge == 2) {
        s->x = (float)(rand() % (width + 1));
        s->y = -margin;
    } else {
        s->x = (float)(rand() % (width + 1));
        s->y = (float)height + margin;
    }

    float to_cx = cx - s->x;
    float to_cy = cy - s->y;
    float len = sqrtf(to_cx * to_cx + to_cy * to_cy);
    if (len < 0.001f) len = 1.0f;
    to_cx /= len;
    to_cy /= len;

    float spread = ((float)(rand() % 1000) / 1000.0f - 0.5f) * 1.25f;
    float cs = cosf(spread);
    float sn = sinf(spread);
    float dirx = to_cx * cs - to_cy * sn;
    float diry = to_cx * sn + to_cy * cs;
    float speed = 35.0f + (float)(rand() % 120);

    s->vx = dirx * speed;
    s->vy = diry * speed;
    s->size = 24.0f + (float)(rand() % 52);
    s->angle = ((float)(rand() % 6283) / 1000.0f);
    s->spin = (((float)(rand() % 2000) / 1000.0f) - 1.0f) * (0.25f + (float)(rand() % 220) / 100.0f);
    s->type = rand() % 3;
    s->color_variant = rand() % 2;
    s->life = 0.0f;
}

void background_draw_animated(
    SDL_Renderer *renderer,
    int width,
    int height,
    int palette_index,
    const BackgroundPalette *palette
) {
    Uint64 now_ms = SDL_GetTicks();
    static BgShape shapes[18];
    static bool init = false;
    static Uint64 prev_ms = 0;

    if (!init) {
        memset(shapes, 0, sizeof(shapes));
        init = true;
    }
    if (prev_ms == 0) prev_ms = now_ms;
    float dt = (float)(now_ms - prev_ms) / 1000.0f;
    prev_ms = now_ms;

    if (palette_index == 0) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 8, 12, 22, 255);
    }
    SDL_RenderClear(renderer);

    const int shape_count = (int)(sizeof(shapes) / sizeof(shapes[0]));
    for (int i = 0; i < shape_count; i++) {
        BgShape *s = &shapes[i];
        if (s->size <= 0.0f) spawn_bg_shape(s, width, height);

        s->x += s->vx * dt;
        s->y += s->vy * dt;
        s->angle += s->spin * dt;
        s->life += dt;

        float margin = s->size + 140.0f;
        bool out = (s->x < -margin || s->x > (float)width + margin || s->y < -margin || s->y > (float)height + margin);
        if (out || s->life > 22.0f) spawn_bg_shape(s, width, height);

        float p = (float)i / (float)shape_count;
        Uint8 r, g, b;
        if (palette_index == 0) {
            if (s->color_variant == 0) {
                r = 255; g = 88; b = 0;
            } else {
                r = 255; g = 213; b = 0;
            }
        } else {
            if (s->color_variant == 0) {
                r = (Uint8)(35 + (palette->normal_r * (0.55f + 0.25f * p)));
                g = (Uint8)(28 + (palette->normal_g * (0.55f + 0.25f * p)));
                b = (Uint8)(38 + (palette->normal_b * (0.55f + 0.25f * p)));
            } else {
                r = (Uint8)(35 + (palette->active_r * (0.45f + 0.35f * (1.0f - p))));
                g = (Uint8)(28 + (palette->active_g * (0.45f + 0.35f * (1.0f - p))));
                b = (Uint8)(38 + (palette->active_b * (0.45f + 0.35f * (1.0f - p))));
            }
        }
        Uint8 a = (Uint8)(85 + 95.0f * (0.4f + 0.6f * fabsf(sinf(s->angle * 0.7f + p * 3.0f))));
        if (s->type == 0) {
            draw_gradient_shape(renderer, 0, s->x, s->y, s->size * 0.62f, s->angle, r, g, b, a);
        } else if (s->type == 1) {
            draw_gradient_shape(renderer, 1, s->x, s->y, s->size * 0.76f, s->angle, r, g, b, a);
        } else {
            draw_gradient_shape(renderer, 2, s->x, s->y, s->size * 0.66f, s->angle, r, g, b, a);
        }
    }
}
