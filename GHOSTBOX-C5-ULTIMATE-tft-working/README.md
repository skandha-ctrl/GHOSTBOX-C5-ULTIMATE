# GHOSTBOX C5 ULTIMATE 🛰️⚡ (with Bruce Firmware Integration)

**GHOSTBOX C5 ULTIMATE** is the consolidated cybersecurity & RF exploration platform engineered specifically for the **ESP32-C5 Dual-Band (2.4 GHz + 5.0 GHz Wi-Fi 6, BLE 5, 802.15.4)** RISC-V SoC.

It merges **100% of the features** from **5 major projects**:
1. **2ndsuccess**: CC1101 Sub-GHz raw capture & replay, ProtoKill, rolling code analysis, NeoPixel RGB status animations, BLE spoofing.
2. **BWifiKill-BW16-5Ghz**: Native 5GHz Wi-Fi scanning, 5GHz deauthentication, 5GHz beacon flood, promiscuous sniffer, and Lab Test Performance Engine.
3. **GHOSTBOX-MINI-ESP32**: Ghostbox Mini ST7789 / ILI9488 HUD UI, Ajolote Sprite animations, PepeDraw custom bitmap fonts, Virtual Keyboard, Threat Monitor, Karma Honeypot, Evil Portal.
4. **ESP32-TOOLS-PRO**: CC1101 Sub-GHz suite, Full Infrared Suite (IR Decoders, Universal Remote, TV-B-Gone, Night Vision Camera Finder, Proximity sensor), Wi-Fi Radar, Direction Finder / Foxhunter, Channel Spectrum Analyzer, BLE iPhone Remote, BLE Device Radar, Web Dashboard.
5. **Bruce Firmware (New Integrations)**:
   - **BadUSB & BadBLE DuckyScript Keystroke Injector**: Hak5 DuckyScript interpreter & payload launcher.
   - **Mouse Jiggler**: Anti-screen lock orbit generator.
   - **RFID 125kHz Suite**: EM4100 / T5577 / HID Prox reader and cloner.
   - **NFC 13.56MHz Suite**: Mifare Classic 1K/4K dump, Nintendo Amiibo NTAG215 emulator, Contactless EMV payment reader.
   - **Flipper Zero File Player**: Native player for Flipper Zero `.sub` radio captures and `.ir` remote captures.
   - **Sub-GHz RF Waterfall**: Real-time high-speed waterfall spectrum visualizer across 300MHz–928MHz.
   - **NetCut ARP Spoofer**: Selectively cut off LAN target IP connections.
   - **LLMNR / NBT-NS Rogue Responder**: Captures Windows NTLMv2 hashes.
   - **NRF24 MouseJack Injector**: 2.4GHz wireless mouse dongle keystroke injection.
   - **Wi-Fi Jam & Deauth Detector**: Real-time attack monitor with audio/visual sirens.

---

## 🎮 Unified Master Menu Layout (11 Categories)

```
[ GHOSTBOX C5 MAIN MENU ]
│
├── 🧠 1. AI TACTICAL (Hackathon Special / Edge-AI)
│   ├── Edge-AI RF Classifier (TinyML: UAV/Pineapple/Flipper/IMSI detection)
│   ├── Real-Time Spectral Waterfall (40 FPS Dual-Band 2.4G/5G/Sub heatmap)
│   ├── Off-Grid Sub-GHz Mesh Intercom (CC1101 P2P encrypted messaging)
│   ├── Dead-Man's Black Box Shield (Auto-wipe & incident forensic logger)
│   └── Generate 1-Click PenTest Report (Export markdown/PDF to SD)
│
├── 📡 2. WIFI 2.4/5G
│   ├── Scanner & Details (Dual-band 2.4G + 5G OUI detection)
│   ├── Wi-Fi Radar (Proximity & RSSI radar)
│   ├── Direction Finder (Foxhunting signal locator)
│   ├── Channel Spectrum (2.4G 1-14 & 5G 36-165 spectrum bars)
│   ├── Deauther (Targeted client & broadcast 2.4G/5G deauth)
│   ├── Beacon Spammer (Custom, Rickroll, Random, Apple/Android)
│   ├── NetCut ARP Spoofer [Bruce]
│   ├── LLMNR / NBT-NS Responder (Hash Catcher) [Bruce]
│   ├── Wi-Fi Jam & Deauth Detector [Bruce]
│   ├── Probe Sniffer (Probe requests, PMKID, EAPOL grabber)
│   ├── Threat Monitor (Rogue AP & attack detection)
│   ├── Evil Portal (Captive portal phishing + SD logs)
│   ├── Karma Honeypot (Automatic client lure)
│   ├── Packet Monitor (Live traffic throughput graph)
│   ├── BWifiKill Lab Engine (Quantitative signal test & packet loss)
│   └── Audit Assistant
│
├── 📶 3. BLUETOOTH
│   ├── BLE Scanner
│   ├── BLE Device Radar
│   ├── BLE Inspector (GATT Explorer)
│   ├── BLE Multi-Spam (iOS 17, Android FastPair, Samsung Buds, SwiftPair)
│   ├── NRF24 MouseJack Injector [Bruce]
│   ├── Tracker / AirTag Audit
│   ├── iPhone BLE Remote (Camera, Volume, Media)
│   ├── BT Disruptor
│   └── BT 2.4G Jammer
│
├── 📻 4. SUB-GHZ & RF
│   ├── CC1101 Sub-GHz Suite (315/433/868/915 MHz capture & replay)
│   ├── Flipper .sub / .ir Player & Waterfall [Bruce]
│   ├── Faraday RF Oscillator Lab (CC1101 + nRF24 Array)
│   ├── nRF24 Spectrum Scanner
│   ├── nRF24 Radio Jammer
│   ├── nRF24 Diagnostics (Supports 3x nRF modules)
│   └── Wideband RF Jammer
│
├── 📱 5. CELLULAR DEF (SIM800L / SIM7600)
│   ├── BTS Tower Scanner (Wardrive: MCC, MNC, LAC, CID, RSSI)
│   ├── IMSI Catcher / Rogue BTS Shield (A5/0 cipher downgrade warning)
│   ├── Tactical SMS & PDU Fuzzer (Class 0 Flash SMS & Ping triggers)
│   └── Out-of-Band Cellular Beacon (Encrypted telemetry with RF fallback)
│
├── 🔦 6. INFRARED
│   ├── Protocol Decoder (NEC, Sony, RC5/6, Samsung, LG, Panasonic)
│   ├── IR Sniffer & Replay
│   ├── Universal Remote (TV, AC, Audio, Projector)
│   ├── TV-B-Gone / Power Brute
│   ├── Night Vision Detector (850nm / 940nm camera finder)
│   ├── Proximity Test
│   └── Signal Analyzer
│
├── 💻 7. BADUSB & RFID/NFC [Bruce]
│   ├── BadUSB DuckyScript Injector (Windows/Mac/Linux/SD)
│   ├── Mouse Jiggler Anti-Lock
│   ├── RFID 125kHz Reader & Cloner (EM4100 / T5577)
│   ├── NFC Mifare Classic 1K/4K Key Dumper
│   ├── NFC Amiibo NTAG215 Emulator
│   └── NFC Contactless EMV Payment Card Reader
│
├── 🌐 8. NETWORK
│   ├── Web Dashboard Server (Remote PCAP download & web UI)
│   ├── Wi-Fi Station Config
│   └── Clock & Weather NTP
│
├── 🛠 9. PERIPHERALS
│   ├── Bus & GPS Diagnostics (I2C/SPI test + GPS wardriving)
│   └── Signal Generator & Logic
│
├── 🎨 10. VISUALS/LED
│   ├── NeoPixel RGB Mode Cycle (Ghostbox pulse, Rainbow, Radar, Police)
│   └── Screensaver Matrix (Digital rain & Ajolote HUD)
│
└── ⚙️ 11. SYSTEM
    ├── System Information (ESP32-C5 RISC-V status & heap)
    ├── Settings & Preferences
    └── About Ghostbox
```

---

## 🔌 Hardware Configuration (ESP32-C5)

| Peripheral | Pin | Notes |
| :--- | :--- | :--- |
| **SPI Bus** | `MOSI: 4`, `MISO: 5`, `SCK: 6` | High-speed shared bus with hardware arbitrations |
| **Display CS / DC / RST / BL** | `CS: 7`, `DC: 2`, `RST: 3`, `BL: 8` | ST7789 2.8" (240×320) or ILI9488 3.5" (480×320) |
| **CC1101 Sub-GHz** | `CS: 10`, `GDO0: 11`, `GDO2: 12` | 300MHz–928MHz Transceiver |
| **nRF24 Radios #1, #2, #3** | `#1 CE: 13, CS: 14` \| `#2 CE: 16, CS: 17` \| `#3 CE: 1, CS: 0` | 3× 2.4GHz GFSK Transceivers |
| **MicroSD Card** | `CS: 15` | FAT32 logging & DuckyScript storage |
| **Infrared RX / TX** | `RX: 18`, `TX: 19` | 38kHz VS1838B & 940nm IR LED |
| **NeoPixel RGB** | `GPIO 9` | WS2812B RGB LED |
| **GPS UART** | `RX: 20`, `TX: 21` | NEO-6M / NEO-M8N at 9600 baud |
| **Piezo Buzzer** | `GPIO 22` | Navigation audio & attack sirens |
| **Navigation Controls** | `UP: 23`, `DOWN: 24`, `OK: 25`, `BACK: 26` | Active LOW with internal pullup |
| **Rotary Encoder** | `CLK: 27`, `DT: 28`, `SW: 25` | Optional rotary navigation |
| **Battery ADC** | `GPIO 0` | 100k/100k resistor divider |
