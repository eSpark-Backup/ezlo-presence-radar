#pragma once

#ifndef MAIN_H_
#define MAIN_H_

//#include <avr/io.h> // all the standard AVR functions
#define __DELAY_BACKWARD_COMPATIBLE__ // this enables uint32 to be used in sleep functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dw3000_uart.h"
#include "dw3000_port.h"
#include "dw3000_device_api.h"
#include "dw3000_shared_functions.h"

#define _BV(n) (1 << n) // sets 1 at position of BIT "n"

typedef uint8_t byte;
typedef bool boolean;

#define HIGH    1
#define LOW     0

#define OUTPUT  GPIO_MODE_OUTPUT
#define INPUT   GPIO_MODE_INPUT

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))


#define delay(ms)  vTaskDelay((ms) / portTICK_PERIOD_MS)

static inline esp_err_t digitalWrite(unsigned char pin, bool state){
	return gpio_set_level((gpio_num_t)pin, state);
}

static inline esp_err_t pinMode(unsigned char pin, gpio_mode_t mode)
{
  return gpio_set_direction((gpio_num_t)pin, mode);
}

#define __INLINE inline

#endif /* MAIN_H_ */
