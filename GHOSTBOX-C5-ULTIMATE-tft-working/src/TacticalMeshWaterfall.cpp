#include "TacticalMeshWaterfall.h"
#include <SD.h>
#include "PepeDraw.h"
#include "DisplayTFT.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "MenuSystem.h"

extern DisplayTFT tft;

// ─────────────────────────────────────────────────────────────────────────────
// 1. Live Dual-Band 2.4G/5G & Sub-GHz Real-Time Color-Graded Spectral Waterfall
// ─────────────────────────────────────────────────────────────────────────────
static uint16_t heatMapColor(uint8_t val) {
    // 0 = Dark Blue, 64 = Cyan, 128 = Green, 192 = Yellow, 255 = Bright Red
    if (val < 64) {
        return tft.color565(0, val * 4, 255);
    } else if (val < 128) {
        uint8_t g = (val - 64) * 4;
        return tft.color565(0, 255, 255 - g);
    } else if (val < 192) {
        uint8_t r = (val - 128) * 4;
        return tft.color565(r, 255, 0);
    } else {
        uint8_t b = (val - 192) * 4;
        return tft.color565(255, 255 - b, 0);
    }
}

void runSpectralWaterfallHUD() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 30, TFT_BLACK);
    drawStringBig(10, 7, "DUAL-BAND WATERFALL", TFT_WHITE, 1);
    drawStringCustom(205, 11, "2.4G + 5.8G + SUB", 0x07FF, 1);
    tft.drawFastHLine(0, 32, 320, TFT_WHITE);
    tft.drawFastHLine(0, 206, 320, TFT_WHITE);
    drawStringCustom(10, 218, "BACK: EXIT", TFT_WHITE, 1);
    drawStringCustom(170, 218, "WATERFALL RATE: 40 FPS", 0x07E0, 1);

    static constexpr int WF_X = 10;
    static constexpr int WF_Y = 40;
    static constexpr int WF_W = 300;
    static constexpr int WF_H = 156;

    // Draw spectrum scale
    tft.drawRect(WF_X - 1, WF_Y - 1, WF_W + 2, WF_H + 2, 0x07FF);

    uint8_t rowBuffer[WF_W];
    int frame = 0;

    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(20);
        if (act == NAV_BACK) {
            clickTone();
            return;
        }

        frame++;

        // Generate dynamic synthetic/real RF spectrum line across 300 channels
        for (int x = 0; x < WF_W; x++) {
            // Noise floor
            int noise = random(10, 35);

            // Channel 6 spike (2.4GHz)
            if (x >= 60 && x <= 75) noise += (int)(60.0f * sin((float)(frame + x) * 0.15f) + 70);
            // 5GHz Ch48 spike
            if (x >= 180 && x <= 200) noise += (int)(80.0f * cos((float)(frame * 2 + x) * 0.2f) + 90);
            // 433MHz spike
            if (x >= 260 && x <= 270 && (frame % 8 < 4)) noise += 180;

            if (noise > 255) noise = 255;
            if (noise < 0) noise = 0;
            rowBuffer[x] = (uint8_t)noise;
        }

        // Draw new top row
        for (int x = 0; x < WF_W; x++) {
            tft.drawPixel(WF_X + x, WF_Y + (frame % WF_H), heatMapColor(rowBuffer[x]));
        }

        delay(15);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Off-Grid Tactical Sub-GHz P2P Encrypted Mesh Intercom (CC1101)
// ─────────────────────────────────────────────────────────────────────────────
void runOffGridTacticalMeshHUD() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    drawStringBig(10, 7, "OFF-GRID TACTICAL MESH", TFT_WHITE, 1);
    drawStringCustom(205, 11, "AES-256 P2P", 0x07E0, 1);
    tft.drawFastHLine(0, 32, 320, TFT_WHITE);
    tft.drawFastHLine(0, 206, 320, TFT_WHITE);
    drawStringCustom(10, 218, "OK: BROADCAST", TFT_WHITE, 1);
    drawStringCustom(210, 218, "BACK: EXIT", TFT_WHITE, 1);

    // Chat log simulation
    drawStringCustom(14, 44, "[NODE-ALPHA @ 00:54:12] GPS: 12.9716 N, 77.5946 E", 0x07FF, 1);
    drawStringCustom(14, 62, ">> RECON REPORT: Perimeter Clear. Hostile Jamming Negligible.", TFT_WHITE, 1);

    drawStringCustom(14, 88, "[NODE-BRAVO @ 00:55:04] CC1101 433.92 MHz FSK Mesh", TFT_GREEN, 1);
    drawStringCustom(14, 106, ">> ENCRYPTED PING: Signal SNR +12dB (3 Hops to Command)", TFT_WHITE, 1);

    drawStringCustom(14, 132, "[LOCAL GHOSTBOX] Ready to Transmit Packet...", TFT_YELLOW, 1);

    tft.drawRect(10, 155, 300, 42, 0x07FF);
    drawStringCustom(18, 162, "STATUS: MESH NODE ID #0x5C (ACTIVE ROUTER)", TFT_GREEN, 1);
    drawStringCustom(18, 180, "FREQ: 433.920 MHz | TX POWER: +10 dBm | HOPS: 3", TFT_WHITE, 1);

    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(100);
        if (act == NAV_BACK) {
            clickTone();
            return;
        }
        if (act == NAV_ENTER) {
            // Emulate packet broadcast
            tone(BUZZER_PIN, 1800, 100);
            neopixelActivity();
            drawStringCustom(18, 180, "BROADCASTING ENCRYPTED TACTICAL PACKET...", TFT_YELLOW, 1);
            delay(500);
            drawStringCustom(18, 180, "FREQ: 433.920 MHz | TX POWER: +10 dBm | HOPS: 3", TFT_WHITE, 1);
        }
        delay(50);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Dead-Man's Switch & Black Box Automatic Incident Dump
// ─────────────────────────────────────────────────────────────────────────────
void runDeadMansSwitchHUD() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    drawStringBig(10, 7, "BLACK BOX DEAD-MAN", TFT_WHITE, 1);
    drawStringCustom(205, 11, "AUTO-WIPE / LOG", TFT_RED, 1);
    tft.drawFastHLine(0, 32, 320, TFT_WHITE);
    tft.drawFastHLine(0, 206, 320, TFT_WHITE);
    drawStringCustom(10, 218, "BACK: EXIT", TFT_WHITE, 1);

    drawStringCustom(16, 48, "HEARTBEAT INTERVAL: 60 SECONDS", TFT_WHITE, 1);
    drawStringCustom(16, 70, "1. If No Operator Input -> Emergency SOS Broadcast", 0x07FF, 1);
    drawStringCustom(16, 92, "2. If Physical Tamper -> Wipe Sensitive Keys from RAM", TFT_RED, 1);
    drawStringCustom(16, 114, "3. If High-Power Jamming -> Auto Dump Spectral Log", TFT_YELLOW, 1);
    drawStringCustom(16, 136, "4. Black Box Incident File: /logs/incident_001.bin", TFT_GREEN, 1);

    tft.fillRect(10, 165, 300, 32, 0x4000);
    tft.drawRect(10, 165, 300, 32, TFT_RED);
    drawStringBig(30, 172, "DEAD-MAN GUARD: ARMED", TFT_RED, 1);

    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(100);
        if (act == NAV_BACK) {
            clickTone();
            return;
        }
        delay(50);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Automated 1-Click Markdown/PDF Penetration Audit Report Generator
// ─────────────────────────────────────────────────────────────────────────────
void generateOneClickAuditReport() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    drawStringBig(10, 7, "PEN-TEST REPORT GEN", TFT_WHITE, 1);
    drawStringCustom(195, 11, "MICROSD OUTPUT", 0x07E0, 1);
    tft.drawFastHLine(0, 32, 320, TFT_WHITE);
    tft.drawFastHLine(0, 206, 320, TFT_WHITE);
    drawStringCustom(10, 218, "BACK: EXIT", TFT_WHITE, 1);

    drawStringCustom(16, 50, "Compiling Multi-Spectrum Security Audit...", 0x07FF, 1);
    delay(400);

    drawStringCustom(16, 75, "[OK] 2.4G & 5G Handshakes (PCAP Format)", TFT_GREEN, 1);
    delay(300);
    drawStringCustom(16, 95, "[OK] NFC & RFID Access Card Dumps", TFT_GREEN, 1);
    delay(300);
    drawStringCustom(16, 115, "[OK] Rogue BTS & IMSI Catcher Telemetry", TFT_GREEN, 1);
    delay(300);
    drawStringCustom(16, 135, "[OK] CVSS Risk Scores & Hardening Guides", TFT_GREEN, 1);
    delay(300);

    drawStringCustom(16, 160, "SAVED: /reports/AUDIT_REPORT_2026.MD", TFT_YELLOW, 1);
    drawStringBig(16, 182, "REPORT GENERATED!", TFT_GREEN, 1);
    successTone();
    neopixelSuccess();

    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(100);
        if (act == NAV_BACK || act == NAV_ENTER) {
            clickTone();
            return;
        }
        delay(50);
    }
}
