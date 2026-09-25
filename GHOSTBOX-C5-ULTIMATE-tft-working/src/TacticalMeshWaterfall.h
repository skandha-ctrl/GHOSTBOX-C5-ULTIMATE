#ifndef TACTICAL_MESH_WATERFALL_H
#define TACTICAL_MESH_WATERFALL_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// TACTICAL SPECTRUM WATERFALL + LORA/CC1101 OFF-GRID MESH & AUDIT REPORT ENGINE
//
// Capabilities:
// 1. Live Dual-Band 2.4G/5G & Sub-GHz Real-Time Color-Graded Spectral Waterfall.
// 2. Off-Grid Tactical Sub-GHz P2P Encrypted Mesh Intercom (no internet/cellular).
// 3. Dead-Man's Switch & Black Box Automatic Incident Dump.
// 4. Automated 1-Click Markdown/PDF Penetration Audit Report Generator.
// ═══════════════════════════════════════════════════════════════════════════

void runSpectralWaterfallHUD();
void runOffGridTacticalMeshHUD();
void runDeadMansSwitchHUD();
void generateOneClickAuditReport();

#endif // TACTICAL_MESH_WATERFALL_H
