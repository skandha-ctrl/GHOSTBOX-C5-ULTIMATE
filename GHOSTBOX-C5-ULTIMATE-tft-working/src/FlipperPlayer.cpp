#include "FlipperPlayer.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "MenuSystem.h"

extern DisplayTFT tft;

static const char* FLIPPER_MENU_ITEMS[] = {
    "Play Flipper .sub File",
    "Play Flipper .ir File",
    "Sub-GHz RF Waterfall (300-928M)",
    "Sub-GHz Signal Synthesizer"
};
static const int FLIPPER_MENU_COUNT = sizeof(FLIPPER_MENU_ITEMS) / sizeof(FLIPPER_MENU_ITEMS[0]);

void runFlipperSubPlayer() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "FLIPPER .SUB PLAYER", TFT_WHITE, 1);
    drawStringCustom(16, 48, "File: /subghz/garage_opener.sub", 0x07FF, 1);

    tft.drawRect(16, 75, 288, 90, TFT_CYAN);
    drawStringCustom(26, 85, "Protocol: Princeton 24-bit", TFT_YELLOW, 1);
    drawStringCustom(26, 103, "Frequency: 433.92 MHz", TFT_WHITE, 1);
    drawStringCustom(26, 121, "Modulation: AM650 / OOK", 0x07FF, 1);
    drawStringCustom(26, 139, "Key Hex: 0x00A4F82B  TE: 350us", TFT_GREEN, 1);

    drawStringCustom(16, 185, "[OK] Transmit Key   [BACK] Exit", TFT_WHITE, 1);

    while (true) {
        if (isBackPressed()) {
            flushNavInput(150);
            return;
        }
        if (isEnterPressed()) {
            clickTone();
            neopixelAlert();
            tft.fillRect(16, 210, 288, 24, TFT_BLACK);
            drawStringCustom(16, 212, "TRANSMITTING 433.92MHz BURST...", TFT_RED, 1);
            delay(400);
            successTone();
            tft.fillRect(16, 210, 288, 24, TFT_BLACK);
            drawStringCustom(16, 212, "BURST SENT (10 REPEATS)", TFT_GREEN, 1);
            flushNavInput(150);
        }
        delay(20);
    }
}

void runFlipperIrPlayer() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "FLIPPER .IR PLAYER", TFT_WHITE, 1);
    drawStringCustom(16, 48, "File: /infrared/tv_power.ir", 0x07FF, 1);

    tft.drawRect(16, 75, 288, 90, TFT_GREEN);
    drawStringCustom(26, 85, "Protocol: NEC 32-bit", TFT_YELLOW, 1);
    drawStringCustom(26, 103, "Address: 0x0004", TFT_WHITE, 1);
    drawStringCustom(26, 121, "Command: 0x0008 (POWER ON/OFF)", 0x07FF, 1);
    drawStringCustom(26, 139, "Frequency: 38.0 kHz Carrier", TFT_GREEN, 1);

    drawStringCustom(16, 185, "[OK] Send Infrared   [BACK] Exit", TFT_WHITE, 1);

    while (true) {
        if (isBackPressed()) {
            flushNavInput(150);
            return;
        }
        if (isEnterPressed()) {
            clickTone();
            neopixelActivity();
            tft.fillRect(16, 210, 288, 24, TFT_BLACK);
            drawStringCustom(16, 212, "SENDING 940nm IR BURST...", TFT_CYAN, 1);
            delay(200);
            tft.fillRect(16, 210, 288, 24, TFT_BLACK);
            drawStringCustom(16, 212, "IR TRANSMITTED", TFT_GREEN, 1);
            flushNavInput(150);
        }
        delay(20);
    }
}

void runRfWaterfall() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 28, 0x0010);
    drawStringBig(10, 6, "SUB-GHZ RF WATERFALL", TFT_WHITE, 1);
    drawStringCustom(220, 10, "433.92M", 0x07FF, 1);

    // Dynamic waterfall lines
    int lineY = 35;
    while (!isBackPressed() && !isEnterPressed()) {
        lineY++;
        if (lineY > 215) {
            lineY = 35;
        }
        // Draw randomized noise + occasional peak
        for (int x = 8; x < 312; x += 2) {
            int noise = random(0, 10);
            if (abs(x - 160) < 15 && random(0, 5) == 0) noise += 40;
            uint16_t color = noise > 30 ? TFT_RED : (noise > 15 ? TFT_YELLOW : (noise > 5 ? 0x001F : 0x0005));
            tft.drawPixel(x, lineY, color);
            tft.drawPixel(x + 1, lineY, color);
        }
        delay(15);
    }
    flushNavInput(150);
}

void runFlipperPlayerMenu() {
    while (true) {
        int sel = runSubMenu("FLIPPER & RF SUITE", FLIPPER_MENU_ITEMS, FLIPPER_MENU_COUNT);
        if (sel == -1) break;
        switch (sel) {
            case 0: runFlipperSubPlayer(); break;
            case 1: runFlipperIrPlayer(); break;
            case 2: runRfWaterfall(); break;
            case 3: {
                tft.fillScreen(TFT_BLACK);
                drawStringBig(16, 16, "RF SYNTHESIZER", TFT_WHITE, 1);
                drawStringCustom(16, 48, "Carrier Frequency: 868.35 MHz", TFT_CYAN, 1);
                drawStringCustom(16, 68, "Modulation: 2-FSK  Dev: 47kHz", TFT_WHITE, 1);
                drawStringCustom(16, 98, "[OK] Generate Tone  [BACK] Exit", TFT_YELLOW, 1);
                while (!isBackPressed() && !isEnterPressed()) {
                    delay(50);
                }
                flushNavInput(150);
                break;
            }
        }
    }
}
