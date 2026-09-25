#include "WifiScanner.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "SoundUtils.h"
#include "WifiCore.h"
#include <vector>

// ═════════════════════════════════════════════════════════════════════════════
// Configuration
// ═════════════════════════════════════════════════════════════════════════════
#define MAX_NETWORKS     40    // Network list limit (protects stack)
#define VISIBLE_LINES    6     // Visible lines in list

// ═════════════════════════════════════════════════════════════════════════════
// TABLA OUI — Fabricantes comunes
// ═════════════════════════════════════════════════════════════════════════════
struct OuiEntry {
    uint32_t prefix;
    const char* vendor;
};

static const OuiEntry OUI_TABLE[] = {
    // ── Huawei ──
    {0x001882, "Huawei"},   {0x00E0FC, "Huawei"},   {0x0CFC18, "Huawei"},
    {0xE00630, "Huawei"},   {0xF0A0B1, "Huawei"},   {0x404F42, "Huawei"},
    {0x7066B9, "Huawei"},   {0xC4A1AE, "Huawei"},   {0x001E10, "Huawei"},
    {0x002568, "Huawei"},   {0x70723C, "Huawei"},   {0xACE215, "Huawei"},

    // ── Sercomm ──
    {0x0022F7, "Sercomm"},  {0x14C03E, "Sercomm"},  {0xA84E3F, "Sercomm"},
    {0x5004B8, "Sercomm"},  {0x74884E, "Sercomm"},

    // ── Arcadyan ──
    {0x0015E0, "Arcadyan"}, {0x002308, "Arcadyan"}, {0x60310F, "Arcadyan"},
    {0x507E5D, "Arcadyan"},

    // ── Askey ──
    {0x001D7E, "Askey"},    {0xC85195, "Askey"},    {0x889676, "Askey"},

    // ── Nokia / Alcatel ──
    {0x00603E, "Nokia"},    {0x1CD446, "Nokia"},    {0xF8E4FB, "Nokia"},
    {0x00194B, "Nokia"},    {0x80D4A5, "Nokia"},

    // ── Fiberhome ──
    {0x3086F1, "Fiberhome"},{0xD876E7, "Fiberhome"},

    // ── Arris ──
    {0x0015CE, "Arris"},    {0x0026CE, "Arris"},    {0x54A619, "Arris"},
    {0xB83F5D, "Arris"},    {0x00248C, "Arris"},

    // ── Technicolor ──
    {0x00147F, "Technicolor"},{0x002417, "Technicolor"},{0xC42795, "Technicolor"},
    {0x6C5697, "Technicolor"},{0xD44B5E, "Technicolor"},

    // ── Hitron ──
    {0x008A5A, "Hitron"},   {0x688F2E, "Hitron"},   {0x749D8F, "Hitron"},

    // ── ZTE ──
    {0x0015EB, "ZTE"},      {0x344B50, "ZTE"},      {0xD0154A, "ZTE"},
    {0x4C16F1, "ZTE"},

    // ── TP-Link ──
    {0x14D864, "TP-Link"},  {0x40ED00, "TP-Link"},  {0x68DDB7, "TP-Link"},
    {0x002719, "TP-Link"},  {0x14CC20, "TP-Link"},  {0x50C7BF, "TP-Link"},
    {0xE894F6, "TP-Link"},  {0xA842A1, "TP-Link"},

    // ── Mercusys ──
    {0x50D4F7, "Mercusys"}, {0xB4B024, "Mercusys"},

    // ── Xiaomi ──
    {0x8CBEBE, "Xiaomi"},   {0x28E31F, "Xiaomi"},   {0x508F4C, "Xiaomi"},
    {0xF0B429, "Xiaomi"},   {0x64B473, "Xiaomi"},   {0x74510F, "Xiaomi"},

    // ── Apple ──
    {0x000393, "Apple"},    {0xACBC32, "Apple"},    {0xF45C89, "Apple"},
    {0xA45E60, "Apple"},    {0x001B63, "Apple"},

    // ── Samsung ──
    {0x0012FB, "Samsung"},  {0x34BE00, "Samsung"},  {0x5C0A5B, "Samsung"},
    {0xE8508B, "Samsung"},  {0x001D25, "Samsung"},
};
static const int OUI_COUNT = sizeof(OUI_TABLE) / sizeof(OuiEntry);

// ═════════════════════════════════════════════════════════════════════════════
// HELPERS
// ═════════════════════════════════════════════════════════════════════════════

// Convierte "AA:BB:CC:DD:EE:FF" → 0xAABBCC (primeros 3 bytes)
static uint32_t macToOui(const String& mac) {
    if (mac.length() < 8) return 0;
    char buf[9];
    buf[0] = mac[0]; buf[1] = mac[1];
    buf[2] = mac[3]; buf[3] = mac[4];
    buf[4] = mac[6]; buf[5] = mac[7];
    buf[6] = '\0';
    return (uint32_t) strtoul(buf, nullptr, 16);
}

// Lookup in the tabla OUI
static String lookupVendor(const String& mac) {
    uint32_t oui = macToOui(mac);
    for (int i = 0; i < OUI_COUNT; i++) {
        if (OUI_TABLE[i].prefix == oui) return String(OUI_TABLE[i].vendor);
    }
    return "Unknown";
}

// Texto legible of encriptación
static String authToString(uint8_t type) {
    switch (type) {
        case WIFI_AUTH_OPEN:            return "OPEN";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK:        return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3";
        default:                        return "?";
    }
}

// Short abbreviation (2-3 chars) for list
static String authToShort(uint8_t type) {
    switch (type) {
        case WIFI_AUTH_OPEN:            return "OP";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "W1";
        case WIFI_AUTH_WPA2_PSK:        return "W2";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "W2";
        case WIFI_AUTH_WPA3_PSK:        return "W3";
        case WIFI_AUTH_WPA2_WPA3_PSK:   return "W3";
        default:                        return "?";
    }
}

// Color according to security level
static uint16_t authToColor(uint8_t type) {
    switch (type) {
        case WIFI_AUTH_OPEN:            return TFT_RED;
        case WIFI_AUTH_WEP:             return TFT_ORANGE;
        case WIFI_AUTH_WPA_PSK:         return TFT_YELLOW;
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:    return TFT_GREEN;
        case WIFI_AUTH_WPA3_PSK:
        case WIFI_AUTH_WPA2_WPA3_PSK:   return TFT_CYAN;
        default:                        return TFT_WHITE;
    }
}

// Canal WiFi → frecuencia in MHz (Dual Band 2.4GHz + 5GHz)
static int channelToFreq(int ch) {
    if (ch >= 1 && ch <= 13) return 2407 + ch * 5;
    if (ch == 14) return 2484;
    if (ch >= 36 && ch <= 165) return 5000 + ch * 5;
    return 0;
}

// RSSI → 0-4 bars
static int rssiToBars(int rssi) {
    if (rssi >= -55) return 4;
    if (rssi >= -67) return 3;
    if (rssi >= -75) return 2;
    if (rssi >= -85) return 1;
    return 0;
}

// Color of the bars according to fuerza
static uint16_t barsColor(int bars) {
    if (bars >= 3) return TFT_GREEN;
    if (bars == 2) return TFT_YELLOW;
    return TFT_RED;
}

// draws 4 bloques verticales tipo señal celular
static void drawSignalBars(int x, int y, int bars) {
    const int bw = 3;       // width of each bloque
    const int gap = 2;      // separación
    const int heights[4] = {4, 8, 12, 16};
    uint16_t onColor = barsColor(bars);
    for (int i = 0; i < 4; i++) {
        int bx = x + i * (bw + gap);
        int by = y + (16 - heights[i]);
        if (i < bars) tft.fillRect(bx, by, bw, heights[i], onColor);
        else          tft.drawRect(bx, by, bw, heights[i], UI_ACCENT);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// DETAILS SCREEN
// ═════════════════════════════════════════════════════════════════════════════
static void showDetails(const NetworkObservation& net) {
    ledcWriteTone(0, 0);

    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 320, 30, TFT_WHITE);
    drawStringCustom(10, 6, "NETWORK DETAILS", TFT_BLACK, 2);

    int y = 36;

    // SSID (with detección of oculta)
    bool hidden = (net.ssid.length() == 0 || net.ssid == "<hidden>");
    String displaySsid = hidden ? "<HIDDEN>" : net.ssid;
    drawStringCustom(10, y, "SSID:", UI_ACCENT, 1);
    if (!hidden && getTextWidth(displaySsid, 2) > 300) {
        drawStringFit(10, y + 10, displaySsid, TFT_WHITE, 300, 1);
    } else {
        drawStringFit(10, y + 10, displaySsid,
                      hidden ? TFT_RED : TFT_WHITE, 300, 2);
    }
    y += 32;

    // Band + Radio Source
    drawStringCustom(10, y, "BAND / RADIO:", UI_ACCENT, 1);
    String bandStr = (net.band == BAND_5G) ? "5 GHz (C5 Coprocessor)" : "2.4 GHz (S3 Master)";
    uint16_t bandCol = (net.band == BAND_5G) ? TFT_CYAN : TFT_GREEN;
    drawStringCustom(10, y + 10, bandStr, bandCol, 2);
    y += 32;

    // Canal + frecuencia
    String chStr = "CH " + String(net.channel) + "  (" +
                   String(channelToFreq(net.channel)) + " MHz)";
    drawStringCustom(10, y, "CHANNEL:", UI_ACCENT, 1);
    drawStringCustom(10, y + 10, chStr, TFT_WHITE, 2);
    y += 32;

    // RSSI + bars
    drawStringCustom(10, y, "SIGNAL:", UI_ACCENT, 1);
    drawStringCustom(10, y + 10, String(net.rssi) + " dBm", TFT_WHITE, 2);
    drawSignalBars(150, y + 10, rssiToBars(net.rssi));
    y += 32;

    // MAC + fabricante
    String vendor = lookupVendor(net.bssidStr);
    drawStringCustom(10, y, "BSSID:", UI_ACCENT, 1);
    drawStringCustom(10, y + 10, net.bssidStr, TFT_YELLOW, 1);
    drawStringCustom(180, y + 10, "(" + vendor + ")",
                     vendor == "Unknown" ? UI_ACCENT : TFT_CYAN, 1);
    y += 20;

    // Seguridad
    drawStringCustom(10, y, "SECURITY:", UI_ACCENT, 1);
    drawStringCustom(10, y + 10, authToString(net.authType),
                     authToColor(net.authType), 2);

    // Footer
    tft.drawFastHLine(0, 215, 320, TFT_WHITE);
    drawStringCustom(10, 222, "OK/BACK TO RETURN", TFT_WHITE, 2);

    delay(400);
    while (!navEnterPressed() && !navBackPressed()) delay(10);
    while (navEnterPressed() || navBackPressed()) delay(5);
    delay(200);
    ledcWriteTone(0, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// animation of SCANNING
// ═════════════════════════════════════════════════════════════════════════════
static void drawScanningAnim(int tick) {
    const char dots[][4] = {"   ", ".  ", ".. ", "..."};
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(30, 80, 260, 70, TFT_WHITE);
    drawStringCustom(50, 92, "DUAL-BAND SCAN", TFT_CYAN, 2);
    drawStringCustom(50, 118, "Scanning 2.4G + 5G", TFT_WHITE, 1);
    drawStringCustom(200, 118, dots[tick % 4], TFT_WHITE, 2);
}

// ═════════════════════════════════════════════════════════════════════════════
// LIST of networks
// ═════════════════════════════════════════════════════════════════════════════
static void drawList(const NetworkObservation* nets, int n, int cursor, int scrollOffset) {
    // Header
    tft.fillRect(0, 0, 320, 25, TFT_WHITE);
    String hdr = "NETS: " + String(n) + "  SEL:" + String(cursor + 1);
    drawStringCustom(10, 5, hdr, TFT_BLACK, 2);

    // List
    for (int i = 0; i < VISIBLE_LINES; i++) {
        int idx = i + scrollOffset;
        int yPos = 35 + (i * 30);
        tft.fillRect(5, yPos - 4, 305, 26, TFT_BLACK);

        if (idx < n) {
            const NetworkObservation& net = nets[idx];
            bool isSelected = (cursor == idx);

            if (isSelected) tft.fillRect(5, yPos - 4, 305, 26, TFT_WHITE);
            uint16_t fg = isSelected ? TFT_BLACK : TFT_WHITE;

            // bars of señal
            drawSignalBars(8, yPos + 2, rssiToBars(net.rssi));

            // Band Badge [2.4G] or [5G]
            uint16_t bandTagColor = isSelected ? TFT_BLACK : (net.band == BAND_5G ? TFT_CYAN : TFT_GREEN);
            String bandTag = (net.band == BAND_5G) ? "5G" : "2G";
            drawStringCustom(32, yPos + 3, bandTag, bandTagColor, 1);

            // SSID (or <HIDDEN>)
            String label = (net.ssid.length() == 0 || net.ssid == "<hidden>") ? "<HIDDEN>" : net.ssid;
            uint16_t ssidColor = (label == "<HIDDEN>") ? TFT_RED : fg;
            
            if (getTextWidth(label, 2) <= 180) {
                drawStringCustom(56, yPos, label, ssidColor, 2);
            } else {
                drawStringFit(56, yPos + 4, label, ssidColor, 180, 1);
            }

            // Channel number
            String chLabel = "C" + String(net.channel);
            drawStringCustom(242, yPos + 3, chLabel, isSelected ? TFT_BLACK : UI_ACCENT, 1);

            // Encryption tag (colorcodeado)
            uint16_t encCol = isSelected ? TFT_BLACK : authToColor(net.authType);
            drawStringCustom(280, yPos, authToShort(net.authType), encCol, 2);
        }
    }

    // Scroll bar lateral (if there is more entradas of the visibles)
    tft.fillRect(314, 30, 4, 180, TFT_BLACK);
    int totalEntries = n;
    if (totalEntries > VISIBLE_LINES) {
        int barH = map(VISIBLE_LINES, 0, totalEntries, 20, 180);
        int barY = map(scrollOffset, 0, totalEntries - VISIBLE_LINES, 30, 210 - barH);
        tft.fillRect(314, barY, 4, barH, UI_ACCENT);
    }

    tft.drawFastHLine(0, 218, 320, UI_ACCENT);
    drawStringCustom(8, 225, "OK:DETAILS  BACK/OK(H):EXIT", UI_ACCENT, 1);
}

// ═════════════════════════════════════════════════════════════════════════════
// MAIN — SCAN of networks
// ═════════════════════════════════════════════════════════════════════════════
void runWifiScan() {
#if BUZZER_PIN >= 0
    // Init buzzer
    ledcSetup(0, 2000, 8);
    ledcAttachPin(BUZZER_PIN, 0);
    ledcWriteTone(0, 0);
#endif

    drawScanningAnim(0);

    std::vector<NetworkObservation> scanResults;
    int n = performUnifiedScan(scanResults, 6000);

    if (n <= 0) {
        tft.fillScreen(TFT_BLACK);
        drawStringCustom(40, 105, "NO NETS FOUND", TFT_RED, 3);
        delay(2000);
        return;
    }

    if (n > MAX_NETWORKS) n = MAX_NETWORKS;

    NetworkObservation networks[MAX_NETWORKS];
    for (int i = 0; i < n; i++) {
        networks[i] = scanResults[i];
    }

    // Ordenar by RSSI descendente (señal fuerte first)
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (networks[j].rssi < networks[j + 1].rssi) {
                NetworkObservation tmp = networks[j];
                networks[j]            = networks[j + 1];
                networks[j + 1]        = tmp;
            }
        }
    }

    // ── Main UI ─────────────────────────────────────────────────────────────
    int cursor = 0;
    int scrollOffset = 0;
    bool exitScan = false;
    bool needsRedraw = true;

    tft.fillScreen(TFT_BLACK);

    while (!exitScan) {
        if (navBackPressed() || isBackPressed()) {
            exitScan = true;
            while (isBackPressed()) delay(5);
            flushNavInput(60);
            continue;
        }

        if (needsRedraw) {
            drawList(networks, n, cursor, scrollOffset);
            needsRedraw = false;
        }

        // DOWN
        if (navDownPressed()) {
            cursor = (cursor + 1) % n;
            if (cursor < scrollOffset) scrollOffset = cursor;
            if (cursor >= scrollOffset + VISIBLE_LINES)
                scrollOffset = cursor - VISIBLE_LINES + 1;
            needsRedraw = true;
            beep(2000, 30);
            delay(70);
        }

        // UP
        if (navUpPressed()) {
            cursor = (cursor + n - 1) % n;
            if (cursor < scrollOffset) scrollOffset = cursor;
            if (cursor >= scrollOffset + VISIBLE_LINES)
                scrollOffset = cursor - VISIBLE_LINES + 1;
            needsRedraw = true;
            beep(2000, 30);
            delay(70);
        }

        // OK
        if (navEnterPressed()) {
            bool held = waitOkReleaseWasLong();
            if (held) {
                exitScan = true;
            } else {
                beep(1200, 50);
                showDetails(networks[cursor]);
                tft.fillScreen(TFT_BLACK);
                needsRedraw = true;
            }
            delay(250);
        }

        delay(10);
    }

    ledcWriteTone(0, 0);
}
