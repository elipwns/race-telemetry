# System Configurations

The race telemetry system is designed around four configurations depending on the car. Each builds on the same core stack (Wireless Tracker → LoRa → base station → AWS → dashboard) and differs only in how engine/vehicle data is sourced.

---

## Tier 1 — OBD2 Plug-and-Play

**Target:** Stock or lightly modified car with an OBD2 port (US cars 1996+, CAN-based 2008+). Weekend warriors, track day drivers, anyone who wants to add telemetry with zero permanent modification.

**How it works:** ELM327 UART module handles OBD2 protocol translation. You send ASCII AT commands (`010D\r` for speed, `010C\r` for RPM, etc.) and get ASCII hex back — no CAN library or protocol knowledge required. Entire unit lives in a 3D printed enclosure that plugs directly into the OBD2 port. Power comes from OBD2 pin 16 (12V unswitched) through a small buck converter. GPS antenna sticks to the windshield via suction mount.

**Hardware (beyond base car unit BOM):**

| Component | Part | ~Cost | Notes |
|---|---|---|---|
| OBD2 protocol bridge | ELM327 UART module (3.3V) | $5–10 | "ELM327 TTL" — get the 3.3V variant, no level shifting needed |
| Power regulation | MP1584 or LM2596 buck converter module | $2 | 12V → 5V; feeds Wireless Tracker via USB-C |
| OBD2 connector | Male OBD2 pigtail (6-inch) or printed housing | $3–5 | Wire pins 4 (GND), 5 (sig GND), 6 (CAN-H), 14 (CAN-L), 16 (12V) |
| GPS antenna | Suction-cup active patch antenna (IPEX) | $8 | Windshield mount, short cable into enclosure |
| Enclosure | 3D printed (see below) | — | All-in-one: OBD2 plug face + electronics cavity |

**Enclosure design references:**
- MrDIY ESP32 CAN Shield v1.3 Case — circuit layout and R/RW toggle switch approach (makerworld.com/en/models/673956)
- OBD2 plug for DIY projects — printable OBD2 male connector housing (makerworld.com/en/models/1662389)

**Key design notes:**
- Include a R/RW toggle switch on the ELM327 TX line — physical read-only lockout prevents accidental CAN writes
- Account for CAN transceiver dominant-state-at-boot (the ELM327 handles this internally, but worth verifying on first test)
- GPS antenna must have sky view — suction mount on windshield, cable routed into the enclosure

**Data available:** GPS position + speed, vehicle speed (OBD2 PID 0x0D), RPM (0x0C), throttle (0x11), engine load (0x04), brake switch status (where available).

**Test candidate:** Any OBD2 EV or modern car — plug in, drive around the block, validate GPS trace and OBD2 speed match.

---

## Tier 2 — GPS Only / Self-Contained

**Target:** Carbureted engine, vintage racer, anything with just 12V and a fuel tank. No ECU, no OBD2, no CAN bus. Also the fallback for any car where OBD2 integration isn't worth the effort.

**How it works:** Wireless Tracker in a small weatherproof box powered from the car's 12V electrical system. GPS + LoRa only — position, speed, heading, lap timing. No engine data. Simplest possible install.

**Hardware (beyond base car unit BOM):**

| Component | Part | ~Cost | Notes |
|---|---|---|---|
| Power regulation | MP1584 buck converter module | $2 | 12V → 5V for Wireless Tracker |
| Power connector | Ring terminals or accessory fuse tap | $2 | Tap to any switched 12V circuit |
| GPS antenna | Active patch with magnetic base (IPEX) | $10 | Roof mount preferred |
| Enclosure | 3D printed or IP65 ABS project box | $5–8 | Mount on dash or roll cage |

**Data available:** GPS position, speed, heading, satellites. Lap timing via start/finish line crossing (Phase 2). Nothing from the engine.

**Install time:** ~15 minutes. Suitable for loaner cars, rental track cars, or any situation where modifying the car's electrics isn't possible.

---

## Tier 3 — Analog Sensor Hybrid

**Target:** Car that has *some* electronics but not full OBD2 CAN — OBD1 cars (pre-1996), heavily modified engines where stock sensors are gone, cars with a standalone dash or sensor cluster that doesn't expose CAN. Also useful when OBD2 exists but you want higher resolution data than PID polling allows.

**How it works:** GPS + LoRa from Tier 2, plus direct analog/digital wiring from sensors to the Wireless Tracker's ADC pins. Sensors are tapped at the source (throttle position pot, brake pressure transducer, steering angle sensor) rather than read from a bus. More wiring effort, but completely independent of whatever ECU (or lack thereof) the car has.

**Example sensors:**

| Signal | Sensor | Interface | Notes |
|---|---|---|---|
| Throttle position | Existing TPS tap or hall sensor | 0–3.3V analog (ADC) | Most TPS output 0–5V — use voltage divider |
| Brake pressure | 0.5–4.5V pressure transducer | 0–3.3V analog (ADC) | Tee into brake line |
| Steering angle | Rotary hall effect sensor | 0–3.3V analog or SPI | Mounted to steering column |
| Wheel speed | Hall effect sensor + PCNT | Digital pulse count | See issue #4 for PCNT detail |

**Data available:** GPS + any sensors physically wired in. Completely custom per car.

---

## Tier 4 — Aftermarket ECU / Full CAN

**Target:** Race car running an aftermarket ECU with a configurable CAN bus — Haltech, MoTeC, AEM, Link/Vi-PEC, Megasquirt, Ecumaster, etc. This is the richest data source and the likely configuration for the team's own car.

**How it works:** MCP2515 CAN transceiver on the Wireless Tracker's SPI bus (shares hardware SPI with LoRa, different CS pin). A configurable CAN ID mapping layer in firmware defines which CAN message ID and byte offset maps to which telemetry field — ECU-agnostic, works across all major platforms by loading the right config.

**Hardware (beyond base car unit BOM):**

| Component | Part | ~Cost | Notes |
|---|---|---|---|
| CAN transceiver | MCP2515 module (SPI) | $3–5 | Shares SPI bus with LoRa (different CS pin) |
| CAN termination | 120Ω resistor | $0.10 | Only if this node is at a bus end |
| OBD2 or CAN connector | Deutsch DT or direct ECU harness tap | $5–10 | Depends on ECU and harness layout |

**Data available (ECU-dependent):** wheel speeds x4, RPM, throttle %, gear position, oil pressure/temp, fuel pressure, lambda/AFR, boost pressure, brake pressure, steering angle — whatever the ECU is logging.

**CAN config format** (firmware, Phase 3):
```
# Example: Haltech Elite CAN stream
0x360: byte 0-1 = RPM (uint16, factor 1)
0x360: byte 2-3 = TPS (uint16, factor 0.1, unit %)
0x362: byte 0-1 = MAP (uint16, factor 0.1, unit kPa)
```
Document configs for common ECUs in this file as they're validated.

---

## Car Unit Enclosure Design

The car unit enclosure is designed to be universal — the same box works across all four tiers. What changes is what's on the other end of the cables.

### Interface points

**Top/front face — TFT window**
Clear panel or open cutout exposing the Wireless Tracker's built-in TFT display. Debug and setup use only — not readable at speed. Allows status checking without opening the case.

**End A — USB-C**
Power input and firmware flashing. Aligned to the board's onboard USB-C port. This is the universal power interface — car USB port, external power bank, or a buck converter pigtail from 12V all terminate here via cable.

**End B — 2× SMA bulkhead**
LoRa and GNSS antenna connections. IPEX-to-SMA pigtails route internally from the board's U.FL connectors. Leave 15–20mm clearance inside for the pigtail arc — U.FL connectors don't tolerate sharp bends. Both external antennas (LoRa magnetic roof mount, GNSS windshield mount) plug in here via standard SMA cables.

**Side/back — GX16 8-pin aviation connector**
The vehicle harness connector. One connector, threaded locking ring, carries everything the car provides and everything the unit outputs. Weatherproof when mated. ~$3–5.

| Pin | Signal | Notes |
|-----|--------|-------|
| 1 | 12V in | From car harness via buck converter → 5V → USB-C (or use USB-C directly and leave unpopulated) |
| 2 | GND | |
| 3 | Display UART TX | To external driver display (lap delta, pit signals) |
| 4 | Display UART RX | |
| 5 | Data UART TX | OBD2 ELM327 or future expansion |
| 6 | Data UART RX | |
| 7 | Spare | |
| 8 | Spare | |

The harness on the car side is bespoke per installation. The enclosure never changes.

### Power

**No onboard LiPo for race car use.** Dash temperatures in a race car (80–90°C+ in summer sun) combined with crash risk and vibration make LiPo in the cockpit an unacceptable risk. LiPo thermal runaway in a cockpit is not recoverable.

**Power options by use case:**

| Situation | Power source | What plugs into USB-C |
|---|---|---|
| Race car permanent install | 12V tap → buck converter → USB-C cable | Buck converter pigtail |
| Track day / daily driver | Car USB-A or USB-C port | Standard cable |
| Bench / EV test | External power bank | USB-C cable |
| OBD2 Tier 1 | OBD2 pin 16 (12V) → buck converter | Buck converter pigtail |

LiPo remains an option for bench testing and non-race environments where temperatures are controlled, but should be called out clearly in documentation as not suitable for race car cockpit use.

### Material

**ASA** for final parts — ~95–100°C HDT, UV resistant, similar print profile to ABS. Right choice for anything that might see direct sun through a windshield.

**PETG for fit checks** — print the first iteration in PETG to validate board fitment, SMA hole positions, GX16 placement, USB-C alignment. Once geometry is confirmed, reprint in ASA.

Avoid PLA (deforms at 60°C) and standard PETG as the final race car material.

### Mounting

The back face of the enclosure carries all mounting features. The goal is to cover the most common install scenarios without making the box complicated — most features are zero-cost print geometry.

**Built into the enclosure (standard features):**

| Feature | Detail | Use case |
|---|---|---|
| Zip tie channels | Recessed slots around the perimeter, or D-ring posts on the back | Roll cage tubes, dash edges, wiring looms — anything you can loop a zip tie around |
| Countersunk bolt holes | 4× M3 holes in the corners of the back face, flush-mount hardware | Screwing to any flat surface: dash, trans tunnel, floor, firewall |
| Keyhole slots | 2× keyhole cutouts on the back | Screw heads into the car surface, twist-lock the box on — fast removal without tools, good for track day use moving between cars |

**Separate printed bracket (bolt onto the box):**

| Bracket | Detail | Use case |
|---|---|---|
| Roll cage tube clamp | Two-piece clamp grabs a 1.5" or 1.75" tube (standard roll cage sizes), box bolts to it via the back face M3 holes | Permanent race car install on the main hoop or dash bar |
| Suction cup plate | Flat plate with 1–2 suction cups, box clips/slides onto it | EV test, daily driver, glass or smooth dash surface — no permanent attachment |

The main enclosure is universal. Cage tube clamp sizes and suction cup variants are printed as accessories — swap per car without reprinting the box.

**Design notes for CAD:**
- Zip tie channels: 4mm wide × 3mm deep slots, one on each long side, centered. Two posts on the back also work and are stronger.
- Countersunk holes: M3 clearance (3.4mm), 90° countersink for M3 flat-head screws. Space to match a common bracket hole pattern if possible.
- Keyhole slots: 6mm entry, 3.5mm retention slot, 5mm travel. Works with M3 pan-head screws.
- Back face should be flat and uninterrupted between features — the cage clamp bracket needs a clean mating surface.
- Keep the back face as the mounting face; nothing on the bottom — enclosure should sit flat on a bench.

### Design references
- MrDIY ESP32 CAN Shield v1.3 — good reference for aviation connector placement and board mounting standoff approach (makerworld.com/en/models/673956)

---

## Choosing a Configuration

| Situation | Tier |
|---|---|
| Stock daily driver / EV / modern track car with OBD2 | 1 |
| Old car, carb engine, vintage racer, rental/loaner | 2 |
| OBD1 car, modified engine, want analog sensor data | 3 |
| Aftermarket ECU (Haltech, MoTeC, Link, Megasquirt, etc.) | 4 |

Tiers 1 and 2 can coexist — a Tier 1 setup that loses OBD2 comms falls back gracefully to GPS-only data automatically (Lambda drops no-data fields, GPS still works).

---

## Immediate Test Plan — EV Drive Test

**Goal:** Validate end-to-end GPS pipeline. No OBD2 hardware needed yet — this is a Tier 2 test on a car that happens to be an EV.

**Prerequisites:**
- [ ] External IPEX GNSS antenna connected to Wireless Tracker
- [ ] Base station powered, connected to mobile hotspot
- [ ] Dashboard open on pit laptop (`http://race-telemetry-dashboard.s3-website-us-west-2.amazonaws.com`)

**Steps:**
1. Place car unit near windshield (suction or dash mount), antenna facing up with clear sky view
2. Power on — wait for `sats > 0` on TFT display
3. Open dashboard, confirm "Connected" status
4. Drive a few blocks — verify car marker moves and traces the route
5. Check speed readout roughly matches speedometer
6. Return — confirm GPS trace looks correct on map

**Phase 3 EV test (once ELM327 module arrives):**
- Same setup + ELM327 UART wired to Wireless Tracker UART2
- Validate OBD2 speed matches GPS speed
- Log RPM, throttle, engine load during drive
