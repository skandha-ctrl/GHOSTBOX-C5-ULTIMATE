#ifndef CELLULAR_AUDIT_H
#define CELLULAR_AUDIT_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// CELLULAR SECURITY & BASE STATION INTEGRITY AUDIT ENGINE (SIM800L / SIM7600)
//
// Capabilities:
// 1. Tower Wardriving: Scans MCC, MNC, LAC, Cell ID (CID), RSSI (dBm)
// 2. Rogue Tower / IMSI Catcher Detector: Detects abnormal LAC switching,
// unencrypted A5/0 cipher downgrade flags & sudden high-signal BTS.
// 3. Tactical SMS / PDU Injector & Security Pager Fuzzer.
// 4. Out-of-Band Tactical Emergency Cellular Beacon.
// ═══════════════════════════════════════════════════════════════════════════

void initCellularModem();
void runCellularAuditMenu();
void runCellularTowerScanner();
void runImsiCatcherDetector();
void runTacticalSmsFuzzer();
void runCellularBeacon();

#endif // CELLULAR_AUDIT_H
