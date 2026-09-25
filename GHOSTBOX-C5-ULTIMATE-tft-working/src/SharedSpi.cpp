#include "SharedSpi.h"
#include <SPI.h>
#include "Pins.h"

// ── Dedicated SPI buses for modules that can't tristate MISO ─────────────
// SD card : MISO = GPIO 4 (isolated from nRF24 bus on GPIO 13)
// CC1101 : MISO = GPIO 5 (isolated from nRF24 bus on GPIO 13)
// SCK/MOSI are shared (outputs only — no bus fight possible on output lines)
SPIClass spiSD(HSPI);   // SD card dedicated bus
SPIClass spiCC(FSPI);   // CC1101 dedicated bus (FSPI = SPI2 on S3)

void sharedSpiInitPins(bool radioCeLow) {
    if (TFT_CS_PIN >= 0) { pinMode(TFT_CS_PIN, OUTPUT); digitalWrite(TFT_CS_PIN, HIGH); }
    if (SD_CS_PIN >= 0)  { pinMode(SD_CS_PIN, OUTPUT); digitalWrite(SD_CS_PIN, HIGH); }
    if (CC1101_CS_PIN >= 0) { pinMode(CC1101_CS_PIN, OUTPUT); digitalWrite(CC1101_CS_PIN, HIGH); }
    if (NRF1_CSN_PIN >= 0) { pinMode(NRF1_CSN_PIN, OUTPUT); digitalWrite(NRF1_CSN_PIN, HIGH); }
    if (NRF2_CSN_PIN >= 0) { pinMode(NRF2_CSN_PIN, OUTPUT); digitalWrite(NRF2_CSN_PIN, HIGH); }
    if (NRF3_CSN_PIN >= 0) { pinMode(NRF3_CSN_PIN, OUTPUT); digitalWrite(NRF3_CSN_PIN, HIGH); }

    if (NRF1_CE_PIN >= 0) { pinMode(NRF1_CE_PIN, OUTPUT); digitalWrite(NRF1_CE_PIN, LOW); }
    if (NRF2_CE_PIN >= 0) { pinMode(NRF2_CE_PIN, OUTPUT); digitalWrite(NRF2_CE_PIN, LOW); }
    if (NRF3_CE_PIN >= 0) { pinMode(NRF3_CE_PIN, OUTPUT); digitalWrite(NRF3_CE_PIN, LOW); }

    sharedSpiRelease(radioCeLow);
}

void sharedSpiRelease(bool radioCeLow) {
    if (TFT_CS_PIN >= 0)    digitalWrite(TFT_CS_PIN, HIGH);
    if (SD_CS_PIN >= 0)     digitalWrite(SD_CS_PIN, HIGH);
    if (CC1101_CS_PIN >= 0) digitalWrite(CC1101_CS_PIN, HIGH);
    if (NRF1_CSN_PIN >= 0)  digitalWrite(NRF1_CSN_PIN, HIGH);
    if (NRF2_CSN_PIN >= 0)  digitalWrite(NRF2_CSN_PIN, HIGH);
    if (NRF3_CSN_PIN >= 0)  digitalWrite(NRF3_CSN_PIN, HIGH);

    if (radioCeLow) {
        if (NRF1_CE_PIN >= 0) digitalWrite(NRF1_CE_PIN, LOW);
        if (NRF2_CE_PIN >= 0) digitalWrite(NRF2_CE_PIN, LOW);
        if (NRF3_CE_PIN >= 0) digitalWrite(NRF3_CE_PIN, LOW);
    }

    delayMicroseconds(50);
}

void sharedSpiBeginMainBus() {
    sharedSpiRelease(true);
    SPI.end();
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
    pinMode(MISO_PIN, INPUT_PULLUP);
}

void sharedSpiPrepareDisplay(bool radioCeLow) {
    sharedSpiRelease(radioCeLow);
}

void sharedSpiPrepareRadio(bool radioCeLow) {
    sharedSpiRelease(radioCeLow);
}

void sharedSpiPrepareSd() {
    sharedSpiRelease(true);
}
