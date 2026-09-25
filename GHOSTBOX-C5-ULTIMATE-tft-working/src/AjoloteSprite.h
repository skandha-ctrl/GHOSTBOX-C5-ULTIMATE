#ifndef AJOLOTE_SPRITE_H
#define AJOLOTE_SPRITE_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// AXOLOTL SPRITE · Ghostbox Mascot (96x80 px)
// · Monochrome bitmap 96x80 pixels (960 bytes in PROGMEM)
// · 1 bit per pixel: 1 = mascot color, 0 = transparent
// · Left to right, top to bottom, MSB first
// ═══════════════════════════════════════════════════════════════════════════

extern const uint8_t  AJOLOTE_BITMAP[] PROGMEM;
extern const uint16_t AJOLOTE_WIDTH;
extern const uint16_t AJOLOTE_HEIGHT;

// Draws axolotl line by line up to maxLines (progressive scan-in effect).
// Used by the splash boot scan-in animation.
void drawAjoloteScan(int16_t x0, int16_t y0, uint8_t maxLines, uint16_t color = 0x07FF);

// Draws full axolotl at original size (96x80) at (x0, y0).
void drawAjolote(int16_t x0, int16_t y0, uint16_t color = 0x07FF);

// Draws axolotl scaled: scale=1.0 normal, 0.5 half (48x40), 2.0 double.
// For scales smaller than 1.0, use drawAjoloteHalf (more efficient).
void drawAjoloteScaled(int16_t x0, int16_t y0, float scale, uint16_t color = 0x07FF);

// Draws axolotl at half size (48x40) — optimized version
// sampling 1 in every 2 pixels per axis.
void drawAjoloteHalf(int16_t x0, int16_t y0, uint16_t color = 0x07FF);

#endif
