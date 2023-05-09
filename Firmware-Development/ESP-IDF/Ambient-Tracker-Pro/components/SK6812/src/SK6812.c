
#include "SK6812.h"

/*
    @func   :   sk6812_setup
                (Initializes the strip with SK6812 RGB LED )

    @param  :   strip (Pointer of LED strip descriptor)
*/
led_strip_t *sk6812_setup(led_strip_t *strip)
{
    led_strip_install();
    sk6812_config(strip);
    led_strip_init(strip);
    return strip;
}
/*
    @func   :   sk6812_config
                ( sets strip to SK6812 configuration (Ambient Pro Tracker)  )
*/
esp_err_t sk6812_config(led_strip_t *strip)
{
    if (strip == NULL)
        return ESP_FAIL;
    else
    {
        strip->type = LED_TYPE;
        strip->length = LED_STRIP_LEN;
        strip->gpio = LED_GPIO;
        strip->buf = NULL;
        strip->brightness = 255;
        strip->channel = RMT_CHANNEL_0;
        return ESP_OK;
    }
}
/*  @func  :    sk6812_set_color
                (It sets the color for the RGB-LED in the strip struct.)

    @param :    strip (Pointer of LED strip descriptor)
                colorCode (24bit color-codes for e.g: 0xFF0000 - Red color)
*/
esp_err_t sk6812_set_color(led_strip_t *strip, uint32_t color_code)
{
    rgb_t color = (rgb_t)rgb_from_code(color_code);
    esp_err_t ret = led_strip_fill(strip, 0, strip->length, color);
    return ret;
}

/*  @func  :    sk6812_set_color
                (It sets the color for the RGB-LED in the strip struct.)

    @param :    strip (Pointer of LED strip descriptor)
                brightness ( 0 - 255)
*/
esp_err_t sk6812_set_brightness(led_strip_t *strip, uint8_t brightness)
{
    esp_err_t ret = led_strip_set_brightness(strip, brightness);
    return ret;
}

/*  sk6812_show : Update Strip Buffer To LED.
    This function must called after set_color or set_brightness to update LED.
*/
esp_err_t sk6812_show(led_strip_t *strip)
{
    esp_err_t ret = led_strip_flush(strip);
    return ret;
}

void sk6812(void *pvParameter)
{
    led_strip_t sk6812;

    sk6812_setup(&sk6812);
    sk6812_set_color(&sk6812, CYAN);

    while (1)
    {
        for (uint8_t brightness = 0; brightness < 255; brightness++)
        {
            sk6812_set_brightness(&sk6812, brightness);
            sk6812_show(&sk6812);
            vTaskDelay(50 / portTICK_RATE_MS);
        }
    }

    vTaskDelete(NULL);
}

TaskHandle_t sk6812_task_handle = NULL;
esp_err_t sk6812_init(void)
{
    BaseType_t ret;
    ret = xTaskCreatePinnedToCore(sk6812, "sk6812", 2048, NULL, 10, &sk6812_task_handle, 1);
    if (pdPASS == ret)
    {
        return ESP_OK;
    }
    return ESP_FAIL;
}

void sk6812_deinit(void)
{
    if (sk6812_task_handle != NULL)
    {
        vTaskDelete(sk6812_task_handle);
        sk6812_task_handle = NULL;
    }
}