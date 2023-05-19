#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pms5003.h"
#include "SK6812.h"
#include "w5500.h"

led_strip_t sk6812;
PM25_AQI_Data data;
w5500_t w5500;
s_ezlopi_spi_master_t spi;

void app_main()
{
    pms_init(&data);
    sk6812_setup(&sk6812); 
    sk6812_set_color(&sk6812, CYAN);
    ezlo_w5500_setup(&w5500, &spi);

    while (1)
    {
        pms_print_data(&data);
       for(uint8_t brightness = 0; brightness <= 254; brightness++)
       {
            sk6812_set_brightness(&sk6812, brightness);
            sk6812_show(&sk6812);
            vTaskDelay(50 / portTICK_RATE_MS);
       }
    }
}
