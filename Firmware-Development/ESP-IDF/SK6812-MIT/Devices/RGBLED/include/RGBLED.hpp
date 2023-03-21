#ifndef RGBLED_HPP
#define RGBLED_HPP

#include <led_strip.h> 
#include "driver/gpio.h"
#include "color_codes.h"

class RGBLED
{
private:
    led_strip_t strip;
public:
    RGBLED(led_strip_type_t ledType, gpio_num_t ledGPIO, size_t ledStripLen);
    ~RGBLED();
    void setup();
    void setColor(rgb_t color);
    void setBrigtness(uint8_t brigtness);
};

#endif