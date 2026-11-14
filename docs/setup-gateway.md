# Setup 1: gateway (Raspberry Pi + Elecrow LR1302)

Hardware: Raspberry Pi with the Elecrow LR1302 LoRaWAN HAT (SPI, EU868 version) and its antenna. Software: Raspberry Pi OS (Debian 13 "trixie") and Semtech's `sx1302_hal`, which contains the packet forwarder.

## 1. Enable SPI

```bash
sudo raspi-config        # Interface Options > SPI > Enable
ls /dev/spidev0.0        # must exist after a reboot
```

## 2. Build the packet forwarder

Follow Elecrow's guide for the LR1302 HAT to clone and build `sx1302_hal`. The result is `~/sx1302_hal/packet_forwarder/lora_pkt_fwd`.

## 3. Configure it

In `~/sx1302_hal/packet_forwarder/`, copy `global_conf.json.sx1250.EU868` to `global_conf.json` and set, inside `gateway_conf`:

```json
"gateway_ID": "YOUR_GATEWAY_EUI",
"server_address": "eu1.cloud.thethings.network",
"serv_port_up": 1700,
"serv_port_down": 1700
```

## 4. Register the gateway on TTN

TTN console (eu1) > Gateways > Register gateway. Use the same Gateway EUI as in `global_conf.json` and the frequency plan **Europe 863-870 MHz (SF9 for RX2 - recommended)**.

## 5. Install the reset workaround

The stock `reset_lgw.sh` did not work on this OS version (see [troubleshooting.md](troubleshooting.md#gateway)). Copy the three files from [`gateway/`](../gateway/) into `~/sx1302_hal/packet_forwarder/`, replacing the stock `reset_lgw.sh`:

```bash
sudo apt install python3-rpi.gpio
cp reset_gpio.py reset_lgw.sh start_gateway.sh ~/sx1302_hal/packet_forwarder/
chmod +x ~/sx1302_hal/packet_forwarder/*.sh
```

## 6. Start it

```bash
~/sx1302_hal/packet_forwarder/start_gateway.sh
```

Healthy output ends with the concentrator starting and then a stream of lines like `INFO: [down] PULL_ACK received in 45 ms`. Those are heartbeats between the gateway and TTN. The TTN console shows the gateway as **Connected**.

`WARNING: [main] impossible to open /dev/ttyS0 for GPS sync` is expected: there is no GPS module.

Leave this terminal open. Open a second terminal (or SSH session) for anything else, because stopping the forwarder stops all data.

## 7. Check that a node is heard

TTN console > Gateways > your gateway > **Live data**. Press RST on a node. A `Receive uplink message` or join-request line within a few seconds proves the radio path works, before any device registration is involved.
