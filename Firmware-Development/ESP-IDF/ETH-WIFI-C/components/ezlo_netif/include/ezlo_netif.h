#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

typedef struct 
{
    esp_netif_t *eth_netif_spi;         /* pointer to esp-netif object for SPI Ethernet*/
    esp_eth_handle_t eth_handle_spi;    /* handle of Ethernet driver */
}netif_spi_eth_t;

esp_err_t netif_begin();
esp_err_t netif_eth_spi_setup(netif_spi_eth_t* netif_obj);
esp_err_t netif_create_spi_instance(netif_spi_eth_t* netif_obj);
void netif_set_eth_glue(netif_spi_eth_t* netif_obj);

