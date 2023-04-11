#include <stdio.h>
#include "ezlo_netif.h"
#include "ezlo_w5500.h"
#include "ezlo_spi.h"
#include "w5500_ambient_pro.h"

static const char* TAG = "Main-App";

void app_main(void)
{
    w5500_t w5500;
    netif_spi_eth_t netif_obj;
    spi_t eth_spi;

    w5500_setup(&w5500);
    netif_eth_spi_setup(&netif_obj);

    spi_init(&eth_spi, W5500_HOST, W5500_MOSI, W5500_MISO, W5500_SCLK);
    spi_register_device(&eth_spi, 0, W5500_CS, W5500_ADDR_BITS, W5500_CMD_BITS, 10);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}
