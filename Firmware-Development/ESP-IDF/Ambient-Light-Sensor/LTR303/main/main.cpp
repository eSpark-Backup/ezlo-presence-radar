#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "ALS_LTR303.hpp"

extern "C" 
{ 
	void app_main(); 
} 

LightSensor* Light;
void app_main()
{
    Light = new ALS_LTR303;
    Light->setup();

    while (1)
    { 
        if(Light->loop() == ESP_OK) 
            printf(" lux: %f\n",Light->getLux());
    }  
}

