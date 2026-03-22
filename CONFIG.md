# Configuration

After deploying AWS infrastructure (`terraform apply`), configure the firmware and dashboard with the values from Terraform outputs.

## Firmware: Car Unit

File: `firmware/car-unit/car_unit.ino`

```cpp
#define SESSION_ID "S001"   // Increment each session (S001, S002, ...)
```

Change `SESSION_ID` before each session so data is queryable by session.

## Firmware: Base Station

File: `firmware/base-station/base_station.ino`

```cpp
#define WIFI_SSID     "your_hotspot_ssid"
#define WIFI_PASSWORD "your_hotspot_password"
#define API_TELEMETRY "https://XXXX.execute-api.us-west-2.amazonaws.com/prd/telemetry"
#define API_WEATHER   "https://XXXX.execute-api.us-west-2.amazonaws.com/prd/weather"
```

Get the API base URL from Terraform output `api_endpoint`.

## Lambda: websocket-broadcast

File: `lambda/websocket-broadcast/lambda_function.py`

```python
WEBSOCKET_ENDPOINT = 'https://XXXX.execute-api.us-west-2.amazonaws.com/prod'
```

Get value from Terraform output `websocket_endpoint`. This must be updated and redeployed (`terraform apply`) after first deploy.

## Lambda: telemetry-ingest

File: `lambda/telemetry-ingest/lambda_function.py`

```python
SNS_TOPIC_ARN = 'arn:aws:sns:us-west-2:ACCOUNT_ID:telemetry-events'
```

Get value from Terraform output `sns_topic_arn`.

## Dashboard

File: `dashboard/app.js`

```js
const WEBSOCKET_URL = 'wss://XXXX.execute-api.us-west-2.amazonaws.com/prod';
```

Get value from Terraform output `websocket_endpoint` (replace `https://` with `wss://`).
