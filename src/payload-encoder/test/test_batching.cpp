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

// Test: Batch with 2 identical readings
void test_batch_two_identical_readings(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);
    encoder.addReading(reading);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 2 * (4 (mask) + 2 (co2)) = 2 + 12 = 14
    TEST_ASSERT_EQUAL_INT32(14, size);

    // Verify header is only written once
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[1]);

    // Verify first reading
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[2]);  // Mask (bit 2)
    TEST_ASSERT_EQUAL_UINT8(0x90, buffer[6]);  // CO2 low byte
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[7]);  // CO2 high byte

    // Verify second reading
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[8]);  // Mask (bit 2)
    TEST_ASSERT_EQUAL_UINT8(0x90, buffer[12]); // CO2 low byte
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[13]); // CO2 high byte
}

// Test: Batch with different readings
void test_batch_different_readings(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    // Reading 1: Temperature only
    SensorReading reading1;
    initSensorReading(&reading1);
    setFlag(&reading1, FLAG_TEMP);
    reading1.temp[0] = 2500;

    // Reading 2: CO2 only
    SensorReading reading2;
    initSensorReading(&reading2);
    setFlag(&reading2, FLAG_CO2);
    reading2.co2 = 400;

    encoder.addReading(reading1);
    encoder.addReading(reading2);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + (4 + 2) + (4 + 2) = 14
    TEST_ASSERT_EQUAL_INT32(14, size);

    // Verify first reading mask (bit 0 = temp)
    TEST_ASSERT_EQUAL_UINT8(0x01, buffer[2]);

    // Verify second reading mask (bit 2 = co2)
    TEST_ASSERT_EQUAL_UINT8(0x04, buffer[8]);
}

// Test: Batch with 5 readings
void test_batch_five_readings(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    for (int i = 0; i < 5; i++) {
        SensorReading reading;
        initSensorReading(&reading);
        setFlag(&reading, FLAG_CO2);
        reading.co2 = 400 + i * 10;  // Different CO2 values
        encoder.addReading(reading);
    }

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 5 * (4 (mask) + 2 (co2)) = 2 + 30 = 32
    TEST_ASSERT_EQUAL_INT32(32, size);

    TEST_ASSERT_EQUAL_UINT8(5, encoder.getReadingCount());
}

// Test: Batch with 10 readings
void test_batch_ten_readings(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    for (int i = 0; i < 10; i++) {
        SensorReading reading;
        initSensorReading(&reading);
        setFlag(&reading, FLAG_CO2);
        reading.co2 = 400 + i;
        encoder.addReading(reading);
    }

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 10 * 6 = 62
    TEST_ASSERT_EQUAL_INT32(62, size);
}

// Test: Batch with 20 readings (MAX_BATCH_SIZE)
void test_batch_max_readings(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    for (int i = 0; i < MAX_BATCH_SIZE; i++) {
        SensorReading reading;
        initSensorReading(&reading);
        setFlag(&reading, FLAG_CO2);
        reading.co2 = 400 + i;
        encoder.addReading(reading);
    }

    uint8_t buffer[512];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Size: 2 (header) + 20 * 6 = 122
    TEST_ASSERT_EQUAL_INT32(122, size);
    TEST_ASSERT_EQUAL_UINT8(MAX_BATCH_SIZE, encoder.getReadingCount());
}

// Test: Batch with different presence masks per reading
void test_batch_variable_masks(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    // Reading 1: Temp + CO2
    SensorReading reading1;
    initSensorReading(&reading1);
    setFlag(&reading1, FLAG_TEMP);
    setFlag(&reading1, FLAG_CO2);
    reading1.temp[0] = 2500;
    reading1.co2 = 400;

    // Reading 2: Hum only
    SensorReading reading2;
    initSensorReading(&reading2);
    setFlag(&reading2, FLAG_HUM);
    reading2.hum[0] = 6000;

    // Reading 3: CO2 + TVOC + NOX
    SensorReading reading3;
    initSensorReading(&reading3);
    setFlag(&reading3, FLAG_CO2);
    setFlag(&reading3, FLAG_TVOC);
    setFlag(&reading3, FLAG_NOX);
    reading3.co2 = 450;
    reading3.tvoc = 100;
    reading3.nox = 50;

    encoder.addReading(reading1);
    encoder.addReading(reading2);
    encoder.addReading(reading3);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Reading 1: 4 + 2 + 2 = 8
    // Reading 2: 4 + 2 = 6
    // Reading 3: 4 + 2 + 2 + 2 = 10
    // Total: 2 + 8 + 6 + 10 = 26
    TEST_ASSERT_EQUAL_INT32(26, size);

    // Verify reading 1 mask (bits 0, 2)
    TEST_ASSERT_EQUAL_UINT8(0x05, buffer[2]);

    // Verify reading 2 mask (bit 1)
    TEST_ASSERT_EQUAL_UINT8(0x02, buffer[10]);

    // Verify reading 3 mask (bits 2, 3, 5)
    TEST_ASSERT_EQUAL_UINT8(0x2C, buffer[16]);
}

// Test: Batch dual mode with expandable fields
void test_batch_dual_mode(void) {
    PayloadHeader header = {1, true, false, 5};  // Dual mode
    encoder.init(header);

    // Reading 1: Temp (expandable)
    SensorReading reading1;
    initSensorReading(&reading1);
    setFlag(&reading1, FLAG_TEMP);
    reading1.temp[0] = 2500;
    reading1.temp[1] = 2600;

    // Reading 2: CO2 (scalar)
    SensorReading reading2;
    initSensorReading(&reading2);
    setFlag(&reading2, FLAG_CO2);
    reading2.co2 = 400;

    encoder.addReading(reading1);
    encoder.addReading(reading2);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Reading 1: 4 + 4 (temp dual) = 8
    // Reading 2: 4 + 2 (co2) = 6
    // Total: 2 + 8 + 6 = 16
    TEST_ASSERT_EQUAL_INT32(16, size);

    // Verify metadata shows dual mode
    TEST_ASSERT_EQUAL_UINT8(0x09, buffer[0]);
}

// Test: Size calculation for batches
void test_batch_size_calculation(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);
    encoder.addReading(reading);
    encoder.addReading(reading);

    uint32_t total_size = encoder.calculateTotalSize();

    // Expected: 2 (header) + 3 * (4 + 2) = 20
    TEST_ASSERT_EQUAL_UINT32(20, total_size);

    uint8_t buffer[256];
    int32_t encoded_size = encoder.encode(buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_UINT32(total_size, (uint32_t)encoded_size);
}

// Test: Large batch with many sensors
void test_batch_large_payload(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    for (int i = 0; i < 15; i++) {
        SensorReading reading;
        initSensorReading(&reading);

        // Multiple sensors per reading
        setFlag(&reading, FLAG_TEMP);
        setFlag(&reading, FLAG_HUM);
        setFlag(&reading, FLAG_CO2);
        setFlag(&reading, FLAG_PM_25);

        reading.temp[0] = 2500 + i;
        reading.hum[0] = 6000 + i;
        reading.co2 = 400 + i;
        reading.pm_25[0] = 25 + i;

        encoder.addReading(reading);
    }

    uint8_t buffer[512];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Verify encoding succeeded
    TEST_ASSERT_GREATER_THAN(0, size);
    TEST_ASSERT_EQUAL_UINT8(15, encoder.getReadingCount());
}

// Test: Batch with mix of 16-bit and 32-bit fields
void test_batch_mixed_field_sizes(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    // Reading 1: 16-bit fields
    SensorReading reading1;
    initSensorReading(&reading1);
    setFlag(&reading1, FLAG_CO2);
    reading1.co2 = 400;

    // Reading 2: 32-bit fields
    SensorReading reading2;
    initSensorReading(&reading2);
    setFlag(&reading2, FLAG_O3_WE);
    reading2.o3_we = 1000;

    encoder.addReading(reading1);
    encoder.addReading(reading2);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    // Reading 1: 4 + 2 = 6
    // Reading 2: 4 + 4 = 8
    // Total: 2 + 6 + 8 = 16
    TEST_ASSERT_EQUAL_INT32(16, size);
}

// Test: Reset clears batch
void test_batch_reset(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);
    encoder.addReading(reading);
    encoder.addReading(reading);

    TEST_ASSERT_EQUAL_UINT8(3, encoder.getReadingCount());

    encoder.reset();

    TEST_ASSERT_EQUAL_UINT8(0, encoder.getReadingCount());

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT32(0, size);  // No readings
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_batch_two_identical_readings);
    RUN_TEST(test_batch_different_readings);
    RUN_TEST(test_batch_five_readings);
    RUN_TEST(test_batch_ten_readings);
    RUN_TEST(test_batch_max_readings);
    RUN_TEST(test_batch_variable_masks);
    RUN_TEST(test_batch_dual_mode);
    RUN_TEST(test_batch_size_calculation);
    RUN_TEST(test_batch_large_payload);
    RUN_TEST(test_batch_mixed_field_sizes);
    RUN_TEST(test_batch_reset);

    return UNITY_END();
}
