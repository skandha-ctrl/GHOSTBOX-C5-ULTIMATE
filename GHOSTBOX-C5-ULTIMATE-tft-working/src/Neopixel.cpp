#include "Neopixel.h"
#include "Pins.h"
#include "Settings.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel* pixels = nullptr;
static bool neoPixelActive = true;
static unsigned long lastAnimMs = 0;
static uint16_t animStep = 0;

void neopixelSetup() {
#if defined(NEOPIXEL_PIN) && (NEOPIXEL_PIN >= 0)
    if (!pixels) {
        pixels = new Adafruit_NeoPixel(NEOPIXEL_NUM, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    }
    if (pixels) {
        pixels->begin();
        pixels->setBrightness(40);
        pixels->clear();
        pixels->show();
    }
    neoPixelActive = (neopixelMode > 0);
#endif
}

void setNeoPixelRgb(uint8_t r, uint8_t g, uint8_t b) {
#if defined(NEOPIXEL_PIN) && (NEOPIXEL_PIN >= 0)
    if (pixels) {
        pixels->setPixelColor(0, pixels->Color(r, g, b));
        pixels->show();
    }
#endif
}

void setNeoPixelColour(const std::string& colour) {
#if defined(NEOPIXEL_PIN) && (NEOPIXEL_PIN >= 0)
    if (!pixels) return;
    uint32_t colorValue = 0;
    if (colour == "red") colorValue = pixels->Color(40, 0, 0);
    else if (colour == "green") colorValue = pixels->Color(0, 40, 0);
    else if (colour == "blue") colorValue = pixels->Color(0, 0, 40);
    else if (colour == "yellow") colorValue = pixels->Color(35, 35, 0);
    else if (colour == "purple") colorValue = pixels->Color(30, 0, 35);
    else if (colour == "cyan") colorValue = pixels->Color(0, 35, 35);
    else if (colour == "white") colorValue = pixels->Color(30, 30, 30);
    else if (colour == "null" || colour == "0" || colour == "off") colorValue = pixels->Color(0, 0, 0);

    pixels->setPixelColor(0, colorValue);
    pixels->show();
#endif
}

void setNeoPixelMode(uint8_t mode) {
    neopixelMode = mode;
    neoPixelActive = (mode > 0);
    if (!neoPixelActive) {
        setNeoPixelColour("null");
    }
}

void flash(int numberOfFlashes, const std::vector<std::string>& colors, const std::string& finalColour) {
    if (numberOfFlashes <= 0 || colors.empty()) return;
    for (int i = 0; i < numberOfFlashes; ++i) {
        for (const auto& color : colors) {
            setNeoPixelColour(color);
            delay(120);
        }
    }
    setNeoPixelColour(finalColour);
}

void neopixelAlert() {
    setNeoPixelRgb(255, 0, 0);
    delay(50);
    setNeoPixelColour("null");
}

void neopixelSuccess() {
    setNeoPixelRgb(0, 255, 60);
    delay(50);
    setNeoPixelColour("null");
}

void neopixelError() {
    setNeoPixelRgb(255, 0, 0);
    delay(60);
    setNeoPixelColour("null");
}

void neopixelActivity() {
    setNeoPixelRgb(0, 180, 255);
    delay(20);
    setNeoPixelColour("null");
}

static uint32_t Wheel(byte WheelPos) {
    if (!pixels) return 0;
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return pixels->Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return pixels->Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return pixels->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void neopixelLoop() {
    if (!neoPixelActive || neopixelMode == 0 || !pixels) return;
    unsigned long now = millis();
    if (now - lastAnimMs < 40) return;
    lastAnimMs = now;
    animStep++;

    switch (neopixelMode) {
        case 1: { // Ghostbox Pulse (Cyan to Magenta)
            uint8_t wave = (animStep % 128);
            uint8_t bri = wave < 64 ? (wave * 4) : ((128 - wave) * 4);
            uint8_t r = (bri * 30) / 255;
            uint8_t g = (bri * 10) / 255;
            uint8_t b = (bri * 50) / 255;
            setNeoPixelRgb(r, g, b);
            break;
        }
        case 2: { // Rainbow Cycle
            uint8_t pos = (animStep & 255);
            uint32_t c = Wheel(pos);
            pixels->setPixelColor(0, c);
            pixels->show();
            break;
        }
        case 3: { // Alert / Activity Pulse
            uint8_t wave = (animStep % 64);
            uint8_t r = wave < 32 ? (wave * 6) : ((64 - wave) * 6);
            setNeoPixelRgb(r, 0, 0);
            break;
        }
        case 4: { // Police Strobe
            if ((animStep % 20) < 10) {
                setNeoPixelRgb(0, 0, 60);
            } else {
                setNeoPixelRgb(60, 0, 0);
            }
            break;
        }
        case 5: { // Radar Spin (Green beacon)
            uint8_t wave = (animStep % 50);
            uint8_t g = wave < 10 ? 80 : 0;
            setNeoPixelRgb(0, g, 0);
            break;
        }
        default:
            break;
    }
}
