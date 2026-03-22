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
  lat: 45.5950       # TrackAddict reference — confirm on-site
  lon: -122.6940     # TrackAddict reference — confirm on-site
  heading: 90        # approximate, eastbound on main straight
map_default:
  center_lat: 45.5968
  center_lon: -122.6955
  zoom: 15
notes: >
  Clockwise circuit. Long back straight (~2000ft) between T2 and T3.
  Chicane in infield. Hairpin at south end. Carousel complex before
  returning to main straight. OSM centerline (86 nodes, way IDs
  5529392/1184207349/1315957509) used for demo at elikloft.com/telemetry.
  Confirm S/F coords on-site for Phase 2 lap timing.
```
