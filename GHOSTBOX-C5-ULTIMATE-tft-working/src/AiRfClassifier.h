#ifndef AI_RF_CLASSIFIER_H
#define AI_RF_CLASSIFIER_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// ON-DEVICE EDGE AI / HEURISTIC RF THREAT CLASSIFIER (CYBER-DEFENSE ENGINE)
//
// Capabilities:
// 1. Real-time RF signal feature extraction (Peak amplitude, spectral variance,
// frame inter-arrival timing jitter, pulse-width modulation signatures).
// 2. On-device decision tree & heuristic classifier detecting:
// - Rogue Wi-Fi Pineapple Deauth Floods
// - Flipper Zero Sub-GHz Jammer / Rolling Code Replays
// - DJI / Autel Drone OcuSync 2.4GHz / 5.8GHz C2 Telemetry
// - Apple AirTag / SmartTag BLE Stalking Beacons
// - IMSI Catcher False Base Station Downgrades
// 3. Live Threat Scoring (0 - 100%) with RGB / Audio Alerting.
// ═══════════════════════════════════════════════════════════════════════════

enum RfThreatType {
    THREAT_NONE,
    THREAT_WIFI_DEAUTH_FLOOD,
    THREAT_FLIPPER_SUBGHZ_REPLAY,
    THREAT_DRONE_OCUSYNC_C2,
    THREAT_BLE_STALKER_BEACON,
    THREAT_IMSI_CATCHER_ROGUE_BTS,
    THREAT_WIDEBAND_RF_JAMMER
};

struct ThreatClassificationResult {
    RfThreatType type;
    const char*  name;
    float        confidencePercent;
    int          rssiDbm;
    uint32_t     frequencyHz;
    const char*  tacticalAction;
};

void initAiClassifier();
void runAiThreatClassifierHUD();
ThreatClassificationResult evaluateCurrentRfSpectrum();

#endif // AI_RF_CLASSIFIER_H
