# Architecture

## System Overview

Two hardware units communicate over LoRa radio. The base station posts data to AWS over WiFi. AWS stores data and broadcasts to browser dashboards via WebSocket.

```
┌─────────────────────────────────────────────────────────────────────────┐
│ HARDWARE                                                                │
│                                                                         │
│  [Car Unit]                    [Base Station]                           │
│  Heltec LoRa 32 V3             Heltec LoRa 32 V3                       │
│  + u-blox NEO-M8N GPS          + DHT22 (temp/humidity)                 │
│  LoRa TX @ 5Hz                 + BMP388 (pressure)                     │
│                                LoRa RX + WiFi                          │
│                                                                         │
│  TEL:{session}:{lat}:{lon}:{speed}:{heading}:{sats}:{ms}               │
│  ─────────────────────────────────────────────────────>                │
└─────────────────────────────────────────────────────────────────────────┘
                                      │ HTTPS POST
                                      ├── /telemetry  (per LoRa packet)
                                      └── /weather    (every 30s)
                                      ▼
┌─────────────────────────────────────────────────────────────────────────┐
│ AWS (us-west-2)                                                         │
│                                                                         │
│  API Gateway REST                                                       │
│  ├── POST /telemetry ──> Lambda: telemetry-ingest                       │
│  │                              ├── DynamoDB: telemetry-runs            │
│  │                              └── SNS: telemetry-events               │
│  │                                          │                          │
│  └── POST /weather ───> Lambda: weather-ingest                         │
│                                └── DynamoDB: weather-readings          │
│                                                                         │
│                                       Lambda: websocket-broadcast <────┘
│                                                │                       │
│  API Gateway WebSocket                         │                       │
│  ├── $connect    ──> Lambda: websocket-connect │                       │
│  └── $disconnect ──> Lambda: websocket-disconnect                      │
│                                                                         │
│  S3: race-telemetry-dashboard (static site)                            │
└─────────────────────────────────────────────────────────────────────────┘
                                      │ WebSocket
                                      ▼
                              Browser Dashboard
                              (Leaflet map + readouts)
```

## LoRa Message Protocol

**Car → Base (5Hz, ~55 bytes):**
```
TEL:{session_id}:{lat}:{lon}:{speed_kph}:{heading}:{satellites}:{millis}
```

Example:
```
TEL:S001:35.123456:-97.654321:112.4:247:8:34521
```

Fields:
| Field | Type | Notes |
|---|---|---|
| session_id | string | Set at firmware boot, e.g. `S001` |
| lat | float | 7 decimal places (±11cm precision) |
| lon | float | 7 decimal places |
| speed_kph | float | 1 decimal |
| heading | int | Degrees 0-359 |
| satellites | int | GPS fix quality indicator |
| millis | uint32 | ESP32 uptime, for sequence ordering |

At SF7/125kHz BW: ~35ms airtime per packet, ~17% duty cycle at 5Hz. Well within FCC Part 15 limits.

**Base → Cloud (not over LoRa):**
- `POST /telemetry`: JSON with parsed TEL fields + server-side ISO timestamp
- `POST /weather`: JSON with temp_c, humidity_pct, pressure_hpa every 30s

## DynamoDB Schema

### `telemetry-runs`
```
PK: session_id  (e.g. "S001")
SK: timestamp   (ISO 8601, e.g. "2026-06-15T14:23:00.123Z")

Attributes:
  lat          Decimal
  lon          Decimal
  speed_kph    Decimal
  heading      Number
  satellites   Number
  sequence     Number   (millis from car unit, for gap detection)
  expire_at    Number   (TTL, Unix epoch — 90 days from write)
```

### `weather-readings`
```
PK: session_id
SK: timestamp

Attributes:
  temp_c       Decimal
  humidity_pct Decimal
  pressure_hpa Decimal
```

### `websocket-connections`
```
PK: connectionId  (API Gateway assigns this)

Attributes:
  timestamp    String   (ISO 8601)
```

## Design Decisions

**Why `session_id` as DynamoDB PK?**
Enables efficient `Query` by session without full-table scans. The data scientist can pull all data for a specific track session in one boto3 call. Compare to the `timing-system` predecessor which used `date` as PK — that made cross-session analysis harder.

**Why fire-and-forget LoRa (no ACK)?**
An ACK round-trip would double the channel time and complicate the car firmware. At 5Hz with SF7, occasional dropped packets are acceptable for live telemetry — the dashboard will skip a frame. DynamoDB has the sequence number to detect gaps if needed.

**Why not send weather over WebSocket?**
Weather changes slowly (30s interval) so broadcasting it to all WebSocket clients on every update would be unnecessary. Phase 2 will add a REST GET endpoint the dashboard can poll for weather updates.

**Why vanilla JS + Leaflet for the dashboard?**
No build pipeline, no dependencies to update, easy for any team member to modify. Leaflet is the standard open-source map library with OpenStreetMap tiles (no API key, free).
