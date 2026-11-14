// TTN uplink payload formatter (Applications > Payload formatters > Uplink,
// formatter type "Custom Javascript formatter").
//
// One formatter serves the whole application, so it tells the two nodes
// apart by payload length:
//
//   8 bytes  node 1  BME280 + LDR
//            0-1 temperature  int16,  degrees C x 100
//            2-3 humidity     uint16, percent x 100
//            4-5 pressure     uint16, hPa
//            6-7 light        uint16, raw ADC 0-4095
//
//   3 bytes  node 2  soil moisture
//            0-1 soil ADC     uint16, raw 0-4095 (high = dry)
//            2   threshold    1 = sensor DO pin high, 0 = low
//
// The decoded names become InfluxDB fields with the prefix
// "uplink_message_decoded_payload_", e.g.
// uplink_message_decoded_payload_temperature.

function decodeUplink(input) {
  var bytes = input.bytes;
  var decoded = {};

  if (bytes.length === 8) {
    var temp = (bytes[0] << 8) | bytes[1];
    if (temp > 32767) temp -= 65536;          // int16: allow below zero
    decoded.temperature = temp / 100.0;

    decoded.humidity = ((bytes[2] << 8) | bytes[3]) / 100.0;
    decoded.pressure = (bytes[4] << 8) | bytes[5];
    decoded.light = (bytes[6] << 8) | bytes[7];
  } else if (bytes.length === 3) {
    var soilADC = (bytes[0] << 8) | bytes[1];
    decoded.soil_moisture_adc = soilADC;

    // Uncalibrated: 4095 (dry) -> 0 %, 0 (in water) -> 100 %
    decoded.soil_moisture_percent = Math.round(((4095 - soilADC) / 4095) * 100);

    // Sent as a number (1/0), not true/false: Telegraf's JSON parser drops
    // booleans, so a boolean here never reaches InfluxDB.
    decoded.soil_wet = bytes[2] === 1 ? 1 : 0;
  } else {
    return {
      data: {},
      warnings: ["Unexpected payload length: " + bytes.length],
      errors: []
    };
  }

  return {
    data: decoded,
    warnings: [],
    errors: []
  };
}
