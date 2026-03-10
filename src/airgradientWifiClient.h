/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_WIFI_CLIENT_H
#define AIRGRADIENT_WIFI_CLIENT_H

#ifndef ESP8266
#define ARDUINOJSON_ENABLE_PROGMEM 0

#include <string>
#include <ArduinoJson.h>

#include "airgradientClient.h"

#ifndef ARDUINO
#define MAX_RESPONSE_BUFFER 2048
#endif

class AirgradientWifiClient : public AirgradientClient {
private:
  const char *const TAG = "AgWifiClient";
  uint16_t timeoutMs = 15000; // Default set to 15s
#ifndef ARDUINO
  char responseBuffer[2048];
#endif
public:
  AirgradientWifiClient() {};
  ~AirgradientWifiClient() {};

  bool begin(std::string sn, PayloadType pt);
  std::string httpFetchConfig();
  bool httpPostMeasures(const std::string &payload);
  bool httpPostMeasures(const AirgradientPayload &payload);

private:
  bool _httpGet(const std::string &url, int &responseCode, std::string &responseBody);
  bool _httpPost(const std::string &url, const std::string &payload, int &responseCode);
  void _serialize(JsonDocument &doc, const PayloadBuffer &payload);

};

#endif // ESP8266
#endif // !AIRGRADIENT_WIFI_CLIENT_H
