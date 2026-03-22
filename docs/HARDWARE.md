# Hardware

## Bill of Materials

### Car Unit

| Component | Part | Qty | ~Cost | Notes |
|---|---|---|---|---|
| MCU + LoRa | Heltec LoRa 32 V3 | 1 | $25 | ESP32-S3, 915MHz, built-in OLED |
| GPS | SparkFun GPS-15005 (u-blox NEO-M8N) | 1 | $45 | 10Hz, UART, SMA connector |
| GPS antenna | Active patch antenna (SMA) | 1 | $10 | Place on roof/dash with sky view |
| Battery | 3.7V LiPo 2000mAh | 1 | $8 | Or wire to car 12V via buck converter |
| Charger | TP4056 USB-C module | 1 | $2 | If using LiPo |
| Enclosure | IP65 ABS project box | 1 | $8 | ~100x68x50mm |

**Total: ~$100**

### Base Station

| Component | Part | Qty | ~Cost | Notes |
|---|---|---|---|---|
| MCU + LoRa | Heltec LoRa 32 V3 | 1 | $25 | Same board as car unit |
| Temp/Humidity | DHT22 | 1 | $4 | 3.3V, 1-wire |
| Pressure | Adafruit BMP388 breakout | 1 | $10 | I2C, 3.3V |
| Power | USB power bank (10000mAh) | 1 | $20 | Or 120V AC adapter at timing trailer |
| Enclosure | ABS project box | 1 | $8 | Leave DHT22 exposed to air |

**Total: ~$70**

---

## Car Unit Wiring

```
Heltec LoRa 32 V3 (ESP32-S3)
      │
      ├── 3.3V ──── GPS VCC
      ├── GND  ──── GPS GND
      ├── GPIO 45 (RX1) ─── GPS TX
      └── GPIO 46 (TX1) ─── GPS RX
```

The Heltec V3 has two UART ports. UART1 (Serial1) is used for GPS to keep the USB serial port (Serial0) free for debugging.

**GPS antenna placement**: must have clear sky view. Magnetic mount antenna on the roof is ideal. Avoid placing under metal panels.

---

## Base Station Wiring

```
Heltec LoRa 32 V3 (ESP32-S3)
      │
      ├── 3.3V ──── DHT22 VCC
      ├── 3.3V ──── BMP388 VCC
      ├── GND  ──── DHT22 GND
      ├── GND  ──── BMP388 GND
      │
      ├── GPIO 38 ──── DHT22 DATA (with 10kΩ pull-up to 3.3V)
      │
      ├── GPIO 41 (SDA) ─── BMP388 SDA
      └── GPIO 42 (SCL) ─── BMP388 SCL
```

**DHT22 note**: requires a 10kΩ pull-up resistor between DATA and 3.3V. Most breakout boards include this; bare sensors do not.

**BMP388 I2C address**: 0x77 by default (SDO pin floating or tied to VCC). If you have an address conflict, tie SDO to GND for 0x76.

---

## Firmware Setup

### Prerequisites

Install in Arduino IDE (Tools → Manage Libraries):
- `heltec_unofficial` by ropg
- `SparkFun u-blox GNSS Arduino Library`
- `DHT sensor library` by Adafruit
- `Adafruit BMP3XX Library`
- `ArduinoJson` by Benoit Blanchon

Board selection: `Heltec WiFi LoRa 32(V3)` (under Heltec ESP32 Series in board manager)

### Car Unit

1. Open `firmware/car-unit/car_unit.ino`
2. Set `SESSION_ID` (e.g. `"S001"`) — increment each session
3. Select board: `Heltec WiFi LoRa 32(V3)`, upload speed: 921600
4. Connect GPS, power on — OLED shows satellite count until fix
5. LED blinks on each LoRa transmit

### Base Station

1. Open `firmware/base-station/base_station.ino`
2. Set `WIFI_SSID`, `WIFI_PASSWORD`, `API_TELEMETRY`, `API_WEATHER` (see `CONFIG.md`)
3. Upload to board
4. OLED shows WiFi status, then incoming car data once car unit is on

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| "GPS not detected" on OLED | Wrong UART pins or wiring | Check GPIO 45/46 wiring |
| 0 satellites on car unit | No sky view / antenna | Move antenna, wait 60s for cold fix |
| Base station shows 0 LoRa packets | Frequency mismatch or range | Verify both boards use 905.2 MHz; reduce distance to test |
| WiFi not connecting | Wrong credentials | Re-check SSID/password, 2.4GHz only (ESP32 doesn't support 5GHz) |
| HTTP POST fails | Wrong API endpoint | Confirm API_TELEMETRY matches Terraform output `api_endpoint` |
