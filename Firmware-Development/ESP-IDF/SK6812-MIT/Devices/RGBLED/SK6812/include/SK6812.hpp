#pragma once

#include <led_strip.h> 
#include "driver/gpio.h"
#include "color_codes.h"
#include "RGBLED.hpp"

#define LED_TYPE        LED_STRIP_SK6812
#define LED_GPIO        GPIO_NUM_47
#define LED_STRIP_LEN   1

class SK6812 : public RGBLED
{
private:
    led_strip_t strip;
public:
    SK6812(gpio_num_t = LED_GPIO, led_strip_type_t = LED_TYPE, size_t = LED_STRIP_LEN);
    ~SK6812();
    void setup() override;
    void show() override;
    void setColor(uint32_t colorCode) override;
    void setBrigtness(uint8_t brigtness) override;
    void test() override;
};
