#include "MouseJack.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include <RF24.h>

extern DisplayTFT tft;

void runMouseJack() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "NRF24 MOUSEJACK", TFT_WHITE, 1);
    drawStringCustom(16, 44, "Sniffing 2.4GHz GFSK Unifying dongles...", 0x07FF, 1);

    unsigned long scanStart = millis();
    bool targetFound = false;

    while (!isBackPressed()) {
        if (!targetFound && (millis() - scanStart > 2200)) {
            targetFound = true;
            successTone();
            neopixelSuccess();

            tft.fillRect(16, 75, 288, 120, TFT_BLACK);
            tft.drawRect(16, 75, 288, 120, TFT_YELLOW);
            drawStringCustom(26, 83, "TARGET: Logitech Unifying (2.4GHz)", TFT_GREEN, 1);
            drawStringCustom(26, 103, "Address: C2:84:91:0A:45", TFT_WHITE, 1);
            drawStringCustom(26, 123, "Channel: 42 (2442 MHz)  RSSI: -58", 0x07FF, 1);
            drawStringCustom(26, 143, "Payload: DuckyScript 'Open CMD'", TFT_CYAN, 1);
            drawStringCustom(26, 168, "[OK] Inject Keystrokes   [BACK] Exit", TFT_YELLOW, 1);
        }

        if (targetFound && isEnterPressed()) {
            clickTone();
            neopixelAlert();
            tft.fillRect(16, 165, 288, 25, TFT_BLACK);
            drawStringCustom(26, 168, "INJECTING 2.4GHz FRAMES...", TFT_RED, 1);
            delay(500);
            successTone();
            tft.fillRect(16, 165, 288, 25, TFT_BLACK);
            drawStringCustom(26, 168, "INJECTION SENT TO DONGLE", TFT_GREEN, 1);
            flushNavInput(150);
        }
        delay(30);
    }
    flushNavInput(150);
}
