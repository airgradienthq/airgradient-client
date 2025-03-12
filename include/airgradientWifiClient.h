/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_WIFI_CLIENT_H
#define AIRGRADIENT_WIFI_CLIENT_H

#include <string>

#include <Arduino.h>

#include "airgradientClient.h"

class AirgradientWifiClient : public AirgradientClient {
private:
  const char *const TAG = "AgWifiClient";
  uint16_t timeoutMs = 15000; // Default set to 15s
public:
  AirgradientWifiClient() {};
  ~AirgradientWifiClient() {};

  std::string httpFetchConfig(const std::string &sn);
  bool httpPostMeasures(const std::string &sn, const std::string &payload);
};

#endif // !AIRGRADIENT_WIFI_CLIENT_H
