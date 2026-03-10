#include "unity.h"
#include "payload_encoder.h"

PayloadEncoder encoder;

void setUp(void) {
  // Run before each test
}

void tearDown(void) {
  // Run after each test
}

static PayloadHeader makeHeader(uint8_t interval_minutes) {
  PayloadHeader header = {interval_minutes};
  return header;
}

void test_encoder_init(void) {
  encoder.init(makeHeader(5));
  TEST_ASSERT_EQUAL_UINT8(0, encoder.getReadingCount());
}

void test_encoder_reset(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;

  encoder.addReading(reading);
  TEST_ASSERT_EQUAL_UINT8(1, encoder.getReadingCount());

  encoder.reset();
  TEST_ASSERT_EQUAL_UINT8(0, encoder.getReadingCount());
}

void test_add_multiple_readings(void) {
  encoder.init(makeHeader(5));

  for (int i = 0; i < 5; i++) {
    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = (uint16_t)(400 + i);
    TEST_ASSERT_TRUE(encoder.addReading(reading));
  }

  TEST_ASSERT_EQUAL_UINT8(5, encoder.getReadingCount());
}

void test_batch_full(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;

  for (int i = 0; i < MAX_BATCH_SIZE; i++) {
    TEST_ASSERT_TRUE(encoder.addReading(reading));
  }

  TEST_ASSERT_FALSE(encoder.addReading(reading));
  TEST_ASSERT_EQUAL_UINT8(MAX_BATCH_SIZE, encoder.getReadingCount());
}

void test_encode_empty(void) {
  encoder.init(makeHeader(5));

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_INT32(0, size);
}

void test_encode_null_buffer(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;
  encoder.addReading(reading);

  TEST_ASSERT_EQUAL_INT32(-1, encoder.encode(nullptr, 64));
}

void test_encode_buffer_too_small(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;
  encoder.addReading(reading);

  // Minimal payload for one reading with CO2:
  // 2 (header) + 8 (mask) + 2 (co2) = 12 bytes
  uint8_t buffer[11];
  TEST_ASSERT_EQUAL_INT32(-1, encoder.encode(buffer, sizeof(buffer)));
}

void test_metadata_version_constant(void) {
  encoder.init(makeHeader(5));
  TEST_ASSERT_EQUAL_UINT8(0x00, encoder.encodeMetadata());
}

void test_metadata_shared_mask_bit_set_when_masks_equal(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;

  encoder.addReading(reading);
  encoder.addReading(reading);

  // Version = 0, shared-mask bit (bit 5) = 1
  TEST_ASSERT_EQUAL_UINT8(0x20, encoder.encodeMetadata());
}

void test_metadata_shared_mask_bit_clear_when_masks_differ(void) {
  encoder.init(makeHeader(5));

  SensorReading reading1;
  initSensorReading(&reading1);
  setFlag(&reading1, FLAG_CO2);
  reading1.co2 = 400;

  SensorReading reading2;
  initSensorReading(&reading2);
  setFlag(&reading2, FLAG_TEMP);
  reading2.temp = 2500;

  encoder.addReading(reading1);
  encoder.addReading(reading2);

  TEST_ASSERT_EQUAL_UINT8(0x00, encoder.encodeMetadata());
}

void test_calculate_reading_size_co2_only(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);

  // 8 (mask) + 2 (co2)
  TEST_ASSERT_EQUAL_UINT32(10, encoder.calculateReadingSize(reading));
}

void test_calculate_total_size_shared_mask_two_readings(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);
  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;

  encoder.addReading(reading);
  encoder.addReading(reading);

  // Shared mode:
  // 2 (header) + 8 (shared mask) + 2*2 (two readings of CO2)
  TEST_ASSERT_EQUAL_UINT32(14, encoder.calculateTotalSize());
}

void test_calculate_total_size_per_reading_mask_two_readings(void) {
  encoder.init(makeHeader(5));

  SensorReading reading1;
  initSensorReading(&reading1);
  setFlag(&reading1, FLAG_CO2);
  reading1.co2 = 400;

  SensorReading reading2;
  initSensorReading(&reading2);
  setFlag(&reading2, FLAG_TEMP);
  reading2.temp = 2500;

  encoder.addReading(reading1);
  encoder.addReading(reading2);

  // Per-reading mode:
  // 2 + (8+2) + (8+2)
  TEST_ASSERT_EQUAL_UINT32(22, encoder.calculateTotalSize());
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_encoder_init);
  RUN_TEST(test_encoder_reset);
  RUN_TEST(test_add_multiple_readings);
  RUN_TEST(test_batch_full);
  RUN_TEST(test_encode_empty);
  RUN_TEST(test_encode_null_buffer);
  RUN_TEST(test_encode_buffer_too_small);
  RUN_TEST(test_metadata_version_constant);
  RUN_TEST(test_metadata_shared_mask_bit_set_when_masks_equal);
  RUN_TEST(test_metadata_shared_mask_bit_clear_when_masks_differ);
  RUN_TEST(test_calculate_reading_size_co2_only);
  RUN_TEST(test_calculate_total_size_shared_mask_two_readings);
  RUN_TEST(test_calculate_total_size_per_reading_mask_two_readings);

  return UNITY_END();
}
