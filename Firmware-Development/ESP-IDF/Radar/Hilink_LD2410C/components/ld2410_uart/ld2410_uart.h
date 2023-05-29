#pragma once
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "string.h"

#define RADAR_TX_PIN GPIO_NUM_12
#define RADAR_RX_PIN GPIO_NUM_13

#define UART_PORT_NUM UART_NUM_1
#define F(x) x
#define HEX 0
#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ld2410_uart_setup(void);
int ld2410_uart_available(void); 
size_t ld2410_uart_write_byte(uint8_t byte);
size_t ld2410_uart_write(const uint8_t *buffer, size_t size);
int ld2410_uart_read(void);  

// inline size_t write(const char * buffer, size_t size)
// {
//     return write((uint8_t*) buffer, size);
// }
// inline size_t write(const char * s)
// {
//     return write((uint8_t*) s, strlen(s));
// }
// inline size_t write(unsigned long n)
// {
//     return write((uint8_t) n);
// }
// inline size_t write(long n)
// {
//     return write((uint8_t) n);
// }
// inline size_t write(unsigned int n)
// {
//     return write((uint8_t) n);
// }
// inline size_t write(int n)
// {
//     return write((uint8_t) n);
// }

#ifdef __cplusplus
}
#endif
