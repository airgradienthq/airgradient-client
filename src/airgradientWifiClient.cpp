/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "airgradientClient.h"
#include "config.h"
#ifndef ESP8266
#define ARDUINOJSON_ENABLE_PROGMEM 0

#define JSON_PROP_PM_FIRMWARE "firmware"
#define JSON_PROP_PM01_AE "pm01"
#define JSON_PROP_PM25_AE "pm02"
#define JSON_PROP_PM10_AE "pm10"
#define JSON_PROP_PM01_SP "pm01Standard"
#define JSON_PROP_PM25_SP "pm02Standard"
#define JSON_PROP_PM10_SP "pm10Standard"
#define JSON_PROP_PM25_COMPENSATED "pm02Compensated"
#define JSON_PROP_PM03_COUNT "pm003Count"
#define JSON_PROP_PM05_COUNT "pm005Count"
#define JSON_PROP_PM1_COUNT "pm01Count"
#define JSON_PROP_PM25_COUNT "pm02Count"
#define JSON_PROP_PM5_COUNT "pm50Count"
#define JSON_PROP_PM10_COUNT "pm10Count"
#define JSON_PROP_TEMP "atmp"
#define JSON_PROP_TEMP_COMPENSATED "atmpCompensated"
#define JSON_PROP_RHUM "rhum"
#define JSON_PROP_RHUM_COMPENSATED "rhumCompensated"
#define JSON_PROP_TVOC "tvocIndex"
#define JSON_PROP_TVOC_RAW "tvocRaw"
#define JSON_PROP_NOX "noxIndex"
#define JSON_PROP_NOX_RAW "noxRaw"
#define JSON_PROP_CO2 "rco2"
#define JSON_PROP_VBATT "volt"
#define JSON_PROP_VPANEL "light"
#define JSON_PROP_O3_WE "measure0"
#define JSON_PROP_O3_AE "measure1"
#define JSON_PROP_NO2_WE "measure2"
#define JSON_PROP_NO2_AE "measure3"
#define JSON_PROP_AFE_TEMP "measure4"
#define JSON_PROP_SIGNAL "wifi"

#include "airgradientWifiClient.h"
#include "agLogger.h"
#include "ArduinoJson.h"

#ifdef ARDUINO
#include <HTTPClient.h>
#else
#include "esp_http_client.h"
#endif

bool AirgradientWifiClient::begin(std::string sn, PayloadType pt) {
  serialNumber = sn;
  payloadType = pt;
  return true;
}

std::string AirgradientWifiClient::httpFetchConfig() {
  std::string url = buildFetchConfigUrl(true);
  AG_LOGI(TAG, "Fetch configuration from %s", url.c_str());

  // Perform HTTP GET
  int responseCode;
  std::string responseBody;
  if (_httpGet(url, responseCode, responseBody) == false) {
    lastFetchConfigSucceed = false;
    return {};
  }

  // Define result by response code
  if (responseCode != 200) {
    // client.end();
    AG_LOGE(TAG, "Failed fetch configuration from server with return code %d", responseCode);
    // Return code 400 means device not registered on ag server
    if (responseCode == 400) {
      registeredOnAgServer = false;
    }
    lastFetchConfigSucceed = false;
    return {};
  }

  // Sanity check if response body is empty
  if (responseBody.empty()) {
    AG_LOGW(TAG, "Success fetch configuration from server but somehow body is empty");
    lastFetchConfigSucceed = false;
    return responseBody;
  }

  AG_LOGI(TAG, "Received configuration: (%d) %s", responseBody.length(), responseBody.c_str());

  // Set success state flag
  registeredOnAgServer = true;
  lastFetchConfigSucceed = true;
  AG_LOGI(TAG, "Success fetch configuration from server, still needs to be parsed and validated");

  return responseBody;
}
bool AirgradientWifiClient::httpPostMeasures(const std::string &payload) {
  std::string url = buildPostMeasuresUrl(true);
  AG_LOGI(TAG, "Post measures to %s", url.c_str());
  AG_LOGI(TAG, "Payload: %s", payload.c_str());

  // Perform HTTP POST
  int responseCode;
  if (_httpPost(url, payload, responseCode) == false) {
    lastPostMeasuresSucceed = false;
    return false;
  }

  if ((responseCode != 200) && (responseCode != 429)) {
    AG_LOGE(TAG, "Failed post measures to server with response code %d", responseCode);
    lastPostMeasuresSucceed = false;
    return false;
  }

  lastPostMeasuresSucceed = true;
  AG_LOGI(TAG, "Success post measures to server with response code %d", responseCode);

  return true;
}

bool AirgradientWifiClient::httpPostMeasures(const AirgradientPayload &payload) {
  if (payload.bufferCount > 1) {
    AG_LOGI(TAG, "WiFi payload cannot handle more than 1 buffer");
    return false;
  }

  JsonDocument jdoc;
  jdoc[JSON_PROP_SIGNAL] = payload.signal;
  _serialize(jdoc, payload.payloadBuffer[0]);

  // Serialize the JSON document to a string
  std::string toSend;
  size_t bytesWritten = serializeJson(jdoc, toSend);
  if (bytesWritten == 0) {
    AG_LOGE(TAG, "Serialize json failed");
    return false;
  }

  return httpPostMeasures(toSend);
}

bool AirgradientWifiClient::_httpGet(const std::string &url, int &responseCode,
                                     std::string &responseBody) {
#ifdef ARDUINO
  // Init http client
  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    AG_LOGE(TAG, "Failed begin HTTPClient using TLS");
    return false;
  }

  responseCode = client.GET();
  responseBody = client.getString().c_str();
  client.end();
  return true;
#else
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.method = HTTP_METHOD_GET;
  config.cert_pem = AG_SERVER_ROOT_CA;

  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (esp_http_client_open(client, 0) != ESP_OK) {
    AG_LOGE(TAG, "Failed perform HTTP GET");
    esp_http_client_cleanup(client);
    return false;
  }
  esp_http_client_fetch_headers(client);
  responseCode = esp_http_client_get_status_code(client);

  int totalRead = 0;
  int readLen;
  while ((readLen = esp_http_client_read(client, responseBuffer + totalRead,
                                         MAX_RESPONSE_BUFFER - totalRead)) > 0) {
    totalRead += readLen;
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  responseBuffer[totalRead] = '\0';
  responseBody = std::string(responseBuffer);

  return true;
#endif
}

bool AirgradientWifiClient::_httpPost(const std::string &url, const std::string &payload,
                                      int &responseCode) {
#ifdef ARDUINO
  HTTPClient client;
  client.setConnectTimeout(timeoutMs); // Set timeout when establishing connection to server
  client.setTimeout(timeoutMs);        // Timeout when waiting for response from AG server
  // By default, airgradient using https
  if (client.begin(String(url.c_str()), AG_SERVER_ROOT_CA) == false) {
    AG_LOGE(TAG, "Failed begin HTTPClient using TLS");
    return false;
  }

  client.addHeader("content-type", "application/json");
  responseCode = client.POST(String(payload.c_str()));
  client.end();
  return true;
#else
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.method = HTTP_METHOD_POST;
  config.cert_pem = AG_SERVER_ROOT_CA;
  config.timeout_ms = timeoutMs;
  esp_http_client_handle_t client = esp_http_client_init(&config);

  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, payload.c_str(), payload.length());

  if (esp_http_client_perform(client) != ESP_OK) {
    AG_LOGE(TAG, "Failed perform HTTP POST");
    esp_http_client_cleanup(client);
    return false;
  }
  responseCode = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  return true;
#endif
}

void AirgradientWifiClient::_serialize(JsonDocument &doc, const PayloadBuffer &payload) {
  // Check and add CO2 value
  if (IS_CO2_VALID(payload.common.rco2)) {
    doc[JSON_PROP_CO2] = payload.common.rco2;
  }

  // Check and add Particle Count
  if (IS_PM_VALID(payload.common.particleCount003)) {
    doc[JSON_PROP_PM03_COUNT] = payload.common.particleCount003;
  }

  // Check and add PM values
  if (IS_PM_VALID(payload.common.pm01)) {
    doc[JSON_PROP_PM01_AE] = payload.common.pm01;
  }
  if (IS_PM_VALID(payload.common.pm25)) {
    doc[JSON_PROP_PM25_AE] = payload.common.pm25;
  }
  if (IS_PM_VALID(payload.common.pm10)) {
    doc[JSON_PROP_PM10_AE] = payload.common.pm10;
  }

  // Check and add TVOC and NOx values
  if (payloadType == MAX_WITH_O3_NO2 || payloadType == MAX_WITHOUT_O3_NO2) {
    // NOTE: currently MAX publish tvoc and nox raw through the index field
    if (IS_TVOC_VALID(payload.common.tvocRaw)) {
      doc[JSON_PROP_TVOC] = payload.common.tvocRaw;
    }
    if (IS_NOX_VALID(payload.common.noxRaw)) {
      doc[JSON_PROP_NOX] = payload.common.noxRaw;
    }
  } else {
    // TODO: Add index
    if (IS_TVOC_VALID(payload.common.tvocRaw)) {
      doc[JSON_PROP_TVOC_RAW] = payload.common.tvocRaw;
    }
    if (IS_NOX_VALID(payload.common.noxRaw)) {
      doc[JSON_PROP_NOX_RAW] = payload.common.noxRaw;
    }
  }

  // Check and add Temperature and Humidity
  if (IS_TEMPERATURE_VALID(payload.common.atmp)) {
    doc[JSON_PROP_TEMP] = payload.common.atmp;
  }
  if (IS_HUMIDITY_VALID(payload.common.rhum)) {
    doc[JSON_PROP_RHUM] = payload.common.rhum;
  }

  // Check and add Voltage-related values
  if (IS_VOLT_VALID(payload.ext.extra.vBat)) {
    doc[JSON_PROP_VBATT] = payload.ext.extra.vBat;
  }
  if (IS_VOLT_VALID(payload.ext.extra.vPanel)) {
    doc[JSON_PROP_VPANEL] = payload.ext.extra.vPanel;
  }
  if (IS_VOLT_VALID(payload.ext.extra.o3WorkingElectrode)) {
    doc[JSON_PROP_O3_WE] = payload.ext.extra.o3WorkingElectrode;
  }
  if (IS_VOLT_VALID(payload.ext.extra.o3AuxiliaryElectrode)) {
    doc[JSON_PROP_O3_AE] = payload.ext.extra.o3AuxiliaryElectrode;
  }
  if (IS_VOLT_VALID(payload.ext.extra.no2WorkingElectrode)) {
    doc[JSON_PROP_NO2_WE] = payload.ext.extra.no2WorkingElectrode;
  }
  if (IS_VOLT_VALID(payload.ext.extra.no2AuxiliaryElectrode)) {
    doc[JSON_PROP_NO2_AE] = payload.ext.extra.no2AuxiliaryElectrode;
  }
  if (IS_VOLT_VALID(payload.ext.extra.afeTemp)) {
    doc[JSON_PROP_AFE_TEMP] = payload.ext.extra.afeTemp;
  }
}

#endif // ESP8266
