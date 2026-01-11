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

// Test: RFC Example - Single Channel (Temp + CO2)
// Metadata: 0x01 (Ver=1, Dual=0)
// Interval: 5 minutes
// Mask: 0x00000005 (Bits 0 & 2 set: Temp + CO2)
// Expected output: [0x01, 0x05, 0x05, 0x00, 0x00, 0x00, temp_low, temp_high, co2_low, co2_high]
void test_rfc_example_single_channel(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Set Temp = 25.00°C -> 2500 (int16_t)
    setFlag(&reading, FLAG_TEMP);
    reading.temp[0] = 2500;

    // Set CO2 = 400 ppm
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 2 (temp) + 2 (co2) = 10 bytes
    TEST_ASSERT_EQUAL_INT32(10, size);

    // Verify header
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);  // Metadata
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[1]);  // Interval

    // Verify presence mask (little-endian 0x00000005)
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);

    // Verify temperature (little-endian 2500 = 0x09C4)
    TEST_ASSERT_EQUAL_UINT8(0xC4, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[7]);

    // Verify CO2 (little-endian 400 = 0x0190)
    TEST_ASSERT_EQUAL_UINT8(0x90, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[9]);
}

// Test: Single channel with humidity only
void test_single_channel_humidity_only(void) {
    PayloadHeader header = {1, false, false, 10};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Set Humidity = 65.50% -> 6550 (uint16_t)
    setFlag(&reading, FLAG_HUM);
    reading.hum[0] = 6550;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 2 (hum) = 8 bytes
    TEST_ASSERT_EQUAL_INT32(8, size);

    // Verify presence mask (0x00000002 - bit 1)
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);

    // Verify humidity (little-endian 6550 = 0x1996)
    TEST_ASSERT_EQUAL_UINT8(0x96, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x19, buffer[7]);
}

// Test: Multiple scalar fields
void test_single_channel_multiple_scalars(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // CO2, TVOC, NOX
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 450;

    setFlag(&reading, FLAG_TVOC);
    reading.tvoc = 100;

    setFlag(&reading, FLAG_NOX);
    reading.nox = 50;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 2 (co2) + 2 (tvoc) + 2 (nox) = 12 bytes
    TEST_ASSERT_EQUAL_INT32(12, size);

    // Verify presence mask (0x0000002C - bits 2, 3, 5)
    TEST_ASSERT_EQUAL_UINT8(0x2C, buffer[2]);
}

// Test: PM sensors in order
void test_single_channel_pm_sensors(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // PM2.5, PM10
    setFlag(&reading, FLAG_PM_25);
    reading.pm_25[0] = 125;  // 12.5 µg/m³ * 10

    setFlag(&reading, FLAG_PM_10);
    reading.pm_10[0] = 250;  // 25.0 µg/m³ * 10

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 2 (pm25) + 2 (pm10) = 10 bytes
    TEST_ASSERT_EQUAL_INT32(10, size);

    // Verify presence mask (0x00000300 - bits 8, 9)
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x03, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[5]);

    // Verify PM2.5 (little-endian 125 = 0x007D)
    TEST_ASSERT_EQUAL_UINT8(0x7D, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[7]);

    // Verify PM10 (little-endian 250 = 0x00FA)
    TEST_ASSERT_EQUAL_UINT8(0xFA, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[9]);
}

// Test: 32-bit fields (O3/NO2 electrodes)
void test_single_channel_32bit_fields(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // O3 Working Electrode
    setFlag(&reading, FLAG_O3_WE);
    reading.o3_we = 0x12345678;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Expected size: 2 (header) + 4 (mask) + 4 (o3_we) = 10 bytes
    TEST_ASSERT_EQUAL_INT32(10, size);

    // Verify O3_WE (little-endian 0x12345678)
    TEST_ASSERT_EQUAL_UINT8(0x78, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0x56, buffer[7]);
    TEST_ASSERT_EQUAL_UINT8(0x34, buffer[8]);
    TEST_ASSERT_EQUAL_UINT8(0x12, buffer[9]);
}

// Test: Negative temperature
void test_single_channel_negative_temperature(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Set Temp = -10.50°C -> -1050 (int16_t)
    setFlag(&reading, FLAG_TEMP);
    reading.temp[0] = -1050;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_INT32(8, size);

    // Verify temperature (little-endian -1050 = 0xFBE6 in two's complement)
    TEST_ASSERT_EQUAL_UINT8(0xE6, buffer[6]);
    TEST_ASSERT_EQUAL_UINT8(0xFB, buffer[7]);
}

// Test: All sensor types
void test_single_channel_all_sensors(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Set all flags
    reading.presence_mask = 0x03FFFFFF;  // All 26 bits set

    // Set some values
    reading.temp[0] = 2500;
    reading.hum[0] = 5000;
    reading.co2 = 400;
    reading.tvoc = 100;
    reading.tvoc_raw = 200;
    reading.nox = 50;
    reading.nox_raw = 75;
    reading.pm_01[0] = 10;
    reading.pm_25[0] = 25;
    reading.pm_10[0] = 50;
    reading.pm_01_sp[0] = 11;
    reading.pm_25_sp[0] = 26;
    reading.pm_10_sp[0] = 51;
    reading.pm_03_pc[0] = 1000;
    reading.pm_05_pc[0] = 2000;
    reading.pm_01_pc[0] = 3000;
    reading.pm_25_pc[0] = 4000;
    reading.pm_5_pc[0] = 5000;
    reading.pm_10_pc[0] = 6000;
    reading.vbat = 3700;
    reading.vpanel = 5000;
    reading.o3_we = 1000;
    reading.o3_ae = 2000;
    reading.no2_we = 3000;
    reading.no2_ae = 4000;
    reading.afe_temp = 250;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Verify size is positive and reasonable
    TEST_ASSERT_GREATER_THAN(0, size);

    // Verify header and mask are correct
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);  // Metadata
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[1]);  // Interval
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[2]);  // Mask byte 0
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[3]);  // Mask byte 1
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[4]);  // Mask byte 2
    TEST_ASSERT_EQUAL_UINT8(0x03, buffer[5]);  // Mask byte 3 (26 bits)
}

// Test: Signal strength field (int8_t)
void test_single_channel_signal(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Signal = -75 dBm
    setFlag(&reading, FLAG_SIGNAL);
    reading.signal = -75;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 4 (mask) + 1 (signal) = 7 bytes
    TEST_ASSERT_EQUAL_INT32(7, size);

    // Verify presence mask (bit 26 = 0x04000000)
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, buffer[4]);
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[5]);

    // Verify signal byte (two's complement: -75 = 0xB5)
    TEST_ASSERT_EQUAL_UINT8(0xB5, buffer[6]);
}

// Test: Dedicated temp/hum sensor in single mode (edge case)
void test_dedicated_sensor_single_mode(void) {
    PayloadHeader header = {1, false, true, 5};  // single mode, dedicated=true
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);

    // Set temp and humidity from dedicated sensor
    setFlag(&reading, FLAG_TEMP);
    reading.temp[0] = 2500;  // 25.00°C

    setFlag(&reading, FLAG_HUM);
    reading.hum[0] = 6000;  // 60.00%

    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 4 (mask) + 2 (temp) + 2 (hum) + 2 (co2) = 12 bytes
    TEST_ASSERT_EQUAL_INT32(12, size);

    // Verify metadata has dedicated flag set (bit 4)
    // Binary: 0001 0001 = 0x11 (version=1, dual=0, dedicated=1)
    TEST_ASSERT_EQUAL_UINT8(0x11, buffer[0]);

    // Verify only 1 value is encoded for temp/hum (same as non-dedicated single mode)
    // This validates correct behavior even though the flag is redundant in single mode
    TEST_ASSERT_EQUAL_UINT8(0x07, buffer[2]);  // Mask bits 0,1,2 (temp,hum,co2)
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_rfc_example_single_channel);
    RUN_TEST(test_single_channel_humidity_only);
    RUN_TEST(test_single_channel_multiple_scalars);
    RUN_TEST(test_single_channel_pm_sensors);
    RUN_TEST(test_single_channel_32bit_fields);
    RUN_TEST(test_single_channel_negative_temperature);
    RUN_TEST(test_single_channel_all_sensors);
    RUN_TEST(test_single_channel_signal);
    RUN_TEST(test_dedicated_sensor_single_mode);

    return UNITY_END();
}
