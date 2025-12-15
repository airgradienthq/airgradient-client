/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "airgradientClient.h"
#include <cmath>
#ifndef ESP8266

#include "airgradientCellularClient.h"
#include <sstream>
#include "cellularModule.h"
#include "common.h"
#include "agLogger.h"
#include "config.h"

#include "CoapError.h"
#include "CoapPacket.h"
#include "CoapBuilder.h"
#include "CoapParser.h"
#include "CoapTypes.h"

#define ONE_OPENAIR_POST_MEASURES_ENDPOINT "cts"
#define OPENAIR_MAX_POST_MEASURES_ENDPOINT "cvn"
#ifdef ARDUINO
#define POST_MEASURES_ENDPOINT ONE_OPENAIR_POST_MEASURES_ENDPOINT
#else
#define POST_MEASURES_ENDPOINT OPENAIR_MAX_POST_MEASURES_ENDPOINT
#endif

AirgradientCellularClient::AirgradientCellularClient(CellularModule *cellularModule)
    : cell_(cellularModule) {}

bool AirgradientCellularClient::begin(std::string sn, PayloadType pt) {
  // Update parent serialNumber variable
  serialNumber = sn;
  payloadType = pt;
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

void AirgradientCellularClient::setAPN(const std::string &apn) { _apn = apn; }

void AirgradientCellularClient::setExtendedPmMeasures(bool enable) { _extendedPmMeasures = enable; }

void AirgradientCellularClient::setNetworkRegistrationTimeoutMs(int timeoutMs) {
  _networkRegistrationTimeoutMs = timeoutMs;
  AG_LOGI(TAG, "Timeout set to %d seconds", (_networkRegistrationTimeoutMs / 1000));
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
  // Format url
  char url[80] = {0};
  sprintf(url, "http://%s/sensors/%s/%s", httpDomain.c_str(), serialNumber.c_str(),
          _getEndpoint().c_str());

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

bool AirgradientCellularClient::httpPostMeasures(const AirgradientPayload &payload) {
  // Build payload using oss, easier to manage if there's an invalid value that should not included
  std::ostringstream oss;

  // Add interval at the first position
  oss << payload.measureInterval;

  if (payloadType == MAX_WITH_O3_NO2 || payloadType == MAX_WITHOUT_O3_NO2) {
    auto *sensor = static_cast<std::vector<MaxSensorPayload> *>(payload.sensor);
    for (auto it = sensor->begin(); it != sensor->end(); ++it) {
      // Seperator between measures cycle
      oss << ",";
      // Serialize each measurement
      _serialize(oss, it->rco2, it->particleCount003, it->pm01, it->pm25, it->pm10, it->tvocRaw,
                 it->noxRaw, it->atmp, it->rhum, payload.signal, it->vBat, it->vPanel,
                 it->o3WorkingElectrode, it->o3AuxiliaryElectrode, it->no2WorkingElectrode,
                 it->no2AuxiliaryElectrode, it->afeTemp, it->particleCount005, it->particleCount01,
                 it->particleCount02, it->particleCount50, it->particleCount10, it->pm25Sp);
    }
  } else {
    // TODO: Add for OneOpenAir payload
  }

  // Compile it
  std::string payloadStr = oss.str();

  return httpPostMeasures(payloadStr);
}

bool AirgradientCellularClient::mqttConnect() { return mqttConnect(mqttDomain, mqttPort); }

bool AirgradientCellularClient::mqttConnect(const char *uri) {
  // Get connection properties from uri; Eg: mqtt://username:password@mqttbroker.com:1883
  std::string protocol;
  std::string username;
  std::string password;
  std::string host;
  int port = -1;
  Common::parseUri(uri, protocol, username, password, host, port);

  if (host.empty()) {
    AG_LOGE(TAG, "MQTT host or port is empty");
    return false;
  }

  if (port == -1) {
    port = 1883;
  }

  return mqttConnect(host, port, username, password);
}

bool AirgradientCellularClient::mqttConnect(const std::string &host, int port, std::string username,
                                            std::string password) {

  AG_LOGI(TAG, "Attempt connection to MQTT broker: %s:%d", host.c_str(), port);
  auto result = cell_->mqttConnect(serialNumber, host, port, username, password);
  if (result != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed connect to mqtt broker");
    return false;
  }
  AG_LOGI(TAG, "Success connect to mqtt broker");

  return true;
}

bool AirgradientCellularClient::mqttDisconnect() {
  if (cell_->mqttDisconnect() != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed disconnect from mqtt broker");
    return false;
  }
  AG_LOGI(TAG, "Success disconnect from mqtt broker");

  return true;
}

bool AirgradientCellularClient::mqttPublishMeasures(const std::string &payload) {
  // TODO: Ensure mqtt connection
  auto topic = buildMqttTopicPublishMeasures();
  AG_LOGI(TAG, "Publish to %s", topic.c_str());
  AG_LOGI(TAG, "Payload: %s", payload.c_str());
  auto result = cell_->mqttPublish(topic, payload);
  if (result != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed publish measures to mqtt server");
    return false;
  }
  AG_LOGI(TAG, "Success publish measures to mqtt server");

  return true;
}

bool AirgradientCellularClient::mqttPublishMeasures(const AirgradientPayload &payload) {

  // Build payload using oss, easier to manage if there's an invalid value that should not included
  std::ostringstream oss;

  // Add interval at the first position
  oss << payload.measureInterval;

  if (payloadType == MAX_WITH_O3_NO2 || payloadType == MAX_WITHOUT_O3_NO2) {
    auto *sensor = static_cast<std::vector<MaxSensorPayload> *>(payload.sensor);
    for (auto it = sensor->begin(); it != sensor->end(); ++it) {
      // Seperator between measures cycle
      oss << ",";
      // Serialize each measurement
      _serialize(oss, it->rco2, it->particleCount003, it->pm01, it->pm25, it->pm10, it->tvocRaw,
                 it->noxRaw, it->atmp, it->rhum, payload.signal, it->vBat, it->vPanel,
                 it->o3WorkingElectrode, it->o3AuxiliaryElectrode, it->no2WorkingElectrode,
                 it->no2AuxiliaryElectrode, it->afeTemp);
    }
  } else {
    // TODO: Add for OneOpenAir payload
  }

  // Compile it
  std::string toSend = oss.str();

  return mqttPublishMeasures(toSend);
}

std::string AirgradientCellularClient::coapFetchConfig(bool keepConnection) {
  if (!_coapConnect()) {
    lastFetchConfigSucceed = false;
    return {};
  }

  CoapPacket::CoapBuilder builder;
  std::vector<uint8_t> buffer;
  uint8_t token[] = {0x12, 0x34};
  uint16_t messageId = 1234;
  auto err = builder.setType(CoapPacket::CoapType::CON)
                 .setCode(CoapPacket::CoapCode::GET)
                 .setMessageId(messageId)
                 .setToken(token, 2)
                 .setUriPath(serialNumber)
                 .buildBuffer(buffer);
  if (err != CoapPacket::CoapError::OK) {
    AG_LOGE(TAG, "CoAP fetch config packet build failed %d", CoapPacket::getErrorMessage(err));
    return {};
  }

  // TODO: Add URI to the path
  AG_LOGI(TAG, "CoAP fetch configuration from %s:%d", coapDomain, coapPort);

  CoapPacket::CoapPacket responsePacket;
  bool success = _coapRequestWithRetry(buffer, messageId, token, 2, &responsePacket);
  if (!success) {
    lastFetchConfigSucceed = false;
    return {};
  }

  // Check request response code
  uint8_t codeClass = CoapPacket::getCodeClass(responsePacket.code);
  uint8_t codeDetail = CoapPacket::getCodeDetail(responsePacket.code);
  if (codeClass != 2) {
    AG_LOGE(TAG, "CoAP fetch configuration response failed (%d.%02d)", codeClass, codeDetail);
    if (codeClass == 4) {
      // Return code 400 means device not registered on ag server
      registeredOnAgServer = false;
    }
    lastFetchConfigSucceed = false;
    return {};
  }

  std::string response(responsePacket.payload.begin(), responsePacket.payload.end());
  AG_LOGI(TAG, "Received configuration: (%d) %s", response.length(), response.c_str());

  // Set state to succeed
  lastFetchConfigSucceed = true;
  registeredOnAgServer = true;

  // Handling disconnection decision
  _coapDisconnect(keepConnection);

  AG_LOGI(TAG, "Success fetch configuration from server, still needs to be parsed and validated");
  return response;
}

bool AirgradientCellularClient::coapPostMeasures(const std::string &payload, bool keepConnection) {
  if (!_coapConnect()) {
    lastPostMeasuresSucceed = false;
    return false;
  }

  CoapPacket::CoapBuilder builder;
  std::vector<uint8_t> buffer;
  uint8_t token[] = {0x13, 0x35};
  uint16_t messageId = 1234;
  auto err = builder.setType(CoapPacket::CoapType::CON)
                 .setCode(CoapPacket::CoapCode::POST)
                 .setMessageId(messageId)
                 .setToken(token, 2)
                 .setUriPath(serialNumber)
                 .setContentFormat(CoapPacket::CoapContentFormat::TEXT_PLAIN)
                 .setPayload(payload)
                 .buildBuffer(buffer);
  if (err != CoapPacket::CoapError::OK) {
    AG_LOGE(TAG, "CoAP post measures packet build failed %d", CoapPacket::getErrorMessage(err));
    lastPostMeasuresSucceed = false;
    return false;
  }

  AG_LOGI(TAG, "CoAP post measures to %s:%d", coapDomain, coapPort); // TODO: Add path
  AG_LOGI(TAG, "Payload: %s", payload.c_str());

  CoapPacket::CoapPacket responsePacket;
  bool success = _coapRequestWithRetry(buffer, messageId, token, 2, &responsePacket);
  if (!success) {
    AG_LOGE(TAG, "CoAP post measures request failed");
    lastPostMeasuresSucceed = false;
    return false;
  }

  // Check request response code
  uint8_t codeClass = CoapPacket::getCodeClass(responsePacket.code);
  uint8_t codeDetail = CoapPacket::getCodeDetail(responsePacket.code);
  if (codeClass != 2) {
    AG_LOGE(TAG, "CoAP post measures response failed (%d.%02d)", codeClass, codeDetail);
    lastPostMeasuresSucceed = false;
    return false;
  }

  AG_LOGI(TAG, "CoAP post measures response success (%d.%02d)", codeClass, codeDetail);
  lastPostMeasuresSucceed = true;

  // TODO: Define propse clientReady state for CoAP

  // Handling disconnection decision
  _coapDisconnect(keepConnection);

  return true;
}

bool AirgradientCellularClient::_coapConnect() {
  if (_isCoapConnected) {
    AG_LOGI(TAG, "CoAP already connected");
    return true;
  }

  if (cell_->udpConnect(coapDomain, coapPort) != CellReturnStatus::Ok) {
    AG_LOGI(TAG, "Failed connect to CoAP server");
    return false;
  }

  _isCoapConnected = true;
  return true;
}

void AirgradientCellularClient::_coapDisconnect(bool keepConnection) {
  if (keepConnection) {
    _isCoapConnected = true;
    return;
  }

  if (cell_->udpDisconnect() == CellReturnStatus::Ok) {
    _isCoapConnected = false;
    return;
  }

  AG_LOGI(TAG, "Failed disconnect to CoAP server");
    _isCoapConnected = true;
  // TODO: Do a force disconnection or something
}

bool AirgradientCellularClient::_coapRequest(const std::vector<uint8_t> &reqBuffer,
                                             uint16_t expectedMessageId,
                                             const uint8_t *expectedToken, uint8_t expectedTokenLen,
                                             CoapPacket::CoapPacket *respPacket, int timeoutMs) {
  // 1. Prepare UDP packet from request buffer
  CellularModule::UdpPacket udpPacket;
  udpPacket.size = reqBuffer.size();
  udpPacket.buff = std::move(reqBuffer); // Move buffer, not copy

  // 2. Send request
  if (cell_->udpSend(udpPacket, coapDomain, coapPort) != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed to send CoAP request via UDP");
    return false;
  }

  AG_LOGI(TAG, "CoAP request sent, waiting for response...");

  // 3. Receive response
  auto response = cell_->udpReceive(timeoutMs);
  if (response.status != CellReturnStatus::Ok) {
    AG_LOGE(TAG, "Failed to receive CoAP response (timeout or error)");
    return false;
  }

  // 4. Parse response
  CoapPacket::CoapError parseErr = CoapPacket::CoapParser::parse(response.data.buff, *respPacket);
  if (parseErr != CoapPacket::CoapError::OK) {
    AG_LOGE(TAG, "Failed to parse CoAP response: %s", CoapPacket::getErrorMessage(parseErr));
    return false;
  }

  // 5. Validate message ID
  if (respPacket->message_id != expectedMessageId) {
    AG_LOGW(TAG, "Response message ID mismatch: expected %d, got %d", expectedMessageId,
            respPacket->message_id);
    return false;
  }

  AG_LOGD(TAG, "Message ID validated");

  // 6. Handle response type and validate token appropriately
  if (respPacket->type == CoapPacket::CoapType::ACK &&
      respPacket->code == CoapPacket::CoapCode::EMPTY) {

    // Empty ACK - Separate response pattern
    // Do NOT validate token on Empty ACK (it has no token)
    AG_LOGI(TAG, "Received empty ACK (Separate response pattern), waiting for actual response...");

    // Receive separate response
    auto separateResp = cell_->udpReceive(timeoutMs);
    if (separateResp.status != CellReturnStatus::Ok) {
      AG_LOGE(TAG, "Failed to receive separate CoAP response");
      return false;
    }

    // Parse separate response
    parseErr = CoapPacket::CoapParser::parse(separateResp.data.buff, *respPacket);
    if (parseErr != CoapPacket::CoapError::OK) {
      AG_LOGE(TAG, "Failed to parse separate CoAP response: %s",
              CoapPacket::getErrorMessage(parseErr));
      return false;
    }

    // NOW validate token on the actual separate response (message ID may differ)
    if (respPacket->token_length != expectedTokenLen) {
      AG_LOGW(TAG, "Separate response token length mismatch: expected %d, got %d", expectedTokenLen,
              respPacket->token_length);
      return false;
    }

    for (uint8_t i = 0; i < expectedTokenLen; i++) {
      if (respPacket->token[i] != expectedToken[i]) {
        AG_LOGW(TAG, "Separate response token mismatch at byte %d", i);
        return false;
      }
    }

    AG_LOGD(TAG, "Separate response received and token validated");

    // If separate response is CON, send ACK back
    if (respPacket->type == CoapPacket::CoapType::CON) {
      AG_LOGI(TAG, "Separate response is CON, sending ACK...");

      // Build ACK inline (ACK with EMPTY code, no token per RFC 7252)
      CoapPacket::CoapBuilder ackBuilder;
      std::vector<uint8_t> ackBuffer;

      auto err = ackBuilder.setType(CoapPacket::CoapType::ACK)
                     .setCode(CoapPacket::CoapCode::EMPTY)
                     .setMessageId(respPacket->message_id)
                     .buildBuffer(ackBuffer);

      if (err == CoapPacket::CoapError::OK) {
        CellularModule::UdpPacket ackPacket;
        ackPacket.size = ackBuffer.size();
        ackPacket.buff = std::move(ackBuffer);

        if (cell_->udpSend(ackPacket, coapDomain, coapPort) == CellReturnStatus::Ok) {
          AG_LOGD(TAG, "ACK sent for separate CON response");
        } else {
          AG_LOGW(TAG, "Failed to send ACK for separate CON response");
        }
      } else {
        AG_LOGW(TAG, "Failed to build ACK packet: %s", CoapPacket::getErrorMessage(err));
      }
    }
  } else {
    // Piggyback ACK or direct CON response
    // Validate token NOW (not Empty ACK, so token should be present)
    if (respPacket->token_length != expectedTokenLen) {
      AG_LOGW(TAG, "Response token length mismatch: expected %d, got %d", expectedTokenLen,
              respPacket->token_length);
      return false;
    }

    for (uint8_t i = 0; i < expectedTokenLen; i++) {
      if (respPacket->token[i] != expectedToken[i]) {
        AG_LOGW(TAG, "Response token mismatch at byte %d", i);
        return false;
      }
    }

    AG_LOGD(TAG, "Response token validated");

    // If CON response, send ACK
    if (respPacket->type == CoapPacket::CoapType::CON) {
      AG_LOGD(TAG, "Received CON response, sending ACK...");

      // Build and send ACK (ACK with EMPTY code, no token per RFC 7252)
      CoapPacket::CoapBuilder ackBuilder;
      std::vector<uint8_t> ackBuffer;

      auto err = ackBuilder.setType(CoapPacket::CoapType::ACK)
                     .setCode(CoapPacket::CoapCode::EMPTY)
                     .setMessageId(respPacket->message_id)
                     .buildBuffer(ackBuffer);

      if (err == CoapPacket::CoapError::OK) {
        CellularModule::UdpPacket ackPacket;
        ackPacket.size = ackBuffer.size();
        ackPacket.buff = std::move(ackBuffer);

        if (cell_->udpSend(ackPacket, coapDomain, coapPort) == CellReturnStatus::Ok) {
          AG_LOGI(TAG, "ACK sent for CON response");
        } else {
          AG_LOGW(TAG, "Failed to send ACK for CON response");
        }
      } else {
        AG_LOGW(TAG, "Failed to build ACK packet: %s", CoapPacket::getErrorMessage(err));
      }
    }
    // Otherwise it's a piggyback ACK (Type=ACK with response code) - no ACK needed
  }

  AG_LOGI(TAG, "CoAP request successful");
  return true;
}

bool AirgradientCellularClient::_coapRequestWithRetry(
    const std::vector<uint8_t> &reqBuffer, uint16_t expectedMessageId, const uint8_t *expectedToken,
    uint8_t expectedTokenLen, CoapPacket::CoapPacket *respPacket, int timeoutMs, int maxRetries) {
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    AG_LOGI(TAG, "CoAP request attempt %d/%d", attempt, maxRetries);

    if (_coapRequest(reqBuffer, expectedMessageId, expectedToken, expectedTokenLen, respPacket,
                     timeoutMs)) {
      // Success!
      return true;
    }

    // Failed, log and retry if attempts remain
    if (attempt < maxRetries) {
      AG_LOGW(TAG, "CoAP request failed, retrying...");
    }
  }

  // All attempts failed
  AG_LOGE(TAG, "CoAP request failed after %d attempts", maxRetries);
  clientReady = false;
  return false;
}

void AirgradientCellularClient::_serialize(
    std::ostringstream &oss, int rco2, int particleCount003, float pm01, float pm25, float pm10,
    int tvoc, int nox, float atmp, float rhum, int signal, float vBat, float vPanel,
    float o3WorkingElectrode, float o3AuxiliaryElectrode, float no2WorkingElectrode,
    float no2AuxiliaryElectrode, float afeTemp, int particleCount005, int particleCount01,
    int particleCount02, int particleCount50, int particleCount10, float pm25Sp) {
  // CO2
  if (IS_CO2_VALID(rco2)) {
    oss << std::round(rco2);
  }
  oss << ",";
  // Temperature
  if (IS_TEMPERATURE_VALID(atmp)) {
    oss << std::round(atmp * 10);
  }
  oss << ",";
  // Humidity
  if (IS_HUMIDITY_VALID(rhum)) {
    oss << std::round(rhum * 10);
  }
  oss << ",";
  // PM1.0 atmospheric environment
  if (IS_PM_VALID(pm01)) {
    oss << std::round(pm01 * 10);
  }
  oss << ",";
  // PM2.5 atmospheric environment
  if (IS_PM_VALID(pm25)) {
    oss << std::round(pm25 * 10);
  }
  oss << ",";
  // PM10 atmospheric environment
  if (IS_PM_VALID(pm10)) {
    oss << std::round(pm10 * 10);
  }
  oss << ",";
  // TVOC
  if (IS_TVOC_VALID(tvoc)) {
    oss << tvoc;
  }
  oss << ",";
  // NOx
  if (IS_NOX_VALID(nox)) {
    oss << nox;
  }
  oss << ",";
  // PM 0.3 particle count
  if (IS_PM_VALID(particleCount003)) {
    oss << particleCount003;
  }
  oss << ",";
  // Radio signal
  oss << signal;

  // Only continue for MAX model
  if (payloadType == MAX_WITH_O3_NO2 || payloadType == MAX_WITHOUT_O3_NO2) {
    oss << ",";
    // V Battery
    if (IS_VOLT_VALID(vBat)) {
      oss << std::round(vBat * 100);
    }
    oss << ",";
    // V Solar Panel
    if (IS_VOLT_VALID(vPanel)) {
      oss << std::round(vPanel * 100);
    }

    if (payloadType == MAX_WITH_O3_NO2) {
      oss << ",";
      // Working Electrode O3
      if (IS_VOLT_VALID(o3WorkingElectrode)) {
        oss << std::round(o3WorkingElectrode * 1000);
      }
      oss << ",";
      // Auxiliary Electrode O3
      if (IS_VOLT_VALID(o3AuxiliaryElectrode)) {
        oss << std::round(o3AuxiliaryElectrode * 1000);
      }
      oss << ",";
      // Working Electrode NO2
      if (IS_VOLT_VALID(no2WorkingElectrode)) {
        oss << std::round(no2WorkingElectrode * 1000);
      }
      oss << ",";
      // Auxiliary Electrode NO2
      if (IS_VOLT_VALID(no2AuxiliaryElectrode)) {
        oss << std::round(no2AuxiliaryElectrode * 1000);
      }
      oss << ",";
      // AFE Temperature
      if (IS_VOLT_VALID(afeTemp)) {
        oss << std::round(afeTemp * 10);
      }
    }
  }

  if (!_extendedPmMeasures) {
    return;
  }

  // Extended measures
  oss << ",";
  // PM 0.5 particle count
  if (IS_PM_VALID(particleCount005)) {
    oss << particleCount005;
  }
  oss << ",";
  // PM 1.0 particle count
  if (IS_PM_VALID(particleCount01)) {
    oss << particleCount01;
  }
  oss << ",";
  // PM 2.5 particle count
  if (IS_PM_VALID(particleCount02)) {
    oss << particleCount02;
  }
  oss << ",";
  // PM 5.0 particle count
  if (IS_PM_VALID(particleCount50)) {
    oss << particleCount50;
  }
  oss << ",";
  // PM 10 particle count
  if (IS_PM_VALID(particleCount10)) {
    oss << particleCount10;
  }
  oss << ",";
  // PM 2.5 standard particle
  if (IS_PM_VALID(pm25Sp)) {
    oss << std::round(pm25Sp);
  }
}

std::string AirgradientCellularClient::_getEndpoint() {
  if (_extendedPmMeasures) {
    return "cpm"; // special case
  }

  std::string endpoint;
  switch (payloadType) {
  case AirgradientClient::MAX_WITHOUT_O3_NO2:
    endpoint = "cvl";
    break;
  case AirgradientClient::MAX_WITH_O3_NO2:
    endpoint = "cvn";
    break;
  case AirgradientClient::ONE_OPENAIR:
  case AirgradientClient::ONE_OPENAIR_TWO_PMS:
    endpoint = "cts";
    break;
  };

  return endpoint;
}

#endif // ESP8266
