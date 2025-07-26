/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include <cmath>
#ifndef ESP8266

#include "airgradientCellularClient.h"
#include <sstream>
#include "cellularModule.h"
#include "common.h"
#include "agLogger.h"
#include "config.h"

// TODO: Use kconfig with default ONE_OPENAIR
#define ONE_OPENAIR_POST_MEASURES_ENDPOINT "cts"
#define OPENAIR_MAX_POST_MEASURES_ENDPOINT "cvn"
#define POST_MEASURES_ENDPOINT OPENAIR_MAX_POST_MEASURES_ENDPOINT

AirgradientCellularClient::AirgradientCellularClient(CellularModule *cellularModule)
    : cell_(cellularModule) {}

bool AirgradientCellularClient::begin(std::string sn) {
  // Update parent serialNumber variable
  serialNumber = sn;
  clientReady = false;

  if (!cell_->init()) {
    AG_LOGE(TAG, "Cannot initialized cellular client");
    return false;
  }

  // To make sure module ready to use
  if (cell_->isSimReady() != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "SIM is not ready, please check if SIM is inserted properly!");
    return false;
  }

  // Printout SIM CCID
  auto result = cell_->retrieveSimCCID();
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed get SIM CCID, please check if SIM is inserted properly!");
    return false;
  }
  _iccid = result.data;
  AG_LOGI(TAG, "SIM CCID: %s", result.data.c_str());

  // Register network
  result =
      cell_->startNetworkRegistration(CellTechnology::Auto, _apn, _networkRegistrationTimeoutMs);
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Cellular client failed, module cannot register to network");
    return false;
  }

  AG_LOGI(TAG, "Cellular client ready, module registered to network");
  clientReady = true;

  return true;
}

void AirgradientCellularClient::setNetworkRegistrationTimeoutMs(int timeoutMs) {
  _networkRegistrationTimeoutMs = timeoutMs;
  ESP_LOGI(TAG, "Timeout set to %d seconds", (_networkRegistrationTimeoutMs / 1000));
}

std::string AirgradientCellularClient::getICCID() { return _iccid; }

bool AirgradientCellularClient::ensureClientConnection(bool reset) {
  AG_LOGE(TAG, "Ensuring client connection, restarting cellular module");
  if (reset) {
    if (cell_->reset() == false) {
      AG_LOGW(TAG, "Reset failed, power cycle module...");
      cell_->powerOff(true);
      DELAY_MS(2000);
      cell_->powerOn();
    }

    AG_LOGI(TAG, "Wait for 10s for module to warming up");
    DELAY_MS(10000);
  }

  if (cell_->reinitialize() != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed to reinitialized the cellular module");
    clientReady = false;
    return false;
  }

  // Register network
  auto result =
      cell_->startNetworkRegistration(CellTechnology::Auto, _apn, _networkRegistrationTimeoutMs);
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Cellular client failed, module cannot register to network");
    clientReady = false;
    return false;
  }

  clientReady = true;
  AG_LOGI(TAG, "Cellular client ready, module registered to network");

  return true;
}

std::string AirgradientCellularClient::httpFetchConfig() {
  std::string url = buildFetchConfigUrl();
  AG_LOGI(TAG, "Fetch configuration from %s", url.c_str());

  auto result = cell_->httpGet(url); // TODO: Define timeouts
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Module not return OK when call httpGet()");
    lastFetchConfigSucceed = false;
    clientReady = false;
    return {};
  }

  // Reset client ready state
  clientReady = true;

  // Response status check if fetch failed
  if (result.data.statusCode != 200) {
    AG_LOGW(TAG, "Failed fetch configuration from server with return code %d",
            result.data.statusCode);
    // Return code 400 means device not registered on ag server
    if (result.data.statusCode == 400) {
      registeredOnAgServer = false;
    }
    lastFetchConfigSucceed = false;

    return {};
  }

  // Set success state flag
  registeredOnAgServer = true;
  lastFetchConfigSucceed = true;

  // Sanity check if response body is empty
  if (result.data.bodyLen == 0 || result.data.body == nullptr) {
    // TODO: How to handle this? perhaps cellular module failed to read the buffer
    AG_LOGW(TAG, "Success fetch configuration from server but somehow body is empty");
    return {};
  }

  AG_LOGI(TAG, "Received configuration: (%d) %s", result.data.bodyLen, result.data.body.get());

  // Move the string from unique_ptr
  std::string body = std::string(result.data.body.get());

  AG_LOGI(TAG, "Success fetch configuration from server, still needs to be parsed and validated");

  return body;
}

bool AirgradientCellularClient::httpPostMeasures(const std::string &payload) {
  // std::string url = buildPostMeasuresUrl();
  char url[80] = {0};
  sprintf(url, "http://%s/sensors/%s/%s", httpDomain.c_str(), serialNumber.c_str(),
          POST_MEASURES_ENDPOINT);
  AG_LOGI(TAG, "Post measures to %s", url);
  AG_LOGI(TAG, "Payload: %s", payload.c_str());

  auto result = cell_->httpPost(url, payload); // TODO: Define timeouts
  if (result.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Module not return OK when call httpPost()");
    lastPostMeasuresSucceed = false;
    clientReady = false;
    return false;
  }

  // Reset client ready state
  clientReady = true;

  // Response status check if post failed
  if ((result.data.statusCode != 200) && (result.data.statusCode != 429) &&
      (result.data.statusCode != 201)) {
    AG_LOGW(TAG, "Failed post measures to server with response code %d", result.data.statusCode);
    lastPostMeasuresSucceed = false;
    return false;
  }

  lastPostMeasuresSucceed = true;
  AG_LOGI(TAG, "Success post measures to server with response code %d", result.data.statusCode);

  return true;
}

bool AirgradientCellularClient::httpPostMeasures(int measureInterval,
                                                 std::vector<OpenAirMaxPayload> data) {
  // Build payload using oss, easier to manage if there's an invalid value that should not included
  std::ostringstream payload;

  // Add interval at the first position
  payload << measureInterval;

  for (const OpenAirMaxPayload &v : data) {
    // Seperator between measures cycle
    payload << ",";
    // CO2
    if (IS_CO2_VALID(v.rco2)) {
      payload << std::round(v.rco2);
    }
    payload << ",";
    // Temperature
    if (IS_TEMPERATURE_VALID(v.atmp)) {
      payload << std::round(v.atmp * 10);
    }
    payload << ",";
    // Humidity
    if (IS_HUMIDITY_VALID(v.rhum)) {
      payload << std::round(v.rhum * 10);
    }
    payload << ",";
    // PM1.0 atmospheric environment
    if (IS_PM_VALID(v.pm01)) {
      payload << std::round(v.pm01 * 10);
    }
    payload << ",";
    // PM2.5 atmospheric environment
    if (IS_PM_VALID(v.pm25)) {
      payload << std::round(v.pm25 * 10);
    }
    payload << ",";
    // PM10 atmospheric environment
    if (IS_PM_VALID(v.pm10)) {
      payload << std::round(v.pm10 * 10);
    }
    payload << ",";
    // TVOC
    if (IS_TVOC_VALID(v.tvocRaw)) {
      payload << v.tvocRaw;
    }
    payload << ",";
    // NOx
    if (IS_NOX_VALID(v.noxRaw)) {
      payload << v.noxRaw;
    }
    payload << ",";
    // PM 0.3 particle count
    if (IS_PM_VALID(v.particleCount003)) {
      payload << v.particleCount003;
    }
    payload << ",";
    // Radio signal
    if (v.signal > 0) {
      int dbm = cell_->csqToDbm(v.signal);
      if (dbm != 0) {
        payload << dbm;
      }
    }
    payload << ",";
    // V Battery
    if (IS_VOLT_VALID(v.vBat)) {
      payload << std::round(v.vBat * 100);
    }
    payload << ",";
    // V Solar Panel
    if (IS_VOLT_VALID(v.vPanel)) {
      payload << std::round(v.vPanel * 100);
    }
    payload << ",";
    // Working Electrode O3
    if (IS_VOLT_VALID(v.o3WorkingElectrode)) {
      payload << std::round(v.o3WorkingElectrode * 1000);
    }
    payload << ",";
    // Auxiliary Electrode O3
    if (IS_VOLT_VALID(v.o3AuxiliaryElectrode)) {
      payload << std::round(v.o3AuxiliaryElectrode * 1000);
    }
    payload << ",";
    // Working Electrode NO2
    if (IS_VOLT_VALID(v.no2WorkingElectrode)) {
      payload << std::round(v.no2WorkingElectrode * 1000);
    }
    payload << ",";
    // Auxiliary Electrode NO2
    if (IS_VOLT_VALID(v.no2AuxiliaryElectrode)) {
      payload << std::round(v.no2AuxiliaryElectrode * 1000);
    }
    payload << ",";
    // AFE Temperature
    if (IS_VOLT_VALID(v.afeTemp)) {
      payload << std::round(v.afeTemp * 10);
    }
  }

  // Compile it
  std::string toSend = payload.str();

  return httpPostMeasures(toSend);
}

bool AirgradientCellularClient::mqttConnect() {
  auto result = cell_->mqttConnect(serialNumber, mqttDomain, mqttPort);
  if (result != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed connect to airgradient mqtt server");
    return false;
  }
  AG_LOGI(TAG, "Success connect to airgardient mqtt server");

  return true;
}

bool AirgradientCellularClient::mqttDisconnect() {
  if (cell_->mqttDisconnect() != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed disconnect from airgradient mqtt server");
    return false;
  }
  AG_LOGI(TAG, "Success disconnect from airgradient mqtt server");

  return true;
}

bool AirgradientCellularClient::mqttPublishMeasures(const std::string &payload) {
  // TODO: Ensure mqtt connection
  auto topic = buildMqttTopicPublishMeasures();
  auto result = cell_->mqttPublish(topic, payload);
  if (result != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed publish measures to mqtt server");
    return false;
  }
  AG_LOGI(TAG, "Success publish measures to mqtt server");

  return true;
}

#endif // ESP8266
