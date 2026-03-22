/*
 * Race Telemetry - BASE STATION
 * Heltec LoRa 32 V3 (ESP32) + DHT22 + BMP388
 *
 * Receives telemetry from car unit via LoRa, posts to AWS.
 * Also reads weather sensors and posts on a separate interval.
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
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_BMP3XX.h>

// --- Configuration ---
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_TELEMETRY "YOUR_API_GATEWAY_ENDPOINT/telemetry"
#define API_WEATHER   "YOUR_API_GATEWAY_ENDPOINT/weather"

// LoRa settings (must match car unit)
#define FREQUENCY        905.2
#define BANDWIDTH        125.0
#define SPREADING_FACTOR 7
#define TRANSMIT_POWER   14

// Sensor pins
#define DHT_PIN  38
#define DHT_TYPE DHT22

// Weather post interval (ms)
#define WEATHER_INTERVAL_MS 30000

// --- Globals ---
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP3XX bmp;

volatile bool rxFlag = false;
String rxdata;
unsigned long lastWeatherPost = 0;
String lastSession = "";
float lastSpeed = 0;
byte lastSats = 0;

void rx() { rxFlag = true; }

void setup() {
  heltec_setup();
  both.println("BASE STATION");

  // Initialize LoRa in receive mode
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));

  // Initialize weather sensors
  dht.begin();
  Wire.begin(41, 42);
  if (!bmp.begin_I2C()) {
    both.println("BMP388 not found");
  } else {
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  }

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  both.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    both.print(".");
  }
  both.println("\nWiFi connected");

  updateDisplay();
}

void loop() {
  heltec_loop();

  // Handle incoming LoRa telemetry
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);

    if (_radiolib_status == RADIOLIB_ERR_NONE && rxdata.startsWith("TEL:")) {
      handleTelemetry(rxdata);
    }

    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // Post weather on interval
  if (millis() - lastWeatherPost >= WEATHER_INTERVAL_MS) {
    lastWeatherPost = millis();
    postWeather();
  }
}

// Parse TEL packet and POST to cloud
// Format: TEL:{session}:{lat}:{lon}:{speed_kph}:{heading}:{sats}:{millis}
void handleTelemetry(const String& packet) {
  // Split on ':'
  int fields[8];
  int idx = 0;
  int start = 4; // skip "TEL:"

  String parts[8];
  int partIdx = 0;
  for (int i = 4; i <= packet.length() && partIdx < 8; i++) {
    if (i == packet.length() || packet[i] == ':') {
      parts[partIdx++] = packet.substring(start, i);
      start = i + 1;
    }
  }

  if (partIdx < 7) return; // malformed

  String session = parts[0];
  String lat     = parts[1];
  String lon     = parts[2];
  String speed   = parts[3];
  String heading = parts[4];
  String sats    = parts[5];
  String ms      = parts[6];

  lastSession = session;
  lastSpeed   = speed.toFloat();
  lastSats    = sats.toInt();

  updateDisplay();

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(API_TELEMETRY);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["session_id"] = session;
  doc["lat"]        = lat.toFloat();
  doc["lon"]        = lon.toFloat();
  doc["speed_kph"]  = speed.toFloat();
  doc["heading"]    = heading.toInt();
  doc["satellites"] = sats.toInt();
  doc["sequence"]   = ms.toInt();

  String body;
  serializeJson(doc, body);
  http.POST(body);
  http.end();
}

void postWeather() {
  float temp     = dht.readTemperature();
  float humidity = dht.readHumidity();
  float pressure = bmp.performReading() ? bmp.pressure / 100.0 : 0;

  if (isnan(temp) || isnan(humidity)) return;
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(API_WEATHER);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> doc;
  doc["session_id"]    = lastSession.length() > 0 ? lastSession : "unknown";
  doc["temp_c"]        = temp;
  doc["humidity_pct"]  = humidity;
  doc["pressure_hpa"]  = pressure;

  String body;
  serializeJson(doc, body);
  http.POST(body);
  http.end();
}

void updateDisplay() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "BASE STATION  " + lastSession);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 14, String(lastSpeed, 0) + " kph  " + String(lastSats) + " sats");
  display.setFont(ArialMT_Plain_10);

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  if (!isnan(temp) && !isnan(hum)) {
    display.drawString(0, 36, "Temp: " + String(temp, 1) + "C  Hum: " + String(hum, 0) + "%");
  }

  String wifiStatus = WiFi.status() == WL_CONNECTED ? "WiFi OK" : "No WiFi";
  display.drawString(0, 50, wifiStatus);
  display.display();
}
