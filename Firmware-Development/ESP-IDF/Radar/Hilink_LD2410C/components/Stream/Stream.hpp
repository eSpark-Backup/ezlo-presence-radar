#pragma once
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"

#define RADAR_TX_PIN GPIO_NUM_43
#define RADAR_RX_PIN GPIO_NUM_44

static const char *TAG = "uart_echo";
#define UART_PORT_NUM UART_NUM_0
#define F(x) x
#define HEX 0

class Stream 
{
    public:
        esp_err_t setup(void);
        int available(void); 
        size_t write(uint8_t byte);
        size_t write(const uint8_t *buffer, size_t size);
        inline size_t write(const char * buffer, size_t size)
        {
            return write((uint8_t*) buffer, size);
        }
        inline size_t write(const char * s)
        {
            return write((uint8_t*) s, strlen(s));
        }
        inline size_t write(unsigned long n)
        {
            return write((uint8_t) n);
        }
        inline size_t write(long n)
        {
            return write((uint8_t) n);
        }
        inline size_t write(unsigned int n)
        {
            return write((uint8_t) n);
        }
        inline size_t write(int n)
        {
            return write((uint8_t) n);
        }
        int read(void);  

        // Body not implemented 
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