#include "BeaconSpam.h"

#include "DisplayTFT.h"

#include <WiFi.h>

#include "esp_wifi.h"

#include "PepeDraw.h"

#include "Pins.h"

#include "SoundUtils.h"



extern DisplayTFT tft;



// Mode 1: TACTICAL / GHOSTBOX (Special SSID list)

static const char* SSIDS_MEXI[] = {

    "💀Ghost_In_The_Shell", "⚡Ghostbox_C5_Node", "👾Neuromancer_Grid",

    "📡Tachyon_Uplink", "🔥Skynet_Global_Defense", "👽Area51_Alien_Lab",

    "💣ZeroDay_Exploit", "🤖BladeRunner_2049", "🧠Neuralink_Mesh",

    "🔞DARPA_Classified", "⚠️Kernel_Panic_Alert", "🐍Hydra_SubNet",

    "💥DarkNet_Relay_01", "🔍Echelon_Surveillance", "🔥Matrix_Glitch_Detected",

    "💾Quantum_Core_Down", "🚨NSA_Listening_Post", "🖕HackThePlanet_99",

    "🤡Mr_Robot_fsociety", "🧼Null_Pointer_Exception", "🕵Interpol_Cyber_Unit",

    "👮Starfleet_Command", "🤮Rootkit_Active", "🍄Crypto_Miner_Locker",

    "🧨Trojan_Injector", "🌚Black_Hole_Horizon", "🥀ZeroTrust_Isolated",

    "🍗Free_Public_Proxy", "🍖GhostBox_AirGap", "🦶Satellite_Downlink_8",

    "👅Cisco_Backdoor_Lab", "🧟Zombie_Botnet_Relay", "👺DeadMans_Switch",

    "🍕Cyber_Vault_Unlocked", "🌑Dark_Matter_Flux", "💊Red_Or_Blue_Pill",

    "📉Buffer_Overflow_X", "📡Overwatch_Drone_04", "⚡Tesla_Wardenclyffe",

    "💀Defcon_Badge_WiFi"

};

static const int COUNT_MEXI = sizeof(SSIDS_MEXI) / sizeof(char*);



// ═══════════════════════════════════════════════════════════════════════════

// mode 2: MEMES CLÁSICOS

// ═══════════════════════════════════════════════════════════════════════════

static const char* SSIDS_MEMES[] = {

    "FBI_Surveillance_Van_07",

    "Virus.exe_Connect_Now",

    "Free_WiFi_Definitely_Safe",

    "Please_Connect_To_Me",

    "DO_NOT_CONNECT",

    "Tell_My_WiFi_Love_Her",

    "WiFi_Under_Maintenance",

    "Pretty_Fly_For_A_WiFi",

    "Drop_It_Like_Its_Hotspot",

    "The_LAN_Before_Time",

    "Pay_For_Your_Own_WiFi",

    "Steal_Netflix_Here",

    "Router_In_The_Restroom",

    "404_Network_Unavailable",

    "Bill_Wi_The_Science_Fi",

    "Abraham_Linksys",

    "Wi_Believe_I_Can_Fi",

    "Win_Free_iPhone_Click",

    "ClickBait_WiFi_Free",

    "Connect_If_You_Dare"

};

static const int COUNT_MEMES = sizeof(SSIDS_MEMES) / sizeof(char*);



// ═══════════════════════════════════════════════════════════════════════════

// mode 3: PARANOIA

// ═══════════════════════════════════════════════════════════════════════════

static const char* SSIDS_PARANOIA[] = {

    "Covert_Camera_Active",

    "We_Are_Recording_You",

    "Mic_Listening_Room_3",

    "Cyber_Police_Intercept",

    "Phone_Compromised_Warning",

    "Unsecured_Network_Alert",

    "Infected_Host_Detected",

    "Malware_Telemetry_Active",

    "Bank_Session_Hijacked",

    "Location_Tracking_Beacon",

    "Compromised_Gateway",

    "Gov_Listening_Station",

    "Ransomware_In_Progress",

    "Photos_Exfiltration_Node",

    "Rogue_Access_Point_404",

    "Password_Leak_Detected",

    "Interpol_Cyber_Division",

    "Admin_Terminal_Open",

    "Target_Acquired_Locked",

    "Wireless_Wiretap_Unit"

};

static const int COUNT_PARANOIA = sizeof(SSIDS_PARANOIA) / sizeof(char*);



// ═══════════════════════════════════════════════════════════════════════════

// mode 4: CHAOS UTF-8 (puros emojis and caracteres raros)

// ═══════════════════════════════════════════════════════════════════════════

static const char* SSIDS_CHAOS[] = {

    "💀💀💀💀💀",

    "🔥🔥🔥HACK🔥🔥🔥",

    "👻👻👻👻",

    "🚨ALERTA🚨",

    "💣💣💣💣",

    "☠️☠️☠️",

    "🎃🎃🎃",

    "🤡🤡🤡",

    "👹👹👹👹",

    "🌚🌚🌚",

    "💩💩💩💩💩",

    "🧠🧠🧠",

    "⚡⚡⚡⚡",

    "🔴🔴🔴",

    "🟢🟡🔴",

    "🆘🆘🆘",

    "❌❌❌❌",

    "✅❌✅❌",

    "🔞🔞🔞",

    "📡📡📡📡"

};

static const int COUNT_CHAOS = sizeof(SSIDS_CHAOS) / sizeof(char*);



// MODE SELECTION

enum SpamMode {

    MODE_MEXI     = 0,

    MODE_MEMES    = 1,

    MODE_PARANOIA = 2,

    MODE_CHAOS    = 3,

    MODE_MIX      = 4

};



static const char* MODE_NAMES[] = {

    "Cyberpunk",

    "Classic Memes",

    "Paranoia",

    "Chaos UTF-8",

    "Total Mix (all)"

};

static const char* MODE_DESCS[] = {

    "40 Cyberpunk & sci-fi SSIDs",

    "Internet classics & memes",

    "Surveillance paranoia test",

    "Emojis and raw UTF-8 glyphs",

    "Random mix across all sets"

};

static const int MODE_COUNT = 5;



// ═══════════════════════════════════════════════════════════════════════════

// State

// ═══════════════════════════════════════════════════════════════════════════

static volatile unsigned long beaconsSent = 0;

static String  currentSSID = "";

static int     currentChannel = 1;

static SpamMode activeMode = MODE_MEXI;



// ═══════════════════════════════════════════════════════════════════════════

// FRAME 802.11 BEACON RAW

// Plantilla base that then rellenamos with SSID/BSSID/channel dynamic

// ═══════════════════════════════════════════════════════════════════════════

static uint8_t beaconFrame[200] = {

    // Frame Control (2 bytes): Beacon type 0x80

    0x80, 0x00,

    // Duration (2 bytes)

    0x00, 0x00,

    // Destination (6 bytes): broadcast FF:FF:FF:FF:FF:FF

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    // Source / BSSID (6 bytes): llena dinámicamente

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // BSSID duplicado (6 bytes)

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Sequence number (2 bytes)

    0x00, 0x00,

    // ── Frame body ──

    // Timestamp (8 bytes)

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Beacon interval (2 bytes): 0x0064 = 100 TU = ~102.4 ms

    0x64, 0x00,

    // Capability info (2 bytes): 0x0401 = ESS + Short Preamble

    0x01, 0x04,

    // ── Tagged parameters ──

    // SSID tag: tag=0x00, length=N, then bytes of the SSID

    0x00, 0x00,        // placeholder (length in [37])

    // (aquí va the SSID, from offset 38)

};



// Offset inside of the frame where comienza the SSID length tag

static const int SSID_LENGTH_OFFSET = 37;

static const int SSID_START_OFFSET  = 38;



// Tail: "supported rates" + "DS parameter" (channel)

// construye dinámicamente after of the SSID

static const uint8_t FRAME_TAIL[] = {

    // Supported rates (tag=0x01, length=8)

    0x01, 0x08,

    0x82, 0x84, 0x8B, 0x96, 0x24, 0x30, 0x48, 0x6C,

    // DS Parameter Set (tag=0x03, length=1, channel)

    0x03, 0x01, 0x00    // último byte = canal current

};

static const int FRAME_TAIL_SIZE = sizeof(FRAME_TAIL);



// ═══════════════════════════════════════════════════════════════════════════

// HELPERS

// ═══════════════════════════════════════════════════════════════════════════



// Obtiene the número of SSIDs disponibles for a mode

static int countSSIDsForMode(SpamMode mode) {

    switch (mode) {

        case MODE_MEXI:     return COUNT_MEXI;

        case MODE_MEMES:    return COUNT_MEMES;

        case MODE_PARANOIA: return COUNT_PARANOIA;

        case MODE_CHAOS:    return COUNT_CHAOS;

        case MODE_MIX:      return COUNT_MEXI + COUNT_MEMES +

                                   COUNT_PARANOIA + COUNT_CHAOS;

        default:            return 0;

    }

}



// Obtiene the SSID of a mode by índice

static const char* getSSIDForMode(SpamMode mode, int idx) {

    switch (mode) {

        case MODE_MEXI:     return SSIDS_MEXI[idx];

        case MODE_MEMES:    return SSIDS_MEMES[idx];

        case MODE_PARANOIA: return SSIDS_PARANOIA[idx];

        case MODE_CHAOS:    return SSIDS_CHAOS[idx];

        case MODE_MIX: {

            if (idx < COUNT_MEXI)

                return SSIDS_MEXI[idx];

            idx -= COUNT_MEXI;

            if (idx < COUNT_MEMES)

                return SSIDS_MEMES[idx];

            idx -= COUNT_MEMES;

            if (idx < COUNT_PARANOIA)

                return SSIDS_PARANOIA[idx];

            idx -= COUNT_PARANOIA;

            if (idx < COUNT_CHAOS)

                return SSIDS_CHAOS[idx];

            return "?";

        }

        default: return "?";

    }

}



// Construye and transmits a beacon with the SSID and canal dados

static void sendBeacon(const char* ssid, int channel) {

    int ssidLen = strlen(ssid);

    if (ssidLen > 32) ssidLen = 32;   // 802.11 limit



    // ── BSSID aleatorio (MAC of the "router" falso) ───────────────────────

    // the primeros 2 bits of the first byte the ponemos a 0 for that

    // parezca a MAC unicast normal, no multicast

    for (int i = 0; i < 6; i++) {

        beaconFrame[10 + i] = (uint8_t)random(0, 256);

        beaconFrame[16 + i] = beaconFrame[10 + i];   // BSSID duplicado

    }

    beaconFrame[10] &= 0xFE;   // quitar bit multicast



    // ── SSID tag ───────────────────────────────────────────────────────

    beaconFrame[SSID_LENGTH_OFFSET] = (uint8_t)ssidLen;

    memcpy(&beaconFrame[SSID_START_OFFSET], ssid, ssidLen);



    // ── Tail with canal ─────────────────────────────────────────────────

    int tailOffset = SSID_START_OFFSET + ssidLen;

    memcpy(&beaconFrame[tailOffset], FRAME_TAIL, FRAME_TAIL_SIZE);

    beaconFrame[tailOffset + FRAME_TAIL_SIZE - 1] = (uint8_t)channel;



    int frameLen = tailOffset + FRAME_TAIL_SIZE;



    // ── transmit ──────────────────────────────────────────────────────

    // Canal 0 = interfaz WIFI_IF_STA (requiere that the canal ya esté fijado)

    esp_wifi_80211_tx(WIFI_IF_STA, beaconFrame, frameLen, false);



    beaconsSent++;

}



// ═══════════════════════════════════════════════════════════════════════════

// DISCLAIMER

// ═══════════════════════════════════════════════════════════════════════════

static bool showDisclaimer() {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);



    drawStringBig(30, 10, "BEACON SPAM", UI_SELECT, 2);

    tft.drawFastHLine(0, 50, 320, UI_SELECT);



    int y = 62;

    drawStringCustom(10, y, "Transmits fake WiFi beacon frames", UI_MAIN, 1); y += 12;

    drawStringCustom(10, y, "appearing in nearby device lists.", UI_MAIN, 1); y += 20;



    drawStringCustom(10, y, "Does not disrupt real connections,", UI_ACCENT, 1); y += 12;

    drawStringCustom(10, y, "only broadcasts virtual SSIDs.", UI_ACCENT, 1); y += 20;



    drawStringCustom(10, y, "Use responsibly in authorized labs:", UI_MAIN, 1); y += 12;

    drawStringCustom(20, y, "- RF research and testing", UI_ACCENT, 1); y += 12;

    drawStringCustom(20, y, "- Do NOT run in critical zones", UI_ACCENT, 1); y += 20;



    drawStringCustom(10, y, "You are responsible for operation.", UI_MAIN, 1);



    tft.drawFastHLine(0, 210, 320, UI_MAIN);

    drawStringCustom(10, 218, "OK: ACCEPT    BACK: CANCEL", UI_ACCENT, 1);



    while (true) {

        if (navEnterPressed()) {

            beep(2200, 60);

            while (navEnterPressed() || navBackPressed()) delay(5);

            delay(100);

            return true;

        }

        if (navBackPressed() || navUpPressed() || navDownPressed()) {

            beep(1000, 80);

            while (navBackPressed() || navUpPressed() || navDownPressed())

                delay(5);

            delay(100);

            return false;

        }

        delay(20);

    }

}



// MODE SELECTION MENU

static void drawModeMenuRow(int idx, bool selected) {

    int y = 40 + idx * 26;

    uint16_t bg = selected ? UI_SELECT : UI_BG;

    uint16_t colMain = selected ? UI_BG : UI_MAIN;

    uint16_t colSub  = selected ? UI_BG : UI_ACCENT;



    tft.fillRect(5, y - 2, 310, 22, bg);

    drawStringCustom(15, y + 2, MODE_NAMES[idx], colMain, 2);

    drawStringCustom(15, y + 14, MODE_DESCS[idx], colSub, 1);

}



static void drawModeMenu(int cursor) {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_MAIN);



    drawStringBig(10, 8, "BEACON SPAM", UI_MAIN, 1);

    tft.drawFastHLine(0, 30, 320, UI_ACCENT);



    int totalItems = MODE_COUNT;



    for (int i = 0; i < totalItems; i++) {

        drawModeMenuRow(i, i == cursor);

    }



    tft.drawFastHLine(0, 215, 320, UI_ACCENT);

    drawStringCustom(10, 222, "OK:START   BACK/OK(H):BACK", UI_ACCENT, 1);

}



static int selectMode() {

    int cursor = 0;

    int totalItems = MODE_COUNT;



    drawModeMenu(cursor);



    while (true) {

        if (navBackPressed()) {

            beep(1000, 40);

            while (navBackPressed()) delay(5);

            return -1;

        }

        if (navUpPressed()) {

            int oldCursor = cursor;

            cursor = (cursor - 1 + totalItems) % totalItems;

            beep(2100, 20);

            tft.startWrite();

            drawModeMenuRow(oldCursor, false);

            drawModeMenuRow(cursor, true);

            tft.endWrite();

            delay(70);

        }

        if (navDownPressed()) {

            int oldCursor = cursor;

            cursor = (cursor + 1) % totalItems;

            beep(2100, 20);

            tft.startWrite();

            drawModeMenuRow(oldCursor, false);

            drawModeMenuRow(cursor, true);

            tft.endWrite();

            delay(70);

        }

        if (navEnterPressed()) {

            bool held = waitOkReleaseWasLong();

            beep(held ? 1000 : 1800, 40);

            delay(100);

            if (held) return -1;

            return cursor;

        }

        delay(20);

    }

}



// ═══════════════════════════════════════════════════════════════════════════

// screen of attack

// ═══════════════════════════════════════════════════════════════════════════

static void drawAttackFrame() {

    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, UI_SELECT);

    tft.drawRect(1, 1, 318, 238, UI_SELECT);



    drawStringBig(10, 10, "BEACON SPAM", UI_SELECT, 1);

    drawStringCustom(200, 16, "[BROADCAST]", TFT_GREEN, 1);

    tft.drawFastHLine(0, 36, 320, UI_SELECT);



    drawStringCustom(10, 44, "Mode: " + String(MODE_NAMES[activeMode]),

                     UI_MAIN, 1);



    // Labels estáticos

    drawStringCustom(10, 62,  "Channel:",      UI_ACCENT, 1);

    drawStringCustom(10, 82,  "Current SSID:", UI_ACCENT, 1);

    drawStringCustom(10, 122, "Beacons:",      UI_ACCENT, 1);

    drawStringCustom(10, 142, "Rate:",         UI_ACCENT, 1);



    // Activity bar frame

    tft.drawRect(10, 170, 300, 16, UI_ACCENT);



    tft.drawFastHLine(0, 210, 320, UI_SELECT);

    drawStringCustom(10, 220, "BACK / OK(HOLD): STOP", TFT_RED, 1);

}



static void drawAttackStats(unsigned long pkts, float rate) {

    // Channel

    tft.fillRect(80, 58, 230, 14, TFT_BLACK);

    drawStringCustom(80, 62, "CH " + String(currentChannel), TFT_YELLOW, 1);



    // Current SSID (may contain emojis = wider width, visually truncate)

    tft.fillRect(10, 95, 300, 18, TFT_BLACK);

    String s = currentSSID;

    drawStringFit(20, 97, s, TFT_CYAN, 280, 1);



    // Beacons

    tft.fillRect(80, 118, 230, 14, TFT_BLACK);

    drawStringCustom(80, 122, String(pkts), TFT_GREEN, 1);



    // Rate

    tft.fillRect(80, 138, 230, 14, TFT_BLACK);

    char rbuf[24];

    snprintf(rbuf, sizeof(rbuf), "%d beacons/s", (int)rate);

    drawStringCustom(80, 142, String(rbuf), TFT_CYAN, 1);



    // Activity bar animada

    tft.fillRect(12, 172, 296, 12, TFT_BLACK);

    int fillW = random(60, 290);

    tft.fillRect(12, 172, fillW, 12, UI_SELECT);

}



// ═══════════════════════════════════════════════════════════════════════════

// loop for attack

// ═══════════════════════════════════════════════════════════════════════════

static void runAttackLoop() {

    drawAttackFrame();

    beep(2400, 40); delay(20);

    beep(3000, 60); delay(20);

    beep(3600, 80);



    // ── Setup WiFi for raw tx ──────────────────────────────────────────

    WiFi.mode(WIFI_MODE_NULL);

    esp_wifi_set_promiscuous(false);



    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&cfg);

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wifi_start();

    esp_wifi_set_promiscuous(true);   // enables tx raw

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    currentChannel = 1;



    int ssidCount = countSSIDsForMode(activeMode);



    beaconsSent = 0;

    unsigned long startMs         = millis();

    unsigned long lastStatsUpdate = millis();

    unsigned long lastChannelHop  = millis();

    unsigned long lastPktCount    = 0;

    float rate = 0;



    // Channels that vamos a rotar: 1, 6, 11 (the no-overlapping in 2.4GHz)

    const int channels[] = {1, 6, 11};

    int channelIdx = 0;



    int ssidIdx = 0;



    bool stopAttack = false;

    unsigned long okPressStart = 0;

    bool okHeld = false;



    while (!stopAttack) {

        if (navBackPressed()) {

            stopAttack = true;

            while (navBackPressed()) delay(5);

            continue;

        }



        // ── send a beacon ──────────────────────────────────────────

        const char* ssid = getSSIDForMode(activeMode, ssidIdx);

        currentSSID = String(ssid);

        sendBeacon(ssid, currentChannel);



        ssidIdx = (ssidIdx + 1) % ssidCount;



        // ── Channel hop each 500 ms ───────────────────────────────────

        if (millis() - lastChannelHop > 500) {

            channelIdx = (channelIdx + 1) % 3;

            currentChannel = channels[channelIdx];

            esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);

            lastChannelHop = millis();

        }



        // ── Update UI each 250 ms ──────────────────────────────────────

        if (millis() - lastStatsUpdate > 250) {

            unsigned long now   = millis();

            unsigned long delta = beaconsSent - lastPktCount;

            unsigned long dt    = now - lastStatsUpdate;

            rate = (delta * 1000.0f) / dt;

            lastPktCount    = beaconsSent;

            lastStatsUpdate = now;

            drawAttackStats(beaconsSent, rate);

        }



        // ── Detectar OK HOLD ───────────────────────────────────────────

        if (navEnterPressed()) {

            if (!okHeld) {

                okPressStart = millis();

                okHeld = true;

            } else if (millis() - okPressStart > 500) {

                stopAttack = true;

            }

        } else {

            okHeld = false;

        }



        // yield al watchdog + pequeño delay for rate ~150-200 pkt/s

        yield();

        delay(5);

    }



    // ── Cleanup ─────────────────────────────────────────────────────────

    esp_wifi_set_promiscuous(false);

    esp_wifi_stop();

    esp_wifi_deinit();

    delay(100);



    beep(1800, 40); delay(20);

    beep(1200, 60);



    while (navEnterPressed() || navBackPressed()) delay(5);

    delay(150);

}



// ═══════════════════════════════════════════════════════════════════════════

// MAIN

// ═══════════════════════════════════════════════════════════════════════════

void runBeaconSpam() {

    // Wait for button release of OK

    while (navEnterPressed() || navBackPressed()) delay(5);

    delay(100);



    if (!showDisclaimer()) return;



    while (true) {

        int mode = selectMode();

        if (mode < 0) break;



        activeMode = (SpamMode)mode;

        runAttackLoop();

    }

}

