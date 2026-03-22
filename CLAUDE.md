# CLAUDE.md

This file provides guidance to Claude Code when working in this repository.

## Project Overview

Race telemetry system for amateur road racing (Lucky Dog Racing series). ESP32/LoRa hardware communicates between a car unit (GPS) and a base station (weather sensors + WiFi). The base station posts data to AWS, which broadcasts to a browser dashboard in real time.

Data flow:
```
[Car Unit] --LoRa--> [Base Station] --HTTPS POST--> API Gateway REST
                                                           |
                                               +----------+----------+
                                       Lambda: telemetry-ingest   Lambda: weather-ingest
                                               |                         |
                                        DynamoDB: telemetry-runs  DynamoDB: weather-readings
                                               |
                                          SNS: telemetry-events
                                               |
                                       Lambda: websocket-broadcast
                                               |
                                   API Gateway WebSocket --> Browser Dashboard
```

## AWS Infrastructure Commands

```bash
cd terraform
terraform init       # first time only
terraform plan
terraform apply      # also re-zips and deploys Lambda code changes
```

Lambda functions are auto-zipped from `lambda/` subdirectories by Terraform's `archive_file` data sources.

## Hardcoded AWS Values in Lambda Code

These must be updated after `terraform apply` (use values from Terraform outputs):

- `lambda/telemetry-ingest/lambda_function.py`: SNS topic ARN
- `lambda/websocket-broadcast/lambda_function.py`: WebSocket management endpoint URL

After updating these files, run `terraform apply` again to redeploy the Lambda functions.

See `CONFIG.md` for the full list of values to configure.

## Firmware

**Active sketches:**
- `firmware/car-unit/car_unit.ino` → Car unit (ESP32 + u-blox NEO-M8N GPS)
- `firmware/base-station/base_station.ino` → Base station (ESP32 + DHT22 + BMP388)

Board: Heltec LoRa 32 V3 (ESP32-S3). Library: `heltec_unofficial`.
LoRa: 905.2 MHz, SF7, 125 kHz BW, 14 dBm TX power.

Before flashing base station: set `WIFI_SSID`, `WIFI_PASSWORD`, `API_TELEMETRY`, `API_WEATHER` in the .ino file.
Before each session: increment `SESSION_ID` in car unit .ino.

## Lambda Functions (Python 3.12)

| Function | Trigger | Purpose |
|---|---|---|
| `telemetry-ingest` | REST POST /telemetry | Writes to DynamoDB `telemetry-runs`, publishes to SNS |
| `weather-ingest` | REST POST /weather | Writes to DynamoDB `weather-readings` |
| `websocket-connect` | WebSocket `$connect` | Saves connectionId to DynamoDB |
| `websocket-disconnect` | WebSocket `$disconnect` | Removes connectionId from DynamoDB |
| `websocket-broadcast` | SNS subscription | Broadcasts telemetry to all WebSocket clients |

## DynamoDB Tables

- `telemetry-runs`: PK=`session_id`, SK=`timestamp` (ISO 8601)
- `weather-readings`: PK=`session_id`, SK=`timestamp`
- `websocket-connections`: PK=`connectionId`

## Terraform Structure

All infrastructure in `terraform/` as flat `.tf` files. Uses `terraform` AWS profile.
