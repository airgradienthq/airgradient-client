#include "payload_encoder.h"
#include <stdio.h>
#include <string.h>

static void printHex(const char *label, const uint8_t *buffer, int32_t size) {
  printf("%s (%d bytes):\n", label, (int)size);
  printf("  ");
  for (int32_t i = 0; i < size; i++) {
    printf("%02X ", buffer[i]);
    if ((i + 1) % 16 == 0 && i < size - 1) {
      printf("\n  ");
    }
  }
  printf("\n\n");
}

static void printHeaderSummary(const uint8_t *buffer, int32_t size) {
  if (size < 2) {
    return;
  }

  const uint8_t metadata = buffer[0];
  const uint8_t interval = buffer[1];
  const uint8_t version = (uint8_t)(metadata & 0x1F);
  const uint8_t shared_mask = (uint8_t)((metadata >> AG_METADATA_SHARED_PRESENCE_MASK_BIT) & 1U);

  printf("Header: metadata=0x%02X (ver=%u, shared_mask=%u), interval=%u\n", metadata, version,
         shared_mask, interval);

  if (shared_mask && size >= 10) {
    printf("Shared mask bytes (LE): ");
    for (int i = 0; i < 8; i++) {
      printf("%02X ", buffer[2 + i]);
    }
    printf("\n");
  }
  printf("\n");
}

static void example_single_temp_co2(void) {
  printf("=== Example: Single Reading (Temp + CO2) ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_TEMP);
  reading.temp = 2550; // 25.50C (scaled by 100)

  setFlag(&reading, FLAG_CO2);
  reading.co2 = 412;

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));
  printHex("Encoded Payload", buffer, size);
  printHeaderSummary(buffer, size);
}

static void example_batch_shared_mask(void) {
  printf("=== Example: Batch (3 CO2 readings; shared mask) ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {10};
  encoder.init(header);

  for (int i = 0; i < 3; i++) {
    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = (uint16_t)(400 + i * 10);
    encoder.addReading(reading);
  }

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));
  printHex("Encoded Batch Payload", buffer, size);
  printHeaderSummary(buffer, size);
}

static void example_batch_per_reading_masks(void) {
  printf("=== Example: Batch (2 readings; per-reading masks) ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {5};
  encoder.init(header);

  // Reading 1: temp
  SensorReading r1;
  initSensorReading(&r1);
  setFlag(&r1, FLAG_TEMP);
  r1.temp = 2500;
  encoder.addReading(r1);

  // Reading 2: co2
  SensorReading r2;
  initSensorReading(&r2);
  setFlag(&r2, FLAG_CO2);
  r2.co2 = 400;
  encoder.addReading(r2);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));
  printHex("Encoded Batch Payload", buffer, size);
  printHeaderSummary(buffer, size);
}

static void example_pm25_two_channel(void) {
  printf("=== Example: PM2.5 Two-Channel (CH1 + CH2) ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_PM_25_CH1);
  reading.pm_25[0] = 125; // 12.5 ug/m3 (scaled by 10)

  setFlag(&reading, FLAG_PM_25_CH2);
  reading.pm_25[1] = 135; // 13.5 ug/m3

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));
  printHex("Encoded PM2.5 Payload", buffer, size);
  printHeaderSummary(buffer, size);
}

static void example_invalid_zero_mask(void) {
  printf("=== Example: Invalid (zero presence mask) ===\n");

  PayloadEncoder encoder;
  PayloadHeader header = {5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);
  // Intentionally DO NOT set any flags
  reading.co2 = 400; // ignored
  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));
  printf("encode() returned: %d (expected -1)\n\n", (int)size);
}

static void printUsage(const char *argv0) {
  printf("Usage: %s [example]\n", argv0);
  printf("Examples:\n");
  printf("  all         Run all examples (default)\n");
  printf("  single      Single reading: Temp + CO2\n");
  printf("  shared      Batch: 3 CO2 readings (shared mask)\n");
  printf("  per-reading Batch: 2 readings with different masks\n");
  printf("  pm25        Two-channel PM2.5 (CH1 + CH2)\n");
  printf("  invalid     Zero-mask payload (encoder returns -1)\n");
}

int main(int argc, char **argv) {
  const char *which = (argc >= 2) ? argv[1] : "all";

  if (strcmp(which, "all") == 0) {
    example_single_temp_co2();
    example_batch_shared_mask();
    example_batch_per_reading_masks();
    example_pm25_two_channel();
    example_invalid_zero_mask();
    return 0;
  }

  if (strcmp(which, "single") == 0) {
    example_single_temp_co2();
    return 0;
  }

  if (strcmp(which, "shared") == 0) {
    example_batch_shared_mask();
    return 0;
  }

  if (strcmp(which, "per-reading") == 0) {
    example_batch_per_reading_masks();
    return 0;
  }

  if (strcmp(which, "pm25") == 0) {
    example_pm25_two_channel();
    return 0;
  }

  if (strcmp(which, "invalid") == 0) {
    example_invalid_zero_mask();
    return 0;
  }

  printUsage(argv[0]);
  return 2;
}
