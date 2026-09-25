#ifndef DEAUTHER_H
#define DEAUTHER_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// DEAUTHER · 802.11 deauthentication frame injection engine
// · Targets specific BSSIDs or broadcasts deauth
// · Supports dual-band channels 2.4 GHz & 5.0 GHz
// ═══════════════════════════════════════════════════════════════════════════

// Deauther interactive loop.
void runDeauther();

#endif
