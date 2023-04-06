#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "SK6812.hpp"

led_strip_t sk6812;

extern "C" void app_main()
{
    sk6812_setup(&sk6812); 
    sk6812_set_color(&sk6812, CYAN);

    while (1)
    {
       for(uint8_t brightness = 0; brightness <= 255; brightness++)
       {
            sk6812_set_brightness(&sk6812, brightness);
            sk6812_show(&sk6812);
            vTaskDelay(50 / portTICK_RATE_MS);
       }
    }
}
