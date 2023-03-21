#include "RGBLED.hpp"

RGBLED::RGBLED(led_strip_type_t ledType, gpio_num_t ledGPIO, size_t ledStripLen)
{
        strip.type = ledType;
        strip.length = ledStripLen;
        strip.gpio = ledGPIO;
        strip.buf = NULL;
        strip.brightness = 255;
}

void RGBLED::setup()
{
    led_strip_init(&strip);
}

void RGBLED::setColor(rgb_t color)
{
    led_strip_fill(&strip, 0, strip.length, color);
}

void RGBLED::setBrigtness(uint8_t brigtness)
{
    led_strip_set_brightness(&strip, brigtness);
}