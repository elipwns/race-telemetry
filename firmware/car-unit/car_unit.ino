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
 *   - Adafruit GFX Library
 *   - Adafruit ST7735 and ST7789 Library
 */

#include <RadioLib.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

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
#define GNSS_VCC_PIN   3
#define GNSS_BAUD   9600

// --- TFT display pins (ST7735S 80x160) ---
#define TFT_CS    38
#define TFT_DC    40
#define TFT_RST   39
#define TFT_MOSI   2
#define TFT_SCLK   3   // shared with GNSS_VCC_PIN — init GNSS first, then display
#define TFT_BL    21

#define TX_INTERVAL_MS 1000

// LoRa on FSPI (SPI2), display on HSPI (SPI3) — two separate buses
SPIClass loraSPI(FSPI);
SPIClass dispSPI(HSPI);

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSPI);
TinyGPSPlus gps;
HardwareSerial gnssSerial(1);
Adafruit_ST7735 tft = Adafruit_ST7735(&dispSPI, TFT_CS, TFT_DC, TFT_RST);

unsigned long lastTx = 0;
int txCount = 0;

void setup() {
  Serial.begin(115200);

  // Power GNSS before display takes GPIO3 as SPI SCLK
  pinMode(GNSS_VCC_PIN, OUTPUT);
  digitalWrite(GNSS_VCC_PIN, HIGH);
  delay(100);
  gnssSerial.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);

  // Display
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  dispSPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  drawBooting();

  // LoRa
  loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR,
                           CODING_RATE, RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                           TRANSMIT_POWER);
  if (state != RADIOLIB_ERR_NONE) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.println("LoRa FAIL");
    tft.println(state);
    Serial.println("LoRa init failed: " + String(state));
    while (true);
  }
  radio.setDio2AsRfSwitch(true);

  Serial.println("CAR UNIT ready. Session: " + String(SESSION_ID));
}

void loop() {
  while (gnssSerial.available()) {
    gps.encode(gnssSerial.read());
  }

  if (millis() - lastTx >= TX_INTERVAL_MS) {
    lastTx = millis();
    transmitTelemetry();
  }
}

void transmitTelemetry() {
  double lat      = gps.location.isValid() ? gps.location.lat()          : 0.0;
  double lon      = gps.location.isValid() ? gps.location.lng()          : 0.0;
  double speedKph = gps.speed.isValid()    ? gps.speed.kmph()            : 0.0;
  int    heading  = gps.course.isValid()   ? (int)gps.course.deg()       : 0;
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

void drawBooting() {
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("CAR UNIT  " + String(SESSION_ID));
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("Waiting for fix...");
}

void updateDisplay(float speedMph, int sats, double lat, double lon, bool txOk) {
  tft.fillScreen(ST77XX_BLACK);

  // Top row: sats + tx count
  tft.setTextSize(1);
  tft.setTextColor(sats > 0 ? ST77XX_GREEN : ST77XX_YELLOW);
  tft.setCursor(0, 0);
  tft.print("SAT:");
  tft.print(sats);
  tft.setTextColor(txOk ? ST77XX_GREEN : ST77XX_RED);
  tft.setCursor(80, 0);
  tft.print("TX:");
  tft.println(txCount);

  // Speed (large)
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 12);
  tft.print((int)speedMph);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.print(" mph");

  // Coordinates
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(0, 50);
  tft.println(String(lat, 5));
  tft.setCursor(0, 62);
  tft.println(String(lon, 5));
}
