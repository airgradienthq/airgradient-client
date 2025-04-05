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
  CellularModule *cell_ = nullptr;

public:
  AirgradientCellularClient(CellularModule *cellularModule);
  ~AirgradientCellularClient() {};

  bool begin(std::string sn);
  bool ensureClientConnection();
  std::string httpFetchConfig();
  bool httpPostMeasures(const std::string &payload);
  bool mqttConnect();
  bool mqttDisconnect();
  bool mqttPublishMeasures(const std::string &payload);
};

#endif // ESP8266
#endif // AIRGRADIENT_CELLULAR_CLIENT_H
