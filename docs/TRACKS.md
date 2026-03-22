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

_Tracks to be filled in as we attend events. Record coordinates on-site for accuracy._

<!-- Add tracks here as you attend events:

### [Track Name]
...

-->
