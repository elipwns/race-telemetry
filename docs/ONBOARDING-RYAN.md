# Data Scientist Onboarding — Ryan Francis

Welcome to the race telemetry project. This doc gets you from zero to querying live track data in a Jupyter notebook.

## What this project is

A DIY telemetry system for amateur road racing (Lucky Dog Racing League). A GPS unit in the car broadcasts position and speed over LoRa radio to a base station in the pits, which posts data to AWS in real time. You'll have access to that data for analysis.

Current data: GPS position (lat/lon), speed (kph), heading, satellite count, timestamp — at 1 Hz. Engine data (RPM, throttle, brake) coming in Phase 3 via OBD2 / CAN bus.

Full architecture and roadmap: [README.md](../README.md), [TODO.md](../TODO.md).

---

## AWS Access

**Account ID**: 772255980793
**Console URL**: https://772255980793.signin.aws.amazon.com/console
**Username**: `ryan-francis`
**Temp password**: shared via secure channel — you'll be prompted to reset on first login
**Region**: us-west-2 (Oregon)

Your user can assume the `race-telemetry-data-analyst` role, which has read-only access to:
- DynamoDB table: `telemetry-runs`
- DynamoDB table: `weather-readings` (once weather sensors are wired)

---

## Local Setup

**Clone the repo:**
```bash
git clone git@github.com:elipwns/race-telemetry.git
cd race-telemetry
```

**Install dependencies:**
```bash
pip install boto3 pandas matplotlib folium jupyter
```

**Configure AWS credentials** — choose one:

*Option A — access keys (simplest for local dev):*
Ask for programmatic access keys to be generated for your user, then:
```bash
aws configure --profile race-telemetry
# enter access key ID, secret, region: us-west-2, output: json
```

*Option B — role assumption (more secure, uses your console login):*
Add to `~/.aws/config`:
```ini
[profile race-telemetry]
role_arn = arn:aws:iam::772255980793:role/race-telemetry-data-analyst
source_profile = default
region = us-west-2
```

**Test it:**
```bash
aws dynamodb list-tables --profile race-telemetry
# should return {"TableNames": ["telemetry-runs", "weather-readings", ...]}
```

---

## Starter Notebook

Open the starter notebook — it handles connecting to DynamoDB, loading a session into pandas, basic plots, and has stubs for distance accumulation and lap detection:

```bash
jupyter notebook notebooks/session_explorer.ipynb
```

Change `AWS_PROFILE = 'race-telemetry'` at the top if your profile name differs.

---

## Data Schema

### `telemetry-runs`

| Field | Type | Notes |
|---|---|---|
| `session_id` | str | Partition key. One session = one track outing (e.g. `S003`) |
| `timestamp` | str | Sort key. ISO 8601 UTC |
| `lat` | Decimal | Latitude, 7 decimal places (~1 cm resolution) |
| `lon` | Decimal | Longitude, 7 decimal places |
| `speed_kph` | float | GPS-derived speed |
| `heading` | int | Degrees 0–359 |
| `satellites` | int | GPS lock quality — ≥4 = valid, ≥8 = good |
| `sequence` | int | Car unit uptime in ms — use to detect dropped LoRa packets |

DynamoDB returns Decimal types — cast to float before numeric operations.

---

## Open Questions (Issue #9)

There's an open issue with analysis questions we'd love input on before designing the Phase 2 data pipeline: [github.com/elipwns/race-telemetry/issues/9](https://github.com/elipwns/race-telemetry/issues/9)

The core problems:
- **Lap alignment**: time is the wrong X axis for comparing laps. Distance around the track is better — the notebook has a Haversine distance accumulation cell as a starting point.
- **Lap detection**: detecting when the car crosses the start/finish GPS coordinate.
- **Trace overlay**: once two laps are on a distance axis, overlaying speed/throttle/brake traces and computing delta time.
- **Lap validity filtering**: excluding in/out laps, yellow flag laps, off-track excursions.

No pressure to answer all of these — even a rough take on the right approach for one or two would help a lot.

---

## Stack Flexibility

The notebook uses Python / boto3 / pandas / folium as a starting point — that's just what's already set up, not a requirement. If you have a preferred stack (R, Julia, SQL via Athena, a BI tool, etc.) that works better for this kind of time series + geospatial analysis, go for it. The data is in DynamoDB now and will move to S3 Parquet in Phase 2 which opens up Athena/SQL if that's useful.

The one ask: if you find an approach that works well, document it in `notebooks/` or `docs/` so it's reproducible for the team.

---

## What's Coming (Data Pipeline)

Phase 2 will add:
- S3 export: completed sessions auto-export to `s3://race-telemetry-data/sessions/{session_id}/telemetry.parquet` — easier for bulk analysis than paginating DynamoDB
- Athena table over the S3 data — SQL queries against full session history
- Your IAM role will get S3 read access at that point

Phase 3 adds engine data channels (throttle, brake, RPM, gear) via OBD2 / CAN bus — same DynamoDB table, new fields.
