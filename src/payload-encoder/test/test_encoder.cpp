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

// Test: Encoder initialization
void test_encoder_init(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    TEST_ASSERT_EQUAL_UINT8(0, encoder.getReadingCount());
}

// Test: Encoder reset
void test_encoder_reset(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);
    TEST_ASSERT_EQUAL_UINT8(1, encoder.getReadingCount());

    encoder.reset();
    TEST_ASSERT_EQUAL_UINT8(0, encoder.getReadingCount());
}

// Test: Add single reading
void test_add_single_reading(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    bool result = encoder.addReading(reading);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(1, encoder.getReadingCount());
}

// Test: Add multiple readings
void test_add_multiple_readings(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    for (int i = 0; i < 5; i++) {
        SensorReading reading;
        initSensorReading(&reading);
        setFlag(&reading, FLAG_CO2);
        reading.co2 = 400 + i;

        bool result = encoder.addReading(reading);
        TEST_ASSERT_TRUE(result);
    }

    TEST_ASSERT_EQUAL_UINT8(5, encoder.getReadingCount());
}

// Test: Batch full (MAX_BATCH_SIZE)
void test_batch_full(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    // Fill to MAX_BATCH_SIZE
    for (int i = 0; i < MAX_BATCH_SIZE; i++) {
        bool result = encoder.addReading(reading);
        TEST_ASSERT_TRUE(result);
    }

    TEST_ASSERT_EQUAL_UINT8(MAX_BATCH_SIZE, encoder.getReadingCount());

    // Try to add one more - should fail
    bool result = encoder.addReading(reading);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_UINT8(MAX_BATCH_SIZE, encoder.getReadingCount());
}

// Test: Encode with no readings
void test_encode_empty(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    uint8_t buffer[256];
    int32_t size = encoder.encode(buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_INT32(0, size);  // No readings
}

// Test: Encode with null buffer
void test_encode_null_buffer(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;
    encoder.addReading(reading);

    int32_t size = encoder.encode(nullptr, 256);
    TEST_ASSERT_EQUAL_INT32(-1, size);  // Error
}

// Test: Encode with buffer too small
void test_encode_buffer_too_small(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;
    encoder.addReading(reading);

    uint8_t buffer[5];  // Too small
    int32_t size = encoder.encode(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT32(-1, size);  // Error
}

// Test: Metadata encoding - version
void test_metadata_version(void) {
    PayloadHeader header = {0, false, false, 5};
    encoder.init(header);
    uint8_t metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x00, metadata);

    header.version = 1;
    encoder.init(header);
    metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x01, metadata);

    header.version = 7;
    encoder.init(header);
    metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x07, metadata);
}

// Test: Metadata encoding - dual mode
void test_metadata_dual_mode(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);
    uint8_t metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x01, metadata);  // Version 1, dual_mode=0

    header.dual_mode = true;
    encoder.init(header);
    metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x09, metadata);  // Version 1, dual_mode=1 (bit 3 set)
}

// Test: Metadata encoding - dedicated temp/hum sensor
void test_metadata_dedicated_temphum(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);
    uint8_t metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x01, metadata);  // Version 1, dedicated=0

    header.dedicated_temphum_sensor = true;
    encoder.init(header);
    metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x11, metadata);  // Version 1, dedicated=1 (bit 4 set)

    header.dual_mode = true;
    encoder.init(header);
    metadata = encoder.encodeMetadata();
    TEST_ASSERT_EQUAL_UINT8(0x19, metadata);  // Version 1, dual=1, dedicated=1 (bits 3,4 set)
}

// Test: isExpandable function
void test_is_expandable(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    // Expandable fields
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_TEMP));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_HUM));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_PM_01));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_PM_25));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_PM_10));

    // Scalar fields
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_CO2));
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_TVOC));
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_NOX));
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_VBAT));
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_O3_WE));
}

// Test: isExpandable with dedicated temp/hum sensor
void test_is_expandable_with_dedicated_sensor(void) {
    // Without dedicated sensor - temp/hum are expandable
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_TEMP));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_HUM));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_PM_25));

    // With dedicated sensor - temp/hum are NOT expandable
    header.dedicated_temphum_sensor = true;
    encoder.init(header);
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_TEMP));
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_HUM));

    // PM sensors should still be expandable
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_PM_01));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_PM_25));
    TEST_ASSERT_TRUE(encoder.isExpandable(FLAG_PM_10));

    // Scalar fields remain non-expandable
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_CO2));
    TEST_ASSERT_FALSE(encoder.isExpandable(FLAG_TVOC));
}

// Test: Calculate size for single reading
void test_calculate_reading_size_single(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_TEMP);
    setFlag(&reading, FLAG_CO2);

    // Size = 4 (mask) + 2 (temp, single channel) + 2 (co2) = 8
    uint32_t size = encoder.calculateReadingSize(reading);
    TEST_ASSERT_EQUAL_UINT32(8, size);
}

// Test: Calculate size for dual channel reading
void test_calculate_reading_size_dual(void) {
    PayloadHeader header = {1, true, false, 5};  // dual_mode = true
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_TEMP);
    setFlag(&reading, FLAG_CO2);

    // Size = 4 (mask) + 4 (temp, dual channel: 2*2) + 2 (co2, scalar) = 10
    uint32_t size = encoder.calculateReadingSize(reading);
    TEST_ASSERT_EQUAL_UINT32(10, size);
}

// Test: Calculate total size
void test_calculate_total_size(void) {
    PayloadHeader header = {1, false, false, 5};
    encoder.init(header);

    SensorReading reading;
    initSensorReading(&reading);
    setFlag(&reading, FLAG_CO2);
    reading.co2 = 400;

    encoder.addReading(reading);
    encoder.addReading(reading);

    // Size = 2 (header) + 2 * (4 (mask) + 2 (co2)) = 2 + 12 = 14
    uint32_t size = encoder.calculateTotalSize();
    TEST_ASSERT_EQUAL_UINT32(14, size);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_encoder_init);
    RUN_TEST(test_encoder_reset);
    RUN_TEST(test_add_single_reading);
    RUN_TEST(test_add_multiple_readings);
    RUN_TEST(test_batch_full);
    RUN_TEST(test_encode_empty);
    RUN_TEST(test_encode_null_buffer);
    RUN_TEST(test_encode_buffer_too_small);
    RUN_TEST(test_metadata_version);
    RUN_TEST(test_metadata_dual_mode);
    RUN_TEST(test_metadata_dedicated_temphum);
    RUN_TEST(test_is_expandable);
    RUN_TEST(test_is_expandable_with_dedicated_sensor);
    RUN_TEST(test_calculate_reading_size_single);
    RUN_TEST(test_calculate_reading_size_dual);
    RUN_TEST(test_calculate_total_size);

    return UNITY_END();
}
