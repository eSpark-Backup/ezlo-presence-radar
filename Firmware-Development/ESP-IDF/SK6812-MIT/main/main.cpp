#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "RGBLED.hpp"

#define LED_TYPE        LED_STRIP_SK6812
#define LED_GPIO        GPIO_NUM_47
#define LED_STRIP_LEN   1

void rgbTest(void *pvParameters)
{
    /* Colors to Display on the SK6812 */
    rgb_t colors[] = {
        rgb_from_code(RED),
        rgb_from_code(GREEN),
        rgb_from_code(BLUE),
        rgb_from_code(YELLOW),
        rgb_from_code(MAGENTA),
        rgb_from_code(CYAN),
    };
    size_t totalColor = sizeof(colors) / sizeof(rgb_t);

    RGBLED SK6812(LED_TYPE, LED_GPIO, LED_STRIP_LEN); /* SK6812 RGB LED Configuration */
    SK6812.setup(); /* Initialize SK6812 */
    
    size_t c = 0;
    while (1)
    {
        SK6812.setColor(colors[c]);
        for(int i = 0; i < 255; i++)
        {
            SK6812.setBrigtness(i);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(500));

        if (++c >= totalColor)
            c = 0;
    }
}

extern "C" void app_main()
{
    led_strip_install();
    xTaskCreate(rgbTest, "rgbTest", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
