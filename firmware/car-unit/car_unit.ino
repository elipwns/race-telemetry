/*
 * Race Telemetry - CAR UNIT
 * Heltec LoRa 32 V3 (ESP32) + u-blox NEO-M8N GPS
 *
 * Reads GPS at 5Hz and broadcasts telemetry over LoRa.
 * No WiFi needed — base station handles cloud upload.
 *
 * Libraries required:
 *   - heltec_unofficial (LoRa + OLED)
 *   - SparkFun u-blox GNSS Arduino Library
 *
 * Hardware wiring (GPS to Heltec LoRa 32 V3):
 *   GPS TX  -> ESP32 GPIO 45 (RX1)
 *   GPS RX  -> ESP32 GPIO 46 (TX1)
 *   GPS VCC -> 3.3V
 *   GPS GND -> GND
 *
 * See CONFIG.md for session ID configuration.
 */

#include <heltec_unofficial.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <HardwareSerial.h>

// --- Configuration ---
#define SESSION_ID "S001"         // Change before each session

// LoRa settings (must match base station)
#define FREQUENCY        905.2
#define BANDWIDTH        125.0
#define SPREADING_FACTOR 7
#define TRANSMIT_POWER   14

// GPS serial port
#define GPS_SERIAL_PORT  Serial1
#define GPS_RX_PIN       45
#define GPS_TX_PIN       46
#define GPS_BAUD         38400

// Transmit interval (ms) — 200ms = 5Hz
#define TX_INTERVAL_MS   200

// --- Globals ---
SFE_UBLOX_GNSS gps;
unsigned long lastTx = 0;

void setup() {
  heltec_setup();
  both.println("CAR UNIT");
  both.println("Starting up...");

  // Initialize LoRa (TX only, no receive)
  RADIOLIB_OR_HALT(radio.begin());
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));

  // Initialize GPS
  GPS_SERIAL_PORT.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  if (!gps.begin(GPS_SERIAL_PORT)) {
    both.println("GPS not detected!");
    // Continue — will show 0 satellites until fix
  }

  gps.setNavigationFrequency(5);  // 5Hz updates

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "CAR UNIT");
  display.drawString(0, 20, "Session: " + String(SESSION_ID));
  display.display();
}

void loop() {
  heltec_loop();
  gps.checkUblox();

  if (millis() - lastTx >= TX_INTERVAL_MS) {
    lastTx = millis();
    transmitTelemetry();
  }
}

void transmitTelemetry() {
  long lat = gps.getLatitude();    // degrees * 1e-7
  long lon = gps.getLongitude();   // degrees * 1e-7
  long speed = gps.getSpeedKmph(); // mm/s → kph via library
  long heading = gps.getHeading(); // degrees * 1e-5
  byte sats = gps.getSIV();        // satellites in view

  // Format: TEL:{session}:{lat}:{lon}:{speed_kph}:{heading}:{sats}:{millis}
  // Lat/lon stored as integer degrees * 1e-7, divide for 7 decimal places
  char packet[80];
  snprintf(packet, sizeof(packet),
    "TEL:%s:%.7f:%.7f:%.1f:%d:%d:%lu",
    SESSION_ID,
    lat / 10000000.0,
    lon / 10000000.0,
    speed / 1000.0,    // mm/s to kph
    (int)(heading / 100000),
    sats,
    millis()
  );

  radio.transmit(packet);

  // Update OLED every transmit
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, String(SESSION_ID) + "  Sats:" + String(sats));
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 14, String(speed / 1000.0, 0) + " kph");
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 36, String(lat / 10000000.0, 6));
  display.drawString(0, 48, String(lon / 10000000.0, 6));
  display.display();
}
