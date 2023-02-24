idf_component_register(SRCS "DeviceControl.c" "DeviceConfig.c" "DeviceCalc.c"
                    INCLUDE_DIRS "include" REQUIRES ifxBase ifxRadarDeviceCommon ifxRecording bgt60t13c debug)
