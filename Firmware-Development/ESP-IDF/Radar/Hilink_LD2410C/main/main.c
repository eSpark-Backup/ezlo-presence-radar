#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "ld2410.h"

ld2410_outputs_t ld2410_output;
void app_main(void)
{
    esp_err_t ret = ESP_OK;
    ret = ld2410_setup();

    while (1) 
    {
        if (ESP_OK == ret)
            ld2410_get_data(&ld2410_output);
    }
}
