/*
 * Race Telemetry - CAR UNIT
 * Heltec Wireless Tracker (HTIT-Tracker) — ESP32-S3, SX1262, UC6580 GNSS
 *
 * Board: "Heltec Wireless Tracker"
 * Board manager URL: https://resource.heltec.cn/download/package_heltec_esp32_index.json
 *
 * Libraries required:
 *   - RadioLib
 *   - TinyGPSPlus
 *   - Heltec_ESP32_Dev-Boards  (provides HT_st7735)
 */

#include <RadioLib.h>
#include "HT_TinyGPS++.h"
#include <HardwareSerial.h>
#include <SPI.h>
#include "HT_st7735.h"

// --- Session ID ---
#define SESSION_ID "S001"

// --- LoRa settings (must match base station) ---
#define FREQUENCY        905.2
#define BANDWIDTH        125.0
#define SPREADING_FACTOR 7
#define CODING_RATE      5
#define TRANSMIT_POWER   14

// --- SX1262 pins ---
#define LORA_CS    8
#define LORA_DIO1  14
#define LORA_RST   12
#define LORA_BUSY  13
#define LORA_SCK   9
#define LORA_MISO  11
#define LORA_MOSI  10

// --- UC6580 GNSS pins ---
#define GNSS_RX_PIN   33
#define GNSS_TX_PIN   34
#define GNSS_BAUD   9600

#define TX_INTERVAL_MS 1000

// TFT display — HT_st7735 uses default pins from HT_st7735.h:
//   CS=38, RST=39, DC=40, SCLK=41, MOSI=42, LED_K=21 (active LOW), VTFT=3
HT_st7735 tft;

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
TinyGPSPlus gps;
HardwareSerial gnssSerial(1);

unsigned long lastTx = 0;
int txCount = 0;
unsigned long gnssBytes = 0;

void setup() {
  Serial.begin(115200);

  // Display
  tft.st7735_init();
  tft.st7735_fill_screen(ST7735_BLACK);
  tft.st7735_write_str(0, 10, "CAR UNIT", Font_11x18, ST7735_CYAN, ST7735_BLACK);
  tft.st7735_write_str(0, 30, String(SESSION_ID), Font_11x18, ST7735_WHITE, ST7735_BLACK);
  tft.st7735_write_str(0, 50, "Waiting fix...", Font_7x10, ST7735_YELLOW, ST7735_BLACK);

  // GNSS
  gnssSerial.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);

  // LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR,
                           CODING_RATE, RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                           TRANSMIT_POWER);
  if (state != RADIOLIB_ERR_NONE) {
    tft.st7735_fill_screen(ST7735_BLACK);
    tft.st7735_write_str(0, 10, "LoRa FAIL", Font_11x18, ST7735_RED, ST7735_BLACK);
    tft.st7735_write_str(0, 30, String(state), Font_11x18, ST7735_RED, ST7735_BLACK);
    Serial.println("LoRa init failed: " + String(state));
    while (true);
  }
  radio.setDio2AsRfSwitch(true);

  Serial.println("CAR UNIT ready. Session: " + String(SESSION_ID));
}

void loop() {
  while (gnssSerial.available()) {
    gps.encode(gnssSerial.read());
    gnssBytes++;
  }

  if (millis() - lastTx >= TX_INTERVAL_MS) {
    lastTx = millis();
    transmitTelemetry();
  }
}

void transmitTelemetry() {
  double lat      = gps.location.isValid() ? gps.location.lat()            : 0.0;
  double lon      = gps.location.isValid() ? gps.location.lng()            : 0.0;
  double speedKph = gps.speed.isValid()    ? gps.speed.kmph()              : 0.0;
  int    heading  = gps.course.isValid()   ? (int)gps.course.deg()         : 0;
  int    sats     = gps.satellites.isValid() ? (int)gps.satellites.value() : 0;
  float  speedMph = speedKph * 0.621371;

  char packet[80];
  snprintf(packet, sizeof(packet),
    "TEL:%s:%.7f:%.7f:%.1f:%d:%d:%lu",
    SESSION_ID, lat, lon, speedKph, heading, sats, millis()
  );

  int state = radio.transmit(packet);
  txCount++;

  updateDisplay(speedMph, sats, lat, lon, state == RADIOLIB_ERR_NONE);

  Serial.println("TX: " + String(packet) +
                 "  (" + String(speedMph, 0) + " mph, " + String(sats) + " sats)");
}

void updateDisplay(float speedMph, int sats, double lat, double lon, bool txOk) {
  // One-time clear to wipe boot screen, then overwrite in place each update.
  static bool cleared = false;
  if (!cleared) { tft.st7735_fill_screen(ST7735_BLACK); cleared = true; }

  char buf[24];

  // Row 1: sats (left) + tx count (right) — 7x10 font
  snprintf(buf, sizeof(buf), "SAT:%-2d", sats);
  tft.st7735_write_str(0, 2, buf, Font_7x10,
                        sats > 0 ? ST7735_GREEN : ST7735_YELLOW, ST7735_BLACK);
  snprintf(buf, sizeof(buf), "TX:%-4d", txCount);
  tft.st7735_write_str(90, 2, buf, Font_7x10,
                        txOk ? ST7735_GREEN : ST7735_RED, ST7735_BLACK);

  // Row 2: speed — fixed 7-char field ("  0 mph" to "160 mph")
  snprintf(buf, sizeof(buf), "%3d mph", (int)speedMph);
  tft.st7735_write_str(0, 16, buf, Font_16x26, ST7735_WHITE, ST7735_BLACK);

  // Rows 3-4: coords once fixed, otherwise show GNSS byte counter
  if (sats > 0) {
    snprintf(buf, sizeof(buf), "% 9.5f", lat);
    tft.st7735_write_str(0, 50, buf, Font_7x10, ST7735_CYAN, ST7735_BLACK);
    snprintf(buf, sizeof(buf), "% 10.5f", lon);
    tft.st7735_write_str(0, 62, buf, Font_7x10, ST7735_CYAN, ST7735_BLACK);
  } else {
    snprintf(buf, sizeof(buf), "rx:%-7lu", gnssBytes);
    tft.st7735_write_str(0, 50, buf, Font_7x10,
                          gnssBytes > 0 ? ST7735_YELLOW : ST7735_RED, ST7735_BLACK);
    tft.st7735_write_str(0, 62, "searching...  ", Font_7x10, ST7735_YELLOW, ST7735_BLACK);
  }
}
