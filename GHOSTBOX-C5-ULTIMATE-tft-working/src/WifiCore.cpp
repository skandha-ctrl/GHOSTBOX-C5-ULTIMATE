#include "WifiCore.h"

const uint8_t WIFI_CHANNELS_24G[WIFI_24G_CHAN_COUNT] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
};

const uint8_t WIFI_CHANNELS_5G[WIFI_5G_CHAN_COUNT] = {
    36, 40, 44, 48, 52, 56, 60, 64,
    100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
    149, 153, 157, 161, 165
};

const uint8_t WIFI_ALL_CHANNELS[WIFI_TOTAL_CHANNELS] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    36, 40, 44, 48, 52, 56, 60, 64,
    100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
    149, 153, 157, 161, 165
};

bool is5GHzChannel(uint8_t channel) {
    return channel >= 36;
}

const char* getBandString(uint8_t channel) {
    return is5GHzChannel(channel) ? "5G" : "2.4G";
}

const char* getBandStringFromBand(WifiBand band) {
    return (band == BAND_5G) ? "5G" : "2.4G";
}

static bool parseMacBytes(const String& macStr, uint8_t outMac[6]) {
    if (macStr.length() < 17) return false;
    int values[6];
    if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) == 6) {
        for (int i = 0; i < 6; i++) {
            outMac[i] = (uint8_t)values[i];
        }
        return true;
    }
    return false;
}

void wifiCoreInit() {
    static bool uartStarted = false;
    if (!uartStarted) {
        uartStarted = true;
        // Initialize Hardware UART Link to ESP32-C5 (115200 baud)
        C5_SERIAL.setRxBufferSize(8192);
        C5_SERIAL.begin(C5_BAUD, SERIAL_8N1, C5_RX_PIN, C5_TX_PIN);
        Serial.printf("[S3] UART to C5 initialized on RX=%d, TX=%d @ %d baud\n", C5_RX_PIN, C5_TX_PIN, C5_BAUD);
    }
    
    // Always ensure local S3 Wi-Fi is in Station Mode and ready
    esp_wifi_set_promiscuous(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
}

void wifiCoreInitPromiscuous() {
    wifiCoreInit();
    esp_wifi_set_promiscuous(false);
}

void wifiCoreSetChannel(uint8_t channel) {
    if (is5GHzChannel(channel)) {
        // Forward 5GHz channel switch to ESP32-C5 co-processor
        C5_SERIAL.printf("SET_CH:%d\n", channel);
    } else {
        // Local S3 2.4GHz channel switch
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
}

bool wifiCoreSendRaw80211(const uint8_t *packet, size_t length) {
    if (!packet || length == 0) return false;
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, packet, length, false);
    return (err == ESP_OK);
}

bool c5Ping() {
    wifiCoreInit();
    while (C5_SERIAL.available()) C5_SERIAL.read();
    C5_SERIAL.println("PING");
    unsigned long start = millis();
    while (millis() - start < 300) {
        if (C5_SERIAL.available()) {
            String resp = C5_SERIAL.readStringUntil('\n');
            resp.trim();
            if (resp.indexOf("+PONG") >= 0) return true;
        }
    }
    return false;
}

void c5Scan5G(uint32_t seqId) {
    wifiCoreInit();
    C5_SERIAL.printf("SCAN_5G:%lu\n", seqId);
}

void c5SetSniffer(bool enable) {
    wifiCoreInit();
    if (enable) C5_SERIAL.println("SNIFF_START");
    else C5_SERIAL.println("SNIFF_STOP");
}

bool c5SetChannel(uint8_t channel) {
    wifiCoreInit();
    while (C5_SERIAL.available()) C5_SERIAL.read();
    C5_SERIAL.printf("SET_CH:%d\n", channel);
    unsigned long start = millis();
    while (millis() - start < 200) {
        if (C5_SERIAL.available()) {
            String resp = C5_SERIAL.readStringUntil('\n');
            resp.trim();
            if (resp.indexOf("+OK:CH") >= 0) return true;
        }
    }
    return false;
}

// ── Internal UART line parser: reads one AP line from C5 and pushes to outList ──
static void parseC5Line(const String& line, uint32_t currentSeq,
                        std::vector<NetworkObservation>& outList) {
    int firstColon = line.indexOf(':');
    if (firstColon < 0) return;
    String payload = line.substring(firstColon + 1);

    std::vector<String> tokens;
    int startIdx = 0;
    while (startIdx < (int)payload.length()) {
        int commaIdx = payload.indexOf(',', startIdx);
        if (commaIdx < 0) { tokens.push_back(payload.substring(startIdx)); break; }
        tokens.push_back(payload.substring(startIdx, commaIdx));
        startIdx = commaIdx + 1;
    }

    NetworkObservation obs;
    obs.band  = BAND_5G;
    obs.radio = RADIO_C5_5G;
    obs.seqId = currentSeq;

    if (tokens.size() >= 6) {
        uint32_t rxSeq = (uint32_t)tokens[0].toInt();
        if (rxSeq != 0 && rxSeq != currentSeq) return;
        obs.ssid     = tokens[1];
        obs.channel  = (uint8_t)tokens[2].toInt();
        obs.rssi     = tokens[3].toInt();
        obs.bssidStr = tokens[4];
        obs.authType = (uint8_t)tokens[5].toInt();
    } else if (tokens.size() == 5) {
        obs.ssid     = tokens[0];
        obs.channel  = (uint8_t)tokens[1].toInt();
        obs.rssi     = tokens[2].toInt();
        obs.bssidStr = tokens[3];
        obs.authType = (uint8_t)tokens[4].toInt();
    } else {
        return;
    }

    obs.hidden = (obs.ssid.length() == 0 || obs.ssid == "<hidden>");
    if (obs.hidden) obs.ssid = "<hidden>";

    if (is5GHzChannel(obs.channel) && parseMacBytes(obs.bssidStr, obs.bssid)) {
        outList.push_back(obs);
        Serial.printf("[S3] C5 AP: \"%s\" CH=%d RSSI=%d BSSID=%s\n",
                      obs.ssid.c_str(), obs.channel, obs.rssi, obs.bssidStr.c_str());
    }
}

int performUnifiedScan(std::vector<NetworkObservation>& outList, uint32_t maxWaitMs) {
    wifiCoreInit();
    outList.clear();

    static uint32_t scanSeqCounter = 1;
    uint32_t currentSeq = scanSeqCounter++;

    Serial.printf("\n[S3] === UNIFIED SCAN START (Seq #%lu) ===\n", currentSeq);

    // Step 1: Flush stale UART bytes then dispatch 5GHz scan to C5
    while (C5_SERIAL.available()) C5_SERIAL.read();
    c5Scan5G(currentSeq);
    Serial.println("[S3] SCAN_5G dispatched to C5.");

    // Step 2: Give C5 a 200ms head start, collect any early AP lines
    unsigned long headStart = millis();
    while (millis() - headStart < 200) {
        while (C5_SERIAL.available()) {
            String line = C5_SERIAL.readStringUntil('\n');
            line.trim();
            if (line.startsWith("+AP_5G:") || line.startsWith("+AP:"))
                parseC5Line(line, currentSeq, outList);
        }
        delay(10);
    }

    // Step 3: Synchronous 2.4 GHz scan — proven reliable on ESP32 2.0.x.
    // C5 output accumulates in the 8192-byte UART RX buffer during blocking scan.
    esp_wifi_set_promiscuous(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("[S3] Starting synchronous 2.4G scan...");
    int n24 = WiFi.scanNetworks(false, true); // blocking, show_hidden=true
    if (n24 < 0) n24 = 0;
    Serial.printf("[S3] 2.4G scan done. Found %d APs.\n", n24);

    for (int i = 0; i < n24; i++) {
        uint8_t ch = (uint8_t)WiFi.channel(i);
        if (ch < 1 || ch > 14) continue;
        NetworkObservation obs;
        obs.band     = BAND_2G4;
        obs.radio    = RADIO_S3_2G4;
        obs.ssid     = WiFi.SSID(i);
        obs.channel  = ch;
        obs.rssi     = WiFi.RSSI(i);
        obs.authType = (uint8_t)WiFi.encryptionType(i);
        obs.seqId    = currentSeq;
        obs.hidden   = (obs.ssid.length() == 0 || obs.ssid == "<hidden>");
        if (obs.hidden) obs.ssid = "<hidden>";
        uint8_t* b = WiFi.BSSID(i);
        if (b) {
            memcpy(obs.bssid, b, 6);
            char bBuf[18];
            snprintf(bBuf, sizeof(bBuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                     b[0], b[1], b[2], b[3], b[4], b[5]);
            obs.bssidStr = String(bBuf);
        } else {
            memset(obs.bssid, 0, 6);
            obs.bssidStr = "00:00:00:00:00:00";
        }
        outList.push_back(obs);
        Serial.printf("[S3] 2.4G: \"%s\" CH=%d RSSI=%d\n",
                      obs.ssid.c_str(), ch, obs.rssi);
    }
    WiFi.scanDelete();

    // Step 4: Drain buffered C5 data, wait up to 4s for SCAN_END
    bool c5Done = false;
    unsigned long c5Wait = millis();

    while (millis() - c5Wait < 8000) {
        while (C5_SERIAL.available()) {
            String line = C5_SERIAL.readStringUntil('\n');
            line.trim();
            if (line.length() == 0) continue;
            if (line.startsWith("+AP_5G:") || line.startsWith("+AP:")) {
                parseC5Line(line, currentSeq, outList);
            } else if (line.startsWith("+SCAN_END") || line.startsWith("+SCAN_DONE")) {
                c5Done = true;
                Serial.printf("[S3] C5 SCAN_END. 5G APs: %d\n",
                              (int)outList.size() - n24);
                break;
            }
        }
        if (c5Done) break;
        delay(20);
    }

    if (!c5Done)
        Serial.println("[S3] WARNING: C5 scan timed out - using buffered data.");

    // Deduplicate by BSSID — keep strongest RSSI entry
    std::vector<NetworkObservation> deduped;
    for (auto& obs : outList) {
        bool found = false;
        for (auto& d : deduped) {
            if (memcmp(d.bssid, obs.bssid, 6) == 0) {
                if (obs.rssi > d.rssi) d = obs; // keep stronger signal
                found = true;
                break;
            }
        }
        if (!found) deduped.push_back(obs);
    }
    outList = deduped;

    Serial.printf("[S3] === SCAN DONE: %u total (2.4G=%d 5G=%d) ===\n\n",
                  (unsigned int)outList.size(), n24,
                  (int)outList.size() - n24);
    return (int)outList.size();
}


// ── c5SendDeauth ────────────────────────────────────────────────────────────
// Sends a UART deauth command to the ESP32-C5 co-processor.
// Protocol: DEAUTH:<bssid_hex>,<dst_hex>,<reason>\n
// Example: DEAUTH:AA:BB:CC:DD:EE:FF,FF:FF:FF:FF:FF:FF,7\n
//
// The C5 firmware receives this, builds a DeauthFrame locally and fires
// wifi_tx_raw_frame() on its own 5 GHz radio — which is the only radio
// that can actually transmit on 5 GHz channels (>= 36).
bool c5SendDeauth(const uint8_t bssid[6], const uint8_t dst[6], uint16_t reason) {
    wifiCoreInit();
    char buf[80];
    snprintf(buf, sizeof(buf),
             "DEAUTH:%02X:%02X:%02X:%02X:%02X:%02X,%02X:%02X:%02X:%02X:%02X:%02X,%u\n",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
             dst[0],   dst[1],   dst[2],   dst[3],   dst[4],   dst[5],
             (unsigned)reason);
    C5_SERIAL.print(buf);
    return true;   // fire-and-forget; C5 ACKs asynchronously
}

