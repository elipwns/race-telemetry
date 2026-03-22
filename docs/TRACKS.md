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

_Full 2026 calendar — we won't attend all of these. Record GPS coordinates on-site at each event attended for Phase 2 lap timing._

| Date | Track | Location | Early Dawg Discount |
|------|-------|----------|-------------------|
| Jan 17–18 | WeatherTech Raceway Laguna Seca | Monterey, CA | 12/17/2025 |
| Feb 13–15 | Sonoma Raceway (3 days) | Sonoma, CA | included in entry |
| Feb 28–Mar 1 | Carolina Motorsports Park | Kershaw, SC | 1/28/2026 |
| Apr 24–26 | Ridge Motorsports Park (3 days) | Shelton, WA | 3/25/2026 |
| May 16–17 | Atlanta Motorsports Park | Dawsonville, GA | 4/16/2026 |
| May 30–31 | Buttonwillow Raceway Park *(NEW — The Circuit)* | Buttonwillow, CA | 4/30/2026 |
| Jun 13–14 | Oregon Raceway Park | Grass Valley, OR | 5/13/2026 |
| Jun 26–27 | VIRginia Int'l Raceway | Alton, VA | 5/26/2026 |
| Jul 18–19 | Portland Int'l Raceway | Portland, OR | 6/18/2026 |
| Sept 11–13 | Ridge Motorsports Park (3 days) | Shelton, WA | 6/18/2026 |
| Sept 26–27 | Carolina Motorsports Park | Kershaw, SC | 8/12/2026 |
| Oct 10–11 | Chuckwalla Valley Raceway | Desert Center, CA | 9/10/2026 |
| Nov 14–15 | Thunderhill Raceway Park 5 Mile *(1000 Miles, overnight)* | Willows, CA | 10/14/2026 |
| Dec 12–13 | Barber Motorsports Park | Birmingham, AL | 11/12/2026 |

---

### Portland International Raceway

```yaml
name: Portland International Raceway
location: Portland, OR
website: https://www.portlandraceway.com
length_miles: 1.967
series_events:
  - 2026-07-18
  - 2026-07-19
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
