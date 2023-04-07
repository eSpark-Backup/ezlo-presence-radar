#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pms5003.hpp"

void app_main()
{    
    pms_init();
    PM25_AQI_Data data;

    while(1)
    {
        if(ESP_OK == pms_read(&data) )
            pms_print_data(&data);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
