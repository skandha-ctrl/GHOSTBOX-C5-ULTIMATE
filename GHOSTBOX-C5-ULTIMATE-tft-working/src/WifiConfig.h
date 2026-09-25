#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// WIFI CONFIG · Interactive network connection wizard
// · Connects to saved network or scans and prompts for password
// · Saves credentials securely to NVS
// ═══════════════════════════════════════════════════════════════════════════

// Connects interactively or returns false if canceled.
bool wifiConfigConnect();

#endif
