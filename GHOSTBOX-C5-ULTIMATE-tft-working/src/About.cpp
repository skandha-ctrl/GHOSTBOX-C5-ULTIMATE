#include "About.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "SoundUtils.h"
#include "NVSStore.h"
#include "SystemInfo.h"
#include "AjoloteSprite.h"

extern DisplayTFT tft;

// ═══════════════════════════════════════════════════════════════════════════
// CONFIG
// ═══════════════════════════════════════════════════════════════════════════

#define VIEWPORT_TOP    34     // Where the viewport starts (below the header)
#define VIEWPORT_BOTTOM 218    // Where the viewport ends (above the footer)
#define SCROLL_STEP     20

static int g_scrollY = 0;
static int g_maxScroll = 0;

// ═══════════════════════════════════════════════════════════════════════════
// DRAWING HELPERS WITH CLIPPING (never draws outside viewport)
// ═══════════════════════════════════════════════════════════════════════════

// Draws text ONLY if it falls completely inside the viewport.
// If partially outside, do not draw (avoids overdrawing header/footer).
static void drawScrollableText(int yContent, int x, const String& text,
                              uint16_t color, int size) {
    int yScreen = VIEWPORT_TOP + (yContent - g_scrollY);
    int textH = (size == 1) ? 7 : (size * 8);

    // If completely outside the viewport, do not draw
    if (yScreen + textH < VIEWPORT_TOP) return;
    if (yScreen > VIEWPORT_BOTTOM) return;

    // If partially outside, do not draw either — avoids overflow
    if (yScreen < VIEWPORT_TOP) return;
    if (yScreen + textH > VIEWPORT_BOTTOM) return;

    drawStringCustom(x, yScreen, text, color, size);
}

static void drawScrollableLine(int yContent, uint16_t color) {
    int yScreen = VIEWPORT_TOP + (yContent - g_scrollY);
    if (yScreen < VIEWPORT_TOP || yScreen > VIEWPORT_BOTTOM) return;
    tft.drawFastHLine(15, yScreen, 290, color);
}

// AXOLOTL at half scale (48x40). Clips individual rows if they fall
// partially outside the viewport.
static void drawScrollableAjolote(int yContent) {
    const int W = 48;
    const int H = 40;
    int x = (320 - W) / 2;
    int yBase = VIEWPORT_TOP + (yContent - g_scrollY);

    // If completely outside, return
    if (yBase + H < VIEWPORT_TOP) return;
    if (yBase > VIEWPORT_BOTTOM) return;

    // Draw row by row, skipping rows outside the viewport
    int bytesPerRow = AJOLOTE_WIDTH / 8;
    for (int r = 0; r < AJOLOTE_HEIGHT; r += 2) {
        int outY = yBase + r / 2;
        if (outY < VIEWPORT_TOP) continue;     // Above viewport
        if (outY > VIEWPORT_BOTTOM) break;     // Past bottom of viewport

        for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
            uint8_t bits = pgm_read_byte(
                &AJOLOTE_BMP[r * bytesPerRow + byteIdx]);
            if (bits == 0) continue;
            for (int bit = 0; bit < 8; bit += 2) {
                if (bits & (0x80 >> bit)) {
                    int outX = x + (byteIdx * 8 + bit) / 2;
                    tft.drawPixel(outX, outY, UI_MAIN);
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN CONTENT
// ═══════════════════════════════════════════════════════════════════════════

static void drawAboutContent() {
    // Clear the viewport area (leaves header and footer intact)
    tft.fillRect(2, VIEWPORT_TOP, 316, VIEWPORT_BOTTOM - VIEWPORT_TOP,
                 TFT_BLACK);

    int y = 5;   // Y offset inside virtual content

    // ─── Large Title ───
    String title = "GHOSTBOX";
    int tw = title.length() * 8 * 3;   // Size 3 with FONT_BIG
    drawScrollableText(y, (320 - tw) / 2, title, UI_MAIN, 3);
    y += 32;

    // ─── Version ───
    String version = String(FW_VERSION);
    int vw = version.length() * 6 * 2;
    drawScrollableText(y, (320 - vw) / 2, version, UI_SELECT, 2);
    y += 28;

    drawScrollableLine(y, UI_ACCENT);
    y += 12;

    // ─── AXOLOTL (48x40) ───
    drawScrollableAjolote(y);
    y += 50;

    drawScrollableLine(y, UI_ACCENT);
    y += 14;

    // ─── Author / Project Info ───
    drawScrollableText(y, 70, "GHOSTBOX C5", TFT_YELLOW, 2);
    y += 28;

    drawScrollableText(y, 30, "Ghostbox Labs", UI_MAIN, 2);
    y += 22;

    drawScrollableText(y, 30, "Cybersecurity Research", UI_ACCENT, 1);
    y += 18;

    drawScrollableLine(y, UI_ACCENT);
    y += 14;

    // ─── Repository Links ───
    drawScrollableText(y, 30, "REPOSITORY", UI_MAIN, 1);
    y += 20;

    drawScrollableText(y, 30, "GH:", 0xA81F, 2);
    drawScrollableText(y, 80, "/GHOSTBOX-C5", UI_MAIN, 2);
    y += 30;

    drawScrollableLine(y, UI_ACCENT);
    y += 14;

    // ─── Boot Count ───
    int boots = nvsGetInt("boot_cnt", 0);
    String bootText = "Booted " + String(boots) + " times";
    int bw = bootText.length() * 6;
    drawScrollableText(y, (320 - bw) / 2, bootText, UI_ACCENT, 1);
    y += 18;

    drawScrollableLine(y, UI_ACCENT);
    y += 14;

    // ─── Quote / Philosophy ───
    drawScrollableText(y, 30, "\"Knowledge", TFT_GREEN, 2);
    y += 22;
    drawScrollableText(y, 30, "must be free.\"", TFT_GREEN, 2);
    y += 32;

    // Calculate max scroll
    int viewportH = VIEWPORT_BOTTOM - VIEWPORT_TOP;
    g_maxScroll = y - viewportH;
    if (g_maxScroll < 0) g_maxScroll = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// HEADER AND FOOTER (always redrawn on top to prevent artifacts)
// ═══════════════════════════════════════════════════════════════════════════

static void drawHeader() {
    tft.fillRect(0, 0, 320, VIEWPORT_TOP, TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, UI_MAIN);
    drawStringCustom(110, 10, "ABOUT", UI_MAIN, 3);
    tft.drawFastHLine(2, VIEWPORT_TOP, 316, UI_ACCENT);
}

static void drawFooter() {
    tft.fillRect(0, VIEWPORT_BOTTOM, 320, 240 - VIEWPORT_BOTTOM, TFT_BLACK);
    tft.drawFastHLine(2, VIEWPORT_BOTTOM, 316, UI_ACCENT);

    // Redraw lateral borders in case they were clipped
    tft.drawFastVLine(0, 0, 240, UI_MAIN);
    tft.drawFastVLine(319, 0, 240, UI_MAIN);
    tft.drawFastHLine(0, 239, 320, UI_MAIN);

    if (g_maxScroll > 0) {
        if (g_scrollY == 0) {
            drawStringCustom(10, 226, "DOWN: VIEW MORE  BACK/OK: RETURN",
                             UI_ACCENT, 1);
        } else if (g_scrollY >= g_maxScroll) {
            drawStringCustom(10, 226, "UP: SCROLL UP  BACK/OK: RETURN",
                             UI_ACCENT, 1);
        } else {
            drawStringCustom(10, 226, "UP/DN: SCROLL  BACK/OK: RETURN",
                             UI_ACCENT, 1);
        }

        // Side scroll indicator bar
        int trackTop = VIEWPORT_TOP + 5;
        int trackBot = VIEWPORT_BOTTOM - 5;
        int trackH = trackBot - trackTop;
        int totalContent = g_maxScroll + (VIEWPORT_BOTTOM - VIEWPORT_TOP);
        int barH = (trackH * (VIEWPORT_BOTTOM - VIEWPORT_TOP)) / totalContent;
        if (barH < 10) barH = 10;
        int barY = trackTop;
        if (g_maxScroll > 0) {
            barY = trackTop + (g_scrollY * (trackH - barH)) / g_maxScroll;
        }

        // Clear track before drawing
        tft.fillRect(310, trackTop, 6, trackH, TFT_BLACK);
        tft.drawFastVLine(312, trackTop, trackH, UI_ACCENT);
        tft.fillRect(310, barY, 5, barH, UI_SELECT);
    } else {
        drawStringCustom(96, 226, "OK/BACK: RETURN", UI_ACCENT, 1);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// FULL REDRAW (Strict order: content -> header -> footer)
// ═══════════════════════════════════════════════════════════════════════════

static void redrawAll() {
    drawAboutContent();    // 1. Scrollable content (may overdraw edges)
    drawHeader();          // 2. Header on top -> covers top edge
    drawFooter();          // 3. Footer on top -> covers bottom edge
}

// ═══════════════════════════════════════════════════════════════════════════
// ENTRY POINT
// ═══════════════════════════════════════════════════════════════════════════

void runAbout() {
    while (navEnterPressed() || navBackPressed()) delay(5);
    delay(100);

    g_scrollY = 0;

    // Entry beep (credits jingle)
    beep(2400, 60); delay(40);
    beep(3000, 60); delay(40);
    beep(3600, 100);

    tft.fillScreen(TFT_BLACK);
    redrawAll();

    unsigned long lastBtn = 0;

    while (true) {
        if ((navEnterPressed() || navBackPressed()) && millis() - lastBtn > 200) {
            beep(1800, 50); delay(30);
            beep(1200, 80);
            while (navEnterPressed() || navBackPressed()) delay(5);
            delay(100);
            return;
        }

        if (navUpPressed() && millis() - lastBtn > 150) {
            if (g_scrollY > 0) {
                g_scrollY -= SCROLL_STEP;
                if (g_scrollY < 0) g_scrollY = 0;
                beep(2200, 20);
                redrawAll();
            }
            lastBtn = millis();
        }

        if (navDownPressed() && millis() - lastBtn > 150) {
            if (g_scrollY < g_maxScroll) {
                g_scrollY += SCROLL_STEP;
                if (g_scrollY > g_maxScroll) g_scrollY = g_maxScroll;
                beep(2200, 20);
                redrawAll();
            }
            lastBtn = millis();
        }

        delay(15);
    }
}
