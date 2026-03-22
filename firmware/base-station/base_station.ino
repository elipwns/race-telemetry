/*
 * Race Telemetry - BASE STATION
 * Heltec LoRa 32 V3 (ESP32) + DHT22 + BMP388
 *
 * Receives telemetry from car unit via LoRa, posts to AWS.
 * Also reads weather sensors and posts on a separate interval.
 *
 * Session management: press and hold PRG button (GPIO0) for 2s to increment
 * session ID (S001, S002, ...). Persists across power cycles via Preferences.
 *
 * WiFi resilience: reconnects automatically if hotspot drops. Posts are queued
 * locally (up to 60 entries) and flushed when WiFi comes back, minimising
 * data loss during brief outages.
 *
 * Libraries required:
 *   - heltec_unofficial (LoRa + OLED)
 *   - DHT sensor library (Adafruit)
 *   - Adafruit BMP3XX Library
 *   - ArduinoJson
 *
 * Hardware wiring:
 *   DHT22 data  -> GPIO 38
 *   BMP388 SDA  -> GPIO 41 (I2C SDA)
 *   BMP388 SCL  -> GPIO 42 (I2C SCL)
 *   BMP388 VCC  -> 3.3V
 *   DHT22 VCC   -> 3.3V
 *   Both GND    -> GND
 *
 * See CONFIG.md for WiFi and API endpoint configuration.
 */

#include <heltec_unofficial.h>
#include <Preferences.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_BMP3XX.h>

// --- Configuration ---
#define WIFI_SSID     "Kloft_Guest"
#define WIFI_PASSWORD "loolwut710"
#define API_TELEMETRY "https://pe3daa8fi7.execute-api.us-west-2.amazonaws.com/prd/telemetry"
#define API_WEATHER   "https://pe3daa8fi7.execute-api.us-west-2.amazonaws.com/prd/weather"

// LoRa settings (must match car unit)
#define FREQUENCY        905.2
#define BANDWIDTH        125.0
#define SPREADING_FACTOR 7
#define TRANSMIT_POWER   14

// Sensor pins
#define DHT_PIN  38
#define DHT_TYPE DHT22

// Intervals
#define WEATHER_INTERVAL_MS   30000
#define RECONNECT_INTERVAL_MS 10000

// Post queue — stores up to 60 JSON payloads when WiFi is down (~1 min of telemetry)
#define QUEUE_MAX 60
#define JSON_MAX  300

struct QueuedPost {
  char endpoint[8];   // "TEL" or "WX"
  char json[JSON_MAX];
};

// --- Globals ---
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP3XX bmp;
Preferences prefs;

volatile bool rxFlag = false;
String rxdata;
unsigned long lastWeatherPost  = 0;
unsigned long lastReconnectAt  = 0;
String sessionId = "S001";
float lastSpeed  = 0;
byte  lastSats   = 0;
bool  holdHandled      = false;
unsigned long newSessionFlash = 0;

QueuedPost postQueue[QUEUE_MAX];
int queueHead  = 0;
int queueCount = 0;

void rx() { rxFlag = true; }

// --- Session management ---

void incrementSession() {
  prefs.begin("telemetry", false);
  int num = prefs.getInt("session_num", 1) + 1;
  prefs.putInt("session_num", num);
  prefs.end();
  char buf[8];
  snprintf(buf, sizeof(buf), "S%03d", num);
  sessionId = buf;
  newSessionFlash = millis();
  Serial.println("Session incremented to: " + sessionId);
}

// --- Post queue ---

void enqueue(const char* endpoint, const String& json) {
  if (queueCount >= QUEUE_MAX) {
    // Buffer full — drop oldest to make room for newest
    queueHead = (queueHead + 1) % QUEUE_MAX;
    queueCount--;
  }
  int idx = (queueHead + queueCount) % QUEUE_MAX;
  strncpy(postQueue[idx].endpoint, endpoint, sizeof(postQueue[idx].endpoint) - 1);
  strncpy(postQueue[idx].json,     json.c_str(), JSON_MAX - 1);
  queueCount++;
}

// Flush one entry per call — keeps LoRa receive responsive
void flushOne() {
  if (queueCount == 0 || WiFi.status() != WL_CONNECTED) return;

  QueuedPost& rec = postQueue[queueHead];
  const char* url = strcmp(rec.endpoint, "TEL") == 0 ? API_TELEMETRY : API_WEATHER;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(rec.json);
  http.end();

  if (code > 0) {
    queueHead = (queueHead + 1) % QUEUE_MAX;
    queueCount--;
  }
  // If POST failed, leave in queue and retry next loop
}

// --- WiFi watchdog ---

void checkWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastReconnectAt < RECONNECT_INTERVAL_MS) return;
  lastReconnectAt = millis();
  Serial.println("WiFi lost — reconnecting...");
  WiFi.reconnect();
}

// --- Setup ---

void setup() {
  heltec_setup();

  prefs.begin("telemetry", true);
  int num = prefs.getInt("session_num", 1);
  prefs.end();
  char buf[8];
  snprintf(buf, sizeof(buf), "S%03d", num);
  sessionId = buf;

  both.println("BASE STATION");
  both.println("Session: " + sessionId);

  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  dht.begin();
  Wire.begin(41, 42);
  if (!bmp.begin_I2C()) {
    both.println("BMP388 not found");
  } else {
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  both.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    both.print(".");
  }
  both.println("\nWiFi connected");

  updateDisplay();
}

// --- Loop ---

void loop() {
  heltec_loop();

  // Press and hold PRG button for 2s to increment session ID
  if (button.pressedFor(2000)) {
    if (!holdHandled) {
      holdHandled = true;
      incrementSession();
      updateDisplay();
    }
  } else {
    holdHandled = false;
  }

  // WiFi watchdog — non-blocking reconnect attempt every 10s
  checkWifi();

  // Flush one queued post per loop cycle
  flushOne();

  // Handle incoming LoRa telemetry
  if (rxFlag) {
    rxFlag = false;
    int state = radio.readData(rxdata);

    Serial.print("RX status: "); Serial.println(state);
    Serial.print("RX data: ["); Serial.print(rxdata); Serial.println("]");

    int telIdx = rxdata.indexOf("TEL:");
    if (telIdx > 0) rxdata = rxdata.substring(telIdx);

    if (state == RADIOLIB_ERR_NONE && rxdata.startsWith("TEL:")) {
      handleTelemetry(rxdata);
    }

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // Queue weather on interval
  if (millis() - lastWeatherPost >= WEATHER_INTERVAL_MS) {
    lastWeatherPost = millis();
    queueWeather();
  }
}

// --- Telemetry ---

// Format: TEL:{session}:{lat}:{lon}:{speed_kph}:{heading}:{sats}:{millis}
void handleTelemetry(const String& packet) {
  String parts[8];
  int partIdx = 0, start = 4;
  for (int i = 4; i <= (int)packet.length() && partIdx < 8; i++) {
    if (i == (int)packet.length() || packet[i] == ':') {
      parts[partIdx++] = packet.substring(start, i);
      start = i + 1;
    }
  }
  if (partIdx < 7) return;

  lastSpeed = parts[3].toFloat() * 0.621371;
  lastSats  = parts[5].toInt();
  updateDisplay();

  StaticJsonDocument<256> doc;
  doc["session_id"] = sessionId;
  doc["lat"]        = parts[1].toFloat();
  doc["lon"]        = parts[2].toFloat();
  doc["speed_kph"]  = parts[3].toFloat();
  doc["heading"]    = parts[4].toInt();
  doc["satellites"] = parts[5].toInt();
  doc["sequence"]   = parts[6].toInt();

  String body;
  serializeJson(doc, body);
  enqueue("TEL", body);
}

// --- Weather ---

void queueWeather() {
  float temp     = dht.readTemperature();
  float humidity = dht.readHumidity();
  float pressure = bmp.performReading() ? bmp.pressure / 100.0 : 0;

  if (isnan(temp) || isnan(humidity)) return;

  StaticJsonDocument<200> doc;
  doc["session_id"]   = sessionId;
  doc["temp_c"]       = temp;
  doc["humidity_pct"] = humidity;
  doc["pressure_hpa"] = pressure;

  String body;
  serializeJson(doc, body);
  enqueue("WX", body);
}

// --- Display ---

void updateDisplay() {
  display.clear();
  display.setFont(ArialMT_Plain_10);

  bool flashing = (newSessionFlash > 0 && millis() - newSessionFlash < 2000);
  String header = flashing ? ("NEW SESSION: " + sessionId) : ("BASE STATION  " + sessionId);
  display.drawString(0, 0, header);

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 14, String(lastSpeed, 0) + " mph  " + String(lastSats) + " sats");
  display.setFont(ArialMT_Plain_10);

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  if (!isnan(temp) && !isnan(hum)) {
    display.drawString(0, 36, "Temp: " + String(temp, 1) + "C  Hum: " + String(hum, 0) + "%");
  }

  bool wifiUp = WiFi.status() == WL_CONNECTED;
  String wifiLine = wifiUp
    ? "WiFi OK  Q:" + String(queueCount)
    : "WiFi reconnecting...";
  display.drawString(0, 50, wifiLine);
  display.display();
}
