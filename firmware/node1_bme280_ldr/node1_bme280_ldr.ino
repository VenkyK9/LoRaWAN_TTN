/**
 * Node 1: BME280 (temperature, humidity, pressure) + LDR (light)
 * Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262), LoRaWAN OTAA, EU868, Class A
 *
 * Reads the sensors every 20 s and sends an 8-byte uplink on port 2:
 *
 *   byte 0-1  temperature  int16,  degrees C x 100
 *   byte 2-3  humidity     uint16, percent x 100
 *   byte 4-5  pressure     uint16, hPa
 *   byte 6-7  light        uint16, raw ADC 0-4095
 *
 * Wiring:
 *   BME280 VCC -> 3V3      BME280 SDA -> GPIO 3
 *   BME280 GND -> GND      BME280 SCL -> GPIO 1
 *   LDR + 10k divider midpoint -> GPIO 4
 *
 * Arduino IDE: Board "WiFi LoRa 32(V3)", Tools > LoRaWAN Region > EU868
 * Libraries:   Heltec ESP32 Dev-Boards, Adafruit BME280 Library,
 *              Adafruit Unified Sensor
 *
 * Based on the LoRaWan example from the Heltec ESP32 Dev-Boards library.
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "secrets.h"   // devEui, appEui, appKey (copy secrets.example.h)

// ---------------- Sensors ----------------

#define SDA_PIN 3
#define SCL_PIN 1
#define LDR_PIN 4

Adafruit_BME280 bme;

// ---------------- LoRaWAN settings ----------------

// ABP values. Not used with OTAA, but the Heltec library expects the
// symbols to exist. These are the defaults from the Heltec example.
uint8_t nwkSKey[] = { 0x15, 0xB1, 0xD0, 0xEF, 0xA4, 0x63, 0xDF, 0xBE,
                      0x3D, 0x11, 0x18, 0x1E, 0x1E, 0xC7, 0xDA, 0x85 };
uint8_t appSKey[] = { 0xD7, 0x2C, 0x78, 0x75, 0x8C, 0xDC, 0xCA, 0xBF,
                      0x55, 0xEE, 0x4A, 0x77, 0x8D, 0x16, 0xEF, 0x67 };
uint32_t devAddr = (uint32_t)0x007E6AE1;

// Enable channels 0-7
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;  // set in Tools > LoRaWAN Region
DeviceClass_t loraWanClass = CLASS_A;
bool overTheAirActivation = true;               // OTAA, not ABP
bool loraWanAdr = true;                         // adaptive data rate
bool isTxConfirmed = false;                     // unconfirmed uplinks
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;                  // only used if isTxConfirmed

// Time between uplinks in ms. 20 s is for demos; see the README note on
// the TTN fair use policy before leaving a node running for days.
uint32_t appTxDutyCycle = 20000;

// ---------------- Payload ----------------

static void prepareTxFrame(uint8_t port)
{
  float temperature = bme.readTemperature();        // degrees C
  float humidity = bme.readHumidity();              // percent
  float pressure = bme.readPressure() / 100.0F;     // Pa -> hPa
  uint16_t light = analogRead(LDR_PIN);             // 0-4095

  Serial.printf("BME280 - Temp: %.2f C | Humidity: %.2f %% | Pressure: %.2f hPa | Light ADC: %u\n",
                temperature, humidity, pressure, light);

  // LoRa payloads are raw bytes. Multiplying by 100 keeps two decimal
  // places without sending a 4-byte float. High byte first (big-endian).
  int16_t t100 = (int16_t)(temperature * 100);
  uint16_t h100 = (uint16_t)(humidity * 100);
  uint16_t p = (uint16_t)pressure;

  appDataSize = 8;
  appData[0] = (uint8_t)(t100 >> 8);
  appData[1] = (uint8_t)(t100 & 0xFF);
  appData[2] = (uint8_t)(h100 >> 8);
  appData[3] = (uint8_t)(h100 & 0xFF);
  appData[4] = (uint8_t)(p >> 8);
  appData[5] = (uint8_t)(p & 0xFF);
  appData[6] = (uint8_t)(light >> 8);
  appData[7] = (uint8_t)(light & 0xFF);

  Serial.print("Payload (hex): ");
  for (int i = 0; i < appDataSize; i++) {
    Serial.printf("%02X ", appData[i]);
  }
  Serial.println();
}

// ---------------- Setup ----------------

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);   // "TPYE" is the library's spelling

  Serial.println("Heltec WiFi LoRa 32 V3 - LoRaWAN OTAA + BME280 START");

  // The V3 has no default I2C header pins, so name the pins explicitly.
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.printf("I2C initialized: SDA=GPIO%d, SCL=GPIO%d\n", SDA_PIN, SCL_PIN);

  // BME280 breakout boards use address 0x76 or 0x77. Try both.
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("ERROR: BME280 not found at 0x76 or 0x77. Check wiring (VCC must be 3V3).");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("BME280 initialized successfully!");

  pinMode(LDR_PIN, INPUT);
  Serial.printf("LDR ADC (GPIO%d) initialized\n", LDR_PIN);

  Serial.println("Waiting for LoRa join...");
}

// ---------------- Main state machine ----------------
// The Heltec library drives deviceState. Each pass through loop() handles
// one state: INIT -> JOIN -> SEND -> CYCLE -> SLEEP -> SEND -> ...

void loop()
{
  switch (deviceState)
  {
    case DEVICE_STATE_INIT:
    {
      Serial.println("STATE: INIT");
      LoRaWAN.init(loraWanClass, loraWanRegion);
      LoRaWAN.setDefaultDR(3);
      deviceState = DEVICE_STATE_JOIN;
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      Serial.println("STATE: JOIN (OTAA) - sending JoinRequest");
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      Serial.println("STATE: SEND - sending uplink with sensor data");
      prepareTxFrame(appPort);
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      Serial.println("STATE: CYCLE - scheduling next uplink");
      // A small random offset stops two nodes transmitting in step.
      txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}
