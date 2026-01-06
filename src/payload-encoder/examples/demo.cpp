#include "payload_encoder.h"
#include "payload_types.h"
#include <stdio.h>
#include <string.h>

/**
 * Print buffer contents in hex format
 */
void printHex(const char *label, const uint8_t *buffer, int32_t size) {
  printf("%s (%d bytes):\n", label, size);
  printf("  ");
  for (int32_t i = 0; i < size; i++) {
    printf("%02X ", buffer[i]);
    if ((i + 1) % 16 == 0 && i < size - 1) {
      printf("\n  ");
    }
  }
  printf("\n\n");
}

/**
 * Example 1: Single reading with temperature and CO2
 */
void example1_single_reading() {
  printf("=== Example 1: Single Reading (Temp + CO2) ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, false, false,
                          5}; // Version 1, single mode, no dedicated temp/hum, 5 min
  encoder.init(header);

  // Create a sensor reading
  SensorReading reading;
  initSensorReading(&reading);

  // Set temperature = 25.50°C (2550 after scaling by 100)
  setFlag(&reading, FLAG_TEMP);
  reading.temp[0] = 2550;

  // Set CO2 = 412 ppm
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 412;

  encoder.addReading(reading);

  // Encode to buffer
  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printHex("Encoded Payload", buffer, size);
  printf("Expected size: 10 bytes (2 header + 4 mask + 2 temp + 2 co2)\n\n");
}

/**
 * Example 2: Dual channel mode with expandable fields
 */
void example2_dual_channel() {
  printf("=== Example 2: Dual Channel Mode ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, true, false, 5}; // Version 1, DUAL mode, no dedicated temp/hum, 5 min
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  // Temperature from 2 sensors
  setFlag(&reading, FLAG_TEMP);
  reading.temp[0] = 2500; // 25.00°C
  reading.temp[1] = 2650; // 26.50°C

  // Humidity from 2 sensors
  setFlag(&reading, FLAG_HUM);
  reading.hum[0] = 6000; // 60.00%
  reading.hum[1] = 6250; // 62.50%

  // CO2 (scalar - only one value even in dual mode)
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 425;

  encoder.addReading(reading);

  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printHex("Encoded Payload (Dual Mode)", buffer, size);
  printf("Note: Temperature and humidity send 2 values each\n");
  printf("      CO2 is scalar, so only 1 value\n\n");
}

/**
 * Example 3: Multiple readings (batch)
 */
void example3_batch_readings() {
  printf("=== Example 3: Batch of 3 Readings ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, false, false,
                          10}; // Version 1, single mode, no dedicated temp/hum, 10 min interval
  encoder.init(header);

  // Reading 1
  SensorReading reading1;
  initSensorReading(&reading1);
  setFlag(&reading1, FLAG_CO2);
  reading1.co2 = 400;
  encoder.addReading(reading1);

  // Reading 2
  SensorReading reading2;
  initSensorReading(&reading2);
  setFlag(&reading2, FLAG_CO2);
  reading2.co2 = 410;
  encoder.addReading(reading2);

  // Reading 3
  SensorReading reading3;
  initSensorReading(&reading3);
  setFlag(&reading3, FLAG_CO2);
  reading3.co2 = 420;
  encoder.addReading(reading3);

  printf("Added %d readings to batch\n", encoder.getReadingCount());

  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printHex("Encoded Batch Payload", buffer, size);
  printf("Size breakdown:\n");
  printf("  Header: 2 bytes\n");
  printf("  Each reading: 6 bytes (4 mask + 2 co2)\n");
  printf("  Total: 2 + 3*6 = 20 bytes\n\n");
}

/**
 * Example 4: PM sensors
 */
void example4_pm_sensors() {
  printf("=== Example 4: PM Sensors ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, false, false, 5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  // PM2.5 = 12.5 µg/m³ (125 after scaling by 10)
  setFlag(&reading, FLAG_PM_25);
  reading.pm_25[0] = 125;

  // PM10 = 25.0 µg/m³ (250 after scaling by 10)
  setFlag(&reading, FLAG_PM_10);
  reading.pm_10[0] = 250;

  encoder.addReading(reading);

  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printHex("Encoded PM Sensor Payload", buffer, size);
}

/**
 * Example 5: All sensor types
 */
void example5_all_sensors() {
  printf("=== Example 5: Multiple Sensor Types ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, false, false, 5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  // Temperature
  setFlag(&reading, FLAG_TEMP);
  reading.temp[0] = 2350; // 23.50°C

  // Humidity
  setFlag(&reading, FLAG_HUM);
  reading.hum[0] = 6500; // 65.00%

  // CO2
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 450;

  // TVOC
  setFlag(&reading, FLAG_TVOC);
  reading.tvoc = 120;

  // PM2.5
  setFlag(&reading, FLAG_PM_25);
  reading.pm_25[0] = 135; // 13.5 µg/m³

  // Battery voltage = 3700 mV (scaled by 100)
  setFlag(&reading, FLAG_VBAT);
  reading.vbat = 3700;

  encoder.addReading(reading);

  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printHex("Encoded Multi-Sensor Payload", buffer, size);

  printf("Sensors included:\n");
  printf("  - Temperature: 23.50°C\n");
  printf("  - Humidity: 65.00%%\n");
  printf("  - CO2: 450 ppm\n");
  printf("  - TVOC: 120\n");
  printf("  - PM2.5: 13.5 µg/m³\n");
  printf("  - Battery: 3700 mV\n\n");
}

/**
 * Example 6: Negative temperature
 */
void example6_negative_temp() {
  printf("=== Example 6: Negative Temperature ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, false, false, 5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  // Temperature = -15.25°C (-1525 after scaling)
  setFlag(&reading, FLAG_TEMP);
  reading.temp[0] = -1525;

  encoder.addReading(reading);

  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printHex("Encoded Negative Temperature", buffer, size);
  printf("Temperature: -15.25°C\n\n");
}

/**
 * Example 7: Check buffer size before encoding
 */
void example7_size_calculation() {
  printf("=== Example 7: Size Calculation ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, false, false, 5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_TEMP);
  setFlag(&reading, FLAG_HUM);
  setFlag(&reading, FLAG_CO2);
  reading.temp[0] = 2500;
  reading.hum[0] = 6000;
  reading.co2 = 400;

  encoder.addReading(reading);

  // Calculate size before encoding
  uint32_t needed_size = encoder.calculateTotalSize();
  printf("Calculated size needed: %u bytes\n", needed_size);

  // Allocate appropriate buffer
  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printf("Actual encoded size: %d bytes\n", size);
  printf("Match: %s\n\n", (size == (int32_t)needed_size) ? "YES" : "NO");
}

/**
 * Example 8: Error handling
 */
void example8_error_handling() {
  printf("=== Example 8: Error Handling ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, false, false, 5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;
  encoder.addReading(reading);

  // Try with buffer too small
  uint8_t small_buffer[5];
  int32_t size = encoder.encode(small_buffer, sizeof(small_buffer));

  if (size == -1) {
    printf("Error: Buffer too small (expected)\n");
  }

  // Try with proper buffer
  uint8_t buffer[256];
  size = encoder.encode(buffer, sizeof(buffer));

  if (size > 0) {
    printf("Success: Encoded %d bytes\n", size);
  }

  printf("\n");
}

void example9_dedicated_temphum() {
  printf("=== Example 9: Dedicated temperature humidity===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {1, true, true, 5}; // Version 1, DUAL mode, dedicated temp/hum, 5 min
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  // Temperature
  setFlag(&reading, FLAG_TEMP);
  reading.temp[0] = 2500; // 25.00°C

  // Humidity
  setFlag(&reading, FLAG_HUM);
  reading.hum[0] = 6000; // 60.00%

  // CO2 (scalar - only one value even in dual mode)
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 425;

  // PM2.5 AE (dual)
  setFlag(&reading, FLAG_PM_25);
  reading.pm_25[0] = 112;
  reading.pm_25[1] = 193;

  encoder.addReading(reading);

  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  printHex("Encoded Payload (Dual Mode & dedicated temp hum)", buffer, size);
  printf("Note: Temperature and humidity only send 1 value\n");
  printf("      CO2 is scalar, so only 1 value\n");
  printf("      PM25 is dual since dual mode set \n\n");
}

int main() {
  printf("╔═══════════════════════════════════════════════╗\n");
  printf("║  AirGradient Cellular Payload Encoder        ║\n");
  printf("║  Examples & Usage Demonstration              ║\n");
  printf("╚═══════════════════════════════════════════════╝\n\n");

  example1_single_reading();
  example2_dual_channel();
  example3_batch_readings();
  example4_pm_sensors();
  example5_all_sensors();
  example6_negative_temp();
  example7_size_calculation();
  example8_error_handling();
  example9_dedicated_temphum();

  printf("╔═══════════════════════════════════════════════╗\n");
  printf("║  All examples completed successfully!        ║\n");
  printf("╚═══════════════════════════════════════════════╝\n");

  return 0;
}
