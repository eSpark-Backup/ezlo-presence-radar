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
#include "netif.hpp"
#include "spi.hpp"

class Ethernet
{
    private: 
    // NETIF netif;
    // SPI spi;
    // uint8_t phyAddr;
    esp_eth_mac_t *mac_spi = NULL;
    esp_eth_phy_t *phy_spi = NULL;

    public:
    eth_mac_config_t mac_config_spi;
    eth_phy_config_t phy_config_spi;
    esp_eth_handle_t eth_handle_spi;
    esp_eth_config_t eth_config_spi;
    
    Ethernet(){}
    ~Ethernet(){}
    void setup();
    void configMAC();
    void configPHY();
    void setPHYConfig(uint8_t phy_addr, gpio_num_t resetPin);
    void setEthConfigSPI();
    void installDriver();
    void setMacAddress();
    void attachToNetif(esp_netif_t* eth_netif_spi);
    
};
