/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AIRGRADIENT_CLIENT_H
#define AIRGRADIENT_CLIENT_H

#include "common.h"
#include <string>
#include <vector>

#define AIRGRADIENT_HTTP_DOMAIN "hw.airgradient.com"
#define AIRGRADIENT_COAP_DOMAIN "hw.airgradient.com"
#define AIRGRADIENT_COAP_IP "91.98.9.143"

#define MAXIMUM_PAYLOAD_BUFFER 100

class AirgradientClient {
private:
public:
  AirgradientClient() {};
  virtual ~AirgradientClient() {};

  struct CommonPayload {
    int rco2;
    float atmp;
    float rhum;
    int particleCount003[2];
    int particleCount005;
    int particleCount01;
    int particleCount02;
    int particleCount50;
    int particleCount10;
    float pm01;
    float pm25[2];
    float pm10;
    float pm25Sp[2];
    int tvocRaw;
    int noxRaw;
  };

  struct ExtraPayload {
    float vBat;
    float vPanel;
    float o3WorkingElectrode;
    float o3AuxiliaryElectrode;
    float no2WorkingElectrode;
    float no2AuxiliaryElectrode;
    float afeTemp;
  };

  enum PayloadType {
    MAX_WITH_O3_NO2 = 0,
    MAX_WITHOUT_O3_NO2 = 1,
    ONE_OPENAIR,
    ONE_OPENAIR_TWO_PMS
  };

  struct PayloadBuffer {
    CommonPayload common;
    union {
      ExtraPayload extra;
    } ext;
  };

  struct AirgradientPayload {
    int measureInterval;
    int signal;
    PayloadType payloadType;
    PayloadBuffer payloadBuffer[MAXIMUM_PAYLOAD_BUFFER];
    int bufferCount;
  };

  virtual bool begin(std::string sn, PayloadType pt);
  virtual void setAPN(const std::string &apn);
  virtual void setExtendedPmMeasures(bool enable);
  virtual void setNetworkRegistrationTimeoutMs(int timeoutMs);
  virtual std::string getICCID();
  virtual bool ensureClientConnection(bool reset);
  virtual std::string httpFetchConfig();
  virtual bool httpPostMeasures(const std::string &payload);
  virtual bool httpPostMeasures(const AirgradientPayload &payload);
  virtual bool mqttConnect();
  virtual bool mqttConnect(const char *uri);
  virtual bool mqttConnect(const std::string &host, int port, std::string username = "",
                           std::string password = "");
  virtual bool mqttDisconnect();
  virtual bool mqttPublishMeasures(const std::string &payload);
  virtual bool mqttPublishMeasures(const AirgradientPayload &payload);
  virtual std::string coapFetchConfig(bool keepConnection = false);
  virtual bool coapPostMeasures(const uint8_t* buffer, size_t length, bool keepConnection = false);
  virtual bool coapPostMeasures(const AirgradientPayload &payload, bool keepConnection = false);

  // Implemented on base class, not override function

  /**
   * @brief set http url domain for http request. Eg: hw.airgradient.com
   *
   * @param target target domain that will be used
   */
  void setHttpDomain(const std::string &target);
  void setHttpDomainDefault();
  bool isClientReady();
  void setClientReady(bool isReady);
  void resetFetchConfigurationStatus();
  void resetPostMeasuresStatus();
  bool isLastFetchConfigSucceed();
  bool isLastPostMeasureSucceed();
  bool isRegisteredOnAgServer();

protected:
  PayloadType payloadType;
  std::string httpDomain = AIRGRADIENT_HTTP_DOMAIN;
  std::string coapHostTarget = AIRGRADIENT_COAP_IP;
  const int coapPort = 5683;
  const char *const mqttDomain = "api.airgradient.com";
  const int mqttPort = 1883;
  const char *const AG_SERVER_ROOT_CA =
      "-----BEGIN CERTIFICATE-----\n"
      "MIIF4jCCA8oCCQD7MgvcaVWxkTANBgkqhkiG9w0BAQsFADCBsjELMAkGA1UEBhMC\n"
      "VEgxEzARBgNVBAgMCkNoaWFuZyBNYWkxEDAOBgNVBAcMB01hZSBSaW0xGTAXBgNV\n"
      "BAoMEEFpckdyYWRpZW50IEx0ZC4xFDASBgNVBAsMC1NlbnNvciBMYWJzMSgwJgYD\n"
      "VQQDDB9BaXJHcmFkaWVudCBTZW5zb3IgTGFicyBSb290IENBMSEwHwYJKoZIhvcN\n"
      "AQkBFhJjYUBhaXJncmFkaWVudC5jb20wHhcNMjEwOTE3MTE0NDE3WhcNNDEwOTEy\n"
      "MTE0NDE3WjCBsjELMAkGA1UEBhMCVEgxEzARBgNVBAgMCkNoaWFuZyBNYWkxEDAO\n"
      "BgNVBAcMB01hZSBSaW0xGTAXBgNVBAoMEEFpckdyYWRpZW50IEx0ZC4xFDASBgNV\n"
      "BAsMC1NlbnNvciBMYWJzMSgwJgYDVQQDDB9BaXJHcmFkaWVudCBTZW5zb3IgTGFi\n"
      "cyBSb290IENBMSEwHwYJKoZIhvcNAQkBFhJjYUBhaXJncmFkaWVudC5jb20wggIi\n"
      "MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC6XkVQ4O9d5GcUjPYRgF/uaY6O\n"
      "5ry1xCGvotxkEeKkBk99lB1oNUUfNsP5bwuDci4XKfY9Ro6/jmkfHSVcPAwUnjAt\n"
      "BcHqZtA/cMXykaynf9yXPxPQN7XLu/Rk32RIfb90sIGS318xgNziCYvzWZmlxpxc\n"
      "3gUcAgGtamlgZ6wD3yOHVo8B9aFNvmP16QwkUm8fKDHunJG+iX2Bxa4ka5FJovhG\n"
      "TnUwtso6Vrn0JaWF9qWcPZE0JZMjFW8PYRriyJmHwr/nAXfPPKphD1oRO+oA7/jq\n"
      "dYkrJw6+OHfFXnPB1xkeh4OPBzcCZHT5XWNfwBYazYpjcJa9ngGFSmg8lX1ac23C\n"
      "zea1XJmSrPwbZbWxoQznnf7Y78mRjruYKgSP8rf74KYvBe/HGPL5NQyXQ3l6kwmu\n"
      "CCUqfcC0wCWEtWESxwSdFE2qQii8CZ12kQExzvR2PrOIyKQYSdkGx9/RBZtAVPXP\n"
      "hmLuRBQYHrF5Cxf1oIbBK8OMoNVgBm6ftt15t9Sq9dH5Aup2YR6WEJkVaYkYzZzK\n"
      "X7M+SQcdbXp+hAO8PFpABJxkaDAO2kiB5Ov7pDYPAcmNFqnJT48AY0TZJeVeCa5W\n"
      "sIv3lPvB/XcFjP0+aZxxNSEEwpGPUYgvKUYUUmb0NammlYQwZHKaShPEmZ3UZ0bp\n"
      "VNt4p6374nzO376sSwIDAQABMA0GCSqGSIb3DQEBCwUAA4ICAQB/LfBPgTx7xKQB\n"
      "JNMUhah17AFAn050NiviGJOHdPQely6u3DmJGg+ijEVlPWO1FEW3it+LOuNP5zOu\n"
      "bhq8paTYIxPxtALIxw5ksykX9woDuX3H6FF9mPdQIbL7ft+3ZtZ4FWPui9dUtaPe\n"
      "ZBmDFDi4U29nhWZK68JSp5QkWjfaYLV/vtag7120eVyGEPFZ0UAuTUNqpw+stOt9\n"
      "gJ2ZxNx13xJ8ZnLK7qz1crPe8/8IVAdxbVLoY7JaWPLc//+VF+ceKicy8+4gV7zN\n"
      "Gnq2IyM+CHFz8VYMLbW+3eVp4iJjTa72vae116kozboEIUVN9rgLqIKyVqQXiuoN\n"
      "g3xY+yfncPB2+H/+lfyy6mepPIfgksd3+KeNxFADSc5EVY2JKEdorRodnAh7a8K6\n"
      "WjTYgq+GjWXU2uQW2SyPt6Tu33OT8nBnu3NB80eT8WXgdVCkgsuyCuLvNRf1Xmze\n"
      "igvurpU6JmQ1GlLgLJo8omJHTh1zIbkR9injPYne2v9ciHCoP6+LDEqe+rOsvPCB\n"
      "C/o/iZ4svmYX4fWGuU7GgqZE8hhrC3+GdOTf2ADC752cYCZxBidXGtkrGNoHQKmQ\n"
      "KCOMFBxZIvWteB3tUo3BKYz1D2CvKWz1wV4moc5JHkOgS+jqxhvOkQ/vfQBQ1pUY\n"
      "TMui9BSwU7B1G2XjdLbfF3Dc67zaSg==\n"
      "-----END CERTIFICATE-----\n";

  std::string buildFetchConfigUrl(bool useHttps = false);
  std::string buildPostMeasuresUrl(bool useHttps = false);
  std::string buildMqttTopicPublishMeasures();

  std::string serialNumber;
  bool lastPostMeasuresSucceed = true;
  bool lastFetchConfigSucceed = true;
  bool registeredOnAgServer = true;
  bool clientReady = true;
};
#endif // AIRGRADIENT_CLIENT_H
