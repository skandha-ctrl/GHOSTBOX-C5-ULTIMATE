# GHOSTBOX C5 ULTIMATE · Complete Hardware Wiring & Pin-to-Pin Diagram

This document contains the complete **Pin-to-Pin Interconnection Matrix**, **Power Distribution Scheme**, and **System Architecture Diagram** for assembling the **GHOSTBOX-C5-ULTIMATE** on breadboard, perfboard, or custom PCB.

---

## 1. System Architecture Diagram

```
                              ┌─────────────────────────────────────────┐
                              │           POWER SUBSYSTEM               │
                              │ 18650 Li-Ion (3.7V - 4.2V)              │
                              │    │                                    │
                              │    ├──> TP4056 USB-C Charger & Protect  │
                              │    ├──> VBAT Pin on SIM800L (2A Peak)   │
                              │    └──> AMS1117-3.3V Step-Down LDO      │
                              │           │ (Clean 3.3V Rail)           │
                              └───────────┼─────────────────────────────┘
                                          │
            ┌─────────────────────────────┴──────────────────────────────┐
            │                                                            │
    ┌───────▼────────────────────────────────────────────────────────────▼────────┐
    │                         ESP32-C5 DUAL-BAND SoC                             │
    │                   (2.4GHz + 5.0GHz Wi-Fi 6 + BLE 5)                        │
    └───────┬──────────────┬──────────────┬──────────────┬──────────────┬────────┘
            │              │              │              │              │
      Shared SPI Bus   I2C Bus        UART Buses      IO & ADC       IR Transceiver
      (SCK,MOSI,MISO)  (SDA, SCL)    (Cellular/GPS)   (Keypad/Sound) (TX / RX)
            │              │              │              │              │
    ┌───────┴──────┐ ┌─────┴──────┐ ┌─────┴──────┐ ┌─────┴──────┐ ┌─────┴──────┐
    │ • ST7789 TFT │ │ • Si5351A  │ │ • SIM800L  │ │ • 4x Keys  │ │ • VS1838B  │
    │ • 3x nRF24   │ │ • PN532    │ │ • NEO-6M   │ │ • Rotary   │ │ • 940nm IR │
    │ • CC1101     │ │   NFC/RFID │ │   GPS      │ │ • WS2812B  │ │   LED      │
    │ • MicroSD    │ │            │ │            │ │ • Piezo    │ │            │
    └──────────────┘ └────────────┘ └────────────┘ └────────────┘ └────────────┘
```

---

## 2. Complete Pin-to-Pin Connection Matrix

### 2.1 Display (2.8" ST7789 SPI TFT - 240×320)
| ST7789 Pin | ESP32-C5 Pin | Function / Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Power (from 3.3V Rail) |
| **GND** | **GND** | Ground |
| **SCL / SCK** | **GPIO 12** | SPI Clock (Shared Bus) |
| **SDA / MOSI**| **GPIO 11** | SPI Data In (Shared Bus) |
| **CS** | **GPIO 7** | Dedicated Display Chip Select |
| **DC / RS** | **GPIO 2** | Data / Command Control |
| **RES / RST** | **GPIO 3** | Hardware Reset |
| **BLK / LED** | **GPIO 8** | Backlight Control (or 3.3V) |

---

### 2.2 Shared SPI Transceivers & Storage

All SPI modules share **SCK (GPIO 12)**, **MOSI (GPIO 11)**, and **MISO (GPIO 13)** with dedicated CS/CE lines:

| Module Name | Pin on Module | ESP32-C5 Pin | Purpose |
| :--- | :--- | :--- | :--- |
| **Sub-GHz CC1101** | CSN / CS | **GPIO 10** | Chip Select |
| | GDO0 | **GPIO 11** | Data 0 (or SPI MOSI) |
| | GDO2 | **GPIO 12** | Data 2 (or SPI SCK) |
| | VCC / GND | **3.3V / GND** | Power (Max 3.6V) |
| **nRF24L01+ #1** | CE | **GPIO 13** | Chip Enable Radio 1 |
| | CSN | **GPIO 14** | Chip Select Radio 1 |
| **nRF24L01+ #2** | CE | **GPIO 16** | Chip Enable Radio 2 |
| | CSN | **GPIO 17** | Chip Select Radio 2 |
| **nRF24L01+ #3** | CE | **GPIO 1** | Chip Enable Radio 3 |
| | CSN | **GPIO 0** | Chip Select Radio 3 |
| **MicroSD Slot** | CS | **GPIO 15** | SD Card Chip Select |

> [!TIP]
> Place a **10µF to 47µF electrolytic capacitor** directly across the `VCC` and `GND` pins of each nRF24 and CC1101 module to eliminate voltage drops during high-power packet transmission.

---

### 2.3 I2C Bus Devices (NFC / RFID)

The PN532 connects to the dedicated I2C bus:

| Module | Module Pin | ESP32-C5 Pin | I2C Address |
| :--- | :--- | :--- | :--- |
| **PN532 NFC Module** | SDA (I2C Mode) | **GPIO 20** | `0x24` |
| (Set DIP: 1=ON, 2=OFF)| SCL (I2C Mode) | **GPIO 21** | |
| | VCC / GND | **3.3V / GND** | |


---

### 2.4 Cellular Diagnostic & Sniffer Modem (SIM800L / SIM7600)

| SIM800L Pin | Connected To | Notes |
| :--- | :--- | :--- |
| **NET / ANT** | External Spring / SMA Antenna | Required for GSM/LTE reception |
| **VCC / 5V** | **Battery RAW (3.7V - 4.2V)** | **DO NOT connect to 3.3V rail!** GSM burst currents need up to 2A from the Li-Ion cell. |
| **GND** | **Common System GND** | Common ground with ESP32 |
| **SIM_TXD** | **GPIO 4 (ESP32 RX)** | UART Data from Modem to ESP32 |
| **SIM_RXD** | **GPIO 5 (ESP32 TX)** | UART AT Commands from ESP32 to Modem |
| **RST** | Open / Floating | Not strictly required |

---

### 2.5 GPS Receiver (NEO-6M)

| NEO-6M Pin | ESP32-C5 Pin | Purpose |
| :--- | :--- | :--- |
| **TX** | **GPIO 18** | NMEA Sentence Stream into ESP32 |
| **RX** | **GPIO 17** | Configuration from ESP32 |
| **VCC / GND** | **3.3V / GND** | 3.3V Power |

---

### 2.6 Infrared Transceiver (VS1838B + 940nm LED)

| Component | Pin | ESP32-C5 Pin / Connection |
| :--- | :--- | :--- |
| **VS1838B IR Receiver** | OUT / DATA | **GPIO 18** (Pull-up enabled) |
| | VCC / GND | **3.3V / GND** |
| **940nm IR LED (TX)** | Anode (+) | 3.3V Rail (via 100Ω Resistor) |
| | Cathode (-) | Collector of 2N2222 Transistor |
| **2N2222 NPN Driver** | Base | **GPIO 19** (via 1kΩ Resistor) |
| | Emitter | **GND** |

---

### 2.7 User Controls, Audio, LED & Power Monitoring

| Function | Hardware Part | ESP32-C5 Pin | Wiring |
| :--- | :--- | :--- | :--- |
| **Nav UP** | Tactile Switch | **GPIO 23** | Pin to Switch $\rightarrow$ GND (Internal Pull-Up) |
| **Nav DOWN** | Tactile Switch | **GPIO 24** | Pin to Switch $\rightarrow$ GND (Internal Pull-Up) |
| **Nav OK / SELECT**| Tactile Switch | **GPIO 25** | Pin to Switch $\rightarrow$ GND (Internal Pull-Up) |
| **Nav BACK / EXIT**| Tactile Switch | **GPIO 26** | Pin to Switch $\rightarrow$ GND (Internal Pull-Up) |
| **Rotary Encoder** | CLK / DT / SW | **GPIO 40, 39, 38** | Standard 3-pin Quadrature + Button to GND |
| **Piezo Buzzer** | Passive Buzzer | **GPIO 22** | Positive to Pin, Negative to GND |
| **WS2812B RGB LED** | DIN (Data In) | **GPIO 9** | 5V/3.3V VCC, Data to GPIO 9, GND |
| **Battery Sense** | 100k + 100k Divider | **GPIO 0 (ADC)** | Center of 100k:100k resistor divider across Battery (+) and GND |

---

## 3. Power Distribution Schematic

```
 [ USB-C Port ]
       │
       ▼
 [ TP4056 Charger ] ───> [ 18650 Li-Ion Cell (3.7V - 4.2V) ]
                               │
                ┌──────────────┴────────────────┐
                │                               │
                ▼                               ▼
       [ SIM800L / SIM7600 ]          [ AMS1117-3.3V LDO ]
       (High Current 2A Peak)                   │
                                                ▼
                                    ┌────────────────────────┐
                                    │ Clean 3.3V Power Rail  │
                                    ├────────────────────────┤
                                    │ • ESP32-C5 MCU         │
                                    │ • ST7789 TFT Display   │
                                    │ • 3x nRF24L01+ Radios  │
                                    │ • CC1101 Transceiver   │
                                    │ • Si5351A Synthesizer  │
                                    │ • PN532 NFC Module     │
                                    │ • MicroSD Card Module  │
                                    │ • NEO-6M GPS           │
                                    └────────────────────────┘
```
