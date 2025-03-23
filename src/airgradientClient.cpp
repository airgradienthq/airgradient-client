/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "airgradientClient.h"
#include <string>

bool AirgradientClient::begin(std::string sn) { return true; }

bool AirgradientClient::ensureClientConnection() { return true; }

bool AirgradientClient::isClientReady() { return clientReady; }

std::string AirgradientClient::httpFetchConfig() { return std::string(); }

bool AirgradientClient::httpPostMeasures(const std::string &payload) { return false; }

bool AirgradientClient::mqttConnect() { return false; }

bool AirgradientClient::mqttDisconnect() { return false; }

bool AirgradientClient::mqttPublishMeasures(const std::string &payload) { return false; }

void AirgradientClient::resetFetchConfigurationStatus() { lastFetchConfigSucceed = true; }

void AirgradientClient::resetPostMeasuresStatus() { lastPostMeasuresSucceed = true; }

bool AirgradientClient::isLastFetchConfigSucceed() { return lastFetchConfigSucceed; }

bool AirgradientClient::isLastPostMeasureSucceed() { return lastPostMeasuresSucceed; }

bool AirgradientClient::isRegisteredOnAgServer() { return registeredOnAgServer; }

std::string AirgradientClient::buildFetchConfigUrl(bool useHttps) {
  // http://hw.airgradient.com/sensors/airgradient:aabbccddeeff/one/config
  char url[80] = {0};
  if (useHttps) {
    sprintf(url, "https://%s/sensors/airgradient:%s/one/config", httpDomain, serialNumber.c_str());
  } else {
    sprintf(url, "http://%s/sensors/airgradient:%s/one/config", httpDomain, serialNumber.c_str());
  }

  return std::string(url);
}

std::string AirgradientClient::buildPostMeasuresUrl(bool useHttps) {
  // http://hw.airgradient.com/sensors/airgradient:aabbccddeeff/measures
  char url[80] = {0};
  if (useHttps) {
    sprintf(url, "https://%s/sensors/airgradient:%s/measures", httpDomain, serialNumber.c_str());
  } else {
    sprintf(url, "http://%s/sensors/airgradient:%s/measures", httpDomain, serialNumber.c_str());
  }

  return std::string(url);
}

std::string AirgradientClient::buildMqttTopicPublishMeasures() {
  char topic[50] = {0};
  sprintf(topic, "ag/%s/c-c", serialNumber.c_str());
  return topic;
}
