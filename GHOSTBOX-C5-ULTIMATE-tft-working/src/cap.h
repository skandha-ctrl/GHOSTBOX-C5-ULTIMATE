#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// cap.h — ESP32-S3 port of BW16 packet injection frame structures
// Uses esp_wifi_80211_tx() (Espressif equivalent of BW16 wifi_tx_raw_frame)
// original BW16 source: BWifiKill / packet-injection.h
// ═══════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

// ── IEEE 802.11 Management Frame Structures ──────────────────────────────

// Deauthentication Frame (Type=0x00 / Subtype=0x0C → FC byte0=0xC0)
typedef struct __attribute__((packed)) {
    uint16_t frame_control  = 0x00C0;    // FC: Management, Subtype Deauth
    uint16_t duration       = 0x013A;
    uint8_t  destination[6];             // Addr1: target STA or broadcast
    uint8_t  source[6];                  // Addr2: AP or spoof MAC
    uint8_t  access_point[6];            // Addr3: BSSID
    uint16_t sequence_number = 0;
    uint16_t reason          = 0x0007;   // Reason 7: Class 3 from non-assoc
} DeauthFrame;

// Deauthentication Frame sent in reverse (STA→AP direction for double-sided)
typedef struct __attribute__((packed)) {
    uint16_t frame_control  = 0x00C0;
    uint16_t duration       = 0x013A;
    uint8_t  destination[6];
    uint8_t  source[6];
    uint8_t  access_point[6];
    uint16_t sequence_number = 0;
    uint16_t reason          = 0x0007;
} DeauthFrameReverse;

// Beacon Frame (Type=0x00 / Subtype=0x08 → FC byte0=0x80)
typedef struct __attribute__((packed)) {
    uint16_t frame_control   = 0x0080;   // FC: Management, Subtype Beacon
    uint16_t duration        = 0;
    uint8_t  destination[6];             // Usually broadcast FF:FF:…
    uint8_t  source[6];
    uint8_t  access_point[6];
    uint16_t sequence_number = 0;
    uint64_t timestamp       = 0;
    uint16_t beacon_interval = 0x0064;   // 100 TU ≈ 102.4 ms
    uint16_t ap_capabilities = 0x0421;   // ESS + Short Preamble + Short Slot
    uint8_t  ssid_tag        = 0;        // Element ID 0 = SSID
    uint8_t  ssid_length     = 0;
    uint8_t  ssid[32];                   // Max 32 bytes per 802.11 spec
} BeaconFrame;

// ── Public API ────────────────────────────────────────────────────────────
bool wifi_tx_raw_frame(const void* frame, size_t length);
bool wifi_tx_deauth_frame(void* src_mac, void* dst_mac,
                          uint16_t reason = 0x0007);
bool wifi_tx_beacon_frame(void* src_mac, void* dst_mac, const char* ssid);
