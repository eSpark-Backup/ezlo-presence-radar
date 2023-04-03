#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

class NETIF 
{
    private:
    esp_netif_t *eth_netif_spi = NULL; /* pointer to esp-netif object for SPI Ethernet*/

    public:
    void setup();
    void begin();
    void createSpiInstance();
    esp_netif_t* getInstance();
    void setETHGlue(esp_eth_handle_t eth_handler);

};