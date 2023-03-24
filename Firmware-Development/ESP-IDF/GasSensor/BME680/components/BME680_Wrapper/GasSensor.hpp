#pragma once
#include "esp_system.h"

struct GasSensorOut
{
    float temperature;
    float pressure;
    float humidity;
    float gas_resistance;
    float altitude;
};

class GasSensor 
{
    public:
        virtual esp_err_t setup(void) { return ESP_OK; }
        virtual esp_err_t test(void) { return ESP_OK; }
        virtual esp_err_t read(GasSensorOut*) { return ESP_OK; }
};