#include "PepeDraw.h"

#include "Settings.h"

#include "Pins.h"

#include "NVSStore.h"

#include "WifiConfig.h"

#include "SoundUtils.h"



static int cursor = 0;

static const int MENU_ITEMS = 3;


// ═══════════════════════════════════════════════════════════════════════════

// FORGET WIFI NETWORK — Erases saved credentials from NVS

// ═══════════════════════════════════════════════════════════════════════════

static void runForgetWifi() {

    // Wait for button release of OK

    while (navEnterPressed() || navBackPressed()) delay(5);
    delay(100);



    // Caso 1: no there is network saved

    if (!wifiConfigHasSaved()) {

        tft.fillScreen(TFT_BLACK);

        tft.drawRect(0, 0, 320, 240, TFT_WHITE);

        drawStringCustom(30, 10, "WIFI CONFIG", TFT_WHITE, 3);

        tft.drawFastHLine(0, 45, 320, TFT_WHITE);



        drawStringCustom(40, 90, "NO SAVED NETWORK", UI_ACCENT, 2);

        drawStringCustom(40, 130, "No WiFi credentials are", TFT_WHITE, 1);

        drawStringCustom(40, 145, "currently stored in NVS.", TFT_WHITE, 1);

        drawStringCustom(10, 222, "OK/BACK: Return", UI_ACCENT, 1);


        beep(1500, 60);



        while (!navEnterPressed() && !navBackPressed()) delay(20);
        beep(1800, 40);

        while (navEnterPressed() || navBackPressed()) delay(5);
        delay(100);

        return;

    }



    // Caso 2: there is network saved → confirmar

    String savedSSID = wifiConfigGetSavedSSID();



    tft.fillScreen(TFT_BLACK);

    tft.drawRect(0, 0, 320, 240, TFT_RED);

    tft.drawRect(1, 1, 318, 238, TFT_RED);



    drawStringCustom(40, 12, "FORGET WIFI", TFT_RED, 3);

    tft.drawFastHLine(0, 50, 320, TFT_RED);



    drawStringCustom(20, 70, "Saved Network:", TFT_WHITE, 1);



    if (getTextWidth(savedSSID, 2) <= 280) {
        drawStringCustom(20, 90, savedSSID, UI_SELECT, 2);
    } else {
        drawStringFit(20, 95, savedSSID, UI_SELECT, 280, 1);
    }


    drawStringCustom(20, 130, "Delete saved credentials?", TFT_WHITE, 1);

    drawStringCustom(20, 144, "Next time you use a WiFi tool,", UI_ACCENT, 1);

    drawStringCustom(20, 156, "you will need to select a", UI_ACCENT, 1);

    drawStringCustom(20, 168, "network and authenticate again.", UI_ACCENT, 1);



    tft.drawFastHLine(0, 210, 320, TFT_RED);

    drawStringCustom(10, 220, "OK: CONFIRM FORGET   BACK: CANCEL", UI_ACCENT, 1);


    while (true) {

        if (navEnterPressed()) {

            beep(1200, 80);

            while (navEnterPressed() || navBackPressed()) delay(5);
            delay(100);



            // delete credenciales

            wifiConfigForget();



            // Screen of confirmación

            tft.fillScreen(TFT_BLACK);

            tft.drawRect(0, 0, 320, 240, TFT_WHITE);

            drawStringCustom(40, 90, "NETWORK FORGOTTEN", TFT_GREEN, 3);

            drawStringCustom(40, 140, "Credentials cleared from NVS.", TFT_WHITE, 1);



            beep(2400, 50); delay(30);

            beep(3000, 80);

            delay(1500);

            return;

        }

        if (navBackPressed() || navUpPressed() || navDownPressed()) {
            beep(2000, 40);

            while (navBackPressed() || navUpPressed() || navDownPressed())
                delay(5);

            delay(100);

            return;

        }

        delay(20);

    }

}



// MAIN SETTINGS MENU



void drawSettings() {

    tft.fillScreen(TFT_BLACK);



    tft.drawRect(0, 0, 320, 240, TFT_WHITE);

    drawStringCustom(30, 10, "SETTINGS", TFT_WHITE, 3);

    tft.drawFastHLine(0, 45, 320, TFT_WHITE);



    String soundStr = soundEnabled ? "ON" : "OFF";



    for (int i = 0; i < MENU_ITEMS; i++) {

        int y = 60 + (i * 38);



        if (i == cursor) {

            tft.fillRect(10, y - 5, 300, 30, TFT_WHITE);

        }



        uint16_t textColor = (i == cursor) ? TFT_BLACK : TFT_WHITE;



        if (i == 0) {

            drawStringCustom(20, y, "SOUND: " + soundStr, textColor, 2);

        }

        else if (i == 1) {

            drawStringCustom(20, y, "VOLUME: " + String(soundVolume),

                             textColor, 2);

        }

        else if (i == 2) {

            drawStringCustom(20, y, "FORGET WIFI", textColor, 2);

        }

    }

    tft.drawFastHLine(0, 210, 320, TFT_WHITE);
    drawStringCustom(10, 220, "OK: SELECT   BACK/OK(H): EXIT", UI_ACCENT, 1);
}


void runSettings() {



    cursor = 0;



    // Evitar doble OK

    while (navEnterPressed() || navBackPressed());
    delay(150);



    bool exitMenu = false;



    drawSettings();



    while (!exitMenu) {

        if (navBackPressed()) {
            exitMenu = true;
            beep(1000, 40);
            while (navBackPressed()) delay(5);
            delay(120);
            continue;
        }


        if (navDownPressed()) {

            cursor = (cursor + 1) % MENU_ITEMS;

            drawSettings();

            delay(200);

        }



        if (navUpPressed()) {

            cursor = (cursor - 1 + MENU_ITEMS) % MENU_ITEMS;

            drawSettings();

            delay(200);

        }



        if (navEnterPressed()) {
            bool held = waitOkReleaseWasLong();
            if (held) {
                exitMenu = true;
                beep(1000, 40);
                delay(120);
                continue;
            }

            if (cursor == 0) {
                soundEnabled = !soundEnabled;
                nvsSetBool("sound_on", soundEnabled);

            }

            else if (cursor == 1) {

                soundVolume++;

                if (soundVolume > 5) soundVolume = 1;

                nvsSetInt("sound_vol", soundVolume);

            }

            else if (cursor == 2) {

                // Wait for button release before of entrar a the sub-screen

                while (navEnterPressed() || navBackPressed());
                delay(100);

                runForgetWifi();

            }

            drawSettings();
            delay(150);
        }


        delay(10);

    }

}
