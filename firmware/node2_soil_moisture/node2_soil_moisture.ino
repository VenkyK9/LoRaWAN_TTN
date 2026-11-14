/**
 * Node 2: soil moisture sensor (analog + digital threshold output)
 * Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262), LoRaWAN OTAA, EU868, Class A
 *
 * Reads the sensor every 20 s and sends a 3-byte uplink on port 2:
 *
 *   byte 0-1  soil moisture  uint16, raw ADC 0-4095 (high = dry)
 *   byte 2    threshold flag 1 = DO pin HIGH, 0 = DO pin LOW
 *
 * Wiring:
 *   Sensor VCC -> 3V3      Sensor AO -> GPIO 5
 *   Sensor GND -> GND      Sensor DO -> GPIO 6
 *
 * Arduino IDE: Board "WiFi LoRa 32(V3)", Tools > LoRaWAN Region > EU868
 * Library:     Heltec ESP32 Dev-Boards
 *
 * Based on the LoRaWan example from the Heltec ESP32 Dev-Boards library.
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "secrets.h"   // devEui, appEui, appKey (copy secrets.example.h)

// ---------------- Sensor ----------------

#define SOIL_MOISTURE_AO 5   // analog output
#define SOIL_MOISTURE_DO 6   // digital output, trips at the board's trimmer setting

// ---------------- LoRaWAN settings ----------------

// ABP values. Not used with OTAA, but the Heltec library expects the
// symbols to exist. These are the defaults from the Heltec example.
uint8_t nwkSKey[] = { 0x15, 0xB1, 0xD0, 0xEF, 0xA4, 0x63, 0xDF, 0xBE,
                      0x3D, 0x11, 0x18, 0x1E, 0x1E, 0xC7, 0xDA, 0x85 };
uint8_t appSKey[] = { 0xD7, 0x2C, 0x78, 0x75, 0x8C, 0xDC, 0xCA, 0xBF,
                      0x55, 0xEE, 0x4A, 0x77, 0x8D, 0x16, 0xEF, 0x67 };
uint32_t devAddr = (uint32_t)0x007E6AE1;

uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;  // set in Tools > LoRaWAN Region
DeviceClass_t loraWanClass = CLASS_A;
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = false;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;
uint32_t appTxDutyCycle = 20000;                // ms between uplinks

// ---------------- Payload ----------------

static void prepareTxFrame(uint8_t port)
{
  uint16_t soilAdc = analogRead(SOIL_MOISTURE_AO);       // 0-4095
  uint8_t threshold = digitalRead(SOIL_MOISTURE_DO);

  // Uncalibrated estimate: 4095 (dry, in air) -> 0 %, 0 (in water) -> 100 %.
  // Printed here for debugging only. The TTN formatter does the same maths.
  uint8_t soilPercent = map(soilAdc, 4095, 0, 0, 100);

  Serial.printf("SOIL MOISTURE - ADC: %u | Percent: %u %% | Threshold: %s\n",
                soilAdc, soilPercent, threshold == HIGH ? "WET" : "DRY");

  appDataSize = 3;
  appData[0] = (uint8_t)(soilAdc >> 8);
  appData[1] = (uint8_t)(soilAdc & 0xFF);
  appData[2] = (threshold == HIGH) ? 0x01 : 0x00;

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

  Serial.println("Heltec WiFi LoRa 32 V3 - LoRaWAN OTAA + soil moisture START");

  pinMode(SOIL_MOISTURE_AO, INPUT);
  pinMode(SOIL_MOISTURE_DO, INPUT);
  Serial.printf("Soil sensor: AO=GPIO%d, DO=GPIO%d\n", SOIL_MOISTURE_AO, SOIL_MOISTURE_DO);

  Serial.println("Waiting for LoRa join...");
}

// ---------------- Main state machine ----------------

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
