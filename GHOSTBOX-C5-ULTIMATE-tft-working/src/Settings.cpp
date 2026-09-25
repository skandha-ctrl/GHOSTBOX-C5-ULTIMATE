#include "Settings.h"
#include "NVSStore.h"

bool soundEnabled = true;
int  soundVolume  = 3;   // rango 1-5
uint8_t neopixelMode = 1;

void loadSettings() {
    soundEnabled = nvsGetBool("sound_on", true);
    soundVolume  = nvsGetInt("sound_vol", 3);
    neopixelMode = (uint8_t)nvsGetInt("neo_mode", 1);
}

void saveSettings() {
    nvsSetBool("sound_on", soundEnabled);
    nvsSetInt("sound_vol", soundVolume);
    nvsSetInt("neo_mode", (int)neopixelMode);
}

void initSettings() {
    loadSettings();
}