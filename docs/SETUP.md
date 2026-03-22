# Setup Guide

End-to-end setup from scratch to first live session.

## Prerequisites

- AWS account with `terraform` profile configured (`~/.aws/credentials`)
- Terraform >= 1.5.0
- Arduino IDE 2.x
- Two Heltec LoRa 32 V3 boards
- Hardware from [HARDWARE.md](HARDWARE.md)

---

## Step 1: Deploy AWS Infrastructure

```bash
cd terraform
terraform init
terraform plan    # review what will be created
terraform apply
```

Note the outputs — you'll need them for subsequent steps:
```
api_endpoint         = "https://XXXX.execute-api.us-west-2.amazonaws.com/prd"
websocket_endpoint   = "wss://YYYY.execute-api.us-west-2.amazonaws.com/prod"
sns_topic_arn        = "arn:aws:sns:us-west-2:ACCOUNT:telemetry-events"
dashboard_url        = "http://race-telemetry-dashboard.s3-website-us-west-2.amazonaws.com"
data_analyst_role_arn = "arn:aws:iam::ACCOUNT:role/race-telemetry-data-analyst"
```

---

## Step 2: Update Lambda Hardcoded Values

After `terraform apply`, update these two files with the output values:

**`lambda/telemetry-ingest/lambda_function.py`**
```python
SNS_TOPIC_ARN = 'arn:aws:sns:...'   # from sns_topic_arn output
```

**`lambda/websocket-broadcast/lambda_function.py`**
```python
WEBSOCKET_ENDPOINT = 'https://YYYY.execute-api.us-west-2.amazonaws.com/prod'
# Note: use https://, not wss://
```

Then redeploy:
```bash
terraform apply
```

---

## Step 3: Configure and Flash Base Station Firmware

Edit `firmware/base-station/base_station.ino`:
```cpp
#define WIFI_SSID     "your_hotspot_ssid"
#define WIFI_PASSWORD "your_hotspot_password"
#define API_TELEMETRY "https://XXXX.execute-api.us-west-2.amazonaws.com/prd/telemetry"
#define API_WEATHER   "https://XXXX.execute-api.us-west-2.amazonaws.com/prd/weather"
```

Flash to the base station board. OLED should show "BASE STATION" then "WiFi OK" once connected.

---

## Step 4: Configure and Flash Car Unit Firmware

Edit `firmware/car-unit/car_unit.ino`:
```cpp
#define SESSION_ID "S001"
```

Flash to the car unit board. OLED shows "CAR UNIT" and satellite count. Wait for GPS fix (sats ≥ 4).

---

## Step 5: Deploy Dashboard

Update `dashboard/app.js`:
```js
const WEBSOCKET_URL = 'wss://YYYY.execute-api.us-west-2.amazonaws.com/prod';
```

Deploy to S3:
```bash
aws s3 sync dashboard/ s3://race-telemetry-dashboard --profile terraform
```

Open the dashboard URL from Step 1 outputs.

---

## Step 6: First Run Checklist

- [ ] Car unit OLED shows ≥ 4 satellites
- [ ] Base station OLED shows speed updating (receiving LoRa from car)
- [ ] DynamoDB console → `telemetry-runs` table shows new rows
- [ ] Dashboard opens and shows "Connected" status
- [ ] Car marker appears on map and moves as car drives

---

## At the Track

1. Power on base station with mobile hotspot active
2. Power on car unit — wait for GPS fix before moving (OLED shows sat count)
3. Set `SESSION_ID` to the next unused value and reflash (or keep same for practice)
4. Open dashboard on laptop or phone

**Session IDs**: Use a consistent naming convention, e.g. `S001` = first practice, `S002` = second practice, etc. Document in [TRACKS.md](TRACKS.md).
