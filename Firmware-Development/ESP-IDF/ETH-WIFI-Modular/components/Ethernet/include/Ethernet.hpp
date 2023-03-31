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

    public:
    Ethernet(){}
    ~Ethernet(){}
    void setup();
};
