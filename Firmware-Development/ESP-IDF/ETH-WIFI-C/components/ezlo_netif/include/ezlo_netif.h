#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"

esp_err_t netif_begin();
esp_err_t netif_eth_spi_setup(esp_netif_t **eth_netif_spi);
esp_err_t netif_create_spi_instance(esp_netif_t **eth_netif_spi);