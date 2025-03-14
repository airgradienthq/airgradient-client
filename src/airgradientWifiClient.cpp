/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "airgradientWifiClient.h"

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#else
#include <HTTPClient.h>
#endif

std::string AirgradientWifiClient::httpFetchConfig(const std::string &sn) {
  Serial.println("Fetch configuration from server");
  std::string url = buildFetchConfigUrl(sn, true);

  // Init http client
#ifdef ESP8266
  HTTPClient client;
  WiFiClient wifiClient;
  if (client.begin(wifiClient, url) == false) {
    lastFetchConfigSucceed = false;
    return false;
  }
#else
  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    Serial.println("ERROR begin HTTPClient using TLS");
    lastFetchConfigSucceed = false;
    return {};
  }
#endif

  // Fetch configuration
  int statusCode = client.GET();
  if (statusCode != 200) {
    client.end();
    Serial.printf("Failed fetch configuration from server with return code %d\n");
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
    Serial.println("Success fetch configuration from server but somehow body is empty");
    lastFetchConfigSucceed = false;
    return responseBody;
  }

  // Set success state flag
  registeredOnAgServer = true;
  lastFetchConfigSucceed = true;
  Serial.println("Success fetch configuration from server, still needs to be parsed and validated");

  return responseBody;
}
bool AirgradientWifiClient::httpPostMeasures(const std::string &sn, const std::string &payload) {
  Serial.println("Post measures to server");
  std::string url = buildPostMeasuresUrl(sn, true);

#ifdef ESP8266
  HTTPClient client;
  WiFiClient wifiClient;
  if (client.begin(wifiClient, uri) == false) {
    lastPostMeasuresSucceed = false;
    return false;
  }
#else
  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    Serial.println("ERROR begin HTTPClient using TLS");
    lastPostMeasuresSucceed = false;
    return false;
  }
#endif

  client.addHeader("content-type", "application/json");
  int statusCode = client.POST(String(payload.c_str()));
  client.end();

  if ((statusCode != 200) && (statusCode != 429)) {
    Serial.printf("Failed post measures to server with response code %d\n", statusCode);
    lastPostMeasuresSucceed = false;
    return false;
  }

  lastPostMeasuresSucceed = true;
  Serial.printf("Success post measures to server with response code %d\n", statusCode);

  return true;
}
