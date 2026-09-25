#include "LabStats.h"
#include <string.h>

static LabStatsData currentStats = { false, {0}, 0, 0, 0, 0, 0, 0, 0 };

void labStatsReset(const char *bssid) {
    currentStats.active = (bssid && bssid[0] != '\0');
    if (bssid) {
        strncpy(currentStats.bssid, bssid, sizeof(currentStats.bssid));
        currentStats.bssid[sizeof(currentStats.bssid) - 1] = '\0';
    } else {
        currentStats.bssid[0] = '\0';
    }
    currentStats.samples = 0;
    currentStats.found = 0;
    currentStats.missed = 0;
    currentStats.minRssi = 0;
    currentStats.maxRssi = -120;
    currentStats.sumRssi = 0;
    currentStats.lastChannel = 0;
}

void labStatsAdd(bool found, const TargetLabInfo *target) {
    if (!currentStats.active) return;
    currentStats.samples++;
    if (found && target) {
        currentStats.found++;
        currentStats.lastChannel = target->channel;
        if (currentStats.samples == 1 || target->rssi < currentStats.minRssi) {
            currentStats.minRssi = target->rssi;
        }
        if (currentStats.samples == 1 || target->rssi > currentStats.maxRssi) {
            currentStats.maxRssi = target->rssi;
        }
        currentStats.sumRssi += target->rssi;
    } else {
        currentStats.missed++;
    }
}

bool labStatsActive() {
    return currentStats.active;
}

const LabStatsData &labStatsGet() {
    return currentStats;
}

int32_t labStatsAverageRssi() {
    if (currentStats.found == 0) return 0;
    return currentStats.sumRssi / (int32_t)currentStats.found;
}
