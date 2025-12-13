/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef CELLULAR_MODULE_H
#define CELLULAR_MODULE_H

#include <memory>
#include <string>

enum class CellReturnStatus {
  Ok = 1, // command is success and return expected value
  Failed, // command is success but not return expected value
  Error,  // module return error after command sent
  Timeout // module not return anything
};

template <typename T> struct CellResult {
  CellReturnStatus status;
  T data;
};

enum class CellTechnology { Auto, TWO_G, LTE_M, LTE_NB_IOT, LTE };

class CellularModule {
public:
  struct HttpResponse {
    int statusCode;
    std::unique_ptr<char[]> body;
    int bodyLen;
  };

  // URL, Headers opt?, conn timeout, recv timeout,
  // response: CRS, status code, body

  CellularModule();
  virtual ~CellularModule();

  virtual bool init();
  virtual void powerOn();
  virtual void powerOff(bool force = false);
  virtual bool reset();
  virtual void sleep();
  virtual CellResult<std::string> getModuleInfo();
  virtual CellResult<std::string> retrieveSimCCID();
  virtual CellReturnStatus isSimReady();
  virtual CellResult<int> retrieveSignal();
  virtual CellResult<std::string> retrieveIPAddr();
  virtual CellReturnStatus isNetworkRegistered(CellTechnology ct);
  virtual CellResult<std::string> startNetworkRegistration(CellTechnology ct,
                                                           const std::string &apn,
                                                           uint32_t operationTimeoutMs = 90000);
  virtual CellReturnStatus reinitialize();
  virtual CellResult<HttpResponse> httpGet(const std::string &url, int connectionTimeout = -1,
                                           int responseTimeout = -1);
  virtual CellResult<HttpResponse> httpPost(const std::string &url, const std::string &body,
                                            const std::string &headContentType = "",
                                            int connectionTimeout = -1, int responseTimeout = -1);
  virtual CellReturnStatus mqttConnect(const std::string &clientId, const std::string &host,
                                       int port = 1883, std::string username = "",
                                       std::string password = "");
  virtual CellReturnStatus mqttDisconnect();
  virtual CellReturnStatus mqttPublish(const std::string &topic, const std::string &payload,
                                       int qos = 1, int retain = 0, int timeoutS = 15);


  virtual CellReturnStatus udpConnect(const std::string &host, int port = 5683);
  virtual CellReturnStatus udpDisconnect();

  // Generic functions

  /**
   * @brief convert cellular csq command (signal quality) to RSSI in dbm
   *
   * @param csq signal quality indicator to convert
   *
   * @return RSSI in dbm
   * @return 0 if signal quality indicator is invalid
   */
  int csqToDbm(int csq);

private:
  /* data */
};

#endif // CELLULAR_MODULE_H
