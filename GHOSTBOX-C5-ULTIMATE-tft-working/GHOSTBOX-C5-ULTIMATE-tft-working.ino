// ==============================================================================
//  GHOSTBOX S3 ULTIMATE · ARDUINO IDE ENTRY POINT
//  Board : ESP32S3 Dev Module
//  Partition: Huge App (3MB no OTA)
//
//  REQUIRED LIBRARIES (install via Library Manager):
//    - TFT_eSPI by Bodmer >= 2.5.43
//    - RF24 by nRF24 >= 1.4.10
//    - ArduinoJson by bblanchon >= 7.0.4
//    - TinyGPSPlus by mikalhart >= 1.1.0
//    - Adafruit NeoPixel by Adafruit >= 1.12.0
//    - IRremoteESP8266 by crankyoldgit >= 2.8.6
//    - SmartRC-CC1101 by LSatan >= 2.5.7
//
//  TFT_eSPI: User_Setup.h is NOT needed — defines below configure the driver.
// ==============================================================================

// ── TFT_eSPI Driver Config (ILI9488 3.5" 480x320) ───────────────────────────
#define USER_SETUP_LOADED   1
#define ILI9488_DRIVER      1
#define USE_ILI9488         1
#define TFT_WIDTH         320
#define TFT_HEIGHT        480
#define TFT_MISO           13
#define TFT_MOSI           11
#define TFT_SCLK           12
#define TFT_CS             10
#define TFT_DC             21
#define TFT_RST            14
#define SPI_FREQUENCY      27000000
#define SPI_READ_FREQUENCY  6000000
#define SUPPORT_TRANSACTIONS 1

// setup() and loop() are defined in Main.cpp — Arduino IDE compiles all
// .cpp files in the sketch folder automatically.
