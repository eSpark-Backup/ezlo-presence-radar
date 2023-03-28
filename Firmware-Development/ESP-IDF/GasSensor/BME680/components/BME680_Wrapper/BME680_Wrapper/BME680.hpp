#pragma once
#include "esp_system.h"
#include "Adafruit_BME680.h"
#include "GasSensor.hpp"

#define I2C_MASTER_SCL_IO (gpio_num_t)17
#define I2C_MASTER_SDA_IO (gpio_num_t)18
#define SEALEVELPRESSURE_HPA (1013.25)


class BME680 : public GasSensor
{
    public:
        Fields fields;
        esp_err_t setup(Precision precision) override; 
        esp_err_t test(void) override;
        esp_err_t read(Fields*) override;  
        esp_err_t setPrecision(Precision precision) override;
};