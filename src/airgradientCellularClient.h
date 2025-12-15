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

#include "CoapPacket.h"

#define DEFAULT_AIRGRADIENT_APN "iot.1nce.net"

class AirgradientCellularClient : public AirgradientClient {
private:
  const char *const TAG = "AgCellClient";
  std::string _apn = DEFAULT_AIRGRADIENT_APN;
  std::string _iccid = "";
  CellularModule *cell_ = nullptr;
  int _networkRegistrationTimeoutMs = (3 * 60000);
  bool _extendedPmMeasures = false;
  bool _isCoapConnected = false;

public:
  AirgradientCellularClient(CellularModule *cellularModule);
  ~AirgradientCellularClient() {};

  bool begin(std::string sn, PayloadType pt);
  void setAPN(const std::string &apn);
  void setExtendedPmMeasures(bool enable);
  void setNetworkRegistrationTimeoutMs(int timeoutMs);
  std::string getICCID();
  bool ensureClientConnection(bool reset);
  std::string httpFetchConfig();
  bool httpPostMeasures(const std::string &payload);
  bool httpPostMeasures(const AirgradientPayload &payload);
  bool mqttConnect();
  bool mqttConnect(const char *uri);
  bool mqttConnect(const std::string &host, int port, std::string username = "",
                   std::string password = "");
  bool mqttDisconnect();
  bool mqttPublishMeasures(const std::string &payload);
  bool mqttPublishMeasures(const AirgradientPayload &payload);
  std::string coapFetchConfig(bool keepConnection = false);
  bool coapPostMeasures(const std::string &payload, bool keepConnection = false);

private:
  std::string _getEndpoint();
  void _serialize(std::ostringstream &oss, int rco2, int particleCount003, float pm01, float pm25,
                  float pm10, int tvoc, int nox, float atmp, float rhum, int signal,
                  float vBat = -1.0f, float vPanel = -1.0f, float o3WorkingElectrode = -1.0f,
                  float o3AuxiliaryElectrode = -1.0f, float no2WorkingElectrode = -1.0f,
                  float no2AuxiliaryElectrode = -1.0f, float afeTemp = -1.0f,
                  int particleCount005 = -1, int particleCount01 = -1, int particleCount02 = -1,
                  int particleCount50 = -1, int particleCount10 = -1, float pm25Sp = -1.0f);

  // Single CoAP request attempt - handles Piggyback and Separate ACK
  bool _coapRequest(const std::vector<uint8_t> &reqBuffer,
                    uint16_t expectedMessageId,
                    const uint8_t *expectedToken,
                    uint8_t expectedTokenLen,
                    CoapPacket::CoapPacket *respPacket,
                    int timeoutMs = 60000);

  bool _coapConnect();
  void _coapDisconnect(bool keepConnection);

  // CoAP request with retry logic (up to 3 attempts)
  bool _coapRequestWithRetry(const std::vector<uint8_t> &reqBuffer,
                             uint16_t expectedMessageId,
                             const uint8_t *expectedToken,
                             uint8_t expectedTokenLen,
                             CoapPacket::CoapPacket *respPacket,
                             int timeoutMs = 60000,
                             int maxRetries = 3);
};

#endif // ESP8266
#endif // AIRGRADIENT_CELLULAR_CLIENT_H
