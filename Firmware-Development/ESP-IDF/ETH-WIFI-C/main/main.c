#include <stdio.h>
#include "ezlo_netif.h"
#include "ezlo_w5500.h"
#include "ezlo_spi.h"
#include "w5500_ambient_pro.h"

static const char TAG[] = "Main-App";

void app_main(void)
{
    w5500_t w5500;
    netif_spi_eth_t netif_obj;

    if(ESP_OK == w5500_setup(&w5500))
        ESP_LOGI(TAG, "W5500 - Setup Completed.");
    if(ESP_OK == netif_eth_spi_setup(&netif_obj))
        ESP_LOGI(TAG, "Netif - Ethernet SPI Setup Is Successfull.");
    if(ESP_OK == spi_bus_init(W5500_HOST, W5500_MOSI, W5500_MISO, W5500_SCLK))
        ESP_LOGI(TAG, "SPI - Bus Initialize.");
}
