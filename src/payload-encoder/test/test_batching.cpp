#include "unity.h"
#include "payload_encoder.h"

PayloadEncoder encoder;

void setUp(void) {
}

void tearDown(void) {
}

static PayloadHeader makeHeader(uint8_t interval_minutes) {
  PayloadHeader header = {interval_minutes};
  return header;
}

void test_batch_two_identical_masks_uses_shared_mask(void) {
  encoder.init(makeHeader(5));

  SensorReading r1;
  initSensorReading(&r1);
  setFlag(&r1, FLAG_CO2);
  r1.co2 = 400;

  SensorReading r2;
  initSensorReading(&r2);
  setFlag(&r2, FLAG_CO2);
  r2.co2 = 410;

  encoder.addReading(r1);
  encoder.addReading(r2);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // Shared mode: 2 + 8 + 2 + 2 = 14
  TEST_ASSERT_EQUAL_INT32(14, size);

  // Metadata shared bit set
  TEST_ASSERT_EQUAL_UINT8(0x20, buffer[0]);

  // Shared mask bit 2 => 0x0000000000000004
  TEST_ASSERT_EQUAL_UINT8(0x04, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);

  // Reading 1 CO2 at offset 10
  TEST_ASSERT_EQUAL_UINT8(0x90, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[11]);

  // Reading 2 CO2 immediately after
  TEST_ASSERT_EQUAL_UINT8(0x9A, buffer[12]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[13]);
}

void test_batch_two_different_masks_uses_per_reading_masks(void) {
  encoder.init(makeHeader(5));

  SensorReading r1;
  initSensorReading(&r1);
  setFlag(&r1, FLAG_TEMP);
  r1.temp = 2500;

  SensorReading r2;
  initSensorReading(&r2);
  setFlag(&r2, FLAG_CO2);
  r2.co2 = 400;

  encoder.addReading(r1);
  encoder.addReading(r2);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // Per-reading mode: 2 + (8+2) + (8+2) = 22
  TEST_ASSERT_EQUAL_INT32(22, size);

  // Metadata shared bit clear
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[0]);

  // Reading 1 mask bit 0 => 0x01
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[2]);

  // Reading 1 temp at offset 10
  TEST_ASSERT_EQUAL_UINT8(0xC4, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x09, buffer[11]);

  // Reading 2 mask starts at offset 12
  TEST_ASSERT_EQUAL_UINT8(0x04, buffer[12]);

  // Reading 2 CO2 at offset 20
  TEST_ASSERT_EQUAL_UINT8(0x90, buffer[20]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[21]);
}

void test_batch_max_readings_shared_mask(void) {
  encoder.init(makeHeader(5));

  for (int i = 0; i < MAX_BATCH_SIZE; i++) {
    SensorReading r;
    initSensorReading(&r);
    setFlag(&r, FLAG_CO2);
    r.co2 = (uint16_t)(400 + i);
    encoder.addReading(r);
  }

  uint8_t buffer[128];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 + 8 + 20*2 = 50
  TEST_ASSERT_EQUAL_INT32(50, size);
  TEST_ASSERT_EQUAL_UINT8(0x20, buffer[0]);
}

void test_batch_reset(void) {
  encoder.init(makeHeader(5));

  SensorReading r;
  initSensorReading(&r);
  setFlag(&r, FLAG_CO2);
  r.co2 = 400;

  encoder.addReading(r);
  encoder.addReading(r);
  TEST_ASSERT_EQUAL_UINT8(2, encoder.getReadingCount());

  encoder.reset();
  TEST_ASSERT_EQUAL_UINT8(0, encoder.getReadingCount());

  uint8_t buffer[64];
  TEST_ASSERT_EQUAL_INT32(0, encoder.encode(buffer, sizeof(buffer)));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_batch_two_identical_masks_uses_shared_mask);
  RUN_TEST(test_batch_two_different_masks_uses_per_reading_masks);
  RUN_TEST(test_batch_max_readings_shared_mask);
  RUN_TEST(test_batch_reset);

  return UNITY_END();
}
