# GHOSTBOX C5 ULTIMATE 🛰️⚡

**GHOSTBOX C5 ULTIMATE** is an advanced open-source cybersecurity, RF research, and hardware auditing platform designed for the **ESP32-S3 / ESP32-C5 Dual-Band (2.4 GHz + 5.0 GHz Wi-Fi 6, BLE 5, 802.15.4)** architectures.

---

## 📁 Repository Structure

```
├── GHOSTBOX-C5-ULTIMATE-tft-working/   # Main firmware (PlatformIO / Arduino IDE)
│   ├── src/                             # Core C++ modules, UI engines, drivers
│   ├── enclosure_model.scad             # 3D printable tactical mil-spec enclosure (OpenSCAD)
│   ├── platformio.ini                   # Build environments (ILI9488, ST7789, ESP32-S3/C5)
│   ├── WIRING_PINOUT_GUIDE.md           # Pinout & hardware wiring guide
│   └── README.md                        # Firmware documentation & complete feature map
│
├── C5_COPROCESSOR/                      # High-speed 5 GHz co-processor firmware (ESP32-C5)
│   └── C5_COPROCESSOR.ino               # Promiscuous 5GHz sniffer & deauth engine
│
└── .gitignore                           # Git ignore rules for build & IDE artifacts
```

---

## 🎮 Unified Master Menu Layout (11 Suites)

1. **🧠 AI TACTICAL**: TinyML RF classification, 40 FPS dual-band spectral waterfall, off-grid mesh intercom, dead-man incident logger.
2. **📡 WIFI 2.4/5G**: Dual-band scanning, direction finder / foxhunting, channel spectrum, deauther, beacon flood, ARP spoofer, LLMNR responder, probe sniffer, threat monitor, evil portal, karma honeypot, packet monitor.
3. **📶 BLUETOOTH**: BLE scanner, device radar, GATT explorer, multi-advertisement flood, MouseJack injector, Apple/Android remote controls.
4. **📻 SUB-GHZ & RF**: CC1101 transceiver (315/433/868/915 MHz), Flipper Zero `.sub`/`.ir` player & waterfall, nRF24 multi-radio array & spectrum analyzer.
5. **📱 CELLULAR DEF**: SIM800L / SIM7600 BTS tower wardriving, rogue base station / IMSI catcher shield, SMS fuzzer, encrypted telemetry.
6. **🔦 INFRARED**: Multi-protocol decoding & sniffer, universal remote, TV-B-Gone, night vision camera detector.
7. **💻 BADUSB & RFID/NFC**: DuckyScript keystroke injection, Mouse Jiggler, 125kHz RFID cloner, 13.56MHz NFC dumper, Amiibo emulator, contactless EMV reader.
8. **🌐 NETWORK**: Web dashboard server with PCAP download, WiFi station manager, NTP synchronized clock & weather.
9. **🛠 PERIPHERALS**: I2C/SPI bus diagnostics, GPS wardriving, logic & signal generator.
10. **🎨 VISUALS/LED**: NeoPixel RGB animations, Matrix / Starfield idle screensaver.
11. **⚙️ SYSTEM**: Hardware telemetry, flash / heap monitors, persistent NVS preferences, About Ghostbox.

---

## 🚀 Getting Started

### Option 1: PlatformIO (Recommended)
1. Install [PlatformIO IDE](https://platformio.org/) in VS Code.
2. Open the project folder `GHOSTBOX-C5-ULTIMATE-tft-working`.
3. Select your display target environment (`esp32-s3-ili9488` or `esp32-s3-st7789`).
4. Click **Build** and **Upload**.

### Option 2: Arduino IDE
1. Open `GHOSTBOX-C5-ULTIMATE-tft-working/GHOSTBOX-C5-ULTIMATE-tft-working.ino`.
2. Board: **ESP32S3 Dev Module** (or ESP32-C5).
3. Partition Scheme: **Huge App (3MB No OTA)**.
4. Install required libraries listed in `platformio.ini` via the Arduino Library Manager.
5. Compile and flash.

---

## 📜 License & Disclaimer
This repository is developed for educational, defense testing, and authorized cybersecurity auditing purposes only. Ensure compliance with all local laws and regulations before operating on any wireless network or RF frequency.
