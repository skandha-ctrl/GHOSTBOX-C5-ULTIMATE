#pragma once

#include <Arduino.h>
#include <vector>
#include <string>

void neopixelSetup();
void neopixelLoop();
void setNeoPixelColour(const std::string& colour);
void setNeoPixelRgb(uint8_t r, uint8_t g, uint8_t b);
void setNeoPixelMode(uint8_t mode);
void flash(int numberOfFlashes, const std::vector<std::string>& colors, const std::string& finalColour = "null");
void neopixelAlert();
void neopixelSuccess();
void neopixelError();
void neopixelActivity();

