#include <stdio.h>
#include "payload_encoder.h"

int main(void) {
    printf("=== Struct Sizes ===\n");
    printf("sizeof(SensorReading): %zu bytes\n", sizeof(SensorReading));
    printf("sizeof(PayloadHeader): %zu bytes\n", sizeof(PayloadHeader));
    printf("sizeof(EncoderContext): %zu bytes\n", sizeof(EncoderContext));
    printf("\n");

    // Test Single Channel Mode - All Flags Set
    PayloadEncoder encoder_single;
    PayloadHeader header_single = {1, false, false, 5};
    encoder_single.init(header_single);

    SensorReading reading_all;
    reading_all.presence_mask = 0x03FFFFFF;  // All 26 bits set

    // Set all values
    reading_all.temp[0] = 2500;
    reading_all.temp[1] = 2600;
    reading_all.hum[0] = 5000;
    reading_all.hum[1] = 5100;
    reading_all.co2 = 400;
    reading_all.tvoc = 100;
    reading_all.tvoc_raw = 200;
    reading_all.nox = 50;
    reading_all.nox_raw = 75;
    reading_all.pm_01[0] = 10;
    reading_all.pm_01[1] = 11;
    reading_all.pm_25[0] = 25;
    reading_all.pm_25[1] = 26;
    reading_all.pm_10[0] = 50;
    reading_all.pm_10[1] = 51;
    reading_all.pm_01_sp[0] = 11;
    reading_all.pm_01_sp[1] = 12;
    reading_all.pm_25_sp[0] = 26;
    reading_all.pm_25_sp[1] = 27;
    reading_all.pm_10_sp[0] = 51;
    reading_all.pm_10_sp[1] = 52;
    reading_all.pm_03_pc[0] = 1000;
    reading_all.pm_03_pc[1] = 1001;
    reading_all.pm_05_pc[0] = 2000;
    reading_all.pm_05_pc[1] = 2001;
    reading_all.pm_01_pc[0] = 3000;
    reading_all.pm_01_pc[1] = 3001;
    reading_all.pm_25_pc[0] = 4000;
    reading_all.pm_25_pc[1] = 4001;
    reading_all.pm_5_pc[0] = 5000;
    reading_all.pm_5_pc[1] = 5001;
    reading_all.pm_10_pc[0] = 6000;
    reading_all.pm_10_pc[1] = 6001;
    reading_all.vbat = 3700;
    reading_all.vpanel = 5000;
    reading_all.o3_we = 1000;
    reading_all.o3_ae = 2000;
    reading_all.no2_we = 3000;
    reading_all.no2_ae = 4000;
    reading_all.afe_temp = 250;

    encoder_single.addReading(reading_all);

    uint8_t buffer_single[256];
    int32_t size_single = encoder_single.encode(buffer_single, sizeof(buffer_single));

    printf("=== Encoded Payload Sizes (All 26 Flags Set) ===\n");
    printf("Single Channel Mode: %d bytes\n", size_single);

    // Breakdown
    printf("  Header: 2 bytes (metadata + interval)\n");
    printf("  Presence Mask: 4 bytes\n");
    printf("  Expandable fields (14): 14 * 2 = 28 bytes (1 value each)\n");
    printf("  Scalar 16-bit fields (7): 7 * 2 = 14 bytes\n");
    printf("  Scalar 32-bit fields (5): 5 * 4 = 20 bytes\n");
    printf("  Expected: 2 + 4 + 28 + 14 + 20 = 68 bytes\n");
    printf("\n");

    // Test Dual Channel Mode - All Flags Set
    PayloadEncoder encoder_dual;
    PayloadHeader header_dual = {1, true, false, 5};
    encoder_dual.init(header_dual);
    encoder_dual.addReading(reading_all);

    uint8_t buffer_dual[256];
    int32_t size_dual = encoder_dual.encode(buffer_dual, sizeof(buffer_dual));

    printf("Dual Channel Mode: %d bytes\n", size_dual);
    printf("  Header: 2 bytes (metadata + interval)\n");
    printf("  Presence Mask: 4 bytes\n");
    printf("  Expandable fields (14): 14 * 4 = 56 bytes (2 values each)\n");
    printf("  Scalar 16-bit fields (7): 7 * 2 = 14 bytes\n");
    printf("  Scalar 32-bit fields (5): 5 * 4 = 20 bytes\n");
    printf("  Expected: 2 + 4 + 56 + 14 + 20 = 96 bytes\n");
    printf("\n");

    printf("=== Memory Efficiency ===\n");
    printf("SensorReading struct: %zu bytes\n", sizeof(SensorReading));
    printf("Encoded (single mode): %d bytes (%.1f%% of struct size)\n",
           size_single, (float)size_single / sizeof(SensorReading) * 100);
    printf("Encoded (dual mode): %d bytes (%.1f%% of struct size)\n",
           size_dual, (float)size_dual / sizeof(SensorReading) * 100);

    return 0;
}
