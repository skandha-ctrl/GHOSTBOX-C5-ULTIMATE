#pragma once

#include <Arduino.h>

extern bool soundEnabled;
extern int soundVolume;
extern uint8_t neopixelMode;

void initSettings();
void saveSettings();
void loadSettings();