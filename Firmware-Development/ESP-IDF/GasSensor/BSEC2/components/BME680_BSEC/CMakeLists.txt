idf_component_register(SRCS "bme680_bsec.c"
                    INCLUDE_DIRS "."
                    REQUIRES BME68x_Sensor_library Bosch_BSEC2)