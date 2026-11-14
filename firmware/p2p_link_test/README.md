# Stage 1: point-to-point LoRa link test

The first thing I got working: two Heltec V3 boards talking directly to each other with raw LoRa, no gateway and no network. It proves the boards, antennas, drivers and toolchain are fine before LoRaWAN adds more things that can go wrong.

Raw LoRa is not LoRaWAN. A LoRaWAN gateway ignores these packets.

| Sketch | What it does |
|---|---|
| `lora_sender` | Sends an incrementing counter every 5 s, or at once when the PRG button is pressed |
| `lora_receiver` | Prints every packet with RSSI and SNR on the OLED and serial monitor |

## Radio settings

Both boards must use the same values.

| Parameter | Value |
|---|---|
| Frequency | 868.0 MHz |
| Bandwidth | 250 kHz |
| Spreading factor | 9 |
| TX power | 0 dBm (1 mW, enough on a desk) |

## Setup (Windows, Arduino IDE 2.x)

1. **Antenna first.** Attach the LoRa antenna before powering the board. Transmitting without one can damage the radio.
2. **USB driver.** The board uses a CP2102 USB-to-UART chip. If Device Manager shows it with *Code 28* (no driver), install the **CP210x Universal Windows Driver** from Silicon Labs and replug. It then appears under **Ports (COM & LPT)**.
3. **Board package.** In **Boards Manager** install `esp32` by Espressif Systems and select **WiFi LoRa 32(V3)**.
4. **Libraries.** In **Library Manager** install **Heltec_ESP32_LoRa_v3** (by ropg, provides `heltec_unofficial.h`) and **RadioLib**.
5. **Upload** `lora_receiver` to one board and `lora_sender` to the other, then open the serial monitor at 115200 baud.

With one laptop, both boards can be plugged in at once. Each gets its own COM port and only one serial monitor is open at a time. A programmed board also runs from any USB charger.

## Result

Receiver, boards about a metre apart:

```
RX [73]
  RSSI: -20.00 dBm
  SNR: 11.50 dB
```

- **RSSI** is signal strength in dBm. Closer to 0 is stronger. LoRa still decodes below -120 dBm.
- **SNR** is signal-to-noise ratio in dB. LoRa can decode slightly below 0 dB, which is where its range comes from.
