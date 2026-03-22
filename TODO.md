# TODO / Roadmap

## Current Focus
> Phase 1 MVP — target: Lucky Dog Race #2 (~3 months out from 2026-03-21)

**Next up:**
- [ ] Order hardware (see HARDWARE.md BOM) — get a POC on the road ASAP
- [ ] Wire and bench test car unit + base station
- [ ] Terraform apply + confirm all AWS resources come up clean
- [ ] Get Ryan's email → create AWS console login for his IAM user (see aws-iam-management PR)
- [ ] Bench test: POST to /telemetry via curl, verify DynamoDB row appears

**Terraform note**: `terraform apply` stays in lead dev's hands for now. Open a PR with `.tf` changes, get it merged, then ping to trigger the apply. See CONTRIBUTING.md.

---

## Phase 1 — MVP
_Goal: live car position on Leaflet map from pit laptop over mobile hotspot_

### Hardware
- [ ] Order car unit components (Heltec Wireless Tracker + GNSS antenna + LiPo + enclosure)
- [ ] Order base station components (Heltec LoRa 32 V3 + DHT22 + BMP388 + enclosure)
- [ ] Wire and bench test car unit (GPS fix, LoRa TX visible on another board's Serial Monitor)
- [ ] Wire and bench test base station (LoRa RX, weather sensor readings, WiFi connect)
- [ ] Mount car unit in enclosure, test GPS antenna placement in car

### AWS Infrastructure
- [ ] `terraform init && terraform apply` — verify no errors
- [ ] Update hardcoded values post-apply (SNS ARN in telemetry-ingest, WebSocket endpoint in websocket-broadcast) — see CONFIG.md
- [ ] `terraform apply` again after updating Lambda code
- [ ] Smoke test: POST to /telemetry via curl, verify DynamoDB row appears
- [ ] Smoke test: POST to /weather via curl, verify DynamoDB row appears
- [ ] Smoke test: open dashboard, verify WebSocket connects

### Dashboard
- [ ] Update `WEBSOCKET_URL` in `dashboard/app.js` with Terraform output
- [ ] Deploy dashboard: `aws s3 sync dashboard/ s3://race-telemetry-dashboard --profile terraform`
- [ ] Verify dashboard URL loads and shows "Connected"
- [ ] Test: car marker appears and moves on map during bench drive-test

### Tracks
- [ ] Record start/finish GPS coords at first Lucky Dog event (see TRACKS.md template)
- [ ] Add track entry to TRACKS.md

### Documentation
- [ ] Fill in actual Lucky Dog 2026 schedule dates in TRACKS.md

---

## Phase 2 — Data + Analysis
_Goal: lap timing, session history on dashboard, data scientist self-service_

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

## Phase 3 — Driver Inputs
_Goal: live throttle/brake/steering traces on dashboard_

- [ ] Research throttle position sensor tap on the car (consult with driver)
- [ ] Add throttle (0-3.3V analog), brake pressure transducer (0.5-4.5V), steering angle sensor to car unit wiring
- [ ] Extend TEL message: `...:{throttle_pct}:{brake_pct}:{steering_deg}:{millis}`
- [ ] Update `telemetry-ingest` Lambda to store new fields (no schema change needed in DynamoDB)
- [ ] Dashboard: add Chart.js throttle + brake trace panels (time on X axis)
- [ ] Update HARDWARE.md and ARCHITECTURE.md

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
- [ ] Car unit: no GPS fix indicator LED (OLED shows sat count but no physical LED)

---

## Done
_Move items here when merged to main_

- [x] Initial repo scaffold (firmware, Lambda, Terraform, dashboard, docs)
- [x] GitHub repo created: github.com/elipwns/race-telemetry
