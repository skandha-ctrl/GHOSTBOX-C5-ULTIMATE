#include <Arduino.h>
#include <SPI.h>
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "MenuSystem.h"
#include "Pins.h"
#include "Settings.h"
#include "NVSStore.h"
#include "SplashScreen.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "PeripheralTools.h"
#include "SharedSpi.h"

// ==============================================================================
// GHOSTBOX C5 ULTIMATE · 4-IN-1 UNIFIED PLATFORM
// Target MCU: ESP32-C5 (RISC-V 32-bit Dual-Band 2.4GHz & 5.0GHz Wi-Fi 6 + BLE)
// ==============================================================================

DisplayTFT tft;

static void bumpBootCount() {
    unsigned long bc = nvsGetULong("boot_cnt", 0);
    bc++;
    nvsSetULong("boot_cnt", bc);
    Serial.printf("[NVS] Boot count: %lu\n", bc);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[BOOT] Starting GHOSTBOX C5 ULTIMATE...");
    Serial.flush();

    // 1. Navigation Controls
    Serial.println("[BOOT] 1. Initializing Input Controls...");
    Serial.flush();
    initInput();

    // 2. Sound
    Serial.println("[BOOT] 2. Initializing Sound...");
    Serial.flush();
    initSound();

    // 3. Shared SPI Bus Arbitration (Hold all slave CS high)
    Serial.println("[BOOT] 3. Initializing Shared SPI Pins...");
    Serial.flush();
    sharedSpiInitPins(true);   // All CS HIGH first
    sharedSpiBeginMainBus();

    // ── SD CARD SPI-MODE KICK ────────────────────────────────────────────────
    // SD cards power up in native SD mode and drive MISO LOW until they receive
    // 74+ dummy clock pulses with CS HIGH. Without this, SD MISO kills nRF24s.
    if (SD_CS_PIN >= 0) {
        pinMode(SD_CS_PIN, OUTPUT);
        digitalWrite(SD_CS_PIN, HIGH);          // CS HIGH = deselected
        SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
        for (int i = 0; i < 10; i++) SPI.transfer(0xFF); // 80 clocks
        SPI.endTransaction();
        digitalWrite(SD_CS_PIN, HIGH);
        Serial.println("[BOOT] SD SPI-mode kick done");
    }
    // ────────────────────────────────────────────────────────────────────────

    // ── GPIO-13 (MISO) HARD DIAGNOSTIC ─────────────────────────────────────
    // Tests whether GPIO 13 is shorted to GND or actively driven by a device.
    // Driven HIGH → reads LOW = hard short to GND on the wire/PCB
    // Driven HIGH → reads HIGH = line is free (good)
    // INPUT_PULLUP → reads LOW = a peripheral is actively sinking MISO
    // INPUT_PULLUP → reads HIGH = line is floating free (good)
    Serial.println("[MISO-DIAG] --- GPIO-13 (MISO) BUS HEALTH TEST ---");
    // Step 1: drive HIGH
    pinMode(MISO_PIN, OUTPUT);
    digitalWrite(MISO_PIN, HIGH);
    delayMicroseconds(200);
    int v1 = digitalRead(MISO_PIN);
    Serial.printf("[MISO-DIAG] OUTPUT=HIGH  -> read=%d  (%s)\n", v1,
                  v1 ? "OK - line can go HIGH" : "FAIL - SHORT TO GND!");
    // Step 2: drive LOW
    digitalWrite(MISO_PIN, LOW);
    delayMicroseconds(200);
    int v2 = digitalRead(MISO_PIN);
    Serial.printf("[MISO-DIAG] OUTPUT=LOW   -> read=%d\n", v2);
    // Step 3: input with pullup (SPI peripheral may fight this)
    pinMode(MISO_PIN, INPUT_PULLUP);
    delayMicroseconds(500);
    int v3 = digitalRead(MISO_PIN);
    Serial.printf("[MISO-DIAG] INPUT_PULLUP -> read=%d  (%s)\n", v3,
                  v3 ? "OK - line floats HIGH" : "FAIL - device actively pulling LOW!");
    // Step 4: input with pulldown (confirm device is driving, not just open)
    pinMode(MISO_PIN, INPUT_PULLDOWN);
    delayMicroseconds(500);
    int v4 = digitalRead(MISO_PIN);
    Serial.printf("[MISO-DIAG] INPUT_PULLDOWN -> read=%d\n", v4);
    if (!v1) {
        Serial.println("[MISO-DIAG] >> VERDICT: HARD SHORT TO GND - check soldering/wiring on MISO line!");
    } else if (!v3) {
        Serial.println("[MISO-DIAG] >> VERDICT: PERIPHERAL DRIVING MISO LOW - likely nRF24 not tristating (check VCC/power to modules)");
    } else {
        Serial.println("[MISO-DIAG] >> VERDICT: MISO line is CLEAN - SPI should work");
    }
    Serial.println("[MISO-DIAG] --- END ---");
    // Restore as SPI input with pullup
    pinMode(MISO_PIN, INPUT_PULLUP);
    // ────────────────────────────────────────────────────────────────────────

#if defined(TFT_LED_PIN) && (TFT_LED_PIN >= 0)
    pinMode(TFT_LED_PIN, OUTPUT);
    digitalWrite(TFT_LED_PIN, HIGH);
#endif

    // 4. Peripherals
    Serial.println("[BOOT] 4. Initializing Peripherals...");
    Serial.flush();
    initPeripherals();

    // 5. NVS Preferences
    Serial.println("[BOOT] 5. Initializing NVS...");
    Serial.flush();
    nvsBegin();
    initSettings();
    bumpBootCount();

    // 6. TFT Display Driver Initialization
    Serial.println("[BOOT] 6. Starting TFT Display Driver...");
    Serial.flush();

    sharedSpiPrepareDisplay(true);
    tft.begin();
    tft.setRotation(1);

    // CRITICAL: After TFT_eSPI::begin(), re-assert the shared SPI bus ownership.
    // ILI9488 does NOT tristate MISO when CS is HIGH, so we NEVER connect TFT MISO.
    // TFT_MISO is set to -1 in build flags. Re-init here ensures nRF24 + SD
    // can use GPIO 13 (MISO) without the TFT driver interfering.
    SPI.end();
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
    pinMode(MISO_PIN, INPUT_PULLUP);
    sharedSpiRelease(true);

    Serial.println("[BOOT] 7. Flashing test colors (RED -> GREEN -> BLUE)...");
    Serial.flush();
    tft.fillScreen(TFT_RED);
    delay(600);
    tft.fillScreen(TFT_GREEN);
    delay(600);
    tft.fillScreen(TFT_BLUE);
    delay(600);
    tft.fillScreen(TFT_BLACK);
    delay(200);

    Serial.println("[BOOT] 8. Running Splash Screen...");
    Serial.flush();
    runSplashScreen();

    // Hand over control to Main Carousel Menu (Runs forever)
    Serial.println("[BOOT] 9. Running Main Menu...");
    Serial.flush();
    runMainMenu();
}

void loop() {
    // Execution never reaches here; MenuSystem runs main loop
    delay(1000);
}
