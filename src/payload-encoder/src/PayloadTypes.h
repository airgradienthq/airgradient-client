/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef PAYLOAD_TYPES_H
#define PAYLOAD_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Maximum number of readings in a batch
#define MAX_BATCH_SIZE 20

// Sensor flags enum (matches presence mask bits 0-26)
typedef enum {
    FLAG_TEMP = 0,
    FLAG_HUM = 1,
    FLAG_CO2 = 2,
    FLAG_TVOC = 3,
    FLAG_TVOC_RAW = 4,
    FLAG_NOX = 5,
    FLAG_NOX_RAW = 6,
    FLAG_PM_01 = 7,
    FLAG_PM_25 = 8,
    FLAG_PM_10 = 9,
    FLAG_PM_01_SP = 10,
    FLAG_PM_25_SP = 11,
    FLAG_PM_10_SP = 12,
    FLAG_PM_03_PC = 13,
    FLAG_PM_05_PC = 14,
    FLAG_PM_01_PC = 15,
    FLAG_PM_25_PC = 16,
    FLAG_PM_5_PC = 17,
    FLAG_PM_10_PC = 18,
    FLAG_VBAT = 19,
    FLAG_VPANEL = 20,
    FLAG_O3_WE = 21,
    FLAG_O3_AE = 22,
    FLAG_NO2_WE = 23,
    FLAG_NO2_AE = 24,
    FLAG_AFE_TEMP = 25,
    FLAG_SIGNAL = 26
} SensorFlag;

// Bitmask helpers
#define FLAG_BIT(flag) (1U << (flag))
#define IS_FLAG_SET(mask, flag) (((mask) & FLAG_BIT(flag)) != 0)

// Sensor reading structure
typedef struct {
    uint32_t presence_mask;     // Which fields are present

    // Sensor values (only valid if corresponding bit set in presence_mask)
    // Expandable fields (dual-mode sends 2 values: [0] and [1])
    int16_t temp[2];            // Temperature * 100 (Celsius)
    uint16_t hum[2];            // Humidity * 100 (%)
    uint16_t pm_01[2];          // PM1.0 * 10 (Atmospheric)
    uint16_t pm_25[2];          // PM2.5 * 10 (Atmospheric)
    uint16_t pm_10[2];          // PM10 * 10 (Atmospheric)
    uint16_t pm_01_sp[2];       // PM1.0 * 10 (Standard Particle)
    uint16_t pm_25_sp[2];       // PM2.5 * 10 (Standard Particle)
    uint16_t pm_10_sp[2];       // PM10 * 10 (Standard Particle)
    uint16_t pm_03_pc[2];       // PM0.3 count
    uint16_t pm_05_pc[2];       // PM0.5 count
    uint16_t pm_01_pc[2];       // PM1.0 count
    uint16_t pm_25_pc[2];       // PM2.5 count
    uint16_t pm_5_pc[2];        // PM5.0 count
    uint16_t pm_10_pc[2];       // PM10 count

    // Scalar fields (always single value regardless of mode)
    uint16_t co2;               // CO2 ppm
    uint16_t tvoc;              // TVOC index
    uint16_t tvoc_raw;          // TVOC raw
    uint16_t nox;               // NOx index
    uint16_t nox_raw;           // NOx raw
    uint16_t vbat;              // Battery voltage (mV)
    uint16_t vpanel;            // Panel/Charger voltage (mV)
    uint32_t o3_we;             // O3 Working Electrode (mV/Raw)
    uint32_t o3_ae;             // O3 Aux Electrode (mV/Raw)
    uint32_t no2_we;            // NO2 Working Electrode (mV/Raw)
    uint32_t no2_ae;            // NO2 Aux Electrode (mV/Raw)
    uint16_t afe_temp;          // AFE Chip Temperature * 10
    int8_t signal;              // Signal strength (dBm)
} SensorReading;

// Payload header (Byte 0: Metadata + Byte 1: Interval)
typedef struct {
    uint8_t version;                // Protocol version (0-7)
    bool dual_mode;                 // Dual channel mode
    bool dedicated_temphum_sensor;  // Dedicated temp/hum sensor (not from PM sensor)
    uint8_t interval_minutes;       // Measurement interval in minutes
} PayloadHeader;

// Encoder context
typedef struct {
    PayloadHeader header;
    SensorReading readings[MAX_BATCH_SIZE];
    uint8_t reading_count;
} EncoderContext;

// Helper to initialize a sensor reading
static inline void initSensorReading(SensorReading* reading) {
    reading->presence_mask = 0;
}

// Helper to set a flag in presence mask
static inline void setFlag(SensorReading* reading, SensorFlag flag) {
    reading->presence_mask |= FLAG_BIT(flag);
}

// Helper to clear a flag in presence mask
static inline void clearFlag(SensorReading* reading, SensorFlag flag) {
    reading->presence_mask &= ~FLAG_BIT(flag);
}

// Helper to check if a flag is set
static inline bool isFlagSet(const SensorReading* reading, SensorFlag flag) {
    return IS_FLAG_SET(reading->presence_mask, flag);
}

#endif // PAYLOAD_TYPES_H
