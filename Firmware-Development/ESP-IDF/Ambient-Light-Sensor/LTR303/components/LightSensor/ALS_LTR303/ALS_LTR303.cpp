#include "ALS_LTR303.hpp"
static const char *TAG_MAIN = "LTR303";

esp_err_t ALS_LTR303::setup(void)
{  
    light.begin();
    unsigned char ID;
  
    if (light.getPartID(ID)) {
        printf("Got Sensor Part ID: %02X\n", ID);
    }
    if (light.getManufacID(ID)) {
        printf("Got Manuf Part ID: %02X\n", ID);
    }

    printf("Setting Gain...\n");
    light.setControl(LTR3XX_GAIN_1, false, false);
    
    printf("Set timing...\n");
    light.setMeasurementRate(LTR3XX_INTEGTIME_200,LTR3XX_MEASRATE_200);
    
    printf("Powerup...\n");
    light.setPowerUp();

    return ESP_OK;
}

esp_err_t ALS_LTR303::loop(void)
{
    light.getStatus(valid, gain, intrstatus, dataStatus);  
    if (dataStatus)
    {        
        unsigned int data0, data1;
        if (light.getData(data0,data1)) {
            
            // getData() returned true, communication was successful
            #if DEBUG
                printf("data0: %d\n",data0);
                printf("data1: %d\n",data1);
            #endif
            // To calculate lux, pass all your settings and readings 
            // to the getLux() function.
            
            // The getLux() function will return 1 if the calculation
            // was successful, or 0 if one or both of the sensors was
            // saturated (too much light). If this happens, you can
            // reduce the integration time and/or gain.
        
            double lux;    // Resulting lux value

            // Perform lux calculation:
            light.getLux(gain,integrationTime,data0,data1,lux);
            ALSLux = lux;
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

double ALS_LTR303::getLux(void)
{
    return ALSLux;
}