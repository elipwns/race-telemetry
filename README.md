# Race Telemetry

Live telemetry system for amateur road racing. A car unit (ESP32 + GPS) broadcasts position and speed over LoRa radio to a base station in the pits, which posts data to AWS and serves a real-time browser dashboard.

Built for Lucky Dog Racing. Tested on a Spec Miata.

## Architecture

```
[Car Unit]         [Base Station]         [AWS]                  [Dashboard]
ESP32 + GPS  ---->  ESP32 + LoRa  ---->  API Gateway  ---->  Browser (Leaflet map)
             LoRa   + Weather           Lambda + DynamoDB
                    + WiFi              + SNS + WebSocket
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for full data flow.

## Quick Start

1. **Hardware**: See [docs/HARDWARE.md](docs/HARDWARE.md) for BOM and wiring.
2. **Firmware**: Set WiFi/API credentials in `firmware/base-station/base_station.ino`, set session ID in `firmware/car-unit/car_unit.ino`. See [CONFIG.md](CONFIG.md).
3. **AWS**: Deploy infrastructure:
   ```bash
   cd terraform
   terraform init
   terraform apply
   ```
   Note the outputs — you'll need the API endpoint and WebSocket URL.
4. **Dashboard**: Update `WEBSOCKET_URL` in `dashboard/app.js`, then deploy:
   ```bash
   aws s3 sync dashboard/ s3://race-telemetry-dashboard --profile terraform
   ```
5. **Test**: See [docs/SETUP.md](docs/SETUP.md) for first-run checklist.

## Repository Structure

```
firmware/
  car-unit/         ESP32 + GPS firmware (LoRa TX)
  base-station/     ESP32 + weather sensors firmware (LoRa RX + WiFi)
lambda/
  telemetry-ingest/ REST POST /telemetry → DynamoDB + SNS
  weather-ingest/   REST POST /weather → DynamoDB
  websocket-*/      WebSocket connect/disconnect/broadcast
terraform/          All AWS infrastructure (API Gateway, Lambda, DynamoDB, SNS, S3)
dashboard/          Static web dashboard (Leaflet map + live readouts)
docs/               Architecture, hardware, setup, and data access guides
```

## Hardware

| Unit | Board | Extra Hardware |
|---|---|---|
| Car | Heltec Wireless Tracker (ESP32-S3 + UC6580 GNSS + SX1262) | External IPEX GNSS antenna |
| Base | Heltec LoRa 32 V3 | DHT22 + BMP388 (weather sensors — not yet wired) |

LoRa: 905.2 MHz · SF7 · 125 kHz BW · 14 dBm

## Phases

- **Phase 1 (MVP)**: GPS position + speed, live map, weather station
- **Phase 2**: Lap timing, session history, data export for analysis
- **Phase 3**: Vehicle data via CAN bus (OBD2 or aftermarket ECU — throttle, brake, RPM, wheel speeds)
- **Phase 4**: Car development sensors (tire temps, shock height)

## Data Access

Data scientists can query DynamoDB directly via boto3. See [docs/DATA-ACCESS.md](docs/DATA-ACCESS.md).

## Contributing

See [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md).
