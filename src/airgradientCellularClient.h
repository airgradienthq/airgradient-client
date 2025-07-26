/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_CELLULAR_CLIENT_H
#define AIRGRADIENT_CELLULAR_CLIENT_H

#ifndef ESP8266

#include <string>

#include "airgradientClient.h"
#include "cellularModule.h"

class AirgradientCellularClient : public AirgradientClient {
private:
  const char *const TAG = "AgCellClient";
  std::string _apn = "iot.1nce.net";
  std::string _iccid = "";
  CellularModule *cell_ = nullptr;

  int _networkRegistrationTimeoutMs = (3 * 60000);

public:
  AirgradientCellularClient(CellularModule *cellularModule);
  ~AirgradientCellularClient() {};

  bool begin(std::string sn);
  void setNetworkRegistrationTimeoutMs(int timeoutMs);
  std::string getICCID();
  bool ensureClientConnection(bool reset);
  std::string httpFetchConfig();
  bool httpPostMeasures(const std::string &payload);
  bool httpPostMeasures(int measureInterval, std::vector<OpenAirMaxPayload> data);
  bool mqttConnect();
  bool mqttDisconnect();
  bool mqttPublishMeasures(const std::string &payload);
};

#endif // ESP8266
#endif // AIRGRADIENT_CELLULAR_CLIENT_H
