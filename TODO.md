# TODO / Roadmap

## Current Focus
> Phase 1 MVP — first live test: Oregon Raceway Park, June 13–14 2026 (ideal LoRa conditions — open high desert, base station elevated above circuit with full sightlines). Home track PIR July 18–19 2026.

**Remaining blockers before first real drive test:**
- [ ] Connect external IPEX GNSS antenna to Wireless Tracker — confirmed NMEA bytes flowing, no fix without it
- [ ] Deploy dashboard: `aws s3 sync dashboard/ s3://race-telemetry-dashboard --profile terraform`
- [ ] Verify dashboard loads, WebSocket connects, car marker moves with live GPS data
- [ ] Get Ryan's email → create AWS console login for his IAM user (see aws-iam-management PR)
- [ ] Mount car unit in enclosure, test GPS antenna placement in car

**Terraform note**: `terraform apply` stays in lead dev's hands for now. Open a PR with `.tf` changes, get it merged, then ping to trigger the apply. See CONTRIBUTING.md.

---

## Phase 1 — MVP
_Goal: live car position on Leaflet map from pit laptop over mobile hotspot_

### Hardware
- [x] Order car unit components (Heltec Wireless Tracker + GNSS antenna)
- [x] Order base station components (Heltec LoRa 32 V3)
- [ ] Order / wire weather sensors (DHT22 + BMP388) — not yet set up
- [x] Wire and bench test car unit (LoRa TX confirmed, GNSS receiving NMEA bytes, TFT display working)
- [x] Wire and bench test base station (LoRa RX confirmed, WiFi connected, telemetry posting to AWS)
- [ ] Connect external IPEX GNSS antenna → confirm GPS fix outdoors
- [ ] Mount car unit in enclosure, test GPS antenna placement in car

### AWS Infrastructure
- [x] `terraform init && terraform apply` — all resources up
- [x] Update hardcoded values post-apply (SNS ARN in telemetry-ingest, WebSocket endpoint in websocket-broadcast)
- [x] `terraform apply` again after updating Lambda code (no-fix filter + TTL)
- [x] Smoke test: telemetry reaching DynamoDB confirmed (2,776 records from walk test, cleaned up)
- [ ] Smoke test: open dashboard, verify WebSocket connects
- [ ] Smoke test: POST to /weather via curl, verify DynamoDB row appears (once weather sensors wired)

### Dashboard
- [x] Update `WEBSOCKET_URL` in `dashboard/app.js` with Terraform output
- [x] Deploy dashboard: `aws s3 sync dashboard/ s3://race-telemetry-dashboard --profile terraform`
- [x] Verify dashboard URL loads and shows "Connected"
- [ ] Test: car marker appears and moves on map during drive test with GPS fix

### Tracks
- [ ] Record start/finish GPS coords at first Lucky Dog event (see TRACKS.md template)
- [ ] Add track entry to TRACKS.md

### Documentation
- [ ] Fill in actual Lucky Dog 2026 schedule dates in TRACKS.md

---

## Phase 2 — Data + Analysis
_Goal: lap timing, session history on dashboard, data scientist self-service_

### Data Completeness (Local Storage + WiFi Backfill)
- [ ] Car unit: enable LittleFS partition, write every TEL record to flash with sequence number — provides ~2.5–5 hrs of local storage at 1Hz independent of LoRa success
- [ ] Car unit: on WiFi connect (entering pit lane near hotspot), bulk-upload session file directly to API — fills LoRa gaps automatically. DynamoDB puts are idempotent so duplicate records are harmless.
- [ ] SD card (endurance races): add SD module on ESP32-S3's second SPI bus using free GPIO pins — effectively unlimited storage. Necessary for 7+ hour Lucky Dog endurance events where LittleFS fills up. LoRa handles real-time; SD card is the ground truth archive.

### Pit-to-Car Communications
_Goal: base station can signal the driver without anyone touching the board_

- [ ] Car unit: after each TX, open 500ms LoRa RX window to listen for base station messages
- [ ] Base station: queue outbound messages; transmit immediately upon receiving a car packet
- [ ] Car unit: display incoming commands full-screen on TFT — `PIT IN` (red), `ALL CLEAR` (green), arbitrary text
- [ ] Base station admin web UI: ESP32 hosts a small web server on the hotspot network. Anyone on the same network opens `http://telemetry.local` in a browser — no app install, no IP hunting. Pit crew chief can send pit signals from a phone or tablet without being near the board.
- [ ] Admin UI actions: Pit In, All Clear, custom text field, view live car speed/last-seen
- [ ] Physical keypad on base station: I2C membrane keypad (PCF8574 expander) for quick-action fallback when someone is already standing at the board. Maps to same message queue as web UI.
- [ ] Base station: add LoRa TX capability (currently RX-only)

### Lap Timing + History
- [ ] `telemetry-query` Lambda: `GET /telemetry?session_id=X` returns last N points — dashboard calls on load to show history trail
- [ ] Lap timing: record start/finish GPS coords at PIR on-site (see TRACKS.md), then detect line crossing to calculate lap time. Cross-reference against official Lucky Dog times to validate accuracy. Store in new `lap-times` DynamoDB table
- [ ] Dashboard: display current lap time + best lap
- [ ] Predictive lap delta: compare current position progress vs. reference lap (best lap) in real time. Requires: best lap GPS+timestamp trace stored on-device, per-point time interpolation, delta output to external display. The Wireless Tracker TFT is too small to read at speed — needs a dedicated external display (e.g. large 7-segment or small ruggedized LCD mounted on dash). Green = ahead, red = behind.
- [ ] Session end endpoint: `POST /session/end` triggers S3 CSV export of session data
- [ ] S3 export Lambda: dump `telemetry-runs` session to `s3://race-telemetry-data/sessions/{session_id}/telemetry.csv`
- [ ] Update `data-analyst` IAM role with S3 read access to exports bucket
- [ ] Update DATA-ACCESS.md with S3 query examples
- [ ] Dashboard: poll `/weather` REST endpoint every 60s and update weather card (currently weather card is static)

---

## Phase 3 — Vehicle Data Integration (CAN Bus)
_Goal: live throttle/brake/RPM/wheel speed traces on dashboard — no analog wiring_

Two paths depending on the car. Both use an MCP2515 CAN transceiver on the ESP32-S3 SPI bus (shares bus with LoRa, different CS pin). Zero backend changes — same TEL payload structure, same Lambda, same DynamoDB schema.

**Path A — OBD2 (modern stock ECU cars, 2008+ CAN-mandated)**
- [ ] Add MCP2515 to car unit hardware (SPI, ~$5, powered from Vext)
- [ ] Read OBD2 Mode 1 PIDs: vehicle speed (0x0D), RPM (0x0C), throttle (0x11), engine load (0x04), brake switch
- [ ] Extend TEL message: `...:{throttle_pct}:{brake_pct}:{rpm}:{gear}:{millis}`
- [ ] Update `telemetry-ingest` Lambda to store new fields

**Path B — Aftermarket ECU (race cars on Haltech, MoTeC, AEM, Link, Megasquirt, Ecumaster, etc.)**
- [ ] Add MCP2515 to car unit hardware (same as Path A)
- [ ] Implement configurable CAN ID mapping layer in firmware: define which CAN ID + byte offset maps to which telemetry field — makes it ECU-agnostic across the field
- [ ] Target data: wheel speeds x4, RPM, throttle, gear, oil pressure/temp, fuel pressure, lambda/AFR, boost (where available)
- [ ] Extend TEL message with vehicle data fields (same Lambda/DynamoDB update as Path A)
- [ ] Document CAN config format in HARDWARE.md with examples for common ECUs

**Shared**
- [ ] Dashboard: add Chart.js throttle + brake trace panels (time on X axis)
- [ ] Dashboard: add RPM bar and gear indicator to live panel
- [ ] Update HARDWARE.md and ARCHITECTURE.md

> **Note on true ADR (wheel tick → GPS Kalman filter):** Requires swapping the UC6580 for a u-blox ZED-F9R (~$200 + custom carrier board). The UC6580 doesn't support `UBX-ESF-MEAS`. Deferred to Future — see issue #4.

---

## Phase 4 — Car Development Sensors
_Goal: tire temp + suspension data for setup optimization_

- [ ] Evaluate tire temp sensors (MLX90614 IR x4 — needs TCA9548A I2C multiplexer, all 4 share address 0x5A)
- [ ] Evaluate shock height sensors (linear potentiometer x4 — analog ADC)
- [ ] Determine if second ESP32 is needed on car (Heltec runs out of ADC pins with 4+4 sensors)
- [ ] Design second ESP32 → primary car unit serial/I2C bridge if needed
- [ ] Add setup logging: driver enters toe/camber/damper settings before session, stored with session metadata

---

## Future — In-Car Driver Aids
_Wishlist features for situational awareness and driver feedback_

- [ ] External lap delta display: dedicated dash-mounted display for +/- lap delta. Needs to be large/bright enough to read at speed with helmet on. Options to evaluate: large 7-segment module, small ruggedized LCD, or LED bar (simpler — just ahead/behind/neutral). Driven by serial output from Wireless Tracker.
- [ ] Blind spot / proximity warning: ultrasonic sensors (HC-SR04 or similar) mounted in rear quarter panels to detect cars alongside. Drive a set of indicator LEDs (left side / right side) visible in the driver's peripheral vision — compensates for limited mirror visibility and restricted head movement with HANS device. Needs evaluation of sensor range and false-positive rate at racing speeds before committing to hardware.

---

## Future — Multi-team Product
_Prerequisite: proven on our car across multiple events_

- [ ] Managed session system: `POST /session/start` returns session token (replaces manual SESSION_ID)
- [ ] Multi-tenancy: add `team_id` prefix to DynamoDB keys
- [ ] API Gateway usage plans: per-team API keys
- [ ] "Deploy your own instance" guide (leverage this repo's Terraform)
- [ ] Pricing model / cost estimate per team per season

---

## Bugs / Backlog
_Small issues and improvements that don't fit a phase_

- [ ] Base station firmware: weather card on dashboard shows `—` (weather not sent over WebSocket — tracked in Phase 2)
- [ ] Car unit: no GPS fix indicator LED (TFT shows sat count but no physical LED)

---

## Done
_Move items here when merged to main_

- [x] Initial repo scaffold (firmware, Lambda, Terraform, dashboard, docs)
- [x] GitHub repo created: github.com/elipwns/race-telemetry
- [x] Port car unit firmware to Heltec Wireless Tracker (RadioLib, HT_TinyGPS++, HT_st7735 TFT)
- [x] TFT display: speed (mph), sat count, TX counter, lat/lon / GNSS rx byte counter
- [x] Fix display tearing: one-time clear, fixed-width snprintf strings overwrite in place
- [x] Base station: WiFi credentials, API endpoints, speed in mph, RX debug logging
- [x] Base station: trim garbage prefix bytes on incoming LoRa packets
- [x] Base station: press-and-hold PRG button 2s → increment session ID, persists via Preferences
- [x] Lambda telemetry-ingest: reject no-fix records (satellites == 0)
- [x] Lambda telemetry-ingest: 90-day TTL on all records
- [x] Prefix all WebSocket AWS resources with `rt-` to avoid naming collision with timing-system project
- [x] DynamoDB: clean up 2,776 zero-coord records from initial walk test
- [x] WebSocket URL updated in websocket-broadcast Lambda and dashboard/app.js
- [x] Terraform apply: all infrastructure deployed
- [x] Phase 3 roadmap: reworked around CAN bus (OBD2 + aftermarket ECU paths)
