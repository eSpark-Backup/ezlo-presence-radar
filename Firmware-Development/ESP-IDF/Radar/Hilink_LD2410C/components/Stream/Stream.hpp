#pragma once
#include "esp_system.h"
#define F(x) x
#define HEX 0

class Stream 
{
    public:
        int available(void); 
        size_t write(uint8_t byte);
        uint8_t read(void);  

        size_t print(const char* buf);
        size_t print(const char buf);
        size_t print(float buf);
        size_t print(double buf);
        size_t print(uint32_t buf);
        size_t print(uint16_t buf);
        size_t print(uint8_t buf);
        size_t print(uint8_t buf, uint8_t base);
        size_t println(const char* buf);
        size_t println(const char buf);
        size_t println(float buf);
        size_t println(uint32_t buf);
        size_t println(uint8_t buf);
};