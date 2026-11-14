# Setup 2: nodes and TTN

## 1. Arduino IDE

1. Install the CP210x USB driver and a board package that lists **WiFi LoRa 32(V3)** (see [the stage 1 README](../firmware/p2p_link_test/README.md)).
2. Library Manager: install **Heltec ESP32 Dev-Boards** (gives `LoRaWan_APP.h`). For node 1 also install **Adafruit BME280 Library** and **Adafruit Unified Sensor**.
3. Select the board **WiFi LoRa 32(V3)** and set **Tools > LoRaWAN Region > REGION_EU868**. The region menu matters: with the wrong region the node transmits on channels the gateway is not listening to and nothing shows any error.

## 2. Create the TTN application and devices

1. TTN console (eu1) > Applications > Create application. Mine is `heltec-lora-9-2`.
2. Inside it, End devices > Register end device > **Enter end device specifics manually**:

| Setting | Value |
|---|---|
| Frequency plan | Europe 863-870 MHz (SF9 for RX2 - recommended), same as the gateway |
| LoRaWAN version | 1.0.2 |
| Regional Parameters | The 1.0.2 option the form offers |
| JoinEUI | `0000000000000000` |
| DevEUI | Click **Generate** |
| AppKey | Click **Generate** |

3. Repeat for each node. Mine are `node-1-9-2` (BME280 + LDR) and `heltec-2-17-3` (soil moisture). Every device gets its own DevEUI and AppKey.

## 3. Put the keys in the sketch

In the sketch folder, copy `secrets.example.h` to `secrets.h` and fill in the three arrays from the TTN device page. Keep TTN's byte order (MSB first): DevEUI `70B3D5...` becomes `{ 0x70, 0xB3, 0xD5, ... }`.

`secrets.h` is ignored by Git. The AppKey is the device's password, so it must not be pushed.

## 4. Prove the join first

Upload [`firmware/otaa_join_test`](../firmware/otaa_join_test/) before any sensor code. Serial monitor at 115200 baud:

```
STATE: JOIN (OTAA) - sending JoinRequest
joining...
joined
STATE: SEND - sending uplink
Payload prepared: 11 22 33 44
unconfirmed uplink sending ...
```

TTN > End devices > your device > Live data shows `11223344` every 20 seconds.

If it stays at `joining...`, see [troubleshooting.md](troubleshooting.md#lorawan-join).

## 5. Add the payload formatter

TTN > Applications > your application > Payload formatters > Uplink > **Custom Javascript formatter**. Paste [`ttn/uplink_formatter.js`](../ttn/uplink_formatter.js) and save. Live data then shows decoded values next to the raw bytes.

## 6. Wire and flash the sensor nodes

**Node 1: [`firmware/node1_bme280_ldr`](../firmware/node1_bme280_ldr/)**

| Sensor pin | Heltec V3 pin |
|---|---|
| BME280 VCC | 3V3 (not 5V) |
| BME280 GND | GND |
| BME280 SDA | GPIO 3 |
| BME280 SCL | GPIO 1 |
| LDR, one leg | 3V3 |
| LDR, other leg | GPIO 4, and through a 10 kΩ resistor to GND |

If the BME280 is not found, run [`firmware/i2c_scanner`](../firmware/i2c_scanner/) to see whether anything answers on the bus.

**Node 2: [`firmware/node2_soil_moisture`](../firmware/node2_soil_moisture/)**

| Sensor pin | Heltec V3 pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| AO | GPIO 5 |
| DO | GPIO 6 |

Expected serial output, node 1:

```
BME280 initialized successfully!
STATE: JOIN (OTAA) - sending JoinRequest
joining...
joined
STATE: SEND - sending uplink with sensor data
BME280 - Temp: 25.86 C | Humidity: 33.90 % | Pressure: 948.08 hPa | Light ADC: 790
Payload (hex): 0A 1A 0D 3D 03 B4 03 16
```

## 7. Downlink test (optional)

TTN > End devices > your device > Messaging > Downlink. Enter FPort `2` and payload `01`, then schedule it. A Class A device only listens just after it transmits, so the downlink arrives after the next uplink:

```
received unconfirmed downlink: rssi = -11, snr = 9, datarate = 5
+REV DATA:RXWIN1,RXSIZE 1,PORT 2
+REV DATA:01
```
