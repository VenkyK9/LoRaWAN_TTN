# Setup 3: TTN to InfluxDB to Grafana

Everything here runs on the same Raspberry Pi as the gateway.

## 1. Install the services

Install **InfluxDB 2.x**, **Telegraf** and **Grafana** from their official Debian packages, then check all three are running:

```bash
sudo systemctl status influxdb telegraf grafana-server
curl http://localhost:8086/health        # "status":"pass"
```

| Service | URL from a laptop on the same network |
|---|---|
| InfluxDB | `http://<pi-ip>:8086` |
| Grafana | `http://<pi-ip>:3000` |

`hostname -I` on the Pi prints its IP address.

## 2. InfluxDB: bucket and token

In the InfluxDB web UI, complete the first-run setup:

- Organisation: `Myorg`
- Bucket: `lorawan_data`

Then **Load Data > API Tokens > Generate API Token > Custom API Token** with read and write access to `lorawan_data`. Copy it immediately, it is shown once.

## 3. TTN: MQTT API key

TTN console > your application > Integrations > **MQTT** > Generate new API key. Note:

- Server: `eu1.cloud.thethings.network:1883`
- Username: `<application-id>@ttn`
- Password: the API key (starts with `NNSXS.`)

## 4. Telegraf

Put the two blocks from [`pipeline/telegraf.conf.example`](../pipeline/telegraf.conf.example) into `/etc/telegraf/telegraf.conf` and fill in the application ID, the TTN API key and the InfluxDB token.

```bash
sudo telegraf --config /etc/telegraf/telegraf.conf --test    # prints parsed metrics, no errors
sudo systemctl restart telegraf
sudo journalctl -u telegraf -f
```

A good log shows `[inputs.mqtt_consumer] Connected [tcp://eu1.cloud.thethings.network:1883]` and then stays quiet. Errors such as `401 Unauthorized` appear here.

To watch the raw MQTT messages without Telegraf:

```bash
sudo apt install mosquitto-clients
mosquitto_sub -h eu1.cloud.thethings.network -p 1883 \
  -u "<application-id>@ttn" -P "<api-key>" \
  -t "v3/<application-id>@ttn/devices/+/up" -v
```

## 5. Check data is arriving

InfluxDB UI > Data Explorer > Script editor:

```flux
from(bucket: "lorawan_data")
  |> range(start: -10m)
  |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
  |> limit(n: 20)
```

Rows with a `topic` like `v3/<application-id>@ttn/devices/node-1-9-2/up` mean the whole chain works.

## 6. Grafana

1. Connections > Data sources > Add > **InfluxDB**.
2. Query language: **Flux**. URL: `http://localhost:8086` (Grafana and InfluxDB are on the same machine).
3. Organisation `Myorg`, the token from step 2, default bucket `lorawan_data`. Save & test.
4. Build panels with the queries in [`grafana/queries.md`](../grafana/queries.md).

## After moving the Pi to another network

The Pi gets a new IP address. Only the addresses typed into a browser change. Telegraf and the Grafana data source both use `localhost`, so their configuration stays the same. Then:

1. Start the gateway again (`start_gateway.sh`).
2. `sudo systemctl status telegraf`. If it says `failed`, run `sudo systemctl reset-failed telegraf && sudo systemctl start telegraf`.
