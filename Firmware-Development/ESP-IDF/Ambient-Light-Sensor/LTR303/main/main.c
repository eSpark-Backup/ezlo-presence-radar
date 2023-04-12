#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "ALS_LTR303.h"

void app_main()
{
    ltr303_setup(true); // true : initialize i2c, false : skips i2c initialization

    double lux;
    while (1)
    { 
        if(ltr303_get_val(&lux) == ESP_OK)
        { 
            printf(" lux: %f\n",lux);
        }
    }  
}

