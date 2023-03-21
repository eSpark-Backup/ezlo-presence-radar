#pragma once

class RGBLED
{
private:
    led_strip_t strip;
public:
    RGBLED(gpio_num_t = (gpio_num_t)0, led_strip_type_t = (led_strip_type_t)0, size_t = (size_t)0){}
    ~RGBLED(){}
    virtual void setup(){}
    virtual void show(){}
    virtual void setColor(uint32_t colorCode){}
    virtual void setBrigtness(uint8_t brigtness){}
    virtual void test(){}
};
