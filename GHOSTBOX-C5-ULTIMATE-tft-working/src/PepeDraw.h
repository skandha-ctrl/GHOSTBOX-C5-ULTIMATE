#ifndef PEPE_DRAW_H
#define PEPE_DRAW_H

#include <Arduino.h>
#include "DisplayTFT.h"

// ═══════════════════════════════════════════════════════════════════════════
// UI Draw v2 · Bitmap text rendering library
// · 2 custom fonts: SMALL (5x7) and BIG (8x12)
// · Variable width per character (true kerning)
// · True descenders (g, j, p, q, y)
// · Extended character set support
// · UTF-8 aware
// ═══════════════════════════════════════════════════════════════════════════

// ── UI Colors ─────────────────────────────────────────────────────────────
#define UI_MAIN    TFT_WHITE
#define UI_BG      TFT_BLACK
#define UI_ACCENT  TFT_WHITE
#define UI_CURSOR  TFT_WHITE
#define UI_SELECT  TFT_WHITE

extern DisplayTFT tft;

// ── Font Types ────────────────────────────────────────────────────────────
enum FontType {
    FONT_SMALL = 0,   // 5 width x 7 height (compact, legible)
    FONT_BIG   = 1    // 8 width x 12 height (titles, headers)
};

// ───────────────────────────────────────────────────────────────────────────
// Backward compatible API (preserves existing call sites)
// drawCharCustom / drawStringCustom -> uses FONT_SMALL internally
// ───────────────────────────────────────────────────────────────────────────
void drawCharCustom(int x, int y, char c, uint16_t color, int size);
void drawStringCustom(int x, int y, String txt, uint16_t color, int size);

// ───────────────────────────────────────────────────────────────────────────
// Extended Rendering API
// ───────────────────────────────────────────────────────────────────────────

// Draws with BIG font (8x12)
void drawStringBig(int x, int y, const String& txt, uint16_t color, int size);

// Calculates text width in pixels (useful for centering or alignment)
int  getTextWidth(const String& txt, int size, FontType font = FONT_SMALL);

// Font height in pixels (useful for vertical line spacing)
int  getFontHeight(int size, FontType font = FONT_SMALL);

// Truncates text with ellipsis ("..") if it exceeds maxWidth pixels
String truncateToWidth(const String& txt, int maxWidth,
                       int size, FontType font = FONT_SMALL);

// Draws text clipped / truncated to maximum width
void drawStringFit(int x, int y, const String& txt, uint16_t color,
                   int maxWidth, int size, FontType font = FONT_SMALL);

// Draws text horizontally centered on screen (320px width)
void drawStringCentered(int y, const String& txt, uint16_t color,
                        int size, FontType font = FONT_SMALL);

// Draws text right-aligned (xRight = rightmost bounding edge)
void drawStringRight(int xRight, int y, const String& txt, uint16_t color,
                     int size, FontType font = FONT_SMALL);

#endif
