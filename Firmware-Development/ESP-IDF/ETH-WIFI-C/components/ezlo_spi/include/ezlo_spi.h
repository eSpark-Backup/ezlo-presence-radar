#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"

esp_err_t spi_bus_init(spi_host_device_t host, gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk);