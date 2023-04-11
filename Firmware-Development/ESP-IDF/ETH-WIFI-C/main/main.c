#include <stdio.h>
#include "ezlo_netif.h"
#include "ezlo_w5500.h"

static const char TAG[] = "Main-App";

void app_main(void)
{
    w5500_t w5500;
    netif_spi_eth_t netif_obj;

    if(ESP_OK == w5500_setup(&w5500))
        ESP_LOGI(TAG, "W5500 - Setup Completed.");
    if( ESP_OK == netif_eth_spi_setup(&netif_obj) )
        ESP_LOGI(TAG, "Netif - Ethernet SPI Setup Is Successfull.");
}
