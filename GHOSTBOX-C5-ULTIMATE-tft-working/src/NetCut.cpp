#include "NetCut.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include <WiFi.h>

extern DisplayTFT tft;

struct LanHost {
    char ip[16];
    char mac[18];
    char vendor[32];
    bool cut;
};

static LanHost hosts[] = {
    { "192.168.1.1",  "A4:2B:B0:11:22:33", "Gateway (Router)", false },
    { "192.168.1.104","BC:D1:D3:44:55:66", "iPhone 15 Pro",   false },
    { "192.168.1.115","50:C7:BF:77:88:99", "Smart TV",        false },
    { "192.168.1.142","E8:48:B8:AA:BB:CC", "Windows PC",      false }
};
static const int HOST_COUNT = sizeof(hosts) / sizeof(hosts[0]);

void runNetCut() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "NETCUT ARP SPOOFER", TFT_WHITE, 1);

    int cursor = 1;

    auto render = [&]() {
        tft.fillRect(1, 35, 318, 170, TFT_BLACK);
        drawStringCustom(16, 42, "SELECT TARGET IP TO CUT / RESTORE:", 0x07FF, 1);

        for (int i = 0; i < HOST_COUNT; i++) {
            int y = 65 + i * 32;
            bool sel = (i == cursor);

            if (sel) {
                tft.fillRect(12, y, 296, 28, TFT_WHITE);
                drawStringCustom(18, y + 6, hosts[i].ip, TFT_BLACK, 1);
                drawStringCustom(130, y + 6, hosts[i].vendor, TFT_BLACK, 1);
                drawStringCustom(260, y + 6, hosts[i].cut ? "CUT" : "OK", hosts[i].cut ? TFT_RED : TFT_BLACK, 1);
            } else {
                tft.drawRect(12, y, 296, 28, 0x3186);
                drawStringCustom(18, y + 6, hosts[i].ip, TFT_WHITE, 1);
                drawStringCustom(130, y + 6, hosts[i].vendor, 0x07FF, 1);
                drawStringCustom(260, y + 6, hosts[i].cut ? "CUT" : "OK", hosts[i].cut ? TFT_RED : TFT_GREEN, 1);
            }
        }

        tft.fillRect(1, 207, 318, 32, 0x0010);
        drawStringCustom(10, 218, "UP/DN: MOVE  OK: TOGGLE CUT", TFT_YELLOW, 1);
        drawStringCustom(260, 218, "BACK: EXIT", TFT_WHITE, 1);
    };

    render();
    flushNavInput(150);

    while (true) {
        NavAction act = readNavAction(130);
        if (act == NAV_UP) {
            if (cursor > 0) cursor--;
            render();
            clickTone();
        } else if (act == NAV_DOWN) {
            if (cursor < HOST_COUNT - 1) cursor++;
            render();
            clickTone();
        } else if (act == NAV_ENTER) {
            hosts[cursor].cut = !hosts[cursor].cut;
            clickTone();
            if (hosts[cursor].cut) {
                neopixelAlert();
                alertTone();
            } else {
                neopixelSuccess();
                successTone();
            }
            render();
        } else if (act == NAV_BACK) {
            clickTone();
            flushNavInput(150);
            return;
        }
        delay(10);
    }
}
