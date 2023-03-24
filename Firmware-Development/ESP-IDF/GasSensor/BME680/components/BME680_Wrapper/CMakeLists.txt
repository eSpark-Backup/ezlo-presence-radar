idf_component_register(SRCS "BME680_Wrapper/BME680.cpp"
                        INCLUDE_DIRS "." "BME680_Wrapper"
                        REQUIRES Adafruit_BME680_Driver)