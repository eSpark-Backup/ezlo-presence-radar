#include "SK6812.hpp"

SK6812::SK6812(gpio_num_t ledGPIO, led_strip_type_t ledType , size_t ledStripLen)
{
        strip.type = ledType;
        strip.length = ledStripLen;
        strip.gpio = ledGPIO;
        strip.buf = NULL;
        strip.brightness = 255;
        strip.channel = RMT_CHANNEL_0;
}
/* Initialize the Led Strip */
void SK6812::setup()
{
    led_strip_install();
    led_strip_init(&strip);
}

/*  Update Strip Buffer To LED. 
    This function must called after setColor or setBrightness to update LED.    */
void SK6812::show()
{
    led_strip_flush(&strip);
}
/*  @func setColor
            It sets the color for the RGB-LED in the strip struct. 
    @param : colorCode (24bit color-codes for e.g: 0xFF0000 - Red color)
*/
void SK6812::setColor(uint32_t colorCode)
{
    rgb_t color = (rgb_t)rgb_from_code(colorCode);
    led_strip_fill(&strip, 0, strip.length, color);
}
/*  @func setBrightness
            It sets the brightness for the RGB-LED in the strip struct. 
    @param : brightness ( the level of brightness between 0 - 255 )
*/
void SK6812::setBrigtness(uint8_t brigtness)
{
    led_strip_set_brightness(&strip, brigtness);
}

void SK6812::test()
{
   setColor(RED);
   setBrigtness(10);
   show();
}