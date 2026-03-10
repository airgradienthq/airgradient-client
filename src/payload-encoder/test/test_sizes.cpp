#include <stdio.h>
#include "payload_encoder.h"

int main(void) {
  printf("=== Struct Sizes ===\n");
  printf("sizeof(SensorReading): %zu bytes\n", sizeof(SensorReading));
  printf("sizeof(PayloadHeader): %zu bytes\n", sizeof(PayloadHeader));
  printf("sizeof(EncoderContext): %zu bytes\n", sizeof(EncoderContext));
  printf("\n");

  PayloadEncoder encoder;
  PayloadHeader header = {5};
  encoder.init(header);

  SensorReading reading;
  initSensorReading(&reading);

  // Set all currently-defined flags (0..FLAG_SIGNAL)
  for (uint8_t bit = 0; bit <= (uint8_t)FLAG_SIGNAL; bit++) {
    setFlag(&reading, (SensorFlag)bit);
  }

  // Populate some values
  reading.temp = 2500;
  reading.hum = 5000;
  reading.co2 = 400;
  reading.tvoc = 100;
  reading.tvoc_raw = 200;
  reading.nox = 50;
  reading.nox_raw = 75;

  reading.pm_01 = 10;
  reading.pm_25[0] = 125;
  reading.pm_25[1] = 135;
  reading.pm_10 = 250;
  reading.pm_01_sp = 11;
  reading.pm_25_sp[0] = 260;
  reading.pm_25_sp[1] = 270;
  reading.pm_10_sp = 51;
  reading.pm_03_pc[0] = 1000;
  reading.pm_03_pc[1] = 1001;
  reading.pm_05_pc = 2000;
  reading.pm_01_pc = 3000;
  reading.pm_25_pc = 4000;
  reading.pm_5_pc = 5000;
  reading.pm_10_pc = 6000;

  reading.vbat = 3700;
  reading.vpanel = 5000;
  reading.o3_we = 0x12345678;
  reading.o3_ae = 0xAABBCCDD;
  reading.no2_we = 3000;
  reading.no2_ae = 4000;
  reading.afe_temp = 250;
  reading.signal = -75;

  encoder.addReading(reading);

  uint8_t buffer[512];
  int32_t size = encoder.encode(buffer, sizeof(buffer));
  printf("=== Encoded Payload Size (All Defined Flags Set) ===\n");
  printf("Bytes: %d\n", size);
  printf("Metadata: 0x%02X\n", buffer[0]);

  return 0;
}
