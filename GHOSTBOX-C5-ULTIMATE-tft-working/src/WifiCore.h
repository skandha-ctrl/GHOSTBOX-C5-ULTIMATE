#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "Pins.h"
#include <vector>

// ==============================================================================
// WIFI CORE · ESP32-S3 MASTER + ESP32-C5 5GHz COPROCESSOR HYBRID ENGINE
// ==============================================================================

#define C5_SERIAL Serial1

enum WifiBand : uint8_t {
    BAND_2G4 = 0,
    BAND_5G  = 1
};

enum RadioSource : uint8_t {
    RADIO_S3_2G4 = 0,
    RADIO_C5_5G  = 1
};

struct NetworkObservation {
    WifiBand    band;
    RadioSource radio;
    String      ssid;
    uint8_t     bssid[6];
    String      bssidStr;
    uint8_t     channel;
    int         rssi;
    uint8_t     authType;
    uint32_t    seqId;
    bool        hidden;
};

#define WIFI_24G_CHAN_COUNT 14
#define WIFI_5G_CHAN_COUNT  25
#define WIFI_TOTAL_CHANNELS (WIFI_24G_CHAN_COUNT + WIFI_5G_CHAN_COUNT)

extern const uint8_t WIFI_CHANNELS_24G[WIFI_24G_CHAN_COUNT];
extern const uint8_t WIFI_CHANNELS_5G[WIFI_5G_CHAN_COUNT];
extern const uint8_t WIFI_ALL_CHANNELS[WIFI_TOTAL_CHANNELS];

bool is5GHzChannel(uint8_t channel);
const char* getBandString(uint8_t channel);
const char* getBandStringFromBand(WifiBand band);

void wifiCoreInit();
void wifiCoreInitPromiscuous();
void wifiCoreSetChannel(uint8_t channel);
bool wifiCoreSendRaw80211(const uint8_t *packet, size_t length);

// Unified Observation Database & Scanning
int performUnifiedScan(std::vector<NetworkObservation>& outList, uint32_t maxWaitMs = 5000);

// C5 Co-Processor Bridge Utilities
bool c5Ping();
void c5Scan5G(uint32_t seqId = 0);
void c5SetSniffer(bool enable);
bool c5SetChannel(uint8_t channel);

// Send a deauth frame via the C5 RF coprocessor (5 GHz targets)
// bssid = AP BSSID (6 bytes), dst = target STA or FF:FF:FF:FF:FF:FF
// Sends: DEAUTH:<bssid_hex>,<dst_hex>,<reason>\n over UART
bool c5SendDeauth(const uint8_t bssid[6], const uint8_t dst[6],
                  uint16_t reason = 0x0007);
