/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_LOGGER_H
#define AIRGRADIENT_LOGGER_H

#ifdef ARDUINO
// Define log levels that only applied for arduino
#define AG_LOG_LEVEL_ERROR 1
#define AG_LOG_LEVEL_WARN 2
#define AG_LOG_LEVEL_INFO 3
#define AG_LOG_LEVEL_DEBUG 4
#define AG_LOG_LEVEL_VERBOSE 5

// Default to INFO if not specified
#ifndef AG_LOG_LEVEL
#define AG_LOG_LEVEL AG_LOG_LEVEL_INFO
#endif

#include <Arduino.h>

#if AG_LOG_LEVEL >= AG_LOG_LEVEL_ERROR
    #define AG_LOGE(tag, fmt, ...) Serial.printf("[%s] Error: " fmt "\n", tag, ##__VA_ARGS__)
  #else
    #define AG_LOGE(tag, fmt, ...)
  #endif

  #if AG_LOG_LEVEL >= AG_LOG_LEVEL_WARN
    #define AG_LOGW(tag, fmt, ...) Serial.printf("[%s] Warning: " fmt "\n", tag, ##__VA_ARGS__)
  #else
    #define AG_LOGW(tag, fmt, ...)
  #endif

  #if AG_LOG_LEVEL >= AG_LOG_LEVEL_INFO
    #define AG_LOGI(tag, fmt, ...) Serial.printf("[%s] Info: " fmt "\n", tag, ##__VA_ARGS__)
  #else
    #define AG_LOGI(tag, fmt, ...)
  #endif

  #if AG_LOG_LEVEL >= AG_LOG_LEVEL_DEBUG
    #define AG_LOGD(tag, fmt, ...) Serial.printf("[%s] Debug: " fmt "\n", tag, ##__VA_ARGS__)
  #else
    #define AG_LOGD(tag, fmt, ...)
  #endif

  #if AG_LOG_LEVEL >= AG_LOG_LEVEL_VERBOSE
    #define AG_LOGV(tag, fmt, ...) Serial.printf("[%s] Verbose: " fmt "\n", tag, ##__VA_ARGS__)
  #else
    #define AG_LOGV(tag, fmt, ...)
  #endif

#else
#include "esp_log.h"
#define AG_LOGV(tag, fmt, ...) ESP_LOGV(tag, fmt, ##__VA_ARGS__)
#define AG_LOGD(tag, fmt, ...) ESP_LOGD(tag, fmt, ##__VA_ARGS__)
#define AG_LOGI(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define AG_LOGW(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#define AG_LOGE(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#endif

#endif // AIRGRADIENT_LOGGER_H
