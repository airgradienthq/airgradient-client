set(srcs 
  "src/airgradientClient.cpp"
  "src/airgradientCellularClient.cpp"
  "src/atCommandHandler.cpp"
  "src/cellularModule.cpp"
  "src/cellularModuleA7672xx.cpp"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "src"
		    REQUIRES esp_timer AirgradientSerial esp_driver_gpio
                    )
