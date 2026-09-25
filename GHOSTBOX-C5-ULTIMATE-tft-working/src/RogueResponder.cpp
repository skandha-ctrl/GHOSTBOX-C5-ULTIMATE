#include "RogueResponder.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include <WiFi.h>

extern DisplayTFT tft;

void runRogueResponder() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "LLMNR / NBT RESPONDER", TFT_WHITE, 1);
    drawStringCustom(16, 44, "Listening on UDP 5355 & 137...", 0x07FF, 1);

    tft.drawRect(12, 70, 296, 125, 0x3186);
    drawStringCustom(20, 78, "POISON STATUS: ACTIVE", TFT_GREEN, 1);
    drawStringCustom(20, 98, "Victim IP: 192.168.1.142 (WIN-OFFICE)", TFT_YELLOW, 1);
    drawStringCustom(20, 118, "Query: \\\\SHARE-SERVER\\DOCS", TFT_WHITE, 1);
    drawStringCustom(20, 138, "Captured: NTLMv2 User 'Administrator'", TFT_CYAN, 1);
    drawStringCustom(20, 158, "Hash: Admin::WIN:112233445566...", TFT_GREEN, 1);
    drawStringCustom(20, 178, "Saved to /logs/hashes.txt", 0x5AEB, 1);

    drawStringCustom(16, 218, "Press BACK or OK to Exit", TFT_YELLOW, 1);

    unsigned long lastAnim = 0;
    while (!isBackPressed() && !isEnterPressed()) {
        unsigned long now = millis();
        if (now - lastAnim > 800) {
            lastAnim = now;
            neopixelActivity();
        }
        delay(30);
    }
    flushNavInput(150);
}
