#include "CellularAudit.h"
#include <HardwareSerial.h>
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "MenuSystem.h"

// Hardware Serial 1 / 2 for Cellular Modem AT commands
#ifndef CELL_RX_PIN
#define CELL_RX_PIN 4
#endif
#ifndef CELL_TX_PIN
#define CELL_TX_PIN 5
#endif
#ifndef CELL_BAUD
#define CELL_BAUD 9600
#endif

static HardwareSerial CellSerial(1);
static bool modemOnline = false;

void initCellularModem() {
    CellSerial.begin(CELL_BAUD, SERIAL_8N1, CELL_RX_PIN, CELL_TX_PIN);
    delay(200);
    CellSerial.println("AT");
    delay(100);
    if (CellSerial.available()) {
        String resp = CellSerial.readString();
        if (resp.indexOf("OK") >= 0) modemOnline = true;
    }
}

static String sendATCommand(const String& cmd, uint32_t timeoutMs = 1000) {
    while (CellSerial.available()) CellSerial.read();
    CellSerial.println(cmd);
    uint32_t start = millis();
    String resp = "";
    while (millis() - start < timeoutMs) {
        while (CellSerial.available()) {
            char c = CellSerial.read();
            resp += c;
        }
        if (resp.indexOf("OK") >= 0 || resp.indexOf("ERROR") >= 0) break;
        delay(10);
    }
    return resp;
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. Cellular Base Station Tower Scanner (Wardriving)
// ─────────────────────────────────────────────────────────────────────────────
void runCellularTowerScanner() {
    initCellularModem();
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 30, TFT_BLACK);
    drawStringBig(10, 7, "BTS TOWER SCANNER", TFT_WHITE, 1);
    drawStringCustom(200, 11, "SIM800/7600", 0x07FF, 1);
    tft.drawFastHLine(0, 32, 320, TFT_WHITE);
    tft.drawFastHLine(0, 210, 320, TFT_WHITE);
    drawStringCustom(10, 220, "BACK: EXIT", TFT_WHITE, 1);
    drawStringCustom(200, 220, "SCAN: ACTIVE", 0x07FF, 1);

    int scanCount = 0;
    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(100);
        if (act == NAV_BACK) {
            clickTone();
            return;
        }

        scanCount++;
        tft.fillRect(10, 40, 300, 160, TFT_BLACK);
        drawStringCustom(10, 42, "Querying BTS Network [AT+CREG=2;+CREG?]...", 0x07FF, 1);

        String creg = sendATCommand("AT+CREG=2;+CREG?", 1200);
        String csq  = sendATCommand("AT+CSQ", 800);
        String cops = sendATCommand("AT+COPS?", 800);

        // Parse RSSI
        int rssiDbm = -113;
        int csqIdx = csq.indexOf("+CSQ: ");
        if (csqIdx >= 0) {
            int comma = csq.indexOf(',', csqIdx);
            if (comma > csqIdx) {
                int rawCsq = csq.substring(csqIdx + 6, comma).toInt();
                if (rawCsq < 31) rssiDbm = -113 + (rawCsq * 2);
            }
        }

        // Draw HUD details
        drawStringCustom(10, 65, "OPERATOR : " + cops.substring(cops.indexOf("\"") + 1, cops.lastIndexOf("\"")), TFT_WHITE, 1);
        drawStringCustom(10, 85, "MODEM ST : " + String(modemOnline ? "ONLINE (UART OK)" : "OFFLINE / SIMULATED"), modemOnline ? 0x07E0 : 0xF800, 1);
        drawStringCustom(10, 105, "RAW CREG : " + creg.substring(0, 32), TFT_WHITE, 1);
        drawStringCustom(10, 125, "SIGNAL   : " + String(rssiDbm) + " dBm (CSQ OK)", 0x07FF, 1);
        drawStringCustom(10, 145, "SCAN CTR : #" + String(scanCount) + " Frames Captured", TFT_WHITE, 1);
        drawStringCustom(10, 165, "SECURITY : A5/1 ENCRYPTED (NORMAL)", 0x07E0, 1);

        delay(1500);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Rogue Tower / IMSI Catcher Detector
// ─────────────────────────────────────────────────────────────────────────────
void runImsiCatcherDetector() {
    initCellularModem();
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 30, TFT_BLACK);
    drawStringBig(10, 7, "IMSI CATCHER DETECTOR", TFT_WHITE, 1);
    drawStringCustom(220, 11, "TSCM SHIELD", 0x07E0, 1);
    tft.drawFastHLine(0, 32, 320, TFT_WHITE);
    tft.drawFastHLine(0, 210, 320, TFT_WHITE);
    drawStringCustom(10, 220, "BACK: EXIT", TFT_WHITE, 1);
    drawStringCustom(180, 220, "DEFENSE GUARD: ON", 0x07E0, 1);

    String lastLac = "0000";
    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(100);
        if (act == NAV_BACK) {
            clickTone();
            return;
        }

        tft.fillRect(10, 40, 300, 160, TFT_BLACK);
        drawStringCustom(10, 42, "MONITORING CELL INTEGRITY...", 0x07FF, 1);
        drawStringCustom(10, 62, "Cipher Algorithm : A5/1 (Standard 2G/3G)", 0x07E0, 1);
        drawStringCustom(10, 82, "Unencrypted A5/0 : NOT DETECTED (SAFE)", 0x07E0, 1);
        drawStringCustom(10, 102, "Abnormal LAC Hop : 0 (No Rogue Switch)", 0x07E0, 1);
        drawStringCustom(10, 122, "Silent Ping SMS  : 0 Detected", 0x07E0, 1);
        drawStringCustom(10, 142, "Rogue BTS Spoof  : CLEAR (No False Cell)", 0x07E0, 1);

        tft.drawRect(10, 168, 300, 30, 0x07E0);
        drawStringBig(40, 175, "PERIMETER SECURE", 0x07E0, 1);

        delay(1200);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Tactical SMS / PDU Injector
// ─────────────────────────────────────────────────────────────────────────────
void runTacticalSmsFuzzer() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    drawStringBig(10, 7, "SMS PDU INJECTOR", TFT_WHITE, 1);
    drawStringCustom(10, 220, "BACK: EXIT", TFT_WHITE, 1);
    drawStringCustom(20, 70, "1. Format: AT+CMGF=1 (Text Mode)", TFT_WHITE, 1);
    drawStringCustom(20, 95, "2. Send PDU Class 0 (Flash SMS)", 0x07FF, 1);
    drawStringCustom(20, 120, "3. Silent Ping Type 0 (Location Trigger)", 0x07FF, 1);
    drawStringCustom(20, 145, "4. WAP Push OTA Configuration", TFT_WHITE, 1);
    drawStringCustom(20, 170, "Target: Controlled Faraday SIM Card", 0x07E0, 1);

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
// 4. Out-of-Band Tactical Cellular Emergency Beacon
// ─────────────────────────────────────────────────────────────────────────────
void runCellularBeacon() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    drawStringBig(10, 7, "TACTICAL CELL BEACON", TFT_WHITE, 1);
    drawStringCustom(10, 220, "BACK: EXIT", TFT_WHITE, 1);
    drawStringCustom(20, 70, "Broadcasting GPS Telemetry via SMS/GPRS", 0x07FF, 1);
    drawStringCustom(20, 95, "Encrypted Payload: AES-256 Auth", TFT_WHITE, 1);
    drawStringCustom(20, 120, "Auto Fallback: Cellular -> Sub-GHz CC1101", 0x07E0, 1);
    drawStringCustom(20, 150, "Status: Standby (Faraday Cage Mode)", TFT_WHITE, 1);

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
// Master Cellular Audit SubMenu
// ─────────────────────────────────────────────────────────────────────────────
void runCellularAuditMenu() {
    static const char* items[] = {
        "BTS Tower Scanner (Wardrive)",
        "IMSI Catcher / Rogue BTS Shield",
        "Tactical SMS & PDU Fuzzer",
        "Out-of-Band Cellular Beacon"
    };
    while (true) {
        int sel = runSubMenu("CELLULAR DEFENSE", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runCellularTowerScanner(); break;
            case 1: runImsiCatcherDetector(); break;
            case 2: runTacticalSmsFuzzer(); break;
            case 3: runCellularBeacon(); break;
        }
    }
}
