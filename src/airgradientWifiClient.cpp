/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifdef ARDUINO
#ifndef ESP8266

#include "airgradientWifiClient.h"
#include "agLogger.h"

#include <HTTPClient.h>

bool AirgradientWifiClient::begin(std::string sn) {
  serialNumber = sn;
  return true;
}

std::string AirgradientWifiClient::httpFetchConfig() {
  std::string url = buildFetchConfigUrl(true);
  AG_LOGI(TAG, "Fetch configuration from %s", url.c_str());

  // Init http client
  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    AG_LOGE(TAG, "Failed begin HTTPClient using TLS");
    lastFetchConfigSucceed = false;
    return {};
  }

  // Fetch configuration
  int statusCode = client.GET();
  if (statusCode != 200) {
    client.end();
    AG_LOGE(TAG, "Failed fetch configuration from server with return code %d", statusCode);
    // Return code 400 means device not registered on ag server
    if (statusCode == 400) {
      registeredOnAgServer = false;
    }
    lastFetchConfigSucceed = false;
    return {};
  }

  // Get response body
  std::string responseBody = client.getString().c_str();
  client.end();

  if (responseBody.empty()) {
    AG_LOGW(TAG, "Success fetch configuration from server but somehow body is empty");
    lastFetchConfigSucceed = false;
    return responseBody;
  }

  // Set success state flag
  registeredOnAgServer = true;
  lastFetchConfigSucceed = true;
  AG_LOGI(TAG, "Success fetch configuration from server, still needs to be parsed and validated");

  return responseBody;
}
bool AirgradientWifiClient::httpPostMeasures(const std::string &payload) {
  std::string url = buildPostMeasuresUrl(true);
  AG_LOGI(TAG, "Post measures to %s", url.c_str());

  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    AG_LOGE(TAG, "Failed begin HTTPClient using TLS");
    lastPostMeasuresSucceed = false;
    return false;
  }

  client.addHeader("content-type", "application/json");
  int statusCode = client.POST(String(payload.c_str()));
  client.end();

  if ((statusCode != 200) && (statusCode != 429)) {
    AG_LOGE(TAG, "Failed post measures to server with response code %d", statusCode);
    lastPostMeasuresSucceed = false;
    return false;
  }

  lastPostMeasuresSucceed = true;
  AG_LOGI(TAG, "Success post measures to server with response code %d", statusCode);

  return true;
}

#endif // ESP8266
#endif // ARDUINO
