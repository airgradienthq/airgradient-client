set(srcs
  "src/airgradientClient.cpp"
  "src/airgradientCellularClient.cpp"
  "src/airgradientWifiClient.cpp"
  "src/atCommandHandler.cpp"
  "src/cellularModule.cpp"
  "src/cellularModuleA7672xx.cpp"

  # CoAP
  "src/coap-packet-cpp/src/CoapBuilder.cpp"
  "src/coap-packet-cpp/src/CoapParser.cpp"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "src"
                    REQUIRES esp_timer AirgradientSerial esp_driver_gpio esp_http_client arduinojson
                    )
