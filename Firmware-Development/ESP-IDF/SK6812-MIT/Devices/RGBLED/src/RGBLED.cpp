#include "RGBLED.hpp"

RGBLED::RGBLED(gpio_num_t ledGPIO, led_strip_type_t ledType , size_t ledStripLen)
{
        strip.type = ledType;
        strip.length = ledStripLen;
        strip.gpio = ledGPIO;
        strip.buf = NULL;
        strip.brightness = 255;
        strip.channel = RMT_CHANNEL_0;
}

void RGBLED::setup()
{
    led_strip_install();
    led_strip_init(&strip);
}

void RGBLED::setColor(uint32_t colorCode)
{
    rgb_t color = (rgb_t)rgb_from_code(colorCode);
    led_strip_fill(&strip, 0, strip.length, color);
}

void RGBLED::setBrigtness(uint8_t brigtness)
{
    led_strip_set_brightness(&strip, brigtness);
}

void RGBLED::test()
{
    setColor(RED);
    for(int i = 0; i < 255; i++)
    {
        setBrigtness(i);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}