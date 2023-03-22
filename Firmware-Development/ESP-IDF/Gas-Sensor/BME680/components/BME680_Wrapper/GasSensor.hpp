#pragma once
#include "esp_system.h"

class GasSensor 
{
    public:
        virtual esp_err_t setup(void) { return ESP_OK; }
        virtual esp_err_t loop(void) { return ESP_OK; }
};