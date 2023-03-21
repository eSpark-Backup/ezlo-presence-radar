#pragma once
#include "LTR303.h"
#include "LightSensor.hpp"

#define DEBUG 0

class ALS_LTR303 : public LightSensor
{
    unsigned char gain = LTR3XX_GAIN_1;     // Gain setting, values = 0-7 
    unsigned char integrationTime = LTR3XX_INTEGTIME_200;  // Integration ("shutter") time in milliseconds
    unsigned char measurementRate;  // Interval between DATA_REGISTERS update

    bool valid=0, intrstatus=0, dataStatus=0;
    byte error;
    double ALSLux;

    LTR303 light;

    public:
        esp_err_t setup(void) override; 
        esp_err_t loop(void) override; 
        double getLux(void) override; 
};