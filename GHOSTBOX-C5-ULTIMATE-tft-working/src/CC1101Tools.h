#ifndef CC1101_TOOLS_H
#define CC1101_TOOLS_H

#include <Arduino.h>

struct CC1101DashboardStatus {
    bool ready = false;
    uint32_t freqKHz = 433920;
    uint8_t partnum = 0xFF;
    uint8_t version = 0xFF;
    uint8_t marc = 0xFF;
    uint8_t pktStatus = 0xFF;
    uint8_t lqi = 0xFF;
    uint8_t rssiRaw = 0xFF;
    int rssiDbm = -127;
    int gdo0 = 0;
};

void runCC1101Tools();
bool cc1101DashboardRead(uint32_t freqKHz, CC1101DashboardStatus* out);
void cc1101DashboardStop();

#endif
