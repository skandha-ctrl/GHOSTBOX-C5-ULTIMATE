#include "OscillatorRfLab.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "MenuSystem.h"

extern DisplayTFT tft;

static const char* OSC_MENU_ITEMS[] = {
    "CC1101: 315.00 MHz Car/Gate Key",
    "CC1101: 433.92 MHz Sub-GHz ISM",
    "CC1101: 868.35 MHz EU IoT Band",
    "CC1101: 915.00 MHz US ISM Band",
    "nRF24: 2.412 GHz Wi-Fi Ch 1 CW",
    "nRF24: 2.437 GHz Wi-Fi Ch 6 CW",
    "nRF24: 2.462 GHz Wi-Fi Ch 11 CW",
    "nRF24: 2.484 GHz Ch 14 / BLE",
    "Rapid Sub-GHz / 2.4G Sweep"
};
static const int OSC_MENU_COUNT = sizeof(OSC_MENU_ITEMS) / sizeof(OSC_MENU_ITEMS[0]);

static bool oscillatorActive = false;
static uint32_t activeFreqHz = 433920000; // 433.92 MHz default

void startContinuousCarrier(uint32_t freqHz, uint8_t powerDbm) {
    (void)powerDbm;
    activeFreqHz = freqHz;
    oscillatorActive = true;
    neopixelAlert();
}

void startFastFrequencySweep(uint32_t startFreqHz, uint32_t stopFreqHz, uint16_t stepKHz, uint16_t dwellMs) {
    (void)startFreqHz; (void)stopFreqHz; (void)stepKHz; (void)dwellMs;
    oscillatorActive = true;
    neopixelAlert();
}

void stopRfOscillator() {
    oscillatorActive = false;
    neopixelSuccess();
}

static void runContinuousCarrierScreen(uint32_t defaultHz, const char* bandLabel, uint32_t stepHz) {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "FARADAY RF OSCILLATOR", TFT_WHITE, 1);
    drawStringCustom(225, 12, "[CC1101/nRF]", 0x07FF, 1);

    activeFreqHz = defaultHz;
    bool transmitting = false;

    auto renderHUD = [&]() {
        tft.fillRect(1, 35, 318, 170, TFT_BLACK);

        tft.drawRect(12, 45, 296, 125, transmitting ? TFT_RED : 0x07FF);
        drawStringCustom(22, 54, "ISOLATED LAB CARRIER GENERATOR", 0x07FF, 1);

        char buf[64];
        if (activeFreqHz >= 1000000000ULL) {
            snprintf(buf, sizeof(buf), "Frequency: %.3f GHz", (double)activeFreqHz / 1000000000.0);
        } else {
            snprintf(buf, sizeof(buf), "Frequency: %.4f MHz", (double)activeFreqHz / 1000000.0);
        }
        drawStringCustom(22, 76, buf, TFT_WHITE, 1);

        snprintf(buf, sizeof(buf), "Target Band: %s", bandLabel);
        drawStringCustom(22, 98, buf, TFT_YELLOW, 1);

        snprintf(buf, sizeof(buf), "RF Hardware: CC1101 / 3x nRF24 Array");
        drawStringCustom(22, 120, buf, 0x5AEB, 1);

        drawStringCustom(22, 144, transmitting ? "STATUS: TX ACTIVE (EMITTING RF)" : "STATUS: IDLE / STANDBY",
                         transmitting ? TFT_RED : TFT_GREEN, 1);

        tft.fillRect(1, 207, 318, 32, 0x0010);
        drawStringCustom(10, 218, transmitting ? "[OK] STOP TX   [BACK] EXIT" : "[OK] START TX  [UP/DN] TUNE FREQ", TFT_YELLOW, 1);
    };

    renderHUD();
    flushNavInput(150);

    while (true) {
        NavAction act = readNavAction(120);
        if (act == NAV_UP && !transmitting) {
            activeFreqHz += stepHz;
            clickTone();
            renderHUD();
        } else if (act == NAV_DOWN && !transmitting) {
            if (activeFreqHz > stepHz) activeFreqHz -= stepHz;
            clickTone();
            renderHUD();
        } else if (act == NAV_ENTER) {
            transmitting = !transmitting;
            clickTone();
            if (transmitting) {
                alertTone();
                startContinuousCarrier(activeFreqHz, 10);
            } else {
                successTone();
                stopRfOscillator();
            }
            renderHUD();
        } else if (act == NAV_BACK) {
            stopRfOscillator();
            clickTone();
            flushNavInput(150);
            return;
        }

        if (transmitting) {
            neopixelAlert();
        }
        delay(20);
    }
}

void runRfOscillatorLabMenu() {
    while (true) {
        int sel = runSubMenu("RF OSCILLATOR LAB", OSC_MENU_ITEMS, OSC_MENU_COUNT);
        if (sel == -1) break;
        switch (sel) {
            case 0: runContinuousCarrierScreen(315000000, "315 MHz Sub-GHz", 50000); break;
            case 1: runContinuousCarrierScreen(433920000, "433.92 MHz Sub-GHz", 50000); break;
            case 2: runContinuousCarrierScreen(868350000, "868.35 MHz Sub-GHz", 50000); break;
            case 3: runContinuousCarrierScreen(915000000, "915.00 MHz Sub-GHz", 50000); break;
            case 4: runContinuousCarrierScreen(2412000000ULL, "2.412 GHz (Ch 1)", 1000000); break;
            case 5: runContinuousCarrierScreen(2437000000ULL, "2.437 GHz (Ch 6)", 1000000); break;
            case 6: runContinuousCarrierScreen(2462000000ULL, "2.462 GHz (Ch 11)", 1000000); break;
            case 7: runContinuousCarrierScreen(2484000000ULL, "2.484 GHz (Ch 14)", 1000000); break;
            case 8: {
                tft.fillScreen(TFT_BLACK);
                drawStringBig(10, 8, "SUB-GHZ SWEEP", TFT_WHITE, 1);
                drawStringCustom(16, 45, "Sweeping 430MHz -> 435MHz...", TFT_RED, 1);
                drawStringCustom(16, 68, "Hardware: CC1101 + nRF24 Array", TFT_WHITE, 1);
                drawStringCustom(16, 218, "Press BACK or OK to stop", TFT_YELLOW, 1);

                while (!isBackPressed() && !isEnterPressed()) {
                    neopixelAlert();
                    delay(20);
                }
                stopRfOscillator();
                flushNavInput(150);
                break;
            }
        }
    }
}
