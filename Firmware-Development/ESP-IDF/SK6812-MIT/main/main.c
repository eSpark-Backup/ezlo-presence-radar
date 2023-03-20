#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led_strip.h>

#define LED_TYPE        LED_STRIP_SK6812
#define LED_GPIO        GPIO_NUM_47
#define LED_STRIP_LEN   1

static const rgb_t colors[] = {
    { .r = 0xff, .g = 0x00, .b = 0x00 },    //RED
    { .r = 0x00, .g = 0xff, .b = 0x00 },    //BLUE
    { .r = 0x00, .g = 0x00, .b = 0xff },    //GREEN
};

#define COLORS_TOTAL (sizeof(colors) / sizeof(rgb_t))

void test(void *pvParameters)
{
    led_strip_t strip = {
        .type = LED_TYPE,
        .length = LED_STRIP_LEN,
        .gpio = LED_GPIO,
        .buf = NULL,
        .brightness = 255,
    };

    ESP_ERROR_CHECK(led_strip_init(&strip));

    size_t c = 0;
    while (1)
    {
        ESP_ERROR_CHECK(led_strip_fill(&strip, 0, strip.length, colors[c]));
        if(c == 0)
            strip.brightness = 255;
        else if(c == 1)
            strip.brightness = 100;
        else
            strip.brightness = 50;
        ESP_ERROR_CHECK(led_strip_flush(&strip));

        vTaskDelay(pdMS_TO_TICKS(1000));

        if (++c >= COLORS_TOTAL)
            c = 0;
    }
}

void app_main()
{
    led_strip_install();
    xTaskCreate(test, "test", configMINIMAL_STACK_SIZE * 5, NULL, 5, NULL);
}
