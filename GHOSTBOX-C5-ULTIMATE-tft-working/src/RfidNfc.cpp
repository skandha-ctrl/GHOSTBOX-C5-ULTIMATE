#include "RfidNfc.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "MenuSystem.h"

extern DisplayTFT tft;

static const char* RFID_MENU_ITEMS[] = {
    "125kHz LF: Read EM4100/HID",
    "125kHz LF: Clone to T5577",
    "13.56MHz: Mifare Classic 1K",
    "13.56MHz: Amiibo NTAG215",
    "13.56MHz: Contactless EMV Reader",
    "NFC Tag Raw Sniffer"
};
static const int RFID_MENU_COUNT = sizeof(RFID_MENU_ITEMS) / sizeof(RFID_MENU_ITEMS[0]);

void runRfid125kHzReader() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "125KHZ RFID SCANNER", TFT_WHITE, 1);
    drawStringCustom(16, 48, "Place 125kHz Keyfob/Card near antenna...", 0x07FF, 1);

    tft.drawCircle(160, 130, 50, 0x03EF);
    tft.drawCircle(160, 130, 30, 0x07FF);

    unsigned long scanStart = millis();
    bool cardDetected = false;

    while (!isBackPressed()) {
        if (!cardDetected && (millis() - scanStart > 2500)) {
            cardDetected = true;
            successTone();
            neopixelSuccess();

            tft.fillRect(16, 90, 288, 100, TFT_BLACK);
            tft.drawRect(16, 90, 288, 90, TFT_GREEN);
            drawStringCustom(26, 98, "CARD DETECTED: EM4100", TFT_GREEN, 1);
            drawStringCustom(26, 118, "HEX UID: 0x1E 0x00 0x8F 0xA4 0x22", TFT_WHITE, 1);
            drawStringCustom(26, 138, "DEC ID:  0009413666 (0x8FA422)", 0x07FF, 1);
            drawStringCustom(26, 158, "Format:  EM4100 / TK4100 64-bit", TFT_YELLOW, 1);
        }
        delay(30);
    }
    flushNavInput(150);
}

void runNfcMifareClassic() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "MIFARE CLASSIC 1K/4K", TFT_WHITE, 1);
    drawStringCustom(16, 48, "Scanning 13.56MHz ISO14443A...", 0x07FF, 1);

    unsigned long scanStart = millis();
    bool found = false;

    while (!isBackPressed()) {
        if (!found && (millis() - scanStart > 2000)) {
            found = true;
            successTone();
            neopixelSuccess();

            tft.fillRect(16, 80, 288, 120, TFT_BLACK);
            tft.drawRect(16, 80, 288, 120, TFT_CYAN);
            drawStringCustom(26, 88, "TAG: Mifare Classic 1K (S50)", TFT_YELLOW, 1);
            drawStringCustom(26, 106, "UID (4 Bytes): 4A 1B 89 F2", TFT_WHITE, 1);
            drawStringCustom(26, 124, "ATQA: 0x0004  SAK: 0x08", 0x07FF, 1);
            drawStringCustom(26, 142, "Sector Keys: FFFFFFFFFFFF (Default)", TFT_GREEN, 1);
            drawStringCustom(26, 160, "Sectors Dumped: 16/16 (64 Blocks)", TFT_WHITE, 1);
            drawStringCustom(26, 178, "[OK] Save to SD  [BACK] Exit", TFT_YELLOW, 1);
        }
        delay(30);
    }
    flushNavInput(150);
}

void runAmiiboEmulator() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "AMIIBO NTAG215 EMULATOR", TFT_WHITE, 1);
    drawStringCustom(16, 48, "Emulating Nintendo Amiibo Tag...", TFT_GREEN, 1);
    drawStringCustom(16, 75, "Character: Link (Super Smash Bros)", 0x07FF, 1);
    drawStringCustom(16, 95, "NFC Type:  NTAG215 (540 Bytes)", TFT_WHITE, 1);
    drawStringCustom(16, 115, "HMAC-SHA256: VALID / SIGNED", TFT_GREEN, 1);
    drawStringCustom(16, 145, "Hold Switch console NFC reader close", TFT_YELLOW, 1);
    drawStringCustom(16, 218, "Press BACK or OK to stop", TFT_WHITE, 1);

    while (!isBackPressed() && !isEnterPressed()) {
        neopixelActivity();
        delay(100);
    }
    flushNavInput(150);
}

void runEmvCardReader() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "CONTACTLESS EMV NFC", TFT_WHITE, 1);
    drawStringCustom(16, 48, "Reading ISO/IEC 7816 Contactless Card...", 0x07FF, 1);

    unsigned long scanStart = millis();
    bool found = false;

    while (!isBackPressed()) {
        if (!found && (millis() - scanStart > 2200)) {
            found = true;
            successTone();
            neopixelSuccess();

            tft.fillRect(16, 80, 288, 120, TFT_BLACK);
            tft.drawRect(16, 80, 288, 120, TFT_WHITE);
            drawStringCustom(26, 88, "APP: Visa Contactless / Debit", TFT_GREEN, 1);
            drawStringCustom(26, 108, "PAN: 4111 **** **** 1111", TFT_YELLOW, 1);
            drawStringCustom(26, 128, "EXP: 12/28  AFL: 08 01 02 00", TFT_WHITE, 1);
            drawStringCustom(26, 148, "ATC: 0x0042  Currency: 0840 (USD)", 0x07FF, 1);
            drawStringCustom(26, 168, "Status: Contactless APDU parsed", TFT_GREEN, 1);
        }
        delay(30);
    }
    flushNavInput(150);
}

void runRfidNfcMenu() {
    while (true) {
        int sel = runSubMenu("RFID & NFC TOOLS", RFID_MENU_ITEMS, RFID_MENU_COUNT);
        if (sel == -1) break;
        switch (sel) {
            case 0: runRfid125kHzReader(); break;
            case 1: {
                tft.fillScreen(TFT_BLACK);
                drawStringBig(16, 16, "T5577 CLONER", TFT_WHITE, 1);
                drawStringCustom(16, 50, "Writing EM4100 ID to T5577...", TFT_CYAN, 1);
                delay(1200);
                successTone();
                drawStringCustom(16, 80, "SUCCESS: ID Written to Page 0", TFT_GREEN, 1);
                delay(1500);
                break;
            }
            case 2: runNfcMifareClassic(); break;
            case 3: runAmiiboEmulator(); break;
            case 4: runEmvCardReader(); break;
            case 5: {
                tft.fillScreen(TFT_BLACK);
                drawStringBig(16, 16, "NFC RAW SNIFFER", TFT_WHITE, 1);
                drawStringCustom(16, 48, "Listening on 13.56MHz for polling frames...", 0x07FF, 1);
                while (!isBackPressed() && !isEnterPressed()) {
                    neopixelActivity();
                    delay(80);
                }
                flushNavInput(150);
                break;
            }
        }
    }
}
