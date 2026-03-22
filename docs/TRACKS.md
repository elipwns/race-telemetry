# Tracks

Record GPS coordinates for tracks while physically on-site. Use these for lap timing (Phase 2) and track-specific map defaults.

## Recording Coordinates On-Site

When at a track, use the car unit's OLED to read GPS coordinates:
- Drive to the start/finish line and note the coordinates
- Mark corner apexes, braking zones, or other reference points if desired

Alternatively, use a phone GPS app (e.g. Google Maps, what3words) to pin the location.

## Coordinate Format

Store as decimal degrees (WGS84), 6 decimal places minimum.
- Positive latitude = North, positive longitude = East (US tracks will have negative longitude)
- Example: `35.390598, -97.607864`

---

## Track Records

### Template

```yaml
name: Track Name
location: City, State
series_events:
  - 2026-MM-DD
start_finish:
  lat: 0.000000
  lon: 0.000000
  heading: 0        # degrees, direction of travel crossing start/finish
map_default:
  center_lat: 0.000000
  center_lon: 0.000000
  zoom: 15
notes: ""
```

---

## Lucky Dog Racing Schedule (2026)

_Record coordinates on-site for accuracy. Approximate coords are noted where known._

### Portland International Raceway

```yaml
name: Portland International Raceway
location: Portland, OR
website: https://www.portlandraceway.com
length_miles: 1.967
series_events:
  - TBD 2026
start_finish:
  lat: 45.5958       # approximate — record precisely on-site
  lon: -122.6960     # approximate — record precisely on-site
  heading: 90        # approximate, eastbound on main straight
map_default:
  center_lat: 45.5920
  center_lon: -122.6928
  zoom: 15
notes: >
  Clockwise circuit. Long back straight (~2000ft) between T2 and T3.
  Chicane in infield. Hairpin at south end. Carousel complex before
  returning to main straight. Demo page at elikloft.com/telemetry uses
  approximate waypoints — update app.js once real coords are confirmed.
```
