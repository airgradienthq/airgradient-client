/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef CELLULAR_MODULE_A7672XX_H
#define CELLULAR_MODULE_A7672XX_H

#include <string>

#include "driver/gpio.h"
#include "esp_log.h"

#include "agSerial.h"
#include "atCommandHandler.h"
#include "cellularModule.h"
#include "common.h"

class CellularModuleA7672XX : public CellularModule {
private:
  const char *const TAG = "A7672XX";

  bool _initialized = false;

  AgSerial *agSerial_ = nullptr;
  gpio_num_t _powerIO = GPIO_NUM_NC;
  ATCommandHandler *at_ = nullptr;

public:
  enum NetworkRegistrationState {
    // Check if AT ready
    // Chec if SIM ready
    CHECK_MODULE_READY,
    // Disable network registration URC
    // set cell technology
    PREPARE_REGISTRATION,
    // Check network registration status
    // ensure signal
    CHECK_NETWORK_REGISTRATION,
    // Make sure service available CNSMOD
    ENSURE_SERVICE_READY,
    // cops select
    CONFIGURE_NETWORK,
    // Set APN
    // Activate PDP context
    // Make sure packet domain active
    CONFIGURE_SERVICE,
    // Print IP address,
    // final check signal
    NETWORK_REGISTERED
  };

  CellularModuleA7672XX(AgSerial *agSerial);
  CellularModuleA7672XX(AgSerial *agSerial, int powerPin);
  ~CellularModuleA7672XX();

  bool init();
  void powerOn();
  void powerOff();
  bool reset();
  void sleep();
  CellResult<std::string> getModuleInfo();
  CellResult<std::string> retrieveSimCCID();
  CellReturnStatus isSimReady();
  CellResult<int> retrieveSignal();
  CellResult<std::string> retrieveIPAddr();
  CellReturnStatus isNetworkRegistered(CellTechnology ct);
  CellResult<std::string> startNetworkRegistration(CellTechnology ct, const std::string &apn,
                                                   uint32_t operationTimeoutMs = 90000);
  CellReturnStatus reinitialize();
  CellResult<CellularModule::HttpResponse>
  httpGet(const std::string &url, int connectionTimeout = -1, int responseTimeout = -1);
  CellResult<CellularModule::HttpResponse> httpPost(const std::string &url, const std::string &body,
                                                    const std::string &headContentType = "",
                                                    int connectionTimeout = -1,
                                                    int responseTimeout = -1);
  CellReturnStatus mqttConnect(const std::string &clientId, const std::string &host,
                               int port = 1883);
  CellReturnStatus mqttDisconnect();
  CellReturnStatus mqttPublish(const std::string &topic, const std::string &payload, int qos = 1,
                               int retain = 0, int timeoutS = 15);

private:
  const int DEFAULT_HTTP_CONNECT_TIMEOUT = 120; // seconds
  const int DEFAULT_HTTP_RESPONSE_TIMEOUT = 20; // seconds
  const int HTTPREAD_CHUNK_SIZE = 200;          // bytes

  // Network Registration implementation for each state
  NetworkRegistrationState _implCheckModuleReady();
  NetworkRegistrationState _implPrepareRegistration(CellTechnology ct);
  NetworkRegistrationState _implCheckNetworkRegistration(CellTechnology ct);
  NetworkRegistrationState _implEnsureServiceReady();
  NetworkRegistrationState _implConfigureNetwork();
  NetworkRegistrationState _implConfigureService(const std::string &apn);
  NetworkRegistrationState _implNetworkRegistered();

  // AT Command functions
  CellReturnStatus _disableNetworkRegistrationURC(CellTechnology ct); // depend on CellTech
  CellReturnStatus _applyCellularTechnology(CellTechnology ct);
  CellReturnStatus _applyOperatorSelection();
  CellReturnStatus _checkOperatorSelection();
  CellReturnStatus _isServiceAvailable();
  CellReturnStatus _applyAPN(const std::string &apn);
  CellReturnStatus _ensurePacketDomainAttached();
  CellReturnStatus _activatePDPContext();
  CellReturnStatus _httpInit();
  CellReturnStatus _httpSetParamTimeout(int connectionTimeout, int responseTimeout);
  CellReturnStatus _httpSetUrl(const std::string &url);
  CellReturnStatus _httpAction(int httpMethodCode, int connectionTimeout, int responseTimeout,
                               int *oResponseCode, int *oBodyLen);
  CellReturnStatus _httpTerminate();

  int _mapCellTechToMode(CellTechnology ct);
  std::string _mapCellTechToNetworkRegisCmd(CellTechnology ct);

  /**
   * @brief Calculate timeout in ms to wait +HTTPACTION request finish
   *
   * @param connectionTimeout connectionTimeout provided when call http request
   * in seconds
   * @param responseTimeout responseTimeout provided when call http request in
   * seconds
   * @return int total time in ms to wait for +HTTPACTION
   */
  int _calculateResponseTimeout(int connectionTimeout, int responseTimeout);
};

#endif // CELLULAR_MODULE_A7672XX_H
