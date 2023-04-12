#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "ALS_LTR303.h"

ltr303_data_t light_sensor_data;

void app_main()
{
    ltr303_setup(true); // true : initialize i2c, false : skips i2c initialization

    while (1)
    { 
        if(ltr303_get_val(&light_sensor_data) == ESP_OK)
        { 
            printf(" lux: %f\n",light_sensor_data.lux);
        }
    }  
}

