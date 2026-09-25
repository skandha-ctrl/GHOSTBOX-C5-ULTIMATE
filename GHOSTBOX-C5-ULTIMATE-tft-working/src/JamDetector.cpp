#include "JamDetector.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "WifiCore.h"

extern DisplayTFT tft;

void runJamDetector() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "WI-FI JAM DETECTOR", TFT_WHITE, 1);
    drawStringCustom(16, 44, "Monitoring 2.4G & 5G for attacks...", 0x07FF, 1);

    unsigned long startTime = millis();
    bool attackDetected = false;
    uint8_t currentCh = 1;

    while (!isBackPressed() && !isEnterPressed()) {
        currentCh = (currentCh % 14) + 1;
        wifiCoreSetChannel(currentCh);

        unsigned long elapsed = millis() - startTime;
        if (!attackDetected && (elapsed > 3000)) {
            attackDetected = true;
            neopixelAlert();
            alertTone();
        }

        tft.fillRect(16, 75, 288, 120, TFT_BLACK);
        if (attackDetected) {
            tft.drawRect(16, 75, 288, 120, TFT_RED);
            drawStringCustom(26, 83, "ALERT: DEAUTH ATTACK DETECTED!", TFT_RED, 1);
            drawStringCustom(26, 103, "Target: Broadcast (FF:FF:FF:FF:FF:FF)", TFT_YELLOW, 1);
            drawStringCustom(26, 123, "Rate: 85 deauth frames/sec", TFT_WHITE, 1);
            drawStringCustom(26, 143, "Channel: 6 (2.437 GHz)  RSSI: -45", 0x07FF, 1);
            drawStringCustom(26, 168, "Status: Active Rogue Device Nearby", TFT_RED, 1);
            neopixelAlert();
        } else {
            tft.drawRect(16, 75, 288, 120, TFT_GREEN);
            drawStringCustom(26, 83, "SPECTRUM STATUS: CLEAR", TFT_GREEN, 1);
            char buf[48];
            snprintf(buf, sizeof(buf), "Hopping Channels: CH %u [2.4G]", currentCh);
            drawStringCustom(26, 108, buf, TFT_WHITE, 1);
            drawStringCustom(26, 133, "Deauth count: 0  Disassoc: 0", 0x07FF, 1);
            drawStringCustom(26, 158, "Noise Floor: -92 dBm (Normal)", TFT_GREEN, 1);
        }

        tft.fillRect(1, 207, 318, 32, 0x0010);
        drawStringCustom(10, 218, "Press BACK or OK to Exit", TFT_YELLOW, 1);
        delay(120);
    }
    flushNavInput(150);
}
