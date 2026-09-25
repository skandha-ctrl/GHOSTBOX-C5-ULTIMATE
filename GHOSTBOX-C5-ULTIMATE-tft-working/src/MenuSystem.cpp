#include "MenuSystem.h"

#include <string.h>
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "Settings.h"

// Wi-Fi Tools
#include "WifiScanner.h"
#include "WifiRadar.h"
#include "WifiDirectionFinder.h"
#include "WifiChannelScanner.h"
#include "Deauther.h"
#include "BeaconSpam.h"
#include "ProbeSniffer.h"
#include "ThreatMonitor.h"
#include "EvilPortal.h"
#include "Karma.h"
#include "PacketMonitor.h"
#include "LabTestEngine.h"
#include "WifiAudit.h"
#include "NetCut.h"
#include "RogueResponder.h"
#include "JamDetector.h"
#include "C5Status.h"

// Bluetooth / BLE Tools
#include "BLEScanner.h"
#include "BLEDeviceRadar.h"
#include "BLEInspector.h"
#include "BLESpam.h"
#include "BLEAudit.h"
#include "BLEIPhoneRemote.h"
#include "BTDisruptor.h"
#include "bt_jammer.h"
#include "MouseJack.h"

// Sub-GHz & RF Tools
#include "CC1101Tools.h"
#include "RadioScanner.h"
#include "RadioJammer.h"
#include "NRFDiagnostics.h"
#include "jammer.h"
#include "FlipperPlayer.h"
#include "OscillatorRfLab.h"

// Infrared Tools
#include "IrProtocolDecoder.h"
#include "IrSniffer.h"
#include "IrVirtualRemotes.h"
#include "IrProtocolScanner.h"
#include "IrNightVisionDetector.h"
#include "IrProximityTest.h"
#include "IrAnalyzer.h"

// BadUSB & RFID/NFC Tools
#include "BadUsb.h"
#include "RfidNfc.h"

// Network & Web Tools
#include "WebDashboard.h"
#include "WifiConfig.h"
#include "ClockWeather.h"

// Peripherals & Signals
#include "PeripheralTools.h"
#include "SignalTools.h"

// Visuals & Screensavers
#include "Screensaver.h"

#include "SystemInfo.h"
#include "SettingsMenu.h"
#include "About.h"
#include "CellularAudit.h"
#include "AiRfClassifier.h"
#include "TacticalMeshWaterfall.h"

static constexpr uint16_t MOD_BG     = TFT_BLACK;
static constexpr uint16_t MOD_TEXT   = TFT_WHITE;
static constexpr uint16_t MOD_LINE   = TFT_WHITE;
static constexpr uint16_t MOD_INVERT = TFT_BLACK;
static constexpr uint16_t MOD_GLOW   = 0x07FF;
static constexpr uint16_t MOD_GLOW_2 = 0x03EF;

static constexpr int MENU_TOP = 42;
static constexpr int ROW_H    = 31;
static constexpr int ROW_X    = 8;
static constexpr int ROW_W    = 152;
static constexpr int ROW_BOX_H = 27;
static constexpr int PREVIEW_X = 168;
static constexpr int PREVIEW_Y = 42;
static constexpr int PREVIEW_W = 144;
static constexpr int PREVIEW_H = 156;

// Forward Declarations of Handlers
static void handlerAiTactical();
static void handlerWifi();
static void handlerBT();
static void handlerRadio();
static void handlerCellular();
static void handlerIR();
static void handlerBadUsbRfid();
static void handlerNetwork();
static void handlerPeripherals();
static void handlerVisuals();
static void handlerSystem();

static const MainMenuEntry MAIN_ENTRIES[] = {
    { "AI TACTICAL", "TinyML EW & Radar", ICON_SYSTEM,      handlerAiTactical  },
    { "WIFI 2.4/5G", "Dual-band toolkit", ICON_WIFI,        handlerWifi        },
    { "BLUETOOTH",   "BLE spam & mouse",  ICON_BLUETOOTH,   handlerBT          },
    { "SUB-GHZ & RF","CC1101 + Flipper",  ICON_RADIO,       handlerRadio       },
    { "CELLULAR DEF","BTS & IMSI Sniffer",ICON_RADIO,       handlerCellular    },
    { "INFRARED",    "IR decoders & TV",  ICON_IR,          handlerIR          },
    { "BADUSB/RFID", "Ducky & NFC Suite", ICON_PERIPHERALS, handlerBadUsbRfid  },
    { "NETWORK",     "Web dash & NTP",    ICON_NETWORK,     handlerNetwork     },
    { "PERIPHERALS", "I2C/SPI/GPS tools", ICON_PERIPHERALS, handlerPeripherals },
    { "VISUALS/LED", "NeoPixel & Matrix", ICON_VISUALS,     handlerVisuals     },
    { "SYSTEM",      "Config & info",     ICON_SYSTEM,      handlerSystem      }
};

static const int MAIN_COUNT = sizeof(MAIN_ENTRIES) / sizeof(MainMenuEntry);
static int currentEntry = 0;

static constexpr int SUBMENU_STATE_SLOTS = 16;
struct SubMenuState {
    const char* title;
    int cursor;
    int scrollOffset;
};
static SubMenuState subMenuStates[SUBMENU_STATE_SLOTS] = {};
static int nextSubMenuStateSlot = 0;

static SubMenuState* stateForSubMenu(const char* title) {
    for (int i = 0; i < SUBMENU_STATE_SLOTS; i++) {
        if (subMenuStates[i].title && strcmp(subMenuStates[i].title, title) == 0) {
            return &subMenuStates[i];
        }
    }
    for (int i = 0; i < SUBMENU_STATE_SLOTS; i++) {
        if (!subMenuStates[i].title) {
            subMenuStates[i] = { title, 0, 0 };
            return &subMenuStates[i];
        }
    }
    SubMenuState* state = &subMenuStates[nextSubMenuStateSlot];
    nextSubMenuStateSlot = (nextSubMenuStateSlot + 1) % SUBMENU_STATE_SLOTS;
    *state = { title, 0, 0 };
    return state;
}

static void drawBitmapCenteredIn(int x0, int width, int y, const String& text,
                                 uint16_t color, int size, FontType font) {
    int w = getTextWidth(text, size, font);
    int x = x0 + (width - w) / 2;
    if (x < x0) x = x0;
    if (font == FONT_BIG) drawStringBig(x, y, text, color, size);
    else drawStringCustom(x, y, text, color, size);
}

static void drawBitmapRight(int xRight, int y, const String& text,
                            uint16_t color, int size, FontType font) {
    int w = getTextWidth(text, size, font);
    int x = xRight - w;
    if (font == FONT_BIG) drawStringBig(x, y, text, color, size);
    else drawStringCustom(x, y, text, color, size);
}

static void drawWindowBorders() {
    tft.drawRect(0, 0, 320, 240, MOD_LINE);
    tft.drawFastHLine(0, 34, 320, MOD_LINE);
    tft.drawFastHLine(0, 206, 320, MOD_LINE);
}

static void drawMainHeader() {
    tft.fillRect(1, 1, 318, 32, MOD_BG);
    drawStringBig(10, 8, "GHOSTBOX C5", MOD_TEXT, 1);
    drawStringCustom(140, 13, "DUAL-BAND", MOD_GLOW, 1);

    String count = String(currentEntry + 1) + "/" + String(MAIN_COUNT);
    drawBitmapRight(306, 12, count, MOD_TEXT, 1, FONT_SMALL);

    tft.drawRect(236, 24, 70, 5, MOD_GLOW);
    int fillW = ((currentEntry + 1) * 68) / MAIN_COUNT;
    if (fillW > 0) tft.fillRect(237, 25, fillW, 3, MOD_TEXT);
}

static void drawMainFooter() {
    tft.fillRect(1, 207, 318, 32, MOD_BG);
    drawStringCustom(10, 218, "UP/DN/ENC: SELECT", MOD_TEXT, 1);
    drawBitmapRight(310, 218, "OK: OPEN", MOD_TEXT, 1, FONT_SMALL);
}

static void clearMainArea() {
    tft.fillRect(1, 35, 318, 171, MOD_BG);
}

static void drawGlowBox(int x, int y, int w, int h) {
    tft.drawRect(x - 2, y - 2, w + 4, h + 4, MOD_GLOW_2);
    tft.drawRect(x - 1, y - 1, w + 2, h + 2, MOD_GLOW);
    tft.drawRect(x, y, w, h, MOD_TEXT);
}

static void drawMenuRow(int slot, int entryIndex, bool selected) {
    int y = MENU_TOP + slot * ROW_H;
    const MainMenuEntry& item = MAIN_ENTRIES[entryIndex];

    if (selected) {
        drawGlowBox(ROW_X, y, ROW_W, ROW_BOX_H);
        tft.fillRect(ROW_X + 1, y + 1, ROW_W - 2, ROW_BOX_H - 2, MOD_TEXT);
        drawStringBig(ROW_X + 8, y + 7, item.title, MOD_INVERT, 1);
    } else {
        tft.drawRect(ROW_X, y, ROW_W, ROW_BOX_H, MOD_LINE);
        tft.fillRect(ROW_X + 1, y + 1, ROW_W - 2, ROW_BOX_H - 2, MOD_BG);
        drawStringBig(ROW_X + 8, y + 7, item.title, MOD_TEXT, 1);
    }
}

static void drawMainMenuList() {
    int totalVisible = min(5, MAIN_COUNT);
    int start = currentEntry - 2;
    if (start < 0) start = 0;
    if (start + totalVisible > MAIN_COUNT) start = MAIN_COUNT - totalVisible;

    for (int i = 0; i < totalVisible; i++) {
        int idx = start + i;
        drawMenuRow(i, idx, idx == currentEntry);
    }
}

static void drawPreviewCard() {
    tft.fillRect(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, MOD_BG);
    drawGlowBox(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H);

    const MainMenuEntry& item = MAIN_ENTRIES[currentEntry];
    int heroX = PREVIEW_X + PREVIEW_W / 2;
    int heroY = PREVIEW_Y + 54;

    drawIcon(heroX, heroY, item.icon, MOD_GLOW);

    drawBitmapCenteredIn(PREVIEW_X, PREVIEW_W, PREVIEW_Y + 104, item.title,
                         MOD_TEXT, 1, FONT_BIG);
    drawBitmapCenteredIn(PREVIEW_X, PREVIEW_W, PREVIEW_Y + 128, item.subtitle,
                         MOD_GLOW, 1, FONT_SMALL);
}

static void renderMainMenuFull() {
    clearMainArea();
    drawMainHeader();
    drawMainFooter();
    drawMainMenuList();
    drawPreviewCard();
}

int runSubMenu(const char* title, const char* items[], int count) {
    if (count <= 0) return -1;
    SubMenuState* state = stateForSubMenu(title);
    if (state->cursor >= count) state->cursor = count - 1;
    if (state->cursor < 0) state->cursor = 0;

    static constexpr int SUB_PAGE_SIZE = 5;
    auto fixScroll = [&]() {
        if (state->cursor < state->scrollOffset) {
            state->scrollOffset = state->cursor;
        } else if (state->cursor >= state->scrollOffset + SUB_PAGE_SIZE) {
            state->scrollOffset = state->cursor - SUB_PAGE_SIZE + 1;
        }
    };
    fixScroll();

    auto render = [&]() {
        tft.fillScreen(MOD_BG);
        drawWindowBorders();

        // Header
        tft.fillRect(1, 1, 318, 32, MOD_BG);
        drawStringBig(10, 8, title, MOD_TEXT, 1);
        String countStr = String(state->cursor + 1) + "/" + String(count);
        drawBitmapRight(306, 12, countStr, MOD_TEXT, 1, FONT_SMALL);

        // List
        for (int i = 0; i < SUB_PAGE_SIZE; i++) {
            int idx = state->scrollOffset + i;
            if (idx >= count) break;
            int y = MENU_TOP + i * ROW_H;
            bool sel = (idx == state->cursor);

            if (sel) {
                drawGlowBox(16, y, 288, ROW_BOX_H);
                tft.fillRect(17, y + 1, 286, ROW_BOX_H - 2, MOD_TEXT);
                drawStringCustom(26, y + 8, items[idx], MOD_INVERT, 1);
            } else {
                tft.drawRect(16, y, 288, ROW_BOX_H, MOD_LINE);
                drawStringCustom(26, y + 8, items[idx], MOD_TEXT, 1);
            }
        }

        // Footer
        tft.fillRect(1, 207, 318, 32, MOD_BG);
        drawStringCustom(10, 218, "UP/DN: MOVE", MOD_TEXT, 1);
        drawBitmapRight(310, 218, "OK: EXEC  BACK: EXIT", MOD_TEXT, 1, FONT_SMALL);
    };

    render();
    flushNavInput(120);

    while (true) {
        neopixelLoop();
        NavAction act = readNavAction(130);
        if (act == NAV_UP) {
            if (state->cursor > 0) {
                state->cursor--;
                fixScroll();
                render();
                clickTone();
            }
        } else if (act == NAV_DOWN) {
            if (state->cursor < count - 1) {
                state->cursor++;
                fixScroll();
                render();
                clickTone();
            }
        } else if (act == NAV_ENTER) {
            clickTone();
            flushNavInput(120);
            return state->cursor;
        } else if (act == NAV_BACK) {
            clickTone();
            flushNavInput(120);
            return -1;
        }
        delay(10);
    }
}

// ==============================================================================
// SUBMENU HANDLERS
// ==============================================================================

static void handlerAiTactical() {
    static const char* items[] = {
        "Edge-AI RF Classifier (TinyML)",
        "Real-Time Spectral Waterfall (40FPS)",
        "Off-Grid Sub-GHz Mesh Intercom",
        "Dead-Man's Black Box Shield",
        "Generate 1-Click PenTest Report"
    };
    while (true) {
        int sel = runSubMenu("AI EW & TACTICAL", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runAiThreatClassifierHUD(); break;
            case 1: runSpectralWaterfallHUD(); break;
            case 2: runOffGridTacticalMeshHUD(); break;
            case 3: runDeadMansSwitchHUD(); break;
            case 4: generateOneClickAuditReport(); break;
        }
    }
}

static void handlerWifi() {
    static const char* items[] = {
        "Scanner & Details",
        "Wi-Fi Radar",
        "Direction Finder",
        "Channel Spectrum (2.4G/5G)",
        "Deauther (2.4G & 5G)",
        "Beacon Spammer",
        "NetCut ARP Spoofer (Bruce)",
        "LLMNR / NBT Responder (Bruce)",
        "Wi-Fi Jam Detector (Bruce)",
        "Probe Sniffer",
        "Threat Monitor",
        "Evil Portal",
        "Karma Honeypot",
        "Packet Monitor",
        "BWifiKill Lab Engine",
        "Audit Assistant",
        "ESP32-C5 5G Link Status"
    };
    while (true) {
        int sel = runSubMenu("WIFI 2.4G & 5G", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runWifiScan(); break;
            case 1: runWifiRadar(); break;
            case 2: runWifiDirectionFinder(); break;
            case 3: runWifiChannelScanner(); break;
            case 4: runDeauther(); break;
            case 5: runBeaconSpam(); break;
            case 6: runNetCut(); break;
            case 7: runRogueResponder(); break;
            case 8: runJamDetector(); break;
            case 9: runProbeSniffer(); break;
            case 10: runThreatMonitor(); break;
            case 11: runEvilPortal(); break;
            case 12: runKarma(); break;
            case 13: runPacketMonitor(); break;
            case 14: runLabTestEngine(); break;
            case 15: runWifiAudit(); break;
            case 16: runC5Status(); break;
        }
    }
}

static void handlerBT() {
    static const char* items[] = {
        "BLE Scanner",
        "BLE Device Radar",
        "BLE Inspector",
        "BLE Multi-Spam",
        "MouseJack Injector (Bruce)",
        "Tracker / AirTag Audit",
        "iPhone BLE Remote",
        "BT Disruptor",
        "BT 2.4G Jammer"
    };
    while (true) {
        int sel = runSubMenu("BLUETOOTH & BLE", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runBLEScanner(); break;
            case 1: runBLEDeviceRadar(); break;
            case 2: runBLEInspector(); break;
            case 3: runBLESpam(); break;
            case 4: runMouseJack(); break;
            case 5: runBLEAudit(); break;
            case 6: runBLEIPhoneRemote(); break;
            case 7: runBTDisruptor(); break;
            case 8: runBtJammer(); break;
        }
    }
}

static void handlerRadio() {
    static const char* items[] = {
        "CC1101 Sub-GHz Suite",
        "Flipper .sub/.ir & Waterfall (Bruce)",
        "Faraday RF Oscillator Lab",
        "nRF24 Spectrum Scanner",
        "nRF24 Radio Jammer",
        "nRF24 Diagnostics",
        "Wideband RF Jammer"
    };
    while (true) {
        int sel = runSubMenu("SUB-GHZ & RF", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runCC1101Tools(); break;
            case 1: runFlipperPlayerMenu(); break;
            case 2: runRfOscillatorLabMenu(); break;
            case 3: runRadioScanner(); break;
            case 4: runRadioJammer(); break;
            case 5: runNRFDiagnostics(); break;
            case 6: runJammer(); break;
        }
    }
}

static void handlerIR() {
    static const char* items[] = {
        "Protocol Decoder",
        "IR Sniffer & Replay",
        "Universal Remote",
        "TV-B-Gone / Brute",
        "Night Vision Detector",
        "Proximity Test",
        "Signal Analyzer"
    };
    while (true) {
        int sel = runSubMenu("INFRARED TOOLS", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runIrProtocolDecoder(); break;
            case 1: runIrSniffer(); break;
            case 2: runIrVirtualRemotes(); break;
            case 3: runIrProtocolScanner(); break;
            case 4: runIrNightVisionDetector(); break;
            case 5: runIrProximityTest(); break;
            case 6: runIrAnalyzer(); break;
        }
    }
}

static void handlerBadUsbRfid() {
    static const char* items[] = {
        "BadUSB / DuckyScript (Bruce)",
        "Mouse Jiggler Anti-Lock (Bruce)",
        "RFID 125kHz Reader & Cloner (Bruce)",
        "NFC Mifare Classic 1K/4K (Bruce)",
        "NFC Amiibo NTAG215 (Bruce)",
        "NFC Contactless EMV Reader (Bruce)"
    };
    while (true) {
        int sel = runSubMenu("BADUSB & RFID/NFC", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runBadUSBMenu(); break;
            case 1: runMouseJiggler(); break;
            case 2: runRfid125kHzReader(); break;
            case 3: runNfcMifareClassic(); break;
            case 4: runAmiiboEmulator(); break;
            case 5: runEmvCardReader(); break;
        }
    }
}

static void handlerCellular() {
    runCellularAuditMenu();
}

static void handlerNetwork() {
    static const char* items[] = {
        "Web Dashboard Server",
        "Wi-Fi Station Config",
        "Clock & Weather NTP"
    };
    while (true) {
        int sel = runSubMenu("NETWORK & WEB", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runWebDashboard(); break;
            case 1: runWifiConfig(); break;
            case 2: runClockWeather(); break;
        }
    }
}

static void handlerPeripherals() {
    static const char* items[] = {
        "Hardware Diagnostics (All Devices)",
        "Bus & GPS Diagnostics",
        "Signal Generator & Logic"
    };
    while (true) {
        int sel = runSubMenu("PERIPHERALS", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runHardwareDiagnostics(); break;
            case 1: runPeripheralTools(); break;
            case 2: runSignalTools(); break;
        }
    }
}

static void handlerVisuals() {
    static const char* items[] = {
        "NeoPixel Mode Cycle",
        "Screensaver Matrix"
    };
    while (true) {
        int sel = runSubMenu("VISUALS & LED", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: {
                uint8_t nextMode = (neopixelMode + 1) % 6;
                setNeoPixelMode(nextMode);
                saveSettings();
                clickTone();
                break;
            }
            case 1: runScreensaver(); break;
        }
    }
}

static void handlerSystem() {
    static const char* items[] = {
        "System Information",
        "Hardware Diagnostics (All Devices)",
        "ESP32-C5 5G Link Status",
        "Settings & Preferences",
        "About Ghostbox"
    };
    while (true) {
        int sel = runSubMenu("SYSTEM", items, sizeof(items) / sizeof(items[0]));
        if (sel == -1) break;
        switch (sel) {
            case 0: runSystemInfo(); break;
            case 1: runHardwareDiagnostics(); break;
            case 2: runC5Status(); break;
            case 3: runSettingsMenu(); break;
            case 4: runAbout(); break;
        }
    }
}

void runMainMenu() {
    tft.fillScreen(MOD_BG);
    drawWindowBorders();
    renderMainMenuFull();
    flushNavInput(150);

    while (true) {
        neopixelLoop();
        NavAction action = readNavAction(140);
        if (action == NAV_UP) {
            if (currentEntry > 0) currentEntry--;
            else currentEntry = MAIN_COUNT - 1;
            renderMainMenuFull();
            clickTone();
        } else if (action == NAV_DOWN) {
            if (currentEntry < MAIN_COUNT - 1) currentEntry++;
            else currentEntry = 0;
            renderMainMenuFull();
            clickTone();
        } else if (action == NAV_ENTER) {
            clickTone();
            flushNavInput(150);
            MAIN_ENTRIES[currentEntry].handler();
            tft.fillScreen(MOD_BG);
            drawWindowBorders();
            renderMainMenuFull();
            flushNavInput(150);
        }
        delay(10);
    }
}
