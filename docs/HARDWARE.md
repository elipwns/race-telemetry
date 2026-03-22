# Hardware

## Bill of Materials

### Car Unit

| Component | Part | Qty | ~Cost | Notes |
|---|---|---|---|---|
| MCU + LoRa + GNSS | Heltec Wireless Tracker (HTIT-Tracker) | 1 | $35 | ESP32-S3, SX1262, UC6580 GNSS, TFT, WiFi/BLE all-in-one |
| GNSS antenna | Active patch antenna (IPEX/SMA) | 1 | $8 | Wireless Tracker has IPEX connector; use SMA adapter if needed |
| Battery | 3.7V LiPo 2000mAh | 1 | $8 | Or wire to car 12V via buck converter |
| Charger | TP4056 USB-C module | 1 | $2 | If using LiPo |
| Enclosure | IP65 ABS project box | 1 | $8 | ~100x68x50mm |

**Total: ~$60**

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

The Heltec Wireless Tracker has GNSS (UC6580) built-in — no external GPS wiring needed.
The GNSS module is powered via GPIO3 (set HIGH in firmware). Connect an active GNSS antenna
to the IPEX connector on the board.

```
Heltec Wireless Tracker — internal connections (no wiring required)
  GPIO 3  → UC6580 VCC (power enable)
  GPIO 33 ← UC6580 TX  (GNSS data to ESP32)
  GPIO 34 → UC6580 RX  (ESP32 to GNSS, for config commands)
```

External connections: GNSS antenna only (IPEX connector, active patch recommended).

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

**Board manager**: Add Heltec board package URL in Arduino IDE preferences:
```
https://resource.heltec.cn/download/package_heltec_esp32_index.json
```
Then: Tools → Board → Board Manager → search "Heltec" → install "Heltec ESP32 Series Dev-boards"

**Libraries** (Tools → Manage Libraries):
- `RadioLib` by Jan Gromes
- `TinyGPSPlus` by Mikal Hart
- `DHT sensor library` by Adafruit (base station only)
- `Adafruit BMP3XX Library` (base station only)
- `ArduinoJson` by Benoit Blanchon (base station only)

### Car Unit

1. Open `firmware/car-unit/car_unit.ino`
2. Set `SESSION_ID` (e.g. `"S001"`) — increment each session
3. Select board: `Heltec Wireless Tracker`, upload speed: 921600
4. Connect GNSS antenna, open Serial Monitor at 115200 baud
5. "Waiting for GNSS fix..." then TX lines appear once fix is acquired

### Base Station

1. Open `firmware/base-station/base_station.ino`
2. Set `WIFI_SSID`, `WIFI_PASSWORD`, `API_TELEMETRY`, `API_WEATHER` (see `CONFIG.md`)
3. Upload to board
4. OLED shows WiFi status, then incoming car data once car unit is on

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `sats: 0` in Serial output | No sky view / antenna | Move antenna outside or near window; UC6580 cold start takes ~60s |
| No GNSS data (all zeros) | GNSS power or UART issue | Verify GPIO3 is set HIGH; check Serial1 on pins 33/34 |
| `LoRa init failed: -2` | SPI wiring or pin mismatch | Verify SX1262 pins match your Wireless Tracker hardware version |
| `TX failed` in Serial | LoRa busy or config mismatch | Confirm frequency/SF/BW match base station; check `setDio2AsRfSwitch(true)` |
| Base station shows 0 LoRa packets | Frequency mismatch or range | Verify both boards use 905.2 MHz; test within 10m |
| WiFi not connecting (base station) | Wrong credentials | Re-check SSID/password, 2.4GHz only |
| HTTP POST fails | Wrong API endpoint | Confirm API_TELEMETRY matches Terraform output `api_endpoint` |
