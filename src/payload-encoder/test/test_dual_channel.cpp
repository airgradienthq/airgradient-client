#include "unity.h"
#include "payload_encoder.h"
#include <string.h>

PayloadEncoder encoder;

void setUp(void) {
    // This is run before each test
}

void tearDown(void) {
    // This is run after each test
}

// Test: RFC Example - Dual Channel (Temp + CO2)
// Metadata: 0x09 (Ver=1, Dual=1)
// Interval: 5 minutes
// Mask: 0x00000005 (Bits 0 & 2 set: Temp + CO2)
// Expected: Temp sends 2 values, CO2 sends 1 value (scalar)
void test_rfc_example_dual_channel(void) {
    PayloadHeader header = {1, true, false, 5};  // dual_mode = true
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Set Temp[0] = 25.00°C -> 2500
    // Set Temp[1] = 26.00°C -> 2600
    setFlag(&reading, FLAG_TEMP);
    reading.temp[0] = 2500;
    reading.temp[1] = 2600;

    // Set CO2 = 400 ppm (scalar - only one value)
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 4 (temp: 2*2) + 2 (co2) = 12 bytes
    TEST_ASSERT_EQUAL_INT32(12, size);

    // Verify header
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[0]);  // Metadata (Ver=1, Dual=1)
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[1]);  // Interval

    // Verify presence mask (0x00000005)
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);

    // Verify temperature[0] (2500 = 0x09C4)
    TEST_ASSERT_EQUAL_UINT8(0xC4, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[7]);

    // Verify temperature[1] (2600 = 0x0A28)
    TEST_ASSERT_EQUAL_UINT8(0x28, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x0A, buffer[9]);

    // Verify CO2 (400 = 0x0190)
    TEST_ASSERT_EQUAL_UINT8(0x90, buffer[10]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[11]);
}

// Test: Dual channel with humidity
void test_dual_channel_humidity(void) {
    PayloadHeader header = {1, true, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    setFlag(&reading, FLAG_HUM);
    reading.hum[0] = 6000;  // 60.00%
    reading.hum[1] = 6550;  // 65.50%

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 4 (hum: 2*2) = 10 bytes
    TEST_ASSERT_EQUAL_INT32(10, size);

    // Verify humidity[0] (6000 = 0x1770)
    TEST_ASSERT_EQUAL_UINT8(0x70, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x17, buffer[7]);

    // Verify humidity[1] (6550 = 0x1996)
    TEST_ASSERT_EQUAL_UINT8(0x96, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x19, buffer[9]);
}

// Test: Dual channel PM sensors
void test_dual_channel_pm_sensors(void) {
    PayloadHeader header = {1, true, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // PM2.5 expandable - send 2 values
    setFlag(&reading, FLAG_PM_25);
    reading.pm_25[0] = 125;  // 12.5 µg/m³
    reading.pm_25[1] = 135;  // 13.5 µg/m³

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 4 (pm25: 2*2) = 10 bytes
    TEST_ASSERT_EQUAL_INT32(10, size);

    // Verify PM2.5[0] (125 = 0x007D)
    TEST_ASSERT_EQUAL_UINT8(0x7D, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[7]);

    // Verify PM2.5[1] (135 = 0x0087)
    TEST_ASSERT_EQUAL_UINT8(0x87, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[9]);
}

// Test: Dual channel mixed expandable and scalar
void test_dual_channel_mixed(void) {
    PayloadHeader header = {1, true, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Expandable field
    setFlag(&reading, FLAG_TEMP);
    reading.temp[0] = 2500;
    reading.temp[1] = 2600;

    // Scalar field
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 450;

    // Another expandable
    setFlag(&reading, FLAG_HUM);
    reading.hum[0] = 6000;
    reading.hum[1] = 6100;

    // Another scalar
    setFlag(&reading, FLAG_TVOC);
    reading.tvoc = 100;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 4 (mask) + 4 (temp) + 4 (hum) + 2 (co2) + 2 (tvoc) = 18
    TEST_ASSERT_EQUAL_INT32(18, size);

    // Verify presence mask (bits 0, 1, 2, 3 = 0x0000000F)
    TEST_ASSERT_EQUAL_UINT8(0x0F, buffer[2]);

    // Fields should be in order: temp[0], temp[1], hum[0], hum[1], co2, tvoc
}

// Test: Scalar fields stay single value in dual mode
void test_dual_channel_scalars_single_value(void) {
    PayloadHeader header = {1, true, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // All scalar fields
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    setFlag(&reading, FLAG_TVOC);
    reading.tvoc = 100;

    setFlag(&reading, FLAG_VBAT);
    reading.vbat = 3700;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 4 (mask) + 2 (co2) + 2 (tvoc) + 2 (vbat) = 12
    TEST_ASSERT_EQUAL_INT32(12, size);

    // Presence mask: bits 2, 3, 19 = 0x0008000C
    TEST_ASSERT_EQUAL_UINT8(0x0C, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x08, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);
}

// Test: 32-bit fields remain scalar in dual mode
void test_dual_channel_32bit_scalar(void) {
    PayloadHeader header = {1, true, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // O3 WE (32-bit, scalar)
    setFlag(&reading, FLAG_O3_WE);
    reading.o3_we = 0xAABBCCDD;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 4 (mask) + 4 (o3_we) = 10
    TEST_ASSERT_EQUAL_INT32(10, size);

    // Verify O3_WE (little-endian)
    TEST_ASSERT_EQUAL_UINT8(0xDD, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, buffer[7]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, buffer[9]);
}

// Test: All expandable fields in dual mode
void test_dual_channel_all_expandable(void) {
    PayloadHeader header = {1, true, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Set all expandable fields
    setFlag(&reading, FLAG_TEMP);
    reading.temp[0] = 2500;
    reading.temp[1] = 2600;

    setFlag(&reading, FLAG_HUM);
    reading.hum[0] = 6000;
    reading.hum[1] = 6100;

    setFlag(&reading, FLAG_PM_01);
    reading.pm_01[0] = 10;
    reading.pm_01[1] = 11;

    setFlag(&reading, FLAG_PM_25);
    reading.pm_25[0] = 25;
    reading.pm_25[1] = 26;

    setFlag(&reading, FLAG_PM_10);
    reading.pm_10[0] = 50;
    reading.pm_10[1] = 51;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 + 4 + 5*(2*2) = 2 + 4 + 20 = 26
    TEST_ASSERT_EQUAL_INT32(26, size);

    // Verify metadata shows dual mode
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[0]);
}

// Test: Size calculation in dual mode
void test_dual_channel_size_calculation(void) {
    PayloadHeader header = {1, true, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    setFlag(&reading, FLAG_TEMP);   // Expandable: 4 bytes
    setFlag(&reading, FLAG_CO2);    // Scalar: 2 bytes
    setFlag(&reading, FLAG_HUM);    // Expandable: 4 bytes

    reading.temp[0] = 2500;
    reading.temp[1] = 2600;
    reading.hum[0] = 6000;
    reading.hum[1] = 6100;
    reading.co2 = 400;

    // Expected size for reading: 4 (mask) + 4 (temp) + 4 (hum) + 2 (co2) = 14
    uint32_t reading_size = encoder.calculateReadingSize(reading);
    TEST_ASSERT_EQUAL_UINT32(14, reading_size);

    encoder.addReading(reading);

    // Total size: 2 (header) + 14 (reading) = 16
    uint32_t total_size = encoder.calculateTotalSize();
    TEST_ASSERT_EQUAL_UINT32(16, total_size);
}

// Test: DEDICATED_TEMPHUM_SENSOR flag (temp/hum single value, PM dual)
void test_dedicated_temphum_sensor(void) {
    PayloadHeader header = {1, true, true, 5};  // dual=true, dedicated=true
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Temp from dedicated sensor - should send only 1 value
    setFlag(&reading, FLAG_TEMP);
    reading.temp[0] = 2500;  // 25.00°C
    reading.temp[1] = 2600;  // This value should be ignored

    // Humidity from dedicated sensor - should send only 1 value
    setFlag(&reading, FLAG_HUM);
    reading.hum[0] = 6000;   // 60.00%
    reading.hum[1] = 6100;   // This value should be ignored

    // PM2.5 from PM sensor - should still send 2 values (dual mode)
    setFlag(&reading, FLAG_PM_25);
    reading.pm_25[0] = 125;  // 12.5 µg/m³
    reading.pm_25[1] = 135;  // 13.5 µg/m³

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 4 (mask) + 2 (temp[0]) + 2 (hum[0]) + 4 (pm25[0,1]) = 14 bytes
    TEST_ASSERT_EQUAL_INT32(14, size);

    // Verify metadata has bit 4 set (dedicated sensor)
    // Binary: 0001 1001 = 0x19 (version=1, dual=1, dedicated=1)
    TEST_ASSERT_EQUAL_UINT8(0x19, buffer[0]);

    // Verify presence mask (bits 0, 1, 8)
    TEST_ASSERT_EQUAL_UINT8(0x03, buffer[2]);  // Bits 0-1 (temp, hum)
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[3]);  // Bit 8 (pm25)

    // Verify temperature[0] (2500 = 0x09C4)
    TEST_ASSERT_EQUAL_UINT8(0xC4, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[7]);

    // Verify humidity[0] (6000 = 0x1770)
    TEST_ASSERT_EQUAL_UINT8(0x70, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x17, buffer[9]);

    // Verify PM2.5[0] (125 = 0x007D)
    TEST_ASSERT_EQUAL_UINT8(0x7D, buffer[10]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[11]);

    // Verify PM2.5[1] (135 = 0x0087)
    TEST_ASSERT_EQUAL_UINT8(0x87, buffer[12]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[13]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_rfc_example_dual_channel);
    RUN_TEST(test_dual_channel_humidity);
    RUN_TEST(test_dual_channel_pm_sensors);
    RUN_TEST(test_dual_channel_mixed);
    RUN_TEST(test_dual_channel_scalars_single_value);
    RUN_TEST(test_dual_channel_32bit_scalar);
    RUN_TEST(test_dual_channel_all_expandable);
    RUN_TEST(test_dual_channel_size_calculation);
    RUN_TEST(test_dedicated_temphum_sensor);

    return UNITY_END();
}
