/**
 * LoRa receiver for Heltec WiFi LoRa 32 V3 (SX1262).
 *
 * Listens continuously and prints every packet with its RSSI and SNR on the
 * OLED and the serial monitor.
 *
 * This is raw LoRa modulation, not LoRaWAN.
 *
 * Adapted from the LoRa_rx_tx example in ropg/heltec_esp32_lora_v3 (MIT).
 * Libraries: Heltec_ESP32_LoRa_v3, RadioLib.
 */

#define HELTEC_POWER_BUTTON   // must come before the include
#include <heltec_unofficial.h>

// Radio settings. These must match the sender exactly.
#define FREQUENCY         868.0   // MHz. Check what is legal where you are.
#define BANDWIDTH         250.0   // kHz
#define SPREADING_FACTOR  9

String rxdata;

// Set from the radio interrupt, read in loop(). "volatile" tells the compiler
// the value can change outside the normal program flow.
volatile bool rxFlag = false;

// Interrupt handler: keep it tiny. No printing, no radio calls in here.
void rx() {
  rxFlag = true;
}

void setup() {
  heltec_setup();
  both.println("LoRa receiver init");

  // "radio" is already created by heltec_unofficial.h. Declaring your own
  // "SX1262 radio" gives a "redeclared" compile error.
  RADIOLIB_OR_HALT(radio.begin());
  radio.setDio1Action(rx);   // call rx() when a packet arrives

  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));

  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

void loop() {
  heltec_loop();

  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX [%s]\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());
    }
    // Go back to listening for the next packet.
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}
