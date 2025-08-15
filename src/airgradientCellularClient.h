/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_CELLULAR_CLIENT_H
#define AIRGRADIENT_CELLULAR_CLIENT_H

#include <sstream>
#ifndef ESP8266

#include <string>

#include "airgradientClient.h"
#include "cellularModule.h"

#define DEFAULT_AIRGRADIENT_APN "iot.1nce.net"

class AirgradientCellularClient : public AirgradientClient {
private:
  const char *const TAG = "AgCellClient";
  std::string _apn = DEFAULT_AIRGRADIENT_APN;
  std::string _iccid = "";
  CellularModule *cell_ = nullptr;
  int _networkRegistrationTimeoutMs = (3 * 60000);

public:
  AirgradientCellularClient(CellularModule *cellularModule);
  ~AirgradientCellularClient() {};

  bool begin(std::string sn, PayloadType pt);
  void setAPN(const std::string &apn);
  void setNetworkRegistrationTimeoutMs(int timeoutMs);
  std::string getICCID();
  bool ensureClientConnection(bool reset);
  std::string httpFetchConfig();
  bool httpPostMeasures(const std::string &payload);
  bool httpPostMeasures(const AirgradientPayload &payload);
  bool mqttConnect();
  bool mqttDisconnect();
  bool mqttPublishMeasures(const std::string &payload);

private:
  std::string _getEndpoint();
  void _serialize(std::ostringstream &oss, int rco2, int particleCount003, float pm01, float pm25,
                  float pm10, int tvoc, int nox, float atmp, float rhum, int signal,
                  float vBat = -1.0f, float vPanel = -1.0f, float o3WorkingElectrode = -1.0f,
                  float o3AuxiliaryElectrode = -1.0f, float no2WorkingElectrode = -1.0f,
                  float no2AuxiliaryElectrode = -1.0f, float afeTemp = -1.0f);
};

#endif // ESP8266
#endif // AIRGRADIENT_CELLULAR_CLIENT_H
