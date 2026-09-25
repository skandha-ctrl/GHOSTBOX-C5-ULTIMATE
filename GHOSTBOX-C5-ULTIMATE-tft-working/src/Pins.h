#pragma once

#include <Arduino.h>
#include "Input.h"

// Shared SPI bus: TFT + nRF24 #1 + nRF24 #2 (Matching working_tft.ino for ESP32-S3)
#define SCK_PIN   12
#define MOSI_PIN  11
#define MISO_PIN  13

// Slower nRF24 SPI improves detection on shared bus / wired modules.
#define NRF_SPI_SPEED 1000000

// nRF24 Radio Configuration (Supports Triple Modules #1, #2, #3)
#define NRF_RADIO_COUNT 3
#define NRF2_ENABLED    1
#define NRF3_ENABLED    1

// nRF24 #1 (Primary 2.4GHz Radio)
#define NRF1_CE_PIN   9
#define NRF1_CSN_PIN  3

// nRF24 #2 (Secondary 2.4GHz Radio)
#define NRF2_CE_PIN   6
#define NRF2_CSN_PIN  16

// nRF24 #3 (Tertiary 2.4GHz Radio)
#define NRF3_CE_PIN   19
#define NRF3_CSN_PIN  20

// Backwards-compatible aliases for single-module code
#define CE_PIN        NRF1_CE_PIN
#define CSN_PIN       NRF1_CSN_PIN

// Sub-GHz CC1101 Transceiver (CS on GPIO 0, shared SPI bus GPIO 13)
#define CC1101_CS_PIN       0
#define CC1101_CSN_PIN      CC1101_CS_PIN
#define CC1101_GDO0_PIN     46
#define CC1101_GDO2_PIN     47
#define CC1101_TX_DATA_PIN  CC1101_GDO0_PIN

// MicroSD Card
#define SD_CS_PIN     15
#define SD_SCK_PIN    SCK_PIN
#define SD_MOSI_PIN   MOSI_PIN
#define SD_MISO_PIN   MISO_PIN  // GPIO 13 shared bus

// TFT Display SPI Pins (ILI9488 / ST7789 matching working_tft.ino)
#define TFT_CS_PIN    10
#define TFT_DC_PIN    21
#define TFT_RST_PIN   14
#define TFT_LED_PIN   -1

// GPS NEO-6M (Hardware UART2 on GPIO 18 & 17)
#define GPS_RX_PIN   18
#define GPS_TX_PIN   17
#define GPS_BAUD     9600

// Buttons, wired to GND when pressed
#define BTN_UP    1
#define BTN_DOWN  2
#define BTN_OK    42
#define BTN_BACK  41

// Rotary encoder, wired to GND when active
#define ENC_CLK_PIN  40
#define ENC_DT_PIN   39
#define ENC_SW_PIN   38

// Battery ADC input (-1 if unassigned)
#define VBAT_ADC_PIN -1

#define OK_LONGPRESS_MS 650

static inline bool waitOkReleaseWasLong(unsigned long holdMs = OK_LONGPRESS_MS) {
    unsigned long start = millis();
    bool wasLong = false;
    while (digitalRead(BTN_OK) == LOW || digitalRead(ENC_SW_PIN) == LOW) {
        if (millis() - start >= holdMs) wasLong = true;
        delay(5);
    }
    if (digitalRead(BTN_BACK) == LOW) {
        while (digitalRead(BTN_BACK) == LOW) delay(5);
        return true;
    }
    return wasLong;
}

// Cellular Modem (SIM800L / SIM7600) on UART1/UART2
#define CELL_RX_PIN  4
#define CELL_TX_PIN  5
#define CELL_BAUD    9600

// I2C Bus (PN532 NFC Reader / Writer 0x24)
#define I2C_SDA_PIN  7
#define I2C_SCL_PIN  8

// ==============================================================================
// ESP32-C5 5GHz CO-PROCESSOR LINK (High-Speed UART)
// ==============================================================================
#define C5_RX_PIN    48   // S3 Pin 48 (RX) <-- C5 GPIO11 (TX)
#define C5_TX_PIN    45   // S3 Pin 45 (TX) --> C5 GPIO12 (RX)
#define C5_BAUD      115200

// Status LED (WS2812B NeoPixel) & Sound
#define NEOPIXEL_PIN -1
#define NEOPIXEL_NUM 1
#define BUZZER_PIN   -1

// Infrared Transceiver (Disabled to free pins)
#define IR_RX_PIN    -1
#define IR_TX_PIN    -1

