# Data Access Guide

This guide is for the data science team member. All telemetry and weather data is stored in DynamoDB and queryable via boto3.

## IAM Setup

You'll need AWS credentials with access to assume the `race-telemetry-data-analyst` role. Ask the infrastructure owner for the role ARN (from Terraform output `data_analyst_role_arn`).

Add to `~/.aws/config`:
```ini
[profile race-telemetry-analyst]
role_arn = arn:aws:iam::ACCOUNT_ID:role/race-telemetry-data-analyst
source_profile = default
region = us-west-2
```

Test access:
```bash
aws dynamodb list-tables --profile race-telemetry-analyst
```

---

## Querying Data

### List all data for a session

```python
import boto3
from boto3.dynamodb.conditions import Key
import pandas as pd

session = boto3.Session(profile_name='race-telemetry-analyst')
dynamodb = session.resource('dynamodb', region_name='us-west-2')
table = dynamodb.Table('telemetry-runs')

response = table.query(
    KeyConditionExpression=Key('session_id').eq('S001')
)
items = response['Items']

# Handle DynamoDB pagination (> 1MB of data)
while 'LastEvaluatedKey' in response:
    response = table.query(
        KeyConditionExpression=Key('session_id').eq('S001'),
        ExclusiveStartKey=response['LastEvaluatedKey']
    )
    items.extend(response['Items'])

df = pd.DataFrame(items)
df['lat'] = df['lat'].astype(float)
df['lon'] = df['lon'].astype(float)
df['speed_kph'] = df['speed_kph'].astype(float)
df['timestamp'] = pd.to_datetime(df['timestamp'])
df = df.sort_values('timestamp')
```

### Query a time range within a session

```python
from boto3.dynamodb.conditions import Key

response = table.query(
    KeyConditionExpression=
        Key('session_id').eq('S001') &
        Key('timestamp').between('2026-06-15T14:00:00', '2026-06-15T15:00:00')
)
```

### Get weather for a session

```python
weather_table = dynamodb.Table('weather-readings')
response = weather_table.query(
    KeyConditionExpression=Key('session_id').eq('S001')
)
weather_df = pd.DataFrame(response['Items'])
```

---

## Data Schema

### `telemetry-runs`

| Field | Type | Notes |
|---|---|---|
| session_id | str | Partition key. Identifies one track session |
| timestamp | str | Sort key. ISO 8601 UTC |
| lat | Decimal | Latitude (7 decimal places) |
| lon | Decimal | Longitude (7 decimal places) |
| speed_kph | Decimal | Speed in km/h |
| heading | int | Degrees 0-359 |
| satellites | int | GPS satellite count (≥4 = valid fix, ≥8 = good) |
| sequence | int | Car unit millis() — use to detect dropped packets |

**Note**: DynamoDB returns Decimal types. Cast to float for numeric operations.

### `weather-readings`

| Field | Type | Notes |
|---|---|---|
| session_id | str | Partition key |
| timestamp | str | Sort key. ISO 8601 UTC |
| temp_c | Decimal | Air temperature in Celsius |
| humidity_pct | Decimal | Relative humidity 0-100 |
| pressure_hpa | Decimal | Barometric pressure in hPa |

---

## Example Analysis: Speed Trace

```python
import matplotlib.pyplot as plt

# After querying as above
plt.figure(figsize=(14, 4))
plt.plot(df['timestamp'], df['speed_kph'])
plt.xlabel('Time')
plt.ylabel('Speed (kph)')
plt.title('Speed Trace — Session S001')
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()
```

## Example Analysis: GPS Track Map

```python
import folium

m = folium.Map(location=[df['lat'].mean(), df['lon'].mean()], zoom_start=15)
folium.PolyLine(
    locations=list(zip(df['lat'], df['lon'])),
    color='blue',
    weight=2
).add_to(m)
m.save('track_map.html')
```

---

## Phase 2: S3 Data Export

Phase 2 will add a Lambda function that exports completed session data to S3 as CSV/Parquet when a session ends. This enables:
- Faster bulk loads (vs paginating DynamoDB)
- SQL queries via Amazon Athena
- Easy sharing of session data files

S3 bucket: `race-telemetry-data` (to be created in Phase 2)
Path format: `sessions/{session_id}/telemetry.parquet`

Data scientist will get an S3 read-only IAM policy added to the analyst role at that time.
