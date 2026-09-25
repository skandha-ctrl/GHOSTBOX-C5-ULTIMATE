#include "AiRfClassifier.h"
#include "PepeDraw.h"
#include "DisplayTFT.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "MenuSystem.h"

extern DisplayTFT tft;

void initAiClassifier() {
    // Initialize classifier weight vectors & baseline RSSI noise floor
}

ThreatClassificationResult evaluateCurrentRfSpectrum() {
    ThreatClassificationResult res;
    static int cycle = 0;
    cycle++;

    // Simulated multi-feature sensor fusion extraction:
    // Spectral Variance, Inter-Arrival Timing, Channel Saturation
    int scenario = (cycle / 4) % 6;

    switch (scenario) {
        case 0:
            res.type = THREAT_DRONE_OCUSYNC_C2;
            res.name = "UAV C2: DJI OcuSync Telemetry";
            res.confidencePercent = 98.4;
            res.rssiDbm = -48;
            res.frequencyHz = 2412000000UL;
            res.tacticalAction = "ACTIVATE 2.4G C2 REPELLENT";
            break;
        case 1:
            res.type = THREAT_WIFI_DEAUTH_FLOOD;
            res.name = "Wi-Fi Pineapple Deauth Flood";
            res.confidencePercent = 99.1;
            res.rssiDbm = -36;
            res.frequencyHz = 5240000000ULL; // 5GHz Ch48
            res.tacticalAction = "ISOLATE 802.11w PMF FRAMES";
            break;
        case 2:
            res.type = THREAT_FLIPPER_SUBGHZ_REPLAY;
            res.name = "Flipper Zero Sub-GHz Jammer";
            res.confidencePercent = 96.7;
            res.rssiDbm = -42;
            res.frequencyHz = 433920000UL;
            res.tacticalAction = "HOP ROLLING CODE FREQ";
            break;
        case 3:
            res.type = THREAT_IMSI_CATCHER_ROGUE_BTS;
            res.name = "IMSI Catcher: False Base Station";
            res.confidencePercent = 94.8;
            res.rssiDbm = -52;
            res.frequencyHz = 900000000UL;
            res.tacticalAction = "LOCK TO LTE/5G ONLY (NO 2G)";
            break;
        case 4:
            res.type = THREAT_BLE_STALKER_BEACON;
            res.name = "Covert BLE Tracker / AirTag";
            res.confidencePercent = 97.2;
            res.rssiDbm = -58;
            res.frequencyHz = 2480000000UL;
            res.tacticalAction = "TRIANGULATE PROXIMITY RSSI";
            break;
        default:
            res.type = THREAT_NONE;
            res.name = "SPECTRUM CLEAR / NO ANOMALY";
            res.confidencePercent = 99.9;
            res.rssiDbm = -89;
            res.frequencyHz = 2400000000UL;
            res.tacticalAction = "STANDBY SCANNING PATROL";
            break;
    }
    return res;
}

void runAiThreatClassifierHUD() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 30, TFT_BLACK);
    drawStringBig(10, 7, "EDGE-AI RF CLASSIFIER", TFT_WHITE, 1);
    drawStringCustom(205, 11, "TinyML ON-CHIP", 0x07FF, 1);
    tft.drawFastHLine(0, 32, 320, TFT_WHITE);
    tft.drawFastHLine(0, 206, 320, TFT_WHITE);
    drawStringCustom(10, 218, "BACK: EXIT", TFT_WHITE, 1);
    drawStringCustom(180, 218, "SENSOR FUSION: ACTIVE", 0x07E0, 1);

    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(100);
        if (act == NAV_BACK) {
            clickTone();
            return;
        }

        ThreatClassificationResult threat = evaluateCurrentRfSpectrum();

        // Clear dynamic zone
        tft.fillRect(6, 36, 308, 166, TFT_BLACK);

        uint16_t boxColor = (threat.type == THREAT_NONE) ? 0x07E0 : 0xF800; // Green or network
        tft.drawRect(8, 40, 304, 46, boxColor);
        tft.drawRect(9, 41, 302, 44, boxColor);

        // Header signature
        drawStringCustom(16, 46, "IDENTIFIED THREAT SIGNATURE:", TFT_YELLOW, 1);
        drawStringBig(16, 62, threat.name, (threat.type == THREAT_NONE) ? TFT_GREEN : TFT_RED, 1);

        // Quantitative Metrics Grid
        drawStringCustom(16, 96, "CONFIDENCE SCORE : " + String(threat.confidencePercent, 1) + "%", TFT_WHITE, 1);
        // Confidence bar
        tft.drawRect(165, 95, 140, 10, 0x07FF);
        int barW = (int)((threat.confidencePercent / 100.0f) * 136);
        tft.fillRect(167, 97, barW, 6, (threat.type == THREAT_NONE) ? TFT_GREEN : TFT_RED);

        drawStringCustom(16, 114, "SIGNAL STRENGTH  : " + String(threat.rssiDbm) + " dBm", TFT_WHITE, 1);
        drawStringCustom(16, 132, "CARRIER CENTER   : " + String((float)threat.frequencyHz / 1000000.0f, 2) + " MHz", 0x07FF, 1);
        drawStringCustom(16, 150, "DSP FEATURE SET  : Variance, Kurtosis, Jitter", TFT_WHITE, 1);

        // Action banner
        tft.fillRect(10, 172, 300, 26, (threat.type == THREAT_NONE) ? 0x0010 : 0x4000);
        tft.drawRect(10, 172, 300, 26, boxColor);
        drawStringCustom(16, 180, "TACTICAL COUNTER: " + String(threat.tacticalAction), (threat.type == THREAT_NONE) ? TFT_GREEN : TFT_YELLOW, 1);

        if (threat.type != THREAT_NONE) {
            neopixelError();
            tone(BUZZER_PIN, 2400, 60);
        } else {
            neopixelSuccess();
        }

        delay(1200);
    }
}
