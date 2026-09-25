#include "C5Status.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "WifiCore.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "SharedSpi.h"
#include <SPI.h>
#include <SD.h>
#include <RF24.h>
#include <Wire.h>
#include <TinyGPSPlus.h>

extern DisplayTFT tft;

// ═══════════════════════════════════════════════════════════════════════════
// ESP32-C5 SMART AUTO-DETECTION & PROBING
// ═══════════════════════════════════════════════════════════════════════════
static bool probeC5(String &responseOut, unsigned long &rttMs) {
    // 1. Try currently configured pins
    wifiCoreInit();
    while (C5_SERIAL.available()) C5_SERIAL.read();

    unsigned long start = millis();
    C5_SERIAL.println("PING");

    while (millis() - start < 600) {
        if (C5_SERIAL.available()) {
            String line = C5_SERIAL.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                rttMs = millis() - start;
                responseOut = line;
                if (line.indexOf("+PONG") >= 0 || line.indexOf("PONG") >= 0 || line.indexOf("+READY") >= 0) {
                    return true;
                }
            }
        }
        delay(5);
    }

    rttMs = millis() - start;
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// INDIVIDUAL COMPONENT PROBERS
// ═══════════════════════════════════════════════════════════════════════════
static bool probeNRF(int8_t cePin, int8_t csnPin) {
    if (cePin < 0 || csnPin < 0) return false;
    
    // Ensure all SPI CS lines are deselected (HIGH)
    sharedSpiRelease(true);
    pinMode(csnPin, OUTPUT);
    digitalWrite(csnPin, HIGH);
    if (cePin >= 0) {
        pinMode(cePin, OUTPUT);
        digitalWrite(cePin, LOW);
    }
    pinMode(MISO_PIN, INPUT_PULLUP);
    delay(10);

    // Test raw SPI transfer
    SPISettings nrfTestSettings(1000000, MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(nrfTestSettings);
    digitalWrite(csnPin, LOW);
    delayMicroseconds(10);
    uint8_t status = SPI.transfer(0x00); // NOP -> reads STATUS register
    uint8_t config = SPI.transfer(0x00); // reads CONFIG register
    digitalWrite(csnPin, HIGH);
    SPI.endTransaction();

    RF24 radio(cePin, csnPin, NRF_SPI_SPEED);
    bool ok = radio.begin(&SPI);
    bool chip = false;
    if (ok) {
        radio.setAutoAck(false);
        radio.setPALevel(RF24_PA_LOW);
        radio.setDataRate(RF24_1MBPS);
        radio.stopListening();
        chip = radio.isChipConnected();
        radio.powerDown();
    }
    sharedSpiRelease(true);
    int misoVal = digitalRead(MISO_PIN);
    Serial.printf("[SPI-PROBE] nRF24 (CE:%d CSN:%d) -> MISO_PIN:%d Val:%d | STATUS:0x%02X CONFIG:0x%02X | begin:%s chip:%s\n",
                  cePin, csnPin, MISO_PIN, misoVal, status, config, ok ? "OK" : "FAIL", chip ? "CONNECTED" : "NOT_DETECTED");
    return chip;
}

static bool probeCC1101() {
    if (CC1101_CSN_PIN < 0) return false;

    pinMode(CC1101_CSN_PIN, OUTPUT);
    digitalWrite(CC1101_CSN_PIN, HIGH);
    sharedSpiRelease(true);

    SPISettings ccSpi(2000000, MSBFIRST, SPI_MODE0);
    SPI.beginTransaction(ccSpi);
    digitalWrite(CC1101_CSN_PIN, LOW);
    delayMicroseconds(15);
    SPI.transfer(0x31 | 0xC0); // Read version register burst
    uint8_t ver = SPI.transfer(0x00);
    digitalWrite(CC1101_CSN_PIN, HIGH);
    SPI.endTransaction();
    sharedSpiRelease(true);

    return (ver != 0x00 && ver != 0xFF);
}

static bool probeSD() {
    if (SD_CS_PIN < 0) return false;
    sharedSpiPrepareSd();
    SD.end();
    delay(10);
    bool ok = SD.begin(SD_CS_PIN, SPI, 4000000);
    sharedSpiRelease(true);
    return ok;
}

static bool probeGPS() {
    if (GPS_RX_PIN < 0 || GPS_TX_PIN < 0) return false;
    
    HardwareSerial gpsPort(2);
    gpsPort.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    unsigned long start = millis();
    bool received = false;
    while (millis() - start < 120) {
        if (gpsPort.available() > 0) {
            received = true;
            break;
        }
        delay(5);
    }
    gpsPort.end();
    return received;
}

static bool probeI2C(uint8_t addr) {
    if (I2C_SDA_PIN < 0 || I2C_SCL_PIN < 0) return false;
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}

// ═══════════════════════════════════════════════════════════════════════════
// 1. ESP32-C5 LINK STATUS VIEW
// ═══════════════════════════════════════════════════════════════════════════
void runC5Status() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_CYAN);
    tft.drawRect(1, 1, 318, 238, TFT_CYAN);

    drawStringBig(14, 10, "ESP32-C5 5G STATUS", TFT_CYAN, 2);
    tft.drawFastHLine(0, 34, 320, TFT_CYAN);

    int y = 42;
    drawStringCustom(12, y, "UART LINK CONFIGURATION:", TFT_YELLOW, 1); y += 14;
    drawStringCustom(16, y, "S3 TX (GPIO 45) -> C5 RX (GPIO 12)", TFT_WHITE, 1); y += 13;
    drawStringCustom(16, y, "S3 RX (GPIO 48) <- C5 TX (GPIO 11)", TFT_WHITE, 1); y += 13;
    drawStringCustom(16, y, "Baud: 115200 | Serial1", UI_ACCENT, 1); y += 18;

    tft.drawFastHLine(10, y, 300, 0x3186); y += 8;
    drawStringCustom(12, y, "CO-PROCESSOR PROBE:", TFT_YELLOW, 1); y += 16;
    
    int statusBoxY = y;
    drawStringCustom(16, statusBoxY, "Pinging ESP32-C5...", TFT_CYAN, 1);

    String lastResp = "";
    unsigned long rtt = 0;
    bool isOnline = probeC5(lastResp, rtt);

    auto renderStatus = [&](bool online, const String& resp, unsigned long pingTime) {
        tft.fillRect(16, statusBoxY, 288, 52, TFT_BLACK);
        if (online) {
            tft.fillRect(16, statusBoxY, 110, 20, TFT_DARKGREEN);
            drawStringCustom(22, statusBoxY + 3, "[ ONLINE ]", TFT_GREEN, 2);
            drawStringCustom(140, statusBoxY + 5, String(pingTime) + " ms RTT", TFT_CYAN, 1);
            
            String infoStr = "Resp: " + (resp.length() > 0 ? resp : "+PONG:OK");
            drawStringCustom(16, statusBoxY + 26, infoStr.substring(0, 35), TFT_WHITE, 1);
            drawStringCustom(16, statusBoxY + 38, "5GHz Radio Active & Communicating", TFT_GREEN, 1);
        } else {
            tft.fillRect(16, statusBoxY, 120, 20, TFT_MAROON);
            drawStringCustom(22, statusBoxY + 3, "[ OFFLINE ]", TFT_RED, 2);
            drawStringCustom(150, statusBoxY + 5, "No Response", TFT_ORANGE, 1);

            drawStringCustom(16, statusBoxY + 26, "Check S3 45->C5 12, S3 48<-C5 11", TFT_YELLOW, 1);
            drawStringCustom(16, statusBoxY + 38, "Check Common GND & C5 Power", TFT_RED, 1);
        }
    };

    renderStatus(isOnline, lastResp, rtt);

    tft.drawFastHLine(0, 212, 320, TFT_CYAN);
    drawStringCustom(10, 220, "OK: RE-TEST PING   BACK: EXIT", TFT_WHITE, 1);

    unsigned long lastAutoPing = millis();

    while (true) {
        if (navBackPressed()) {
            beep(1200, 30);
            while (navBackPressed()) delay(5);
            break;
        }

        if (navEnterPressed()) {
            beep(1800, 30);
            while (navEnterPressed()) delay(5);
            
            tft.fillRect(16, statusBoxY, 288, 52, TFT_BLACK);
            drawStringCustom(16, statusBoxY + 8, "Sending PING...", TFT_YELLOW, 2);
            
            isOnline = probeC5(lastResp, rtt);
            renderStatus(isOnline, lastResp, rtt);
            if (isOnline) beep(2400, 50);
            else beep(600, 80);
            lastAutoPing = millis();
        }

        if (millis() - lastAutoPing > 3000) {
            isOnline = probeC5(lastResp, rtt);
            renderStatus(isOnline, lastResp, rtt);
            lastAutoPing = millis();
        }

        delay(30);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 2. FULL HARDWARE DIAGNOSTICS SUITE (ALL PERIPHERALS)
// ═══════════════════════════════════════════════════════════════════════════
struct ComponentStatus {
    const char* name;
    const char* desc;
    bool online;
};

void runHardwareDiagnostics() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    drawStringBig(10, 8, "HARDWARE DIAGNOSTICS", TFT_WHITE, 1);
    drawStringCustom(205, 11, "SCANNING...", TFT_YELLOW, 1);
    tft.drawFastHLine(0, 30, 320, TFT_WHITE);
    tft.drawFastHLine(0, 214, 320, TFT_WHITE);
    drawStringCustom(10, 222, "OK: RE-SCAN ALL   BACK: EXIT", TFT_WHITE, 1);

    auto runProbeAll = []() {
        tft.fillRect(6, 36, 308, 172, TFT_BLACK);
        drawStringCustom(12, 42, "Testing all buses and radios...", TFT_CYAN, 1);

        String dummy = ""; unsigned long rtt = 0;
        bool c5Ok    = probeC5(dummy, rtt);
        bool nrf1Ok  = probeNRF(NRF1_CE_PIN, NRF1_CSN_PIN);
        bool nrf2Ok  = probeNRF(NRF2_CE_PIN, NRF2_CSN_PIN);
        bool nrf3Ok  = probeNRF(NRF3_CE_PIN, NRF3_CSN_PIN);
        bool ccOk    = probeCC1101();
        bool sdOk    = probeSD();
        bool gpsOk   = probeGPS();
        bool nfcOk   = probeI2C(0x24);

        char nrf1Desc[24], nrf2Desc[24], nrf3Desc[24];
        snprintf(nrf1Desc, sizeof(nrf1Desc), "SPI CE%d/CS%d", NRF1_CE_PIN, NRF1_CSN_PIN);
        snprintf(nrf2Desc, sizeof(nrf2Desc), "SPI CE%d/CS%d", NRF2_CE_PIN, NRF2_CSN_PIN);
        snprintf(nrf3Desc, sizeof(nrf3Desc), "SPI CE%d/CS%d", NRF3_CE_PIN, NRF3_CSN_PIN);

        char ccDesc[24];
        if (CC1101_CSN_PIN >= 0) snprintf(ccDesc, sizeof(ccDesc), "SPI CS%d", CC1101_CSN_PIN);
        else snprintf(ccDesc, sizeof(ccDesc), "NOT CONFIGURED");

        char sdDesc[24];
        if (SD_CS_PIN >= 0) snprintf(sdDesc, sizeof(sdDesc), "SPI CS%d", SD_CS_PIN);
        else snprintf(sdDesc, sizeof(sdDesc), "NOT CONFIGURED");

        char gpsDesc[24];
        if (GPS_RX_PIN >= 0 && GPS_TX_PIN >= 0) snprintf(gpsDesc, sizeof(gpsDesc), "UART RX%d/TX%d", GPS_RX_PIN, GPS_TX_PIN);
        else snprintf(gpsDesc, sizeof(gpsDesc), "NOT CONFIGURED");

        char nfcDesc[24];
        if (I2C_SDA_PIN >= 0 && I2C_SCL_PIN >= 0) snprintf(nfcDesc, sizeof(nfcDesc), "I2C 7/8 0x24");
        else snprintf(nfcDesc, sizeof(nfcDesc), "NOT CONFIGURED");

        ComponentStatus comps[] = {
            { "ESP32-C5 (5GHz)",   "UART RX48/TX45",  c5Ok   },
            { "nRF24 #1 (2.4G)",   nrf1Desc,          nrf1Ok },
            { "nRF24 #2 (2.4G)",   nrf2Desc,          nrf2Ok },
            { "nRF24 #3 (2.4G)",   nrf3Desc,          nrf3Ok },
            { "CC1101 (Sub-GHz)",  ccDesc,            ccOk   },
            { "MicroSD Card",      sdDesc,            sdOk   },
            { "GPS NEO-6M",        gpsDesc,           gpsOk  },
            { "PN532 NFC (I2C)",   nfcDesc,           nfcOk  }
        };

        tft.fillRect(6, 36, 308, 172, TFT_BLACK);

        int y = 38;
        for (int i = 0; i < 8; i++) {
            drawStringCustom(12, y + 2, comps[i].name, TFT_WHITE, 1);
            drawStringCustom(150, y + 2, comps[i].desc, 0x8410, 1);

            if (comps[i].online) {
                tft.fillRect(250, y, 58, 14, TFT_DARKGREEN);
                drawStringCustom(255, y + 3, "ONLINE", TFT_GREEN, 1);
            } else {
                tft.fillRect(250, y, 58, 14, TFT_MAROON);
                drawStringCustom(253, y + 3, "OFFLINE", TFT_RED, 1);
            }
            y += 21;
        }

        tft.fillRect(200, 8, 110, 16, TFT_BLACK);
        drawStringCustom(205, 11, "READY", TFT_GREEN, 1);
    };

    runProbeAll();

    while (true) {
        if (navBackPressed()) {
            beep(1200, 30);
            while (navBackPressed()) delay(5);
            break;
        }
        if (navEnterPressed()) {
            beep(1800, 30);
            while (navEnterPressed()) delay(5);
            tft.fillRect(200, 8, 110, 16, TFT_BLACK);
            drawStringCustom(205, 11, "SCANNING...", TFT_YELLOW, 1);
            runProbeAll();
        }
        delay(30);
    }
}


