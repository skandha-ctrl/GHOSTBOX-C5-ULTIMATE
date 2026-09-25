#ifndef BEACON_SPAM_H
#define BEACON_SPAM_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// BEACON SPAM · Broadcasts rogue WiFi networks (802.11 Beacon frames)
// · Multiple presets: Cyberpunk, Funny, Tech, etc.
// · Dual-band support: 2.4 GHz + 5.0 GHz (ESP32-C5 native)
// ═══════════════════════════════════════════════════════════════════════════

// Beacon Spam main screen.
// Blocking loop until user presses BACK.
void runBeaconSpam();

#endif
