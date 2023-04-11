#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"

typedef struct 
{
    gpio_num_t power_pin;
    gpio_num_t reset_pin;
}w5500_t;

esp_err_t w5500_setup(w5500_t* w5500);
esp_err_t w5500_gpio_config(gpio_num_t gpio_num);
esp_err_t w5500_power_pin_config(gpio_num_t power_pin);
esp_err_t w5500_reset_pin_config(gpio_num_t reset_pin);
void w5500_startup_reset(w5500_t* w5500);
