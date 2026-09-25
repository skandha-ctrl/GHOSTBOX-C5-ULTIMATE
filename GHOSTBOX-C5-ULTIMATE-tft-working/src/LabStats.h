#pragma once

#include <Arduino.h>

struct TargetLabInfo {
    char ssid[33];
    char bssid[18];
    uint8_t channel;
    int32_t rssi;
    uint32_t security;
};

struct LabStatsData {
    bool active;
    char bssid[18];
    uint16_t samples;
    uint16_t found;
    uint16_t missed;
    int32_t minRssi;
    int32_t maxRssi;
    int32_t sumRssi;
    uint8_t lastChannel;
};

void labStatsReset(const char *bssid);
void labStatsAdd(bool found, const TargetLabInfo *target);
bool labStatsActive();
const LabStatsData &labStatsGet();
int32_t labStatsAverageRssi();
