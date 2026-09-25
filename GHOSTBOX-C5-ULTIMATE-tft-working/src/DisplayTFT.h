#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayTFT : public TFT_eSPI {
public:
    static constexpr int32_t LOGICAL_W = 320;
    static constexpr int32_t LOGICAL_H = 240;

    DisplayTFT() : TFT_eSPI() {}

    static inline int32_t mapX(int32_t x) {
        return (x * 3) / 2;
    }

    static inline int32_t mapY(int32_t y) {
        return (y * 4) / 3;
    }

    int32_t logicalToPhysicalX(int32_t x) {
        return mapX(x);
    }

    int32_t logicalToPhysicalY(int32_t y) {
        return mapY(y);
    }

    // 1. Primitive: drawPixel (Logical 1x1 mapped to physical sub-block)
    void drawPixel(int32_t x, int32_t y, uint32_t color) override {
        int32_t x0 = mapX(x);
        int32_t x1 = mapX(x + 1);
        int32_t y0 = mapY(y);
        int32_t y1 = mapY(y + 1);
        TFT_eSPI::fillRect(x0, y0, x1 - x0, y1 - y0, color);
    }

    // 2. Primitive: drawFastHLine (Edge-to-edge physical mapping)
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color) override {
        if (w <= 0) return;
        int32_t x0 = mapX(x);
        int32_t x1 = mapX(x + w);
        int32_t y0 = mapY(y);
        int32_t y1 = mapY(y + 1);
        TFT_eSPI::fillRect(x0, y0, x1 - x0, y1 - y0, color);
    }

    // 3. Primitive: drawFastVLine (Edge-to-edge physical mapping)
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color) override {
        if (h <= 0) return;
        int32_t x0 = mapX(x);
        int32_t x1 = mapX(x + 1);
        int32_t y0 = mapY(y);
        int32_t y1 = mapY(y + h);
        TFT_eSPI::fillRect(x0, y0, x1 - x0, y1 - y0, color);
    }

    // 4. Primitive: fillRect (Edge-to-edge physical mapping)
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) override {
        if (w <= 0 || h <= 0) return;
        int32_t x0 = mapX(x);
        int32_t x1 = mapX(x + w);
        int32_t y0 = mapY(y);
        int32_t y1 = mapY(y + h);
        TFT_eSPI::fillRect(x0, y0, x1 - x0, y1 - y0, color);
    }

    // 5. Primitive: drawRect (DIRECT physical mapping - NEVER calls TFT_eSPI::drawRect)
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
        if (w <= 0 || h <= 0) return;
        int32_t x0 = mapX(x);
        int32_t x1 = mapX(x + w);
        int32_t y0 = mapY(y);
        int32_t y1 = mapY(y + h);
        int32_t pw = x1 - x0;
        int32_t ph = y1 - y0;
        if (pw <= 0 || ph <= 0) return;

        // Top edge
        TFT_eSPI::fillRect(x0, y0, pw, 1, color);
        // Bottom edge
        if (ph > 1) {
            TFT_eSPI::fillRect(x0, y1 - 1, pw, 1, color);
        }
        // Left edge
        if (ph > 2) {
            TFT_eSPI::fillRect(x0, y0 + 1, 1, ph - 2, color);
        }
        // Right edge
        if (ph > 2 && pw > 1) {
            TFT_eSPI::fillRect(x1 - 1, y0 + 1, 1, ph - 2, color);
        }
    }

    // 6. Primitive: drawLine (Physical Bresenham directly calling base drawPixel)
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color) override {
        if (x0 == x1) {
            if (y0 <= y1) drawFastVLine(x0, y0, y1 - y0 + 1, color);
            else drawFastVLine(x0, y1, y0 - y1 + 1, color);
            return;
        }
        if (y0 == y1) {
            if (x0 <= x1) drawFastHLine(x0, y0, x1 - x0 + 1, color);
            else drawFastHLine(x1, y0, x0 - x1 + 1, color);
            return;
        }

        int32_t px0 = mapX(x0);
        int32_t py0 = mapY(y0);
        int32_t px1 = mapX(x1);
        int32_t py1 = mapY(y1);

        int32_t dx = abs(px1 - px0);
        int32_t sx = (px0 < px1) ? 1 : -1;
        int32_t dy = -abs(py1 - py0);
        int32_t sy = (py0 < py1) ? 1 : -1;
        int32_t err = dx + dy;

        while (true) {
            TFT_eSPI::drawPixel(px0, py0, color);
            if (px0 == px1 && py0 == py1) break;
            int32_t e2 = 2 * err;
            if (e2 >= dy) { err += dy; px0 += sx; }
            if (e2 <= dx) { err += dx; py0 += sy; }
        }
    }

    void fillScreen(uint32_t color) {
        TFT_eSPI::fillScreen(color);
    }

    void drawCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        if (r < 0) return;
        int32_t px0 = mapX(x0);
        int32_t py0 = mapY(y0);
        int32_t pr = (r * 3) / 2;

        int32_t f = 1 - pr;
        int32_t ddF_x = 1;
        int32_t ddF_y = -2 * pr;
        int32_t x = 0;
        int32_t y = pr;

        TFT_eSPI::drawPixel(px0, py0 + pr, color);
        TFT_eSPI::drawPixel(px0, py0 - pr, color);
        TFT_eSPI::drawPixel(px0 + pr, py0, color);
        TFT_eSPI::drawPixel(px0 - pr, py0, color);

        while (x < y) {
            if (f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            TFT_eSPI::drawPixel(px0 + x, py0 + y, color);
            TFT_eSPI::drawPixel(px0 - x, py0 + y, color);
            TFT_eSPI::drawPixel(px0 + x, py0 - y, color);
            TFT_eSPI::drawPixel(px0 - x, py0 - y, color);
            TFT_eSPI::drawPixel(px0 + y, py0 + x, color);
            TFT_eSPI::drawPixel(px0 - y, py0 + x, color);
            TFT_eSPI::drawPixel(px0 + y, py0 - x, color);
            TFT_eSPI::drawPixel(px0 - y, py0 - x, color);
        }
    }

    void fillCircle(int32_t x0, int32_t y0, int32_t r, uint32_t color) {
        if (r < 0) return;
        int32_t px0 = mapX(x0);
        int32_t py0 = mapY(y0);
        int32_t pr = (r * 3) / 2;
        for (int32_t x = -pr; x <= pr; x++) {
            int32_t h = (int32_t)sqrt((float)(pr * pr - x * x));
            TFT_eSPI::fillRect(px0 + x, py0 - h, 1, 2 * h + 1, color);
        }
    }

    void drawNativeString(int32_t x, int32_t y, const String& text,
                          uint32_t color, uint32_t bg,
                          uint8_t font = 2, uint8_t size = 1) {
        TFT_eSPI::setTextColor(color, bg);
        TFT_eSPI::setTextSize(size);
        TFT_eSPI::setTextDatum(TL_DATUM);
        TFT_eSPI::drawString(text, mapX(x), mapY(y), font);
    }

    void drawNativeStringTransparent(int32_t x, int32_t y, const String& text,
                                     uint32_t color,
                                     uint8_t font = 2, uint8_t size = 1) {
        TFT_eSPI::setTextColor(color);
        TFT_eSPI::setTextSize(size);
        TFT_eSPI::setTextDatum(TL_DATUM);
        TFT_eSPI::drawString(text, mapX(x), mapY(y), font);
    }

    int32_t nativeTextWidth(const String& text, uint8_t font = 2, uint8_t size = 1) {
        TFT_eSPI::setTextSize(size);
        return (TFT_eSPI::textWidth(text, font) * 2) / 3;
    }
};

extern DisplayTFT tft;
