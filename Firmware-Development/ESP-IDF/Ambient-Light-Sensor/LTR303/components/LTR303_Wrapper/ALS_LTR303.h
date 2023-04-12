#pragma once

#include "LTR303.h"

#define LTR303_SDA          GPIO_NUM_18
#define LTR303_SCL          GPIO_NUM_17
#define LTR303_INT_ALS_PIN  GPIO_NUM_16
#define ACTIVE_I2C          I2C_NUM_0

#define DEBUG 0

esp_err_t ltr303_setup(bool initialize_i2c, gpio_num_t sda, gpio_num_t scl, i2c_port_t i2c_num, uint8_t ltr303_address); 
esp_err_t ltr303_loop(void); 
esp_err_t ltr303_get_val(double* lux); 
bool ltr303_is_data_available(void);