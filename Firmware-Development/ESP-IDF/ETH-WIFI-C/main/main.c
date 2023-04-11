#include <stdio.h>
#include "ezlo_netif.h"

static const char* TAG = "Main";

void app_main(void)
{
    netif_spi_eth_t netif_obj;
    if(ESP_OK == netif_eth_spi_setup(&netif_obj) )
    {
        ESP_LOGI(TAG, "Successfully setup netif for ethernet spi.");
    }
}
