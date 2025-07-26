/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef ESP8266

#ifndef AG_COMMON_H
#define AG_COMMON_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define DELAY_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define MILLIS() ((uint32_t)(esp_timer_get_time() / 1000))

class Common {
public:
  static void splitByComma(const std::string &data, int *v1, int *v2) {
    size_t pos = data.find(",");
    if (pos != std::string::npos) {
      // Ignore <ber> value, only <rssi>
      *v1 = std::stoi(data.substr(0, pos));
      *v2 = std::stoi(data.substr(pos + 1, data.length()));
    }
  }
};

#endif // AG_COMMON_H
#endif // ESP8266
