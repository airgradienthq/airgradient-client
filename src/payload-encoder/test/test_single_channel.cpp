#include "payload_encoder.h"
#include "unity.h"
#include <string.h>

PayloadEncoder encoder;

void setUp(void) {
}

void tearDown(void) {
}

static PayloadHeader makeHeader(uint8_t interval_minutes) {
  PayloadHeader header = {interval_minutes};
  return header;
}

static void append_u16_le(uint8_t *buf, uint32_t *offset, uint16_t v) {
  buf[(*offset)++] = (uint8_t)(v & 0xFF);
  buf[(*offset)++] = (uint8_t)((v >> 8) & 0xFF);
}

static void append_u32_le(uint8_t *buf, uint32_t *offset, uint32_t v) {
  buf[(*offset)++] = (uint8_t)(v & 0xFF);
  buf[(*offset)++] = (uint8_t)((v >> 8) & 0xFF);
  buf[(*offset)++] = (uint8_t)((v >> 16) & 0xFF);
  buf[(*offset)++] = (uint8_t)((v >> 24) & 0xFF);
}

// Test: RFC example (Temp + CO2)
void test_encode_temp_and_co2(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_TEMP);
  reading.temp = 2500; // 25.00C

  setFlag(&reading, FLAG_CO2);
  reading.co2 = 400;

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 (header) + 8 (mask) + 2 (temp) + 2 (co2) = 14
  TEST_ASSERT_EQUAL_INT32(14, size);

  // Metadata: version=0, shared-mask bit set (single reading is treated as shared)
  TEST_ASSERT_EQUAL_UINT8(0x20, buffer[0]);
  TEST_ASSERT_EQUAL_UINT8(0x05, buffer[1]);

  // Mask: bits 0 and 2 => 0x0000000000000005 (little-endian)
  TEST_ASSERT_EQUAL_UINT8(0x05, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[6]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[7]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[8]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[9]);

  // Temp (2500 = 0x09C4)
  TEST_ASSERT_EQUAL_UINT8(0xC4, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x09, buffer[11]);

  // CO2 (400 = 0x0190)
  TEST_ASSERT_EQUAL_UINT8(0x90, buffer[12]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[13]);
}

void test_encode_humidity_only(void) {
  encoder.init(makeHeader(10));

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_HUM);
  reading.hum = 6550; // 65.50%

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 + 8 + 2 = 12
  TEST_ASSERT_EQUAL_INT32(12, size);

  // Mask: bit 1 => 0x0000000000000002
  TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);

  // Humidity (6550 = 0x1996)
  TEST_ASSERT_EQUAL_UINT8(0x96, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x19, buffer[11]);
}

void test_encode_pm25_two_channel(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_PM_25_CH1);
  reading.pm_25[0] = 125;

  setFlag(&reading, FLAG_PM_25_CH2);
  reading.pm_25[1] = 135;

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 + 8 + 2 + 2 = 14
  TEST_ASSERT_EQUAL_INT32(14, size);

  // Mask: bits 8 and 9 => 0x0000000000000300
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x03, buffer[3]);

  // Data order: CH1 (bit 8) then CH2 (bit 9)
  TEST_ASSERT_EQUAL_UINT8(0x7D, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[11]);
  TEST_ASSERT_EQUAL_UINT8(0x87, buffer[12]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[13]);
}

void test_encode_o3_we_32bit(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);

  setFlag(&reading, FLAG_O3_WE);
  reading.o3_we = 0x12345678;

  encoder.addReading(reading);

  uint8_t buffer[64];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // 2 + 8 + 4 = 14
  TEST_ASSERT_EQUAL_INT32(14, size);

  // Mask bit 24 => 0x0000000001000000 (low word)
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
  TEST_ASSERT_EQUAL_UINT8(0x01, buffer[5]);

  // O3_WE value little-endian
  TEST_ASSERT_EQUAL_UINT8(0x78, buffer[10]);
  TEST_ASSERT_EQUAL_UINT8(0x56, buffer[11]);
  TEST_ASSERT_EQUAL_UINT8(0x34, buffer[12]);
  TEST_ASSERT_EQUAL_UINT8(0x12, buffer[13]);
}

void test_encode_all_flags_order_matches_presence_mask(void) {
  encoder.init(makeHeader(5));

  SensorReading reading;
  initSensorReading(&reading);

  for (uint8_t bit = 0; bit <= (uint8_t)FLAG_SIGNAL; bit++) {
    setFlag(&reading, (SensorFlag)bit);
  }

  reading.temp = (int16_t)0x1122;
  reading.hum = 0x3344;
  reading.co2 = 0x5566;
  reading.tvoc = 0x7788;
  reading.tvoc_raw = 0x99AA;
  reading.nox = 0xBBCC;
  reading.nox_raw = 0xDDEE;

  reading.pm_01 = 0x0102;
  reading.pm_25[0] = 0x0304;
  reading.pm_25[1] = 0x0506;
  reading.pm_10 = 0x0708;
  reading.pm_01_sp = 0x090A;
  reading.pm_25_sp[0] = 0x0B0C;
  reading.pm_25_sp[1] = 0x0D0E;
  reading.pm_10_sp = 0x0F10;
  reading.pm_03_pc[0] = 0x1112;
  reading.pm_03_pc[1] = 0x1314;
  reading.pm_05_pc = 0x1516;
  reading.pm_01_pc = 0x1718;
  reading.pm_25_pc = 0x191A;
  reading.pm_5_pc = 0x1B1C;
  reading.pm_10_pc = 0x1D1E;

  reading.vbat = 0x1F20;
  reading.vpanel = 0x2122;
  reading.o3_we = 0xA1B2C3D4;
  reading.o3_ae = 0xB1C2D3E4;
  reading.no2_we = 0xC1D2E3F4;
  reading.no2_ae = 0xD1E2F304;
  reading.afe_temp = 0x2324;
  reading.signal = -5;

  encoder.addReading(reading);

  uint8_t buffer[256];
  int32_t size = encoder.encode(buffer, sizeof(buffer));

  // Shared mode: 2 (header) + 8 (mask) + 67 (data) = 77
  TEST_ASSERT_EQUAL_INT32(77, size);

  // Mask bits 0..29 set => lo=0x3FFFFFFF, hi=0
  TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[3]);
  TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[4]);
  TEST_ASSERT_EQUAL_UINT8(0x3F, buffer[5]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[6]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[7]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[8]);
  TEST_ASSERT_EQUAL_UINT8(0x00, buffer[9]);

  uint8_t expected[80];
  uint32_t off = 0;

  append_u16_le(expected, &off, (uint16_t)reading.temp);
  append_u16_le(expected, &off, reading.hum);
  append_u16_le(expected, &off, reading.co2);
  append_u16_le(expected, &off, reading.tvoc);
  append_u16_le(expected, &off, reading.tvoc_raw);
  append_u16_le(expected, &off, reading.nox);
  append_u16_le(expected, &off, reading.nox_raw);
  append_u16_le(expected, &off, reading.pm_01);
  append_u16_le(expected, &off, reading.pm_25[0]);
  append_u16_le(expected, &off, reading.pm_25[1]);
  append_u16_le(expected, &off, reading.pm_10);
  append_u16_le(expected, &off, reading.pm_01_sp);
  append_u16_le(expected, &off, reading.pm_25_sp[0]);
  append_u16_le(expected, &off, reading.pm_25_sp[1]);
  append_u16_le(expected, &off, reading.pm_10_sp);
  append_u16_le(expected, &off, reading.pm_03_pc[0]);
  append_u16_le(expected, &off, reading.pm_03_pc[1]);
  append_u16_le(expected, &off, reading.pm_05_pc);
  append_u16_le(expected, &off, reading.pm_01_pc);
  append_u16_le(expected, &off, reading.pm_25_pc);
  append_u16_le(expected, &off, reading.pm_5_pc);
  append_u16_le(expected, &off, reading.pm_10_pc);
  append_u16_le(expected, &off, reading.vbat);
  append_u16_le(expected, &off, reading.vpanel);
  append_u32_le(expected, &off, reading.o3_we);
  append_u32_le(expected, &off, reading.o3_ae);
  append_u32_le(expected, &off, reading.no2_we);
  append_u32_le(expected, &off, reading.no2_ae);
  append_u16_le(expected, &off, reading.afe_temp);
  expected[off++] = (uint8_t)reading.signal;

  TEST_ASSERT_EQUAL_UINT32(67, off);
  TEST_ASSERT_EQUAL_INT(0, memcmp(expected, &buffer[10], off));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_encode_temp_and_co2);
  RUN_TEST(test_encode_humidity_only);
  RUN_TEST(test_encode_pm25_two_channel);
  RUN_TEST(test_encode_o3_we_32bit);
  RUN_TEST(test_encode_all_flags_order_matches_presence_mask);

  return UNITY_END();
}
