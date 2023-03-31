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

#define SPI_NC_PIN (gpio_num_t)-1

class SPI
{
    private:
    gpio_num_t cs;
    gpio_num_t miso;
    gpio_num_t mosi;
    gpio_num_t sclk;
    gpio_num_t intr;
    gpio_num_t rst;
    uint8_t clkSpeed; //MHZ
    spi_host_device_t host;
    spi_device_handle_t spi_handle = NULL;

    public:
    void setPins(gpio_num_t cs, gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t intr = SPI_NC_PIN, gpio_num_t rst = SPI_NC_PIN);
    void setSpeed(uint8_t clkSpeed);
    void setHost(spi_host_device_t host);
    void initBus();

};