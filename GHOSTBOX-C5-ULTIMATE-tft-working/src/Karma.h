#ifndef KARMA_H
#define KARMA_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// KARMA ATTACK · Responds with beacons to captured probe requests
// · Reuses Probe Sniffer target list
// · Broadcasts each requested SSID as an open available network
// · Channel hopping 1 → 6 → 11
// · Devices with saved open networks may auto-connect
// ═══════════════════════════════════════════════════════════════════════════

void runKarma();

#endif