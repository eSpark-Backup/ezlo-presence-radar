#ifndef RGBLED_HPP
#define RGBLED_HPP

#include <led_strip.h> 
#include "driver/gpio.h"
#include "color_codes.h"

#define LED_TYPE        LED_STRIP_SK6812
#define LED_GPIO        GPIO_NUM_47
#define LED_STRIP_LEN   1

class RGBLED
{
private:
    led_strip_t strip;
    rgb_t color;
public:
    RGBLED(gpio_num_t = LED_GPIO, led_strip_type_t = LED_TYPE, size_t = LED_STRIP_LEN);
    ~RGBLED();
    void setup();
    void setColor(uint32_t colorCode);
    void setBrigtness(uint8_t brigtness);
    void test();
};

#endif