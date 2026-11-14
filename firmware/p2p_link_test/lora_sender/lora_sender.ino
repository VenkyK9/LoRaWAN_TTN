/**
 * LoRa sender for Heltec WiFi LoRa 32 V3 (SX1262).
 *
 * Sends an incrementing counter every PAUSE seconds, or immediately when the
 * PRG button is clicked. Output goes to both the OLED and the serial monitor.
 *
 * This is raw LoRa modulation, not LoRaWAN.
 *
 * Adapted from the LoRa_rx_tx example in ropg/heltec_esp32_lora_v3 (MIT).
 * Libraries: Heltec_ESP32_LoRa_v3, RadioLib.
 */

// Turns the PRG button into a power button (long press = off).
// Must come before the include.
#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// Radio settings. These must match the receiver exactly.
// Keep the decimal point: these are floats, and printing an int with %.2f
// shows 0.00.
#define FREQUENCY         868.0   // MHz. Check what is legal where you are.
#define BANDWIDTH         250.0   // kHz
#define SPREADING_FACTOR  9       // 5..12, higher = slower but longer range
#define TRANSMIT_POWER    0       // dBm. 0 dBm = 1 mW, fine on a desk.

// Seconds between automatic transmissions. 0 = only on button press.
#define PAUSE             5

long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time = 0;
uint64_t minimum_pause = 0;

void setup() {
  heltec_setup();   // serial at 115200, display, button
  both.println("LoRa sender init");

  // "radio" is already created by heltec_unofficial.h with the right pins.
  RADIOLIB_OR_HALT(radio.begin());

  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
}

void loop() {
  heltec_loop();    // keeps the button and power handling running

  bool tx_legal = millis() > last_tx + minimum_pause;
  bool timer_due = PAUSE && tx_legal && (millis() - last_tx > (PAUSE * 1000));

  if (timer_due || button.isSingleClick()) {
    if (!tx_legal) {
      both.printf("Legal limit, wait %i sec.\n",
                  (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
      return;
    }

    both.printf("TX [%ld] ", counter);
    heltec_led(50);
    tx_time = millis();
    RADIOLIB(radio.transmit(String(counter++).c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);

    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\n", (int)tx_time);
    } else {
      both.printf("fail (%i)\n", _radiolib_status);
    }

    // Stay under a 1% duty cycle: wait 100x the airtime before the next send.
    minimum_pause = tx_time * 100;
    last_tx = millis();
  }
}
