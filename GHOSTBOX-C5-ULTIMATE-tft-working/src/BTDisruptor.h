#ifndef BT_DISRUPTOR_H
#define BT_DISRUPTOR_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// BT DISRUPTOR · attack dirigido a a device BLE específico
// · Scan → Select target → Select mode → Attack
// · 4 modes: Connect Flood, L2CAP Ping Storm, Spoof Identity, Chaos
// · Uso educativo/demo — use with responsibility
// ═══════════════════════════════════════════════════════════════════════════

void runBTDisruptor();

#endif