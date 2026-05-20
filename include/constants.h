#ifndef CONSTANTS_H
#define CONSTANTS_H

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 720

#define BAR_COUNT 180
#define MAX_OPS 500000

#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_TONE_MS 14
/*
 * Keep up to ~20 seconds of mono float audio queued so longer motifs
 * can enqueue without getting dropped by the safety cap.
 */
#define AUDIO_MAX_QUEUED_BYTES (AUDIO_SAMPLE_RATE * (int)sizeof(float) * 20)

#define SIDE_MARGIN 26.0f
#define BAR_GAP 1.0f

/* Shared UI scaling and layout constants. */
#define UI_SCALE_MIN 0.95f
#define UI_SCALE_MAX 2.80f
#define HUD_TEXT_SCALE_FACTOR 1.28f
#define HUD_TEXT_SCALE_MIN 1.55f
#define HUD_TEXT_SCALE_MAX 3.30f

#define HUD_BG_X 8.0f
#define HUD_BG_Y 6.0f
#define HUD_BG_W 940.0f
#define HUD_BG_H 64.0f

#define HUD_TEXT_X 12.0f
#define HUD_LINE1_Y 10.0f
#define HUD_LINE2_Y 30.0f
#define HUD_LINE3_Y 50.0f

#define HELP_PANEL_DEFAULT_Y 86.0f
#define HELP_PANEL_MARGIN_SCREEN 8.0f
#define HELP_PANEL_X 12.0f
#define HELP_PANEL_MIN_WIDTH_FACTOR 0.58f
#define HELP_PANEL_PAD_LEFT 8.0f
#define HELP_PANEL_PAD_RIGHT 12.0f
#define HELP_PANEL_PAD_TOP 8.0f
#define HELP_PANEL_PAD_BOTTOM 10.0f
#define HELP_PANEL_GAP_FROM_SNAKE 6.0f
#define HELP_PANEL_GAP_FROM_HUD 12.0f
#define HELP_PANEL_LINE_SPACING 20.0f

#define SNAKE_OVERLAY_TOP 88.0f
#define SNAKE_OVERLAY_HEIGHT_FACTOR 0.30f
#define SNAKE_OVERLAY_EXTRA_BOTTOM 16.0f

#endif
