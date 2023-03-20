#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led_strip.h>

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

    /* SK6812 Configuration */
    led_strip_t strip = {
        .type = LED_TYPE,
        .length = LED_STRIP_LEN,
        .gpio = LED_GPIO,
        .buf = NULL,
        .brightness = 255,
    };
    led_strip_init(&strip);
    
    size_t c = 0;
    while (1)
    {
        led_strip_fill(&strip, 0, strip.length, colors[c]);
        for(int i = 0; i < 255; i++)
        {
            led_strip_set_brightness(&strip, i);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(500));

        if (++c >= totalColor)
            c = 0;
    }
}

void app_main()
{
    led_strip_install();
    xTaskCreate(rgbTest, "rgbTest", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
