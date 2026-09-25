#ifndef PROBE_SNIFFER_H
#define PROBE_SNIFFER_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// PROBE REQUEST SNIFFER · Capture SSIDs broadcast by nearby devices
// · Promiscuous mode, filters only 802.11 probe requests (subtype 0x04)
// · Deduplicates SSIDs and counts occurrences seen for each
// · Channel hopping (1 -> 6 -> 11) every 2 seconds
// · Navigable list with SSID + count + latest RSSI + last seen timestamp
// ═══════════════════════════════════════════════════════════════════════════

void runProbeSniffer();

// API for KARMA honeypot to reuse captured SSID list
struct ProbeEntry {
    char     ssid[33];
    uint16_t count;
    uint16_t clients;
    int8_t   rssi;
    uint8_t  firstChannel;
    uint8_t  lastChannel;
    uint8_t  lastMac[6];
    uint32_t lastSeenMs;
};

int  probeSnifferGetCount();
bool probeSnifferGet(int idx, ProbeEntry& out);

#endif
