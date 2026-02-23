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

void test_pm25_channel2_only(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_PM_25_CH2);
  reading.pm_25[1] = 135;

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 + 8 + 2 = 12
  TEST_ASSERT_EQUAL_INT32(12, size);

  // Mask: bit 9 => 0x0000000000000200
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x02, buffer[3]);

  // Only CH2 value
  TEST_ASSERT_EQUAL_UINT8(0x87, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[11]);
}

void test_pm25_sp_two_channel_order(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_PM_25_SP_CH1);
  reading.pm_25_sp[0] = 260;

  setFlag(&reading, FLAG_PM_25_SP_CH2);
  reading.pm_25_sp[1] = 270;

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 + 8 + 2 + 2 = 14
  TEST_ASSERT_EQUAL_INT32(14, size);

  // Mask bits 12 and 13 => 0x0000000000003000
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x30, buffer[3]);

  // Data order: bit 12 then bit 13
  TEST_ASSERT_EQUAL_UINT8(0x04, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[11]); // 260
  TEST_ASSERT_EQUAL_UINT8(0x0E, buffer[12]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[13]); // 270
}

void test_pm03_pc_channel1_and_channel2(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_PM_03_PC_CH1);
  reading.pm_03_pc[0] = 1000;

  setFlag(&reading, FLAG_PM_03_PC_CH2);
  reading.pm_03_pc[1] = 1001;

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 + 8 + 2 + 2 = 14
  TEST_ASSERT_EQUAL_INT32(14, size);

  // Mask bits 15 and 16 => 0x0000000000018000
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x80, buffer[3]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[4]);

  // Data: 1000 then 1001
  TEST_ASSERT_EQUAL_UINT8(0xE8, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x03, buffer[11]);
  TEST_ASSERT_EQUAL_UINT8(0xE9, buffer[12]);
  TEST_ASSERT_EQUAL_UINT8(0x03, buffer[13]);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pm25_channel2_only);
  RUN_TEST(test_pm25_sp_two_channel_order);
  RUN_TEST(test_pm03_pc_channel1_and_channel2);

  return UNITY_END();
}
