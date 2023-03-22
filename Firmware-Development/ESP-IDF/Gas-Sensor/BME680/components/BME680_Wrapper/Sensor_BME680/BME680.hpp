#pragma once
#include "GasSensor.hpp"

#define DEBUG 0

class BME680 : public GasSensor
{

    public:
        esp_err_t setup(void) override; 
        esp_err_t loop(void) override; 
};