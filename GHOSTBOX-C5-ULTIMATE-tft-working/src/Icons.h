#ifndef ICONS_H
#define ICONS_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// SYSTEM ICONS (16x16 bitmaps)
// · Monochrome 16x16 bitmaps for category and tool HUDs
// ═══════════════════════════════════════════════════════════════════════════

enum IconID {
    ICON_WIFI = 0,
    ICON_BLE,
    ICON_SUBGHZ,
    ICON_IR,
    ICON_ATTACK,
    ICON_MONITOR,
    ICON_TOOLS,
    ICON_SETTINGS,
    ICON_INFO,
    ICON_TOTAL
};

void drawIcon16(int16_t x, int16_t y, IconID id, uint16_t color = 0xFFFF);

#endif
