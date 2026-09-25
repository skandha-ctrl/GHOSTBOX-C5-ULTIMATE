#ifndef BLE_SPAM_H
#define BLE_SPAM_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// BLE SPAM · transmits advertisements BLE falsos
// · 4 protocolos: Apple Continuity, Samsung, Microsoft Swift Pair, Google
// · CHAOS MODE: rota between the 4 aleatoriamente
// · Propósito educativo/demo — use with responsibility
// ═══════════════════════════════════════════════════════════════════════════

void runBLESpam();

#endif