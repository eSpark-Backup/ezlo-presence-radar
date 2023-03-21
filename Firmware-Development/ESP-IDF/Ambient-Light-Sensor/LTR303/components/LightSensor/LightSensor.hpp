#pragma once
#include "esp_system.h"

#define DEBUG 0

class LightSensor 
{
    public:
        virtual esp_err_t setup(void) { return ESP_OK; }
        virtual esp_err_t loop(void) { return ESP_OK; }
        virtual esp_err_t getLux(double* ALSLux) { return 0; }
};