#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pms5003.hpp"

PMS5003* dustSensor;

extern "C" void app_main()
{    
    dustSensor = new PMS5003;
    dustSensor->setup();
    pmsData data;

    while(1)
    {
        data = dustSensor->getData(); 
        ESP_LOGI("dustSensor","PM1.0 = %d , PM2.5 = %d , PM10 = %d in Standard Material. ", 
                    data.stdPM1_0 , data.stdPM2_5, data.stdPM10);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
