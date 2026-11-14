/**
 * OTAA join test: no sensors, fixed payload.
 * Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262), LoRaWAN OTAA, EU868, Class A
 *
 * Joins TTN and sends the bytes 11 22 33 44 every 20 s. Use it to prove
 * the chain node -> gateway -> TTN (-> MQTT -> InfluxDB) before adding
 * sensors. It also prints the keys at start-up so they can be compared
 * with the TTN console character by character.
 *
 * Downlinks: schedule one in the TTN console and the Heltec library
 * prints it after the next uplink, e.g.
 *   +REV DATA:RXWIN1,RXSIZE 1,PORT 2
 *   +REV DATA:01
 *
 * Arduino IDE: Board "WiFi LoRa 32(V3)", Tools > LoRaWAN Region > EU868
 * Library:     Heltec ESP32 Dev-Boards
 *
 * Based on the LoRaWan example from the Heltec ESP32 Dev-Boards library.
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "secrets.h"   // devEui, appEui, appKey (copy secrets.example.h)

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

static void prepareTxFrame(uint8_t port)
{
  appDataSize = 4;
  appData[0] = 0x11;
  appData[1] = 0x22;
  appData[2] = 0x33;
  appData[3] = 0x44;
  Serial.println("Payload prepared: 11 22 33 44");
}

static void printHex(const char *label, const uint8_t *data, int len)
{
  Serial.print(label);
  for (int i = 0; i < len; i++) {
    Serial.printf("%02X", data[i]);
  }
  Serial.println();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);   // "TPYE" is the library's spelling

  Serial.println("Heltec WiFi LoRa 32 V3 - LoRaWAN OTAA join test");

  // TTN silently ignores a join request if any of these differ from the
  // console, so print them. Remove the AppKey line once the join works.
  printHex("+DevEui=", devEui, 8);
  printHex("+AppEui=", appEui, 8);
  printHex("+AppKey=", appKey, 16);
}

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
      Serial.println("STATE: SEND - sending uplink");
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
