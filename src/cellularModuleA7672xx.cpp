/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef ESP8266

#include <cstdint>
#include <memory>

#include "common.h"
#include "agLogger.h"
#include "agSerial.h"
#include "cellularModuleA7672xx.h"
#include "cellularModule.h"
#include "atCommandHandler.h"
#include "cellularModule.h"

#define REGIS_RETRY_DELAY() DELAY_MS(1000);

CellularModuleA7672XX::CellularModuleA7672XX(AgSerial *agSerial) : agSerial_(agSerial) {}

CellularModuleA7672XX::CellularModuleA7672XX(AgSerial *agSerial, int powerPin) {
  agSerial_ = agSerial;
  _powerIO = static_cast<gpio_num_t>(powerPin);
}

CellularModuleA7672XX::~CellularModuleA7672XX() {
  if (at_ != nullptr) {
    delete at_;
    at_ = nullptr;
  }
}

bool CellularModuleA7672XX::init() {
  if (_initialized) {
    AG_LOGI(TAG, "Already initialized");
    return true;
  }

  if (_powerIO != GPIO_NUM_NC) {
    gpio_reset_pin(_powerIO);
    gpio_set_direction(_powerIO, GPIO_MODE_OUTPUT);
    powerOn();
  }

  //! Here assume agSerial_ already initialized and opened
  //! NO! it should initialized here! Right?
  // TODO: Add sanity check

  // Initialize cellular module and wait for module to ready
  at_ = new ATCommandHandler(agSerial_);
  AG_LOGI(TAG, "Checking module readiness...");
  if (!at_->testAT()) {
    AG_LOGW(TAG, "Failed wait cellular module to ready");
    delete at_;
    return false;
  }

  // Reset module, to reset previous session
  // TODO: Add option to reset or not
  // reset();
  // DELAY_MS(20000);

  // TODO: Need to validate the response?
  // Disable echo
  at_->sendAT("E0");
  at_->waitResponse();
  DELAY_MS(2000);

  // TODO: Need to validate the response?
  // Disable GPRS event reporting (URC)
  at_->sendAT("+CGEREP=0");
  at_->waitResponse();
  DELAY_MS(2000);

  _initialized = true;
  return true;
}

void CellularModuleA7672XX::powerOn() {
  gpio_set_level(_powerIO, 0);
  DELAY_MS(500);
  gpio_set_level(_powerIO, 1);
  DELAY_MS(100);
  gpio_set_level(_powerIO, 0);
  DELAY_MS(100);
}

void CellularModuleA7672XX::powerOff() {
  at_->sendAT("+CPOF");
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // Force power off
    AG_LOGW(TAG, "Force module to power off");
    gpio_set_level(_powerIO, 1);
    DELAY_MS(1300);
    gpio_set_level(_powerIO, 0);
    return;
  }

  AG_LOGI(TAG, "Module powered off");
}

bool CellularModuleA7672XX::reset() {
  at_->sendAT("+CRESET");
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    AG_LOGW(TAG, "Failed reset module");
    return false;
  }

  AG_LOGI(TAG, "Success reset module");
  return true;
}

void CellularModuleA7672XX::sleep() {}

CellResult<std::string> CellularModuleA7672XX::getModuleInfo() { return CellResult<std::string>(); }

CellResult<std::string> CellularModuleA7672XX::retrieveSimCCID() {
  CellResult<std::string> result;
  result.status = CellReturnStatus::Timeout;

  at_->sendAT("+CICCID");
  if (at_->waitResponse("+ICCID:") != ATCommandHandler::ExpArg1) {
    return result;
  }

  std::string ccid;
  if (at_->waitAndRecvRespLine(ccid) == -1) {
    return result;
  }

  // receive OK response from the buffer, ignore it
  at_->waitResponse();

  result.status = CellReturnStatus::Ok;
  result.data = ccid;

  return result;
}

CellReturnStatus CellularModuleA7672XX::isSimReady() {
  at_->sendAT("+CPIN?");
  if (at_->waitResponse("+CPIN:") != ATCommandHandler::ExpArg1) {
    return CellReturnStatus::Timeout;
  }

  // NOTE: Add other possible response and maybe add an enum then set it to result.data
  if (at_->waitResponse("READY") != ATCommandHandler::ExpArg1) {
    return CellReturnStatus::Failed;
  }

  // receive OK response from the buffer, ignore it
  at_->waitResponse();

  return CellReturnStatus::Ok;
}

CellResult<int> CellularModuleA7672XX::retrieveSignal() {
  CellResult<int> result;
  result.status = CellReturnStatus::Timeout;

  at_->sendAT("+CSQ");
  if (at_->waitResponse("+CSQ:") != ATCommandHandler::ExpArg1) {
    return result;
  }

  std::string received;
  if (at_->waitAndRecvRespLine(received) == -1) {
    return result;
  }

  // TODO: Make common function to seperate two values by comma
  int signal = 99;
  size_t pos = received.find(",");
  if (pos != std::string::npos) {
    // Ignore <ber> value, only <rssi>
    std::string signalStr = received.substr(0, pos);
    signal = std::stoi(signalStr);
  }

  // receive OK response from the buffer, ignore it
  at_->waitResponse();

  result.status = CellReturnStatus::Ok;
  result.data = signal;

  return result;
}

CellResult<std::string> CellularModuleA7672XX::retrieveIPAddr() {
  // CGPADDR
  CellResult<std::string> result;
  result.status = CellReturnStatus::Timeout;

  // Retrieve address from pdp cid 1
  at_->sendAT("+CGPADDR=1");
  if (at_->waitResponse("+CGPADDR: 1,") != ATCommandHandler::ExpArg1) {
    return result;
  }

  std::string ipaddr;
  if (at_->waitAndRecvRespLine(ipaddr) == -1) {
    return result;
  }

  // receive OK response from the buffer, ignore it
  at_->waitResponse();

  result.status = CellReturnStatus::Ok;
  result.data = ipaddr;

  return result;
}

CellReturnStatus CellularModuleA7672XX::isNetworkRegistered(CellTechnology ct) {
  auto cmdNR = _mapCellTechToNetworkRegisCmd(ct);
  if (cmdNR.empty()) {
    return CellReturnStatus::Error;
  }

  char buf[15] = {0};
  sprintf(buf, "+%s?", cmdNR.c_str());
  at_->sendAT(buf);
  int resp = at_->waitResponse("+CREG:", "+CEREG:", "+CGREG:");
  if (resp != ATCommandHandler::ExpArg1 && resp != ATCommandHandler::ExpArg2 &&
      resp != ATCommandHandler::ExpArg3) {
    return CellReturnStatus::Timeout;
  }

  std::string recv;
  if (at_->waitAndRecvRespLine(recv) == -1) {
    return CellReturnStatus::Timeout;
  }

  auto crs = CellReturnStatus::Ok;
  if (recv != "0,1" && recv != "0,5" && recv != "1,1" && recv != "1,5") {
    crs = CellReturnStatus::Failed;
  }

  // receive OK response from the buffer, ignore it
  at_->waitResponse();

  return crs;
}

CellResult<std::string>
CellularModuleA7672XX::startNetworkRegistration(CellTechnology ct, const std::string &apn,
                                                uint32_t operationTimeoutMs) {
  CellResult<std::string> result;
  result.status = CellReturnStatus::Timeout;

  // Make sure CT is supported
  if (_mapCellTechToMode(ct) == -1) {
    result.status = CellReturnStatus::Error;
    return result;
  }

  // start time for whole operation
  uint32_t startOperationTime = MILLIS();
  // start time for certain state (check network registration and check ensure service ready)
  uint32_t startStateTime = MILLIS();

  NetworkRegistrationState state = CHECK_MODULE_READY;
  bool finish = false;

  AG_LOGI(TAG, "Start operation network registration");
  while ((MILLIS() - startOperationTime) < operationTimeoutMs && !finish) {
    switch (state) {
    case CHECK_MODULE_READY:
      state = _implCheckModuleReady();
      break;
    case PREPARE_REGISTRATION:
      state = _implPrepareRegistration(ct);
      startStateTime = MILLIS();
      break;
    case CHECK_NETWORK_REGISTRATION:
      if ((MILLIS() - startStateTime) > 10000) {
        // Timeout wait network registration to expected status, configuring network
        state = CONFIGURE_NETWORK;
        continue;
      }
      state = _implCheckNetworkRegistration(ct);
      // If registration status as expected, continue to ENSURE_SERVICE_READY
      if (state == ENSURE_SERVICE_READY) {
        // Reset time for ENSURE_SERVICE_READY
        startStateTime = MILLIS();
      }
      break;
    case ENSURE_SERVICE_READY:
      if ((MILLIS() - startStateTime) > 10000) {
        // Timeout wait service ready, configuring service
        state = CONFIGURE_SERVICE;
        continue;
      }
      state = _implEnsureServiceReady();
      break;
    case CONFIGURE_NETWORK:
      // Here's after finish goes back to check network registration status
      state = _implConfigureNetwork();
      // After configuring network, check network registration status
      // Reset time for CHECK_NETWORK_REGISTRATION
      startStateTime = MILLIS();
      break;
    case CONFIGURE_SERVICE:
      // Here's after finish goes back to check network registration status
      state = _implConfigureService(apn);
      // After configuring service, make sure service is now ready
      // Reset time for ENSURE_SERVICE_READY
      startStateTime = MILLIS();
      break;
    case NETWORK_REGISTERED:
      state = _implNetworkRegistered();
      if (state == NETWORK_REGISTERED) {
        // Still the same state, indicating process finish successfully
        finish = true;
        continue;
      }
      // Reset time because it might go back to ENSURE_SERVICE_READY
      startStateTime = MILLIS();
      break;
    }
    // Give CPU a break
    DELAY_MS(1);
  }

  if (!finish) {
    AG_LOGW(TAG, "Register to network operation timeout!");
    return result;
  }

  result.status = CellReturnStatus::Ok;
  return result;
}

CellReturnStatus CellularModuleA7672XX::reinitialize() {
  AG_LOGI(TAG, "Power cycling module");
  powerOff();
  DELAY_MS(1000);
  powerOn();

  AG_LOGI(TAG, "Initialize module");
  if (!at_->testAT()) {
    AG_LOGW(TAG, "Failed wait cellular module to ready");
    return CellReturnStatus::Error;
  }

  // Disable echo
  at_->sendAT("E0");
  at_->waitResponse();
  DELAY_MS(2000);

  // Disable GPRS event reporting (URC)
  at_->sendAT("+CGEREP=0");
  at_->waitResponse();
  DELAY_MS(2000);

  return CellReturnStatus::Ok;
}

CellResult<CellularModule::HttpResponse>
CellularModuleA7672XX::httpGet(const std::string &url, int connectionTimeout, int responseTimeout) {
  CellResult<CellularModule::HttpResponse> result;
  result.status = CellReturnStatus::Error;
  ATCommandHandler::Response response;

  // TODO: Sanity check registration status?

  // +HTTPINIT
  result.status = _httpInit();
  if (result.status != CellReturnStatus::Ok) {
    return result;
  }

  // +HTTPPARA set RECVTO and CONNECTTO
  result.status = _httpSetParamTimeout(connectionTimeout, responseTimeout);
  if (result.status != CellReturnStatus::Ok) {
    // NOTE: Failed set timeout parameter, just continue with default?
    _httpTerminate();
    return result;
  }

  // +HTTPPARA set URL
  result.status = _httpSetUrl(url);
  if (result.status != CellReturnStatus::Ok) {
    _httpTerminate();
    return result;
  }

  // +HTTPACTION
  int statusCode = -1;
  int bodyLen = -1;
  // 0 is GET method defined valus for this module
  result.status = _httpAction(0, connectionTimeout, responseTimeout, &statusCode, &bodyLen);
  if (result.status != CellReturnStatus::Ok) {
    _httpTerminate();
    return result;
  }

  AG_LOGI(TAG, "HTTP response code %d with body len: %d. Retrieving response body...", statusCode,
           bodyLen);

  uint32_t retrieveStartTime = MILLIS();

  char *bodyResponse = nullptr;
  if (bodyLen > 0) {
    // Create temporary memory to handle the buffer
    bodyResponse = new char[bodyLen + 1];
    memset(bodyResponse, 0, bodyLen + 1);

    // +HTTPREAD
    int offset = 0;
    int receivedBufferLen;
    char buf[HTTPREAD_CHUNK_SIZE * 2]; // Just to make sure not overflow

    do {
      memset(buf, 0, sizeof(buf));
      sprintf(buf, "+HTTPREAD=%d,%d", offset, HTTPREAD_CHUNK_SIZE);
      at_->sendAT(buf);
      response = at_->waitResponse("+HTTPREAD:"); // Wait for first +HTTPREAD, skip the OK
      if (response == ATCommandHandler::Timeout) {
        AG_LOGW(TAG, "Timeout wait response +HTTPREAD");
        break;
      } else if (response == ATCommandHandler::ExpArg2) {
        AG_LOGW(TAG, "Error execute HTTPREAD");
        break;
      }

      // Get first +HTTPREAD value
      if (at_->waitAndRecvRespLine(buf, HTTPREAD_CHUNK_SIZE) == -1) {
        AG_LOGW(TAG, "Failed retrieve +HTTPREAD value length");
        break;
      }
      receivedBufferLen = std::stoi(buf);

      // Receive body from http response with include whitespace since its a binary
      // Directly retrieve buffer with expected the expected length
      int receivedActual = at_->retrieveBuffer(buf, receivedBufferLen);
      if (receivedActual != receivedBufferLen) {
        // Size received not the same as expected, handle better
        AG_LOGE(TAG, "receivedBufferLen: %d | receivedActual: %d", receivedBufferLen,
                 receivedActual);
        break;
      }
      at_->clearBuffer();

      AG_LOGV(TAG, "Received body len from buffer: %d", receivedBufferLen);

      // Append response body chunk to result
      memcpy(bodyResponse + offset, buf, receivedBufferLen);

      // Continue to retrieve another 200 bytes
      offset = offset + HTTPREAD_CHUNK_SIZE;

    } while (offset < bodyLen); // TODO: Add timeout?

    // Check if all response body data received
    if (offset < bodyLen) {
      AG_LOGE(TAG, "Failed to retrieve all response body data from module");
      _httpTerminate();
      delete[] bodyResponse;
      return result;
    }
  }

  AG_LOGD(TAG, "Finish retrieve response body from module buffer in %.2fs",
           ((float)MILLIS() - retrieveStartTime) / 1000);

  // set status code and response body for return function
  result.data.statusCode = statusCode;
  result.data.bodyLen = bodyLen;
  if (bodyLen > 0) {
    // // Debug purpose
    // Serial.println("Repsonse body:");
    // for (int i = 0; i < bodyLen; i++) {
    //   if (bodyResponse[i] < 0x10)
    //     Serial.print("0");
    //   Serial.print((uint8_t)bodyResponse[i], HEX);
    // }

    result.data.body = std::unique_ptr<char[]>(bodyResponse);
  }

  _httpTerminate();
  AG_LOGI(TAG, "httpGet() finish");

  result.status = CellReturnStatus::Ok;
  return result;
}

CellResult<CellularModule::HttpResponse>
CellularModuleA7672XX::httpPost(const std::string &url, const std::string &body,
                                const std::string &headContentType, int connectionTimeout,
                                int responseTimeout) {

  CellResult<CellularModule::HttpResponse> result;
  result.status = CellReturnStatus::Error;
  ATCommandHandler::Response response;

  // TODO: Sanity check Registration Status?

  // +HTTPINIT
  result.status = _httpInit();
  if (result.status != CellReturnStatus::Ok) {
    return result;
  }

  // +HTTPPARA set RECVTO and CONNECTTO
  result.status = _httpSetParamTimeout(connectionTimeout, responseTimeout);
  if (result.status != CellReturnStatus::Ok) {
    // NOTE: Failed set timeout parameter, just continue with default?
    _httpTerminate();
    return result;
  }

  if (headContentType != "") {
    // AT+HTTPPARA="CONTENT", contenttype
    char buffer[100] = {0};
    sprintf(buffer, "+HTTPPARA=\"CONTENT\",\"%s\"", headContentType.c_str());
    at_->sendAT(buffer);
    response = at_->waitResponse();
    if (response == ATCommandHandler::Timeout) {
      AG_LOGW(TAG, "Timeout wait response +HTTPPARA CONTENT");
      _httpTerminate();
      result.status = CellReturnStatus::Timeout;
      return result;
    } else if (response == ATCommandHandler::ExpArg2) {
      AG_LOGW(TAG, "Error set HTTP param CONTENT");
      _httpTerminate();
      result.status = CellReturnStatus::Error;
      return result;
    }
  }

  // TODO: Another +HTTPPARA to handle https request SSLCFG

  // +HTTPPARA set URL
  result.status = _httpSetUrl(url);
  if (result.status != CellReturnStatus::Ok) {
    _httpTerminate();
    return result;
  }

  // +HTTPDATA ; Body len needs to be the same as length send after DOWNLOAD, otherwise error
  char buf[25] = {0};
  sprintf(buf, "+HTTPDATA=%d,10", body.length());
  at_->sendAT(buf);
  if (at_->waitResponse("DOWNLOAD") != ATCommandHandler::ExpArg1) {
    // Either timeout wait for expected response or return ERROR
    AG_LOGW(TAG, "Error +HTTPDATA wait for \"DOWNLOAD\" response");
    _httpTerminate();
    result.status = CellReturnStatus::Error;
    return result;
  }

  AG_LOGI(TAG, "Receive \"DOWNLOAD\" event, adding request body");
  at_->sendRaw(body.c_str());
  // Wait for 'OK' after send request body
  // Timeout set based on +HTTPDATA param
  if (at_->waitResponse(10000) != ATCommandHandler::ExpArg1) {
    // Timeout wait "OK"
    AG_LOGW(TAG, "Error +HTTPDATA wait for \"DOWNLOAD\" response");
    _httpTerminate();
    result.status = CellReturnStatus::Error;
    return result;
  }

  // +HTTPACTION
  int statusCode = -1;
  int bodyLen = -1;
  // 1 is GET method defined valus for this module
  result.status = _httpAction(1, connectionTimeout, responseTimeout, &statusCode, &bodyLen);
  if (result.status != CellReturnStatus::Ok) {
    _httpTerminate();
    return result;
  }

  AG_LOGI(TAG, "HTTP response code %d with body len: %d", statusCode, bodyLen);

  // set status code, and ignore response body
  result.data.statusCode = statusCode;
  // TODO: In the future retrieve the response body

  _httpTerminate();
  AG_LOGI(TAG, "httpPost() finish");

  result.status = CellReturnStatus::Ok;
  return result;
}

CellReturnStatus CellularModuleA7672XX::mqttConnect(const std::string &clientId,
                                                    const std::string &host, int port) {
  char buf[150] = {0};
  std::string result;

  // +CMQTTSTART
  at_->sendAT("+CMQTTSTART");
  auto atResult = at_->waitResponse(12000, "+CMQTTSTART:");
  if (atResult == ATCommandHandler::Timeout || atResult == ATCommandHandler::CMxError) {
    AG_LOGW(TAG, "Timeout wait for +CMQTTSTART response");
    return CellReturnStatus::Timeout;
  } else if (atResult == ATCommandHandler::ExpArg1) {
    // +CMQTTSTART response received as arg1
    // Get value of CMQTTSTART, expected is 0
    if (at_->waitAndRecvRespLine(result) == -1) {
      return CellReturnStatus::Timeout;
    }
    if (result != "0") {
      // Failed to start
      AG_LOGE(TAG, "CMQTTSTART failed with value %s", result.c_str());
      return CellReturnStatus::Error;
    }
    // CMQTTSTART ok
  } else if (atResult == ATCommandHandler::ExpArg2) {
    // Here it return error, but based on the document module MQTT context already started
    // Do nothing
    AG_LOGI(TAG, "+CMQTTSTART return error, which means mqtt context already started");
  }

  // +CMQTTACCQ
  sprintf(buf, "+CMQTTACCQ=0,\"%s\",0", clientId.c_str());
  at_->sendAT(buf);
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // ERROR or TIMEOUT, doesn't matter
    return CellReturnStatus::Error;
  }

  DELAY_MS(3000);

  // +CMQTTCONNECT
  // keep alive 120; cleansession 1
  memset(buf, 0, 150);
  sprintf(buf, "+CMQTTCONNECT=0,\"tcp://%s:%d\",120,1", host.c_str(), port);
  at_->sendAT(buf);
  if (at_->waitResponse(30000, "+CMQTTCONNECT: 0,") != ATCommandHandler::ExpArg1) {
    at_->clearBuffer();
    return CellReturnStatus::Ok;
  }

  if (at_->waitAndRecvRespLine(result) == -1) {
    return CellReturnStatus::Timeout;
  }

  // If result not 0, then error occur
  if (result != "0") {
    AG_LOGE(TAG, "+CMQTTCONNECT error result: %s", result.c_str());
    return CellReturnStatus::Error;
  }
  at_->clearBuffer();

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::mqttDisconnect() {
  std::string result;
  // +CMQTTDISC
  at_->sendAT("+CMQTTDISC=0,60"); // Timeout 60s
  /// wait +CMTTDISC until client_index
  if (at_->waitResponse(60000, "+CMQTTDISC: 0,") != ATCommandHandler::ExpArg1) {
    at_->clearBuffer();
    // Error or timeout
    return CellReturnStatus::Error;
  }

  if (at_->waitAndRecvRespLine(result) == -1) {
    return CellReturnStatus::Timeout;
  }

  if (result != "0") {
    AG_LOGE(TAG, "+CMQTTDISC error result: %s", result.c_str());
    return CellReturnStatus::Error;
  }
  at_->clearBuffer();

  // +CMQTTREL
  at_->sendAT("+CMQTTREL=0");
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // Ignore response err code
    at_->clearBuffer();
    return CellReturnStatus::Error;
  }
  at_->clearBuffer();

  // +CMQTTSTOP
  at_->sendAT("+CMQTTSTOP");
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // Ignore response err code
    return CellReturnStatus::Error;
  }
  at_->clearBuffer();

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::mqttPublish(const std::string &topic,
                                                    const std::string &payload, int qos, int retain,
                                                    int timeoutS) {

  char buf[50] = {0};
  std::string result;

  // +CMQTTTOPIC
  sprintf(buf, "+CMQTTTOPIC=0,%d", topic.length());
  at_->sendAT(buf);
  if (at_->waitResponse(">") != ATCommandHandler::ExpArg1) {
    // Either timeout wait for expected response or return ERROR
    AG_LOGW(TAG, "Error +CMQTTTOPIC wait for \">\" response");
    return CellReturnStatus::Error;
  }

  AG_LOGI(TAG, "Receive \">\" event, adding topic");
  at_->sendRaw(topic.c_str());
  // Wait for 'OK' after send topic
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // Timeout wait "OK"
    AG_LOGW(TAG, "Error +CMQTTTOPIC wait for \"OK\" response");
    return CellReturnStatus::Error;
  }

  // +CMQTTPAYLOAD
  memset(buf, 0, 50);
  sprintf(buf, "+CMQTTPAYLOAD=0,%d", payload.length());
  at_->sendAT(buf);
  if (at_->waitResponse(">") != ATCommandHandler::ExpArg1) {
    // Either timeout wait for expected response or return ERROR
    AG_LOGW(TAG, "Error +CMQTTPAYLOAD wait for \">\" response");
    return CellReturnStatus::Error;
  }

  AG_LOGI(TAG, "Receive \">\" event, adding payload");
  at_->sendRaw(payload.c_str());
  // Wait for 'OK' after send payload
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // Timeout wait "OK"
    AG_LOGW(TAG, "Error +CMQTTPAYLOAD wait for \"OK\" response");
    return CellReturnStatus::Error;
  }

  memset(buf, 0, 50);
  sprintf(buf, "+CMQTTPUB=0,%d,%d,%d", qos, timeoutS, retain);
  int timeoutMs = timeoutS * 1000;
  at_->sendAT(buf);
  if (at_->waitResponse(timeoutMs, "+CMQTTPUB: 0,") != ATCommandHandler::ExpArg1) {
    AG_LOGW(TAG, "+CMQTTPUBLISH error");
    return CellReturnStatus::Error;
  }

  // Retrieve the value
  if (at_->waitAndRecvRespLine(result) == -1) {
    AG_LOGW(TAG, "+CMQTTPUB retrieve value timeout");
    return CellReturnStatus::Timeout;
  }

  if (result != "0") {
    AG_LOGE(TAG, "Failed +CMQTTPUB with value %s", result.c_str());
    return CellReturnStatus::Error;
  }

  // Make sure buffer clean
  at_->clearBuffer();

  return CellReturnStatus::Ok;
}

CellularModuleA7672XX::NetworkRegistrationState CellularModuleA7672XX::_implCheckModuleReady() {
  if (at_->testAT() == false) {
    REGIS_RETRY_DELAY();
    // TODO: If too long, try reset module
    return CHECK_MODULE_READY;
  }
  if (isSimReady() != CellReturnStatus::Ok) {
    REGIS_RETRY_DELAY();
    return CHECK_MODULE_READY;
  }

  AG_LOGI(TAG, "Continue: PREPARE_REGISTRATION");
  return PREPARE_REGISTRATION;
}

CellularModuleA7672XX::NetworkRegistrationState
CellularModuleA7672XX::_implPrepareRegistration(CellTechnology ct) {
  // TODO: Check result
  _disableNetworkRegistrationURC(ct);
  _applyCellularTechnology(ct);
  AG_LOGI(TAG, "Continue: CHECK_NETWORK_REGISTRATION");
  return CHECK_NETWORK_REGISTRATION;
}

CellularModuleA7672XX::NetworkRegistrationState
CellularModuleA7672XX::_implCheckNetworkRegistration(CellTechnology ct) {
  CellReturnStatus crs = isNetworkRegistered(ct);
  if (crs == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  } else if (crs == CellReturnStatus::Failed || crs == CellReturnStatus::Error) {
    REGIS_RETRY_DELAY();
    return CHECK_NETWORK_REGISTRATION;
  }

  CellResult<int> result = retrieveSignal();
  if (result.status == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  }
  // Check if returned signal is valid
  if (result.data < 1 || result.data > 31) {
    REGIS_RETRY_DELAY();
    return CHECK_NETWORK_REGISTRATION;
  }

  AG_LOGI(TAG, "Continue: ENSURE_SERVICE_READY");
  return ENSURE_SERVICE_READY;
}

CellularModuleA7672XX::NetworkRegistrationState CellularModuleA7672XX::_implEnsureServiceReady() {
  CellReturnStatus crs = _isServiceAvailable();
  if (crs == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  } else if (crs == CellReturnStatus::Failed || crs == CellReturnStatus::Error) {
    REGIS_RETRY_DELAY();
    return ENSURE_SERVICE_READY;
  }

  AG_LOGI(TAG, "Continue: NETWORK_REGISTERED");
  return NETWORK_REGISTERED;
}

CellularModuleA7672XX::NetworkRegistrationState CellularModuleA7672XX::_implConfigureNetwork() {
  CellResult<int> result = retrieveSignal();
  if (result.status == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  }

  AG_LOGI(TAG, "Cellular signal: %d", result.data);

  CellReturnStatus crs = _checkOperatorSelection();
  if (crs == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  } else if (crs == CellReturnStatus::Ok) {
    // Result already as expected, go back to check network registration status
    return CHECK_NETWORK_REGISTRATION;
  }

  crs = _applyOperatorSelection();
  if (crs == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  } else if (crs == CellReturnStatus::Error) {
    // TODO: What's next if this return error?
  }

  AG_LOGI(TAG, "Continue: CHECK_NETWORK_REGISTRATION");
  return CHECK_NETWORK_REGISTRATION;
}

CellularModuleA7672XX::NetworkRegistrationState
CellularModuleA7672XX::_implConfigureService(const std::string &apn) {
  CellReturnStatus crs = _applyAPN(apn);
  if (crs == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  } else if (crs == CellReturnStatus::Error) {
    // TODO: What's next if this return error?
  }

  crs = _activatePDPContext();
  if (crs == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  } else if (crs == CellReturnStatus::Error) {
    // TODO: What's next if this return error?
  }

  crs = _ensurePacketDomainAttached();
  if (crs == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  } else if (crs == CellReturnStatus::Error) {
    // TODO: What's next if this return error?
  }

  AG_LOGI(TAG, "Continue: CHECK_NETWORK_REGISTRATION");
  return CHECK_NETWORK_REGISTRATION;
}

CellularModuleA7672XX::NetworkRegistrationState CellularModuleA7672XX::_implNetworkRegistered() {
  CellResult<int> result = retrieveSignal();
  if (result.status == CellReturnStatus::Timeout) {
    // Go back to check module ready
    return CHECK_MODULE_READY;
  }

  // Check if returned signal is valid
  if (result.data < 1 || result.data > 31) {
    REGIS_RETRY_DELAY();
    return ENSURE_SERVICE_READY;
  }

  // print signal
  AG_LOGI(TAG, "Signal ready at: %d", result.data);

  CellResult<std::string> resultIP = retrieveIPAddr();
  if (resultIP.data.empty()) {
    // Sanity check if IP is empty, go back ensure service ready
    return ENSURE_SERVICE_READY;
  }

  AG_LOGI(TAG, "IP Addr: %s", resultIP.data.c_str());
  AG_LOGI(TAG, "Continue: finish");
  return NETWORK_REGISTERED;
}

CellReturnStatus CellularModuleA7672XX::_disableNetworkRegistrationURC(CellTechnology ct) {
  auto cmdNR = _mapCellTechToNetworkRegisCmd(ct);
  if (cmdNR.empty()) {
    return CellReturnStatus::Error;
  }

  char buf[15] = {0};
  sprintf(buf, "+%s=0", cmdNR.c_str());
  at_->sendAT(buf);
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    return CellReturnStatus::Timeout;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_applyCellularTechnology(CellTechnology ct) {
  // with assumption CT already validate before calling this function
  int mode = _mapCellTechToMode(ct);
  std::string cmd = std::string("+CNMP=") + std::to_string(mode);
  at_->sendAT(cmd.c_str());
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // TODO: This should be error or timeout
    return CellReturnStatus::Error;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_applyOperatorSelection() {
  at_->sendAT("+COPS=0,2");
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    // TODO: This should be error or timeout
    return CellReturnStatus::Error;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_checkOperatorSelection() {
  at_->sendAT("+COPS?");
  if (at_->waitResponse("+COPS:") != ATCommandHandler::ExpArg1) {
    // TODO: This should have better error check
    return CellReturnStatus::Timeout;
  }

  auto crs = CellReturnStatus::Ok;

  // ignore <oper> value
  if (at_->waitResponse(" 0,2,\"") != ATCommandHandler::ExpArg1) {
    crs = CellReturnStatus::Failed;
  }

  // receive OK response from the buffer, ignore it
  at_->waitResponse();

  return crs;
}

CellReturnStatus CellularModuleA7672XX::_isServiceAvailable() {
  at_->sendAT("+CNSMOD?");
  if (at_->waitResponse("+CNSMOD:") != ATCommandHandler::ExpArg1) {
    return CellReturnStatus::Timeout;
  }

  std::string status;
  if (at_->waitAndRecvRespLine(status) == -1) {
    return CellReturnStatus::Timeout;
  }

  auto crs = CellReturnStatus::Ok;

  // Second value '0' is NO SERVICE, expect other than NO SERVICE
  if (status == "0,0" || status == "1,0") {
    crs = CellReturnStatus::Failed;
  }

  // receive OK response from the buffer, ignore it
  at_->waitResponse();

  return crs;
}

CellReturnStatus CellularModuleA7672XX::_applyAPN(const std::string &apn) {
  // set APN to pdp cid 1
  char buf[100] = {0};
  sprintf(buf, "+CGDCONT=1,\"IP\",\"%s\"", apn.c_str());
  at_->sendAT(buf);
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    return CellReturnStatus::Error;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_ensurePacketDomainAttached() {
  at_->sendAT("+CGATT?");
  if (at_->waitResponse("+CGATT:") != ATCommandHandler::ExpArg1) {
    // If return error or not response consider "error"
    return CellReturnStatus::Error;
  }

  std::string state;
  if (at_->waitAndRecvRespLine(state) == -1) {
    // TODO: What to do?
  }

  if (state == "1") {
    // Already attached
    return CellReturnStatus::Ok;
  }

  // Not attached, attempt to
  at_->sendAT("+CGATT=1");
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    return CellReturnStatus::Failed;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_activatePDPContext() {
  at_->sendAT("+CGACT=1,1");
  if (at_->waitResponse() != ATCommandHandler::ExpArg1) {
    return CellReturnStatus::Error;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_httpInit() {
  at_->sendAT("+HTTPINIT");
  auto response = at_->waitResponse();
  if (response == ATCommandHandler::Timeout) {
    AG_LOGW(TAG, "Timeout wait response +HTTPINIT");
    return CellReturnStatus::Timeout;
  } else if (response == ATCommandHandler::ExpArg2) {
    AG_LOGW(TAG, "Error initialize module HTTP service");
    return CellReturnStatus::Error;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_httpSetParamTimeout(int connectionTimeout,
                                                             int responseTimeout) {
  // Add threshold guard based on module specification (20 - 120). Default 120
  if (connectionTimeout != -1) {
    if (connectionTimeout < 20) {
      connectionTimeout = 20;
    } else if (connectionTimeout > 120) {
      connectionTimeout = 120;
    }
  }

  // Add threshold guard based on module specification (2 - 120). Default 20
  if (responseTimeout != -1) {
    if (responseTimeout < 2) {
      responseTimeout = 2;
    } else if (responseTimeout > 120) {
      responseTimeout = 120;
    }
  }

  // +HTTPPARA set connection timeout if provided
  if (connectionTimeout != -1) {
    // AT+HTTPPARA="CONNECTTO",<conntimeout>
    std::string cmd = std::string("+HTTPPARA=\"CONNECTTO\",") + std::to_string(connectionTimeout);
    at_->sendAT(cmd.c_str());
    auto response = at_->waitResponse();
    if (response == ATCommandHandler::Timeout) {
      AG_LOGW(TAG, "Timeout wait response +HTTPPARA CONNECTTO");
      return CellReturnStatus::Timeout;
    } else if (response == ATCommandHandler::ExpArg2) {
      AG_LOGW(TAG, "Error set HTTP param CONNECTTO");
      return CellReturnStatus::Error;
    }
  }

  // +HTTPPARA set response timeout if provided
  if (responseTimeout != -1) {
    // AT+HTTPPARA="RECVTO",<recv_timeout>
    std::string cmd = std::string("+HTTPPARA=\"RECVTO\",") + std::to_string(responseTimeout);
    at_->sendAT(cmd.c_str());
    auto response = at_->waitResponse();
    if (response == ATCommandHandler::Timeout) {
      AG_LOGW(TAG, "Timeout wait response +HTTPPARA RECVTO");
      return CellReturnStatus::Timeout;
    } else if (response == ATCommandHandler::ExpArg2) {
      AG_LOGW(TAG, "Error set HTTP param RECVTO");
      return CellReturnStatus::Error;
    }
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_httpSetUrl(const std::string &url) {
  char buf[200] = {0};
  sprintf(buf, "+HTTPPARA=\"URL\", \"%s\"", url.c_str());
  at_->sendAT(buf);
  auto response = at_->waitResponse();
  if (response == ATCommandHandler::Timeout) {
    AG_LOGW(TAG, "Timeout wait response +HTTPPARA URL");
    return CellReturnStatus::Timeout;
  } else if (response == ATCommandHandler::ExpArg2) {
    AG_LOGW(TAG, "Error set HTTP param URL");
    return CellReturnStatus::Error;
  }

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_httpAction(int httpMethodCode, int connectionTimeout,
                                                    int responseTimeout, int *oResponseCode,
                                                    int *oBodyLen) {
  int code, bodyLen;
  int waitActionTimeout;
  std::string data;

  // +HTTPACTION
  data = std::string("+HTTPACTION=") + std::to_string(httpMethodCode);
  at_->sendAT(data.c_str());
  auto response = at_->waitResponse(); // Wait for OK
  if (response == ATCommandHandler::Timeout) {
    AG_LOGW(TAG, "Timeout wait response +HTTPACTION GET");
    return CellReturnStatus::Timeout;
  } else if (response == ATCommandHandler::ExpArg2) {
    AG_LOGW(TAG, "Error execute HTTPACTION GET");
    return CellReturnStatus::Error;
  }

  // calculate how long to wait for +HTTPACTION
  waitActionTimeout = _calculateResponseTimeout(connectionTimeout, responseTimeout);

  // +HTTPACTION: <method>,<statuscode>,<datalen>
  // +HTTPACTION: <method>,<errcode>,<datalen>
  // Wait for +HTTPACTION finish execute
  response = at_->waitResponse(waitActionTimeout, "+HTTPACTION:");
  if (response == ATCommandHandler::Timeout) {
    AG_LOGW(TAG, "Timeout wait +HTTPACTION success execution");
    return CellReturnStatus::Timeout;
  }

  // Retrieve +HTTPACTION response value
  at_->waitAndRecvRespLine(data);
  // Sanity check if value is empty
  if (data.empty()) {
    AG_LOGW(TAG, "+HTTPACTION result value empty");
    return CellReturnStatus::Failed;
  }

  AG_LOGI(TAG, "+HTTPACTION finish! retrieve its values");

  // 0,code,size
  // start from code, ignore 0 (GET)
  Common::splitByComma(data.substr(2, data.length()), &code, &bodyLen);
  if (code == -1 || (code > 700 && code < 720)) {
    // -1 means string cannot splitted by comma
    // 7xx This is error code <errcode> not http <status_code>
    // 16.3.2 Description of<errcode> datasheet
    AG_LOGW(TAG, "+HTTPACTION error with module errcode: %d", code);
    return CellReturnStatus::Failed;
  }

  // Assign result to the output variable
  *oResponseCode = code;
  *oBodyLen = bodyLen;

  return CellReturnStatus::Ok;
}

CellReturnStatus CellularModuleA7672XX::_httpTerminate() {
  // +HTTPTERM to stop http service
  // If previous AT return timeout, here just attempt
  at_->sendAT("+HTTPTERM");
  auto response = at_->waitResponse();
  if (response == ATCommandHandler::Timeout) {
    AG_LOGW(TAG, "Timeout wait response +HTTPTERM");
    return CellReturnStatus::Timeout;
  } else if (response == ATCommandHandler::ExpArg2) {
    AG_LOGW(TAG, "Error stop module HTTP service");
    return CellReturnStatus::Error;
  }

  return CellReturnStatus::Ok;
}

int CellularModuleA7672XX::_mapCellTechToMode(CellTechnology ct) {
  int mode = -1;
  switch (ct) {
  case CellTechnology::Auto:
    mode = 2;
    break;
  case CellTechnology::TWO_G:
    mode = 13;
    break;
  case CellTechnology::LTE:
    mode = 38;
    break;
  default:
    AG_LOGE(TAG, "CellTechnology not supported for this module");
    break;
  }

  return mode;
}

std::string CellularModuleA7672XX::_mapCellTechToNetworkRegisCmd(CellTechnology ct) {
  std::string cmd;
  switch (ct) {
  case CellTechnology::Auto:
    cmd = "CREG"; // TODO: Is it thou?
    break;
  case CellTechnology::TWO_G:
    cmd = "CGREG";
    break;
  case CellTechnology::LTE:
    cmd = "CEREG";
    break;
  default:
    AG_LOGE(TAG, "CellTechnology not supported for this module");
    break;
  }

  return cmd;
}

int CellularModuleA7672XX::_calculateResponseTimeout(int connectionTimeout, int responseTimeout) {
  int waitActionTimeout = 0;
  if (connectionTimeout == -1 && responseTimeout == -1) {
    waitActionTimeout = (DEFAULT_HTTP_CONNECT_TIMEOUT + DEFAULT_HTTP_RESPONSE_TIMEOUT) * 1000;
  } else if (connectionTimeout == -1) {
    waitActionTimeout = (DEFAULT_HTTP_CONNECT_TIMEOUT + responseTimeout) * 1000;
  } else if (responseTimeout == -1) {
    waitActionTimeout = (connectionTimeout + DEFAULT_HTTP_RESPONSE_TIMEOUT) * 1000;
  } else {
    waitActionTimeout = (connectionTimeout + responseTimeout) * 1000;
  }

  return waitActionTimeout;
}

#endif // ESP8266
