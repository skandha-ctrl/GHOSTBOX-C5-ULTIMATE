#pragma once

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// FARADAY RF OSCILLATOR LAB (CC1101 SUB-GHZ + nRF24 2.4GHz + ESP32-C5 5GHz)
// ═══════════════════════════════════════════════════════════════════════════

enum RfOscillatorBand {
    OSC_BAND_SUB_GHZ,    // 300 MHz - 928 MHz (CC1101)
    OSC_BAND_2_4_GHZ,    // 2400 MHz - 2525 MHz (nRF24)
    OSC_BAND_5_8_GHZ     // 5150 MHz - 5850 MHz (ESP32-C5)
};

void runRfOscillatorLabMenu();
void startContinuousCarrier(uint32_t freqHz, uint8_t powerDbm);
void startFastFrequencySweep(uint32_t startFreqHz, uint32_t stopFreqHz, uint16_t stepKHz, uint16_t dwellMs);
void stopRfOscillator();
