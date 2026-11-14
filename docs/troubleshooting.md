# Troubleshooting log

Every problem I hit, grouped by the part of the system it belongs to, with what caused it and what fixed it.

A rule that would have saved days: **find out where the data stops before changing anything.** Check in this order and fix the first stage that shows nothing.

| # | Check | Where |
|---|---|---|
| 1 | Is the node transmitting? | Serial monitor: `unconfirmed uplink sending ...` |
| 2 | Is the packet forwarder running? | Pi: `ps aux \| grep lora_pkt_fwd` |
| 3 | Does the gateway hear the node? | TTN > Gateways > Live data |
| 4 | Does TTN accept the device? | TTN > End devices > Live data |
| 5 | Is Telegraf connected and writing? | Pi: `sudo journalctl -u telegraf -f` |
| 6 | Is the data in InfluxDB? | InfluxDB Data Explorer, queries in [`grafana/queries.md`](../grafana/queries.md) |
| 7 | Is the panel query right? | Grafana > Explore |

Contents: [Board and Arduino IDE](#board-and-arduino-ide) · [Gateway](#gateway) · [LoRaWAN join](#lorawan-join) · [Sensors](#sensors) · [Data pipeline](#data-pipeline)

---

## Board and Arduino IDE

### Device Manager: "drivers for this device are not installed (Code 28)"

**Cause:** no driver for the board's CP2102 USB-to-UART chip.
**Fix:** install the CP210x Universal Windows Driver from Silicon Labs, then replug.

### Arduino IDE shows "Unknown COM3 / COM4"

**Cause:** the IDE sees a serial port but cannot tell which board is on it.
**Fix:** normal. Pick the board by hand under **Tools > Board**, then the port.

### `Failed to connect to ESP32-S3: No serial data received`

**Cause:** the board did not enter its bootloader, or something else had the port open.
**Fix:** close any serial monitor, check the cable carries data (some are charge-only), then hold **PRG**, tap **RST**, release **PRG** and upload again. This came back several times during the project and the button sequence fixed it each time.

### The same COM4 for both boards

Not a fault. Windows gives the port number to whichever board is plugged in. With both connected they get different numbers.

### `fatal error: Heltec.h: No such file or directory`, then `'LORA_BANDWIDTH' was not declared in this scope`

**Cause:** example code and installed library did not match. There are several Heltec libraries with similar names and incompatible examples.
**Fix:** only use examples from **File > Examples** of the library that is actually installed.

### `Guru Meditation Error: Core 1 panic'ed (LoadProhibited)`, reset loop, blank OLED

**Cause:** the sketch called `Heltec.display->...` from the `heltec.h` library, and on my V3 the display object was never created, so the code followed an invalid pointer.
**Fix:** for the point-to-point test I switched to the community library **Heltec_ESP32_LoRa_v3** (`heltec_unofficial.h`). OLED, serial and radio worked on the first upload.

### `conflicting declaration 'SX1262 radio'`

**Cause:** my sketch declared `SX1262 radio`, but `heltec_unofficial.h` already creates `radio` with the right pins.
**Fix:** delete my declaration.

---

## Gateway

### `gpioset: unrecognized option '--mode=signal'` then `failed to start the concentrator`

This one took the longest.

`lora_pkt_fwd` runs `./reset_lgw.sh start` to reset the SX1302 chip before talking to it over SPI. The script that came with the HAT uses the **libgpiod v1** command line. My Pi runs Raspberry Pi OS based on Debian 13 (trixie), which ships `gpioset (libgpiod) v2.2.1`. The reset never happened, so the concentrator did not start.

| Attempt | Result |
|---|---|
| `gpioset --mode=signal gpiochip0 17=1` (original) | `unrecognized option '--mode=signal'` |
| `gpioset gpiochip0 17=1` | `invalid line value: 'gpiochip0'` (v2 takes the chip with `-c`) |
| `gpioset 0 17=1` | `invalid line value: '0'` (same reason) |
| `gpioset -c /dev/gpiochip0 17=1` | Terminal hangs: in v2, `gpioset` keeps running to hold the line |
| Same again, or with `-t 100` | Hangs, or `Device or resource busy` |
| `sudo apt install --reinstall gpiod libgpiod2` | `libgpiod2` does not exist on trixie |
| `sudo pkill gpioset`, then retry | Still busy |

How I found the cause of "busy": `sudo grep "" /sys/kernel/debug/gpio` listed the GPIO lines with their owner, and showed lines still held by old `gpioset` processes. A reboot cleared them.

**Fix:** stop using `gpioset`. [`gateway/reset_gpio.py`](../gateway/reset_gpio.py) does the same reset pulse on GPIO 17 with the RPi.GPIO Python library and returns immediately.

### `sh: 1: ./reset_lgw.sh: not found` then `ERROR: failed to reset SX1302`

**Cause:** I deleted the broken `reset_lgw.sh`, but the forwarder calls it by name every time it starts and stops.
**Fix:** a placeholder [`reset_lgw.sh`](../gateway/reset_lgw.sh) that just exits with 0. The real reset runs first, from [`start_gateway.sh`](../gateway/start_gateway.sh).

### Installing `python3-rpi.gpio` removes `python3-rpi-lgpio`

Expected. The two packages provide the same Python module and cannot be installed together.

### `RuntimeWarning: This channel is already in use`

**Cause:** the pin was still configured from an earlier run.
**Fix:** harmless. `GPIO.setwarnings(False)` hides it.

### TTN shows the gateway as disconnected, or nodes "join" forever

**Cause:** the packet forwarder was simply not running. I had stopped it and spent a long time debugging the node and TTN instead. `ps aux | grep lora_pkt_fwd` showed nothing.
**Fix:** start it and leave the terminal open. This is why the checklist at the top starts at the gateway.

### `WARNING: [main] impossible to open /dev/ttyS0 for GPS sync`

Harmless. The configuration expects a GPS module and there is none.

---

## LoRaWAN join

### `lmic.h: No such file`, `'os_init_ex' was not declared`, `'LMIC_setPinMapping' was not declared`

**Cause:** my first attempt used the LMIC stack. Two different LMIC libraries were installed (IBM LMIC framework and MCCI LoRaWAN LMIC) and the IDE picked the wrong one. They share header names and have different functions.
**Fix:** keep only one. The build output line `Multiple libraries were found for "lmic.h"` says which one was used.

### LMIC: `rst:0x8 (TG1WDT_SYS_RST)` reset loop, gateway hears nothing

**Cause:** MCCI LMIC does not know the Heltec V3 (it warned about a missing pin map) and its SX1262 support did not work for me. Serial printed LMIC events, the board kept hitting the watchdog, and TTN's gateway Live data stayed empty.
**Fix:** dropped LMIC for Heltec's own stack: library **Heltec ESP32 Dev-Boards**, `#include "LoRaWan_APP.h"`.

### Heltec stack: stuck at `joining...`

Three separate causes, found one after another:

1. **Packet forwarder not running** (above).
2. **DevEUI byte order.** LMIC wants the DevEUI least-significant byte first; Heltec wants it most-significant byte first, as TTN displays it. I carried the reversed array over from the LMIC sketch, so the node announced itself as `934507D0...` while TTN had `70B3D57E...`. The Heltec library prints `+DevEui=` at start-up, which is how to spot it.
3. **Device not registered.** I had deleted the device and application in TTN. The gateway's Live data showed a valid join request with the right DevEUI, but no application owned that device, so nothing answered.

**Fix:** registered a fresh end device (DevEUI and AppKey generated by the console, LoRaWAN version 1.0.2, same frequency plan as the gateway) and copied the keys into the sketch in TTN's byte order.

**What I learned:** a join request showing up in the *gateway's* Live data proves the radio link. If the *device's* Live data stays empty after that, the problem is registration or keys, not RF.

### `E (117) esp_core_dump_flash: Core dump data check failed`

Harmless. At boot the ESP32 looks for a saved crash dump in flash and reports that there is none.

### The start-up banner prints again between uplinks

Not a crash. As far as I can tell the Heltec library puts the chip into deep sleep between uplinks; waking up restarts the sketch from `setup()`, while the LoRaWAN session is kept, so it goes straight to `STATE: SEND` without joining again.

---

## Sensors

### No GPIO 21 / 22 on the board

**Cause:** most ESP32 tutorials use GPIO 21 (SDA) and 22 (SCL). The Heltec V3 is an ESP32-S3 with different pins broken out.
**Fix:** `Wire.begin(3, 1)` to use GPIO 3 as SDA and GPIO 1 as SCL.

### `ERROR: BME280 not found on I2C!`

**Cause:** the sensor connections were not soldered yet, so the contacts were unreliable.
**Fix:** soldered them, and the sensor initialised on the next boot. [`firmware/i2c_scanner`](../firmware/i2c_scanner/) is the quick way to check this kind of problem: it shows whether anything answers on the bus and at which address (a BME280 is at `0x76` or `0x77`).

### Pressure reads about 948 hPa, not 1013

Not an error. The BME280 reports absolute pressure at its own altitude; 1013 hPa is the sea-level average.

---

## Data pipeline

### Grafana (InfluxDB 1.x): "0 measurements found"

**Cause:** I created the database as `lorawan_data` but typed `Lorawan_data` in the Grafana data source. Database names are case-sensitive.
**Fix:** match the name exactly.

### Grafana (InfluxDB 1.x): `error parsing query: found FROM, expected identifier...`

**Cause:** the panel's query builder had no field selected, producing `SELECT` followed directly by `FROM`.
**Fix:** pick a measurement and a field. `SHOW MEASUREMENTS` in the `influx` shell lists what exists.

### `influxdb.service` failed to start after reinstalling

**Cause:** a broken mix of InfluxDB 1.x and 2.x after several install and remove attempts; the service file pointed at a start script that no longer existed.
**Fix:** removed both and installed cleanly. The final system runs InfluxDB 2.x, which uses organisations, buckets, tokens and the Flux query language instead of databases and InfluxQL.

### TTN webhook: `context deadline exceeded`, then the webhook is deactivated

**Cause:** the webhook URL was `http://192.168.x.x:8086/...`, a private address on my home network. TTN's servers are on the internet and cannot reach it. Testing the same URL with `curl` from inside the network worked in under 100 ms, which made it look like the Pi was fine and TTN was slow.
**Fix:** MQTT. The Pi (Telegraf) connects out to TTN's broker and TTN pushes uplinks down that connection, so nothing has to reach into the home network.

### Telegraf: `failed to write metrics ... (401 Unauthorized)`

**Cause:** the token in `[[outputs.influxdb_v2]]` did not have write access to the bucket.
**Fix:** generated a new token with read and write on `lorawan_data` and checked that `organization` matched exactly, including case.

### Telegraf connected to MQTT but nothing arrives

**Cause:** wrong topic. I subscribed to `heltec-lora-9-2/devices/+/up`.
**Fix:** The Things Stack v3 topics are `v3/<application-id>@ttn/devices/+/up`. Both the `v3/` prefix and the `@ttn` tenant are required.

### `telegraf.service: Start request repeated too quickly` / `Failed to start`

**Cause:** after moving the Pi to another WiFi network the service had failed several times at boot and systemd stopped retrying. The configuration itself was valid: `sudo telegraf --config /etc/telegraf/telegraf.conf --test` ran without errors.
**Fix:** `sudo systemctl reset-failed telegraf && sudo systemctl start telegraf`.

### Grafana panel shows "No data" with no error

**Cause:** a field name that does not exist. Telegraf builds field names from the JSON path, so the decoded temperature is `uplink_message_decoded_payload_temperature`, not `temperature`.
**Fix:** list the real field names with the `distinct(column: "_field")` query in [`grafana/queries.md`](../grafana/queries.md), then copy them exactly.

### `soil_wet` never appears in InfluxDB

**Cause:** the payload formatter returned it as `true` / `false`. Telegraf's JSON parser keeps numbers only and silently drops booleans and strings, so the field never reached the database. TTN's Live data showed it correctly, which made it confusing.
**Fix:** return `1` / `0` from the formatter. This change is in [`ttn/uplink_formatter.js`](../ttn/uplink_formatter.js) but has not been run on the hardware yet.

### No new data after changing WiFi

**Cause:** the Pi's IP address changed (so my browser bookmarks were wrong), the packet forwarder was not running after the reboot, and Telegraf was in a failed state (above).
**Fix:** see "After moving the Pi to another network" in [setup-pipeline.md](setup-pipeline.md).
