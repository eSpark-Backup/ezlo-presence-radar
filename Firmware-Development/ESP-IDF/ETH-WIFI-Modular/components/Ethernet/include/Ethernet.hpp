#pragma once 

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"

class Ethernet
{
    public:
    struct SPIConfig {
        gpio_num_t cs;
        gpio_num_t miso;
        gpio_num_t mosi;
        gpio_num_t sclk;
        gpio_num_t intr;
        gpio_num_t phyReset;
        uint8_t phyAddr;
        uint8_t clkSpeed;
        uint8_t spiHost;
    };

    Ethernet(SPIConfig spi) : spiConfig(spi) {}
    virtual void busInit(){}
    virtual void addDevice(){}
    virtual void spiBegin(){}
    virtual void setup(){}

    private:
    SPIConfig spiConfig;
    eth_mac_config_t mac_config_spi;
    eth_phy_config_t phy_config_spi;
    esp_eth_mac_t *mac;
    esp_eth_phy_t *phy;
    esp_eth_handle_t handle = NULL; /* Handle of Ethernet Driver */
};