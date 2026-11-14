# Grafana queries (Flux)

Data source: InfluxDB, query language **Flux**, URL `http://localhost:8086`, bucket `lorawan_data`.

Everything Telegraf writes lands in one measurement, `mqtt_consumer`. The node is identified by the `topic` tag (it contains the TTN device ID) and each value is a separate field.

| Node | TTN device ID | Fields (prefix `uplink_message_decoded_payload_`) |
|---|---|---|
| 1 | `node-1-9-2` | `temperature`, `humidity`, `pressure`, `light` |
| 2 | `heltec-2-17-3` | `soil_moisture_adc`, `soil_moisture_percent` |

Radio quality for either node: `uplink_message_rx_metadata_0_rssi`, `uplink_message_rx_metadata_0_snr`.

## Panel template

Change the device ID and the field name; the rest stays the same.

```flux
from(bucket: "lorawan_data")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
  |> filter(fn: (r) => r["topic"] =~ /node-1-9-2/)
  |> filter(fn: (r) => r["_field"] == "uplink_message_decoded_payload_temperature")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
```

## Dashboards

**BME280 node (`node-1-9-2`)**

| Panel | Field | Visualisation | Unit |
|---|---|---|---|
| Temperature | `..._temperature` | Time series | °C |
| Humidity | `..._humidity` | Time series | Percent (0-100) |
| Pressure | `..._pressure` | Time series | hPa |
| Light | `..._light` | Time series | raw ADC, 0-4095 |

**Soil moisture node (`heltec-2-17-3`)**

| Panel | Field | Visualisation | Unit |
|---|---|---|---|
| Soil moisture % | `..._soil_moisture_percent` | Time series | Percent (0-100) |
| Soil moisture ADC | `..._soil_moisture_adc` | Time series | raw ADC, 0-4095 |
| Current moisture | `..._soil_moisture_percent` with `last()` | Gauge | Percent (0-100) |

Gauge query:

```flux
from(bucket: "lorawan_data")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
  |> filter(fn: (r) => r["topic"] =~ /heltec-2-17-3/)
  |> filter(fn: (r) => r["_field"] == "uplink_message_decoded_payload_soil_moisture_percent")
  |> last()
```

## Debugging queries

Is anything arriving at all?

```flux
from(bucket: "lorawan_data")
  |> range(start: -10m)
  |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
  |> limit(n: 20)
```

Which fields exist for a node? Run this before writing a panel query: a panel with a misspelt field name shows "No data" with no error.

```flux
from(bucket: "lorawan_data")
  |> range(start: -10m)
  |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
  |> filter(fn: (r) => r["topic"] =~ /heltec-2-17-3/)
  |> group()
  |> distinct(column: "_field")
```
