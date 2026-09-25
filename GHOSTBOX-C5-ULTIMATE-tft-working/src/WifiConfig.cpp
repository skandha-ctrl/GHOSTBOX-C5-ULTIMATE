#include "WifiConfig.h"

#include "VirtualKeyboard.h"

#include "DisplayTFT.h"
#include <WiFi.h>

#include <Preferences.h>

#include "PepeDraw.h"

#include "Pins.h"

#include "SoundUtils.h"



extern DisplayTFT tft;


// ═══════════════════════════════════════════════════════════════════════════

// CONFIG

// ═══════════════════════════════════════════════════════════════════════════

#define WIFI_NS              "wificonfig"

#define KEY_SSID             "ssid"

#define KEY_PASS             "pass"



#define MAX_NETWORKS         20

#define VISIBLE_ROWS         5

#define CONNECT_TIMEOUT_MS   15000

#define AUTOCONNECT_TIMEOUT  10000



// ═══════════════════════════════════════════════════════════════════════════

// HELPERS NVS

// ═══════════════════════════════════════════════════════════════════════════



static String loadSavedSSID() {

    Preferences p;

    if (!p.begin(WIFI_NS, true)) return "";

    String s = p.getString(KEY_SSID, "");

    p.end();

    return s;

}



static String loadSavedPass() {

    Preferences p;

    if (!p.begin(WIFI_NS, true)) return "";

    String s = p.getString(KEY_PASS, "");

    p.end();

    return s;

}



static void saveCredentials(const String& ssid, const String& pass) {

    Preferences p;

    p.begin(WIFI_NS, false);

    p.putString(KEY_SSID, ssid);

    p.putString(KEY_PASS, pass);

    p.end();

}



// ═══════════════════════════════════════════════════════════════════════════

// API EXPORT

// ═══════════════════════════════════════════════════════════════════════════



void wifiConfigForget() {

    Preferences p;

    p.begin(WIFI_NS, false);

    p.clear();

    p.end();

}



String wifiConfigGetSavedSSID() {

    return loadSavedSSID();

}



bool wifiConfigHasSaved() {

    return loadSavedSSID().length() > 0;

}



// ═══════════════════════════════════════════════════════════════════════════

// HELPERS UI

// ═══════════════════════════════════════════════════════════════════════════



static int rssiBars(int rssi) {

    if (rssi >= -55) return 4;

    if (rssi >= -70) return 3;

    if (rssi >= -85) return 2;

    if (rssi >= -95) return 1;

    return 0;

}



static String encTypeStr(wifi_auth_mode_t auth) {

    switch (auth) {

        case WIFI_AUTH_OPEN: return "OPEN";

        case WIFI_AUTH_WEP:  return "WEP";

        case WIFI_AUTH_WPA_PSK: return "WPA";

        case WIFI_AUTH_WPA2_PSK: return "WPA2";

        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA2";

        case WIFI_AUTH_WPA3_PSK: return "WPA3";

        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA3";

        default: return "?";

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// screen: AUTO-CONNECT (intenta credenciales saved)

// ═══════════════════════════════════════════════════════════════════════════



static bool tryAutoConnect() {

    String ssid = loadSavedSSID();

    String pass = loadSavedPass();

    if (ssid.length() == 0) return false;



    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);

    drawStringBig(10, 8, "WIFI CONFIG", UI_MAIN, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    drawStringCustom(10, 50, "Connecting to saved network:", UI_MAIN, 1);

    String ssidDisp = truncateToWidth(ssid, 300, 1, FONT_BIG);
    drawStringBig(10, 70, ssidDisp, UI_SELECT, 1);


    drawStringCustom(10, 100, "BACK/OK(H): cancel & pick other", UI_ACCENT, 1);


    // Spinner

    int spinX = 160, spinY = 160;

    int barX = 10, barY = 200, barW = 300, barH = 8;

    tft.drawRect(barX, barY, barW, barH, UI_ACCENT);



    WiFi.mode(WIFI_STA);

    WiFi.disconnect(true);

    delay(100);

    WiFi.begin(ssid.c_str(), pass.c_str());



    unsigned long start = millis();

    int spinFrame = 0;

    bool userCanceled = false;

    unsigned long okPressStart = 0;

    bool okHeld = false;



    while (millis() - start < AUTOCONNECT_TIMEOUT) {

        wl_status_t st = WiFi.status();

        if (st == WL_CONNECTED) {

            // clear spinner and mostrar success

            tft.fillScreen(TFT_BLACK);

            tft.drawRect(0, 0, 320, 240, UI_MAIN);

            drawStringBig(10, 8, "WIFI CONFIG", UI_MAIN, 1);

            tft.drawFastHLine(0, 30, 320, UI_ACCENT);



            drawStringBig(80, 70, "CONNECTED", TFT_GREEN, 2);

            drawStringCustom(10, 110, "Net:", UI_ACCENT, 1);

            drawStringFit(50, 110, ssid, UI_MAIN, 260, 1);
            drawStringCustom(10, 124, "IP:", UI_ACCENT, 1);

            drawStringCustom(50, 124, WiFi.localIP().toString(), UI_MAIN, 1);



            beep(2400, 50); delay(30);

            beep(3000, 80);

            delay(1000);

            return true;

        }



        // Progress + spinner

        float p = (float)(millis() - start) / AUTOCONNECT_TIMEOUT;

        int fw = (int)((barW - 2) * p);

        tft.fillRect(barX + 1, barY + 1, fw, barH - 2, UI_SELECT);



        // animation spinner (asterisco rotando)

        const char* frames[] = {"|", "/", "-", "\\"};

        tft.fillRect(spinX, spinY, 16, 16, TFT_BLACK);

        drawStringBig(spinX, spinY, String(frames[spinFrame]), UI_MAIN, 2);

        spinFrame = (spinFrame + 1) % 4;



        if (navBackPressed()) {
            userCanceled = true;
            break;
        }

        // OK hold for cancelar
        if (navEnterPressed()) {

            if (!okHeld) { okPressStart = millis(); okHeld = true; }

            else if (millis() - okPressStart > 500) {

                userCanceled = true;

                break;

            }

        } else {

            okHeld = false;

        }



        delay(150);

    }



    WiFi.disconnect(true);



    if (userCanceled) {

        beep(1200, 80);

        while (navEnterPressed() || navBackPressed()) delay(5);
        delay(100);

        return false;   // user wants to pick another network

    }



    // Timeout without conectar

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, TFT_RED);

    drawStringBig(40, 70, "CONNECTION FAILED", TFT_RED, 2);

    drawStringCustom(20, 120, "Could not connect to:", UI_MAIN, 1);

    drawStringFit(20, 134, ssid, UI_ACCENT, 280, 1);
    drawStringCustom(20, 160, "Possible causes:", UI_ACCENT, 1);

    drawStringCustom(30, 174, "- Password changed", UI_ACCENT, 1);

    drawStringCustom(30, 186, "- Network out of range", UI_ACCENT, 1);

    drawStringCustom(20, 215, "OK/BACK: pick another network", UI_MAIN, 1);


    beep(800, 100); delay(50);

    beep(800, 100);



    while (!navEnterPressed() && !navBackPressed()) delay(20);
    while (navEnterPressed() || navBackPressed()) delay(5);
    delay(100);

    return false;

}



// ═══════════════════════════════════════════════════════════════════════════

// screen: SCAN of networks

// ═══════════════════════════════════════════════════════════════════════════



struct ScanNet {

    String  ssid;

    int     rssi;

    int     channel;

    wifi_auth_mode_t auth;

};



static ScanNet scanNetworks[MAX_NETWORKS];

static int     scanNetCount = 0;



static void scanNetworksFn() {

    scanNetCount = 0;



    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);

    drawStringBig(10, 8, "WIFI CONFIG", UI_MAIN, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);

    drawStringCustom(10, 50, "Scanning networks...", UI_MAIN, 1);

    drawStringCustom(10, 64, "Please wait ~8 seconds.", UI_ACCENT, 1);



    int barX = 10, barY = 100, barW = 300, barH = 14;

    tft.drawRect(barX, barY, barW, barH, UI_ACCENT);



    WiFi.mode(WIFI_STA);

    WiFi.disconnect(true);

    delay(100);

    WiFi.scanNetworks(true, true);



    unsigned long start = millis();

    while (millis() - start < 8000) {

        float p = (float)(millis() - start) / 8000.0f;

        int fw = (int)((barW - 2) * p);

        tft.fillRect(barX + 1, barY + 1, fw, barH - 2, UI_SELECT);

        delay(150);

    }



    int n = WiFi.scanComplete();

    while (n == WIFI_SCAN_RUNNING) { delay(100); n = WiFi.scanComplete(); }

    if (n < 0) n = 0;

    if (n > MAX_NETWORKS) n = MAX_NETWORKS;



    for (int i = 0; i < n; i++) {

        scanNetworks[i].ssid = WiFi.SSID(i);

        if (scanNetworks[i].ssid.length() == 0) {

            scanNetworks[i].ssid = "<hidden>";

        }

        scanNetworks[i].rssi = WiFi.RSSI(i);

        scanNetworks[i].channel = WiFi.channel(i);

        scanNetworks[i].auth = WiFi.encryptionType(i);

    }

    scanNetCount = n;

    WiFi.scanDelete();



    // Ordenar by RSSI desc

    for (int i = 0; i < scanNetCount - 1; i++) {

        for (int j = 0; j < scanNetCount - 1 - i; j++) {

            if (scanNetworks[j].rssi < scanNetworks[j + 1].rssi) {

                ScanNet t = scanNetworks[j];

                scanNetworks[j] = scanNetworks[j + 1];

                scanNetworks[j + 1] = t;

            }

        }

    }



    beep(2400, 50);

}



// SCREEN: NETWORK SELECTION
// Returns: chosen index or -1 = rescan, -2 = cancel

static int selectNetwork() {

    int cursor = 0;

    int scrollOffset = 0;

    int total = scanNetCount + 1;   // +1 = RESCAN


    auto draw = [&]() {

        tft.fillScreen(TFT_BLACK);

        tft.drawRect(0, 0, 320, 240, UI_MAIN);

        drawStringBig(10, 8, "SELECT WIFI", UI_MAIN, 1);

        drawStringCustom(220, 12, "[" + String(scanNetCount) + " nets]",

                         UI_ACCENT, 1);

        tft.drawFastHLine(0, 30, 320, UI_ACCENT);



        const int rowH = 32;

        const int listY = 36;



        for (int i = 0; i < VISIBLE_ROWS; i++) {

            int idx = i + scrollOffset;

            if (idx >= total) break;

            int y = listY + i * rowH;

            bool sel = (idx == cursor);

            if (sel) tft.fillRect(5, y, 310, rowH - 2, UI_SELECT);



            uint16_t col1 = sel ? UI_BG : UI_MAIN;

            uint16_t col2 = sel ? UI_BG : UI_ACCENT;



            if (idx == scanNetCount) {

                drawStringCustom(15, y + 10, "RESCAN", col1, 2);

            } else {
                String s = scanNetworks[idx].ssid;
                if (getTextWidth(s, 2) <= 245) {
                    drawStringCustom(10, y + 4, s, col1, 2);
                } else {
                    drawStringFit(10, y + 7, s, col1, 245, 1);
                }


                String meta = encTypeStr(scanNetworks[idx].auth) +

                              "  CH" + String(scanNetworks[idx].channel) +

                              "  " + String(scanNetworks[idx].rssi) + "dBm";

                drawStringCustom(10, y + 20, meta, col2, 1);



                // Bars

                int bars = rssiBars(scanNetworks[idx].rssi);

                int bx = 280, by = 22;

                for (int b = 0; b < 4; b++) {

                    int bh = 3 + b * 2;

                    uint16_t c = (b < bars)

                        ? (sel ? UI_BG : (bars >= 3 ? TFT_GREEN :

                                          bars >= 2 ? TFT_YELLOW : TFT_ORANGE))

                        : (sel ? UI_BG : UI_ACCENT);

                    if (b < bars) tft.fillRect(bx + b*5, by - bh, 3, bh, c);

                    else          tft.drawRect(bx + b*5, by - bh, 3, bh, c);

                }

            }

        }



        if (total > VISIBLE_ROWS) {

            int barH = (VISIBLE_ROWS * 160) / total;

            int barY = 36 + (scrollOffset * (160 - barH)) / (total - VISIBLE_ROWS);

            tft.fillRect(314, barY, 4, barH, UI_ACCENT);

        }



        tft.drawFastHLine(0, 215, 320, UI_ACCENT);

        drawStringCustom(10, 222, "OK:SELECT  BACK/OK(H):CANCEL", UI_ACCENT, 1);
    };

    draw();



    while (true) {
        if (navBackPressed()) {
            beep(1000, 40);
            while (navBackPressed()) delay(5);
            delay(100);
            return -2;
        }
        if (navUpPressed()) {
            cursor = (cursor + total - 1) % total;

            if (cursor < scrollOffset) scrollOffset = cursor;

            if (cursor >= scrollOffset + VISIBLE_ROWS)

                scrollOffset = cursor - VISIBLE_ROWS + 1;

            beep(2100, 15);

            draw();

            delay(70);
        }

        if (navDownPressed()) {

            cursor = (cursor + 1) % total;

            if (cursor < scrollOffset) scrollOffset = cursor;

            if (cursor >= scrollOffset + VISIBLE_ROWS)

                scrollOffset = cursor - VISIBLE_ROWS + 1;

            beep(2100, 15);

            draw();

            delay(70);
        }

        if (navEnterPressed()) {

            bool held = waitOkReleaseWasLong();
            beep(held ? 1000 : 1800, 40);
            delay(100);
            if (held) return -2;                    // CANCEL
            if (cursor == scanNetCount) return -1;       // RESCAN
            return cursor;
        }

        delay(20);

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// CONECTAR with CREDENCIALES NUEVAS

// ═══════════════════════════════════════════════════════════════════════════



static bool connectWithCredentials(const String& ssid, const String& pass) {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);

    drawStringBig(10, 8, "WIFI CONFIG", UI_MAIN, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    drawStringCustom(10, 50, "Connecting to:", UI_MAIN, 1);

    String ssidDisp = truncateToWidth(ssid, 300, 1, FONT_BIG);
    drawStringBig(10, 70, ssidDisp, UI_SELECT, 1);


    int barX = 10, barY = 180, barW = 300, barH = 14;

    tft.drawRect(barX, barY, barW, barH, UI_ACCENT);



    WiFi.mode(WIFI_STA);

    WiFi.disconnect(true);

    delay(100);

    WiFi.begin(ssid.c_str(), pass.c_str());



    unsigned long start = millis();

    while (millis() - start < CONNECT_TIMEOUT_MS) {

        if (WiFi.status() == WL_CONNECTED) {

            tft.fillScreen(TFT_BLACK);

            tft.drawRect(0, 0, 320, 240, UI_MAIN);

            drawStringBig(80, 70, "CONNECTED!", TFT_GREEN, 2);

            drawStringCustom(10, 120, "Net:", UI_ACCENT, 1);

            drawStringFit(50, 120, ssid, UI_MAIN, 260, 1);
            drawStringCustom(10, 140, "IP:", UI_ACCENT, 1);

            drawStringCustom(50, 140, WiFi.localIP().toString(), UI_MAIN, 1);

            drawStringCustom(10, 160, "Credentials saved to NVS.",

                             TFT_GREEN, 1);



            beep(2400, 50); delay(30);

            beep(3000, 50); delay(30);

            beep(3600, 100);



            saveCredentials(ssid, pass);

            delay(1500);

            return true;

        }



        float p = (float)(millis() - start) / CONNECT_TIMEOUT_MS;

        int fw = (int)((barW - 2) * p);

        tft.fillRect(barX + 1, barY + 1, fw, barH - 2, UI_SELECT);

        delay(200);

    }



    // Timeout

    WiFi.disconnect(true);

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, TFT_RED);

    drawStringBig(40, 80, "FAILED", TFT_RED, 2);

    drawStringCustom(20, 130, "Could not connect.", UI_MAIN, 1);

    drawStringCustom(20, 146, "Incorrect password?", UI_ACCENT, 1);

    drawStringCustom(20, 220, "OK/BACK: Retry", UI_MAIN, 1);


    beep(800, 100); delay(50);

    beep(800, 100);



    while (!navEnterPressed() && !navBackPressed()) delay(20);
    while (navEnterPressed() || navBackPressed()) delay(5);
    delay(100);

    return false;

}



// MAIN ENTRY POINT

bool wifiConfigConnect() {
    // If already connected, return true
    if (WiFi.status() == WL_CONNECTED) return true;

    // 1. Try autoconnection with saved credentials
    if (wifiConfigHasSaved()) {
        if (tryAutoConnect()) return true;
        // If failed, fall back to manual selection
    }



    // 2. loop for scan + select + password

    while (true) {

        scanNetworksFn();



        if (scanNetCount == 0) {

            tft.fillScreen(TFT_BLACK);

            tft.drawRect(0, 0, 320, 240, UI_MAIN);

            drawStringBig(20, 80, "NO NETWORKS FOUND", TFT_RED, 1);

            drawStringCustom(20, 130, "No WiFi networks were detected.",

                             UI_ACCENT, 1);

            drawStringCustom(20, 220, "OK: retry  BACK: cancel",
                             UI_MAIN, 1);

            while (true) {

                if (navEnterPressed()) {

                    beep(2000, 40);

                    while (navEnterPressed() || navBackPressed()) delay(5);
                    delay(100);

                    break;

                }

                if (navBackPressed() || navUpPressed() || navDownPressed()) {
                    beep(1000, 80);

                    while (navBackPressed() || navUpPressed() ||
                           navDownPressed()) delay(5);

                    return false;

                }

                delay(20);

            }

            continue;

        }



        int choice = selectNetwork();

        if (choice == -1) continue;        // RESCAN

        if (choice == -2) return false;    // CANCEL



        ScanNet& net = scanNetworks[choice];



        // if the network is OPEN, no necesitamos password

        String pass = "";

        if (net.auth != WIFI_AUTH_OPEN) {

            pass = virtualKeyboardInput("WIFI PASSWORD",

                                          "Net: " + net.ssid,

                                          62,

                                          true);   // mask = true

            if (pass.length() == 0) {
                // User cancelled from keyboard
                continue;
            }
        }

        if (connectWithCredentials(net.ssid, pass)) {
            return true;
        }
        // If failed, loop back for another attempt

    }

}
