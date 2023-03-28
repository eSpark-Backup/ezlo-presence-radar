#pragma once
#include "esp_system.h"

enum Precision
{
    enabled = 1,
    disabled = 0,
};

class GasSensor 
{
    protected:
        struct Fields
        {
            // Inputs
            Precision precision;

            // Outputs
            float temperature;
            float pressure;
            float humidity;
            float gas_resistance;
            float altitude;
        };

    public:
        Fields fields;
        virtual esp_err_t setup(Precision precision) { return ESP_OK; }
        virtual esp_err_t test(void) { return ESP_OK; }
        virtual esp_err_t read(Fields*) { return ESP_OK; }
        virtual esp_err_t setPrecision(Precision precision) { return ESP_OK; }
};