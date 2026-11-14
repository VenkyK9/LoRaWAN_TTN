/**
 * I2C scanner for the Heltec WiFi LoRa 32 V3.
 *
 * Tries every I2C address and prints the ones that answer. Run this first
 * when a sensor "is not found": it separates wiring problems (nothing
 * answers) from address problems (it answers at 0x77, not 0x76).
 *
 * A BME280 shows up at 0x76 or 0x77.
 */

#include <Wire.h>

#define SDA_PIN 3
#define SCL_PIN 1

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.printf("I2C scanner: SDA=GPIO%d, SCL=GPIO%d\n", SDA_PIN, SCL_PIN);
}

void loop()
{
  int found = 0;

  Serial.println("Scanning...");
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {     // 0 = a device acknowledged
      Serial.printf("I2C device found at address 0x%02X\n", address);
      found++;
    }
  }

  if (found == 0) {
    Serial.println("No devices found. Check VCC (3V3), GND, SDA, SCL and solder joints.");
  }

  delay(5000);
}
