#pragma once
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "string.h"

#define LD2410_BAUDRATE 256000
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
int ld2410_uart_read(void);  

#ifdef __cplusplus
}
#endif
