#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pms5003.hpp"

PMS5003* dustSensor;

extern "C" void app_main()
{    
    dustSensor = new PMS5003;
    dustSensor->setup();
    while(1)
    {
        dustSensor->makeMeasurement();
        dustSensor->printData();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
