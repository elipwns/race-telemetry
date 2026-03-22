/*
 * Race Telemetry - CAR UNIT
 * Heltec Wireless Tracker (HTIT-Tracker) — ESP32-S3, SX1262, UC6580 GNSS
 *
 * Reads GNSS at ~1Hz (UC6580 default) and broadcasts telemetry over LoRa.
 * No WiFi needed — base station handles cloud upload.
 *
 * Board: "Heltec Wireless Tracker" (Heltec ESP32 board package)
 * Board manager URL: https://resource.heltec.cn/download/package_heltec_esp32_index.json
 *
 * Libraries required:
 *   - RadioLib        (LoRa — install via Library Manager)
 *   - TinyGPSPlus     (NMEA parsing — install via Library Manager)
 *
 * All pins below are for Wireless Tracker V1. Verify against your board's
 * schematic if something doesn't work: https://resource.heltec.cn/download
 *
 * See CONFIG.md for session ID configuration.
 */

#include <RadioLib.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <SPI.h>

// --- Session ID ---
#define SESSION_ID "S001"         // Change before each session

// --- LoRa settings (must match base station) ---
#define FREQUENCY        905.2
#define BANDWIDTH        125.0
#define SPREADING_FACTOR 7
#define CODING_RATE      5
#define TRANSMIT_POWER   14

// --- SX1262 pins (Wireless Tracker V1) ---
#define LORA_CS    8
#define LORA_DIO1  14
#define LORA_RST   12
#define LORA_BUSY  13
#define LORA_SCK   9
#define LORA_MISO  11
#define LORA_MOSI  10

// --- UC6580 GNSS pins (Wireless Tracker V1) ---
#define GNSS_RX_PIN   33   // ESP32 receives from GNSS TX
#define GNSS_TX_PIN   34   // ESP32 transmits to GNSS RX
#define GNSS_VCC_PIN   3   // Power enable — set HIGH to power GNSS module
#define GNSS_BAUD   9600

// --- Transmit interval ---
#define TX_INTERVAL_MS   1000   // 1Hz — UC6580 default output rate

// --- Globals ---
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
TinyGPSPlus gps;
HardwareSerial gnssSerial(1);
unsigned long lastTx = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("CAR UNIT — Wireless Tracker");
  Serial.println("Session: " + String(SESSION_ID));

  // Power on GNSS module
  pinMode(GNSS_VCC_PIN, OUTPUT);
  digitalWrite(GNSS_VCC_PIN, HIGH);
  delay(100);

  // Init GNSS UART
  gnssSerial.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  Serial.println("GNSS serial started");

  // Init LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR,
                           CODING_RATE, RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                           TRANSMIT_POWER);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init failed: " + String(state));
    while (true);
  }
  // Required for SX1262: use DIO2 as RF switch
  radio.setDio2AsRfSwitch(true);

  Serial.println("LoRa ready");
  Serial.println("Waiting for GNSS fix...");
}

void loop() {
  // Feed all available GNSS bytes to TinyGPS++
  while (gnssSerial.available()) {
    gps.encode(gnssSerial.read());
  }

  if (millis() - lastTx >= TX_INTERVAL_MS) {
    lastTx = millis();
    transmitTelemetry();
  }
}

void transmitTelemetry() {
  double lat      = gps.location.isValid() ? gps.location.lat()     : 0.0;
  double lon      = gps.location.isValid() ? gps.location.lng()     : 0.0;
  double speedKph = gps.speed.isValid()    ? gps.speed.kmph()       : 0.0;
  int    heading  = gps.course.isValid()   ? (int)gps.course.deg()  : 0;
  int    sats     = gps.satellites.isValid() ? (int)gps.satellites.value() : 0;

  // Format: TEL:{session}:{lat}:{lon}:{speed_kph}:{heading}:{sats}:{millis}
  char packet[80];
  snprintf(packet, sizeof(packet),
    "TEL:%s:%.7f:%.7f:%.1f:%d:%d:%lu",
    SESSION_ID, lat, lon, speedKph, heading, sats, millis()
  );

  int state = radio.transmit(packet);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("TX: " + String(packet));
  } else {
    Serial.println("TX failed: " + String(state));
  }
}
