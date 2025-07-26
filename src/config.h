#ifndef __CONFIG_H
#define __CONFIG_H

// #define ENABLE_DEBUG_MODE
// #define DELAY_INSTEAD_OF_DEEP_SLEEP

#ifdef ENABLE_DEBUG_MODE
#define DBG(...)                     \
    {                                \
        Serial.print("[");           \
        Serial.print(__FUNCTION__);  \
        Serial.print("(): ");        \
        Serial.print(__LINE__);      \
        Serial.print(" ] ");         \
        Serial.println(__VA_ARGS__); \
    }
#else
#define DBG(...)
#endif

#define IS_PM_VALID(val) (val >= 0)
#define IS_TEMPERATURE_VALID(val) ((val >= -40) && (val <= 125))
#define IS_HUMIDITY_VALID(val) ((val >= 0) && (val <= 100))
#define IS_CO2_VALID(val) ((val >= 0) && (val <= 10000))
#define IS_TVOC_VALID(val) (val >= 0)
#define IS_NOX_VALID(val) (val >= 0)
#define IS_VOLT_VALID(val) (val >= 0)

#endif
