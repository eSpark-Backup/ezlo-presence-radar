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

typedef struct
{
    spi_bus_config_t bus_cfg;
    spi_device_interface_config_t interface_cfg;
    spi_device_handle_t handle;
    spi_host_device_t host;       
    spi_transaction_t transaction;
}spi_t;

esp_err_t spi_init(spi_t* spi_obj, const spi_host_device_t spi_peripheral, const int pin_miso, const int pin_mosi, const int pin_sclk);
esp_err_t spi_register_device(spi_t* spi_obj ,const uint8_t mode, const int ss, const int addr_length, const int command_length, const int clock_speed_devider);