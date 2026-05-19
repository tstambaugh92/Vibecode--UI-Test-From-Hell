#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL3/SDL.h>

typedef struct {
    Uint8 normal_r;
    Uint8 normal_g;
    Uint8 normal_b;
    Uint8 active_r;
    Uint8 active_g;
    Uint8 active_b;
} BackgroundPalette;

void background_draw_animated(
    SDL_Renderer *renderer,
    int width,
    int height,
    int palette_index,
    const BackgroundPalette *palette
);

#endif
