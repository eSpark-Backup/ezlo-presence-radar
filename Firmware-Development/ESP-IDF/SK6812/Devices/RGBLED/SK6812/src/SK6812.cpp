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
    setColor(color.r, color.g, color.b);
}

/*  @func setColor
            It sets the color for the RGB-LED in the strip struct. 
    @params  
    uint8_t r (Red) , uint8_t g (Green), uint8_t b (Blue). 
        Takes individual rgb values and set the color accordingly.
*/
void SK6812::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    rgb_t color = {
        .r = r,
        .g = g,
        .b = b,
    };
    led_strip_fill(&strip, 0, strip.length, color);
}

/*  @func setColor
            It sets the color for the RGB-LED in the strip struct. 
    @params  
        uint8_t rgb[3] - takes an address of array which contains rgb values.
*/
void SK6812::setColor(uint8_t* rgb)
{
    setColor(rgb[0], rgb[1], rgb[2]);
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
    setColor(MAGENTA);
    setBrigtness(10);
    show();
}