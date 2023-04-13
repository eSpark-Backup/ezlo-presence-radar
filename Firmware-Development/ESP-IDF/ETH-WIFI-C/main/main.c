#include <stdio.h>
#include "ezlo_netif.h"
#include "ezlo_w5500.h"
#include "ezlo_spi.h"
#include "w5500_ambient_pro.h"

static const char* TAG = "Main-App";

esp_netif_t* eth_netif_spi = NULL;
esp_eth_handle_t eth_handle_spi = NULL;

w5500_t w5500;
spi_t eth_spi;
    
void app_main(void)
{
    // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
    gpio_install_isr_service(0);

    // **** W5500 Begin ****
    //W5500 & Netif Setup
    w5500_setup(&w5500);
    netif_eth_spi_setup(&eth_netif_spi);

    //SPI SETUP
    spi_init(&eth_spi, W5500_HOST, W5500_MISO, W5500_MOSI, W5500_SCLK);
    spi_register_device(&eth_spi, 0, W5500_CS, W5500_ADDR_BITS, W5500_CMD_BITS,  12000000);
    // **** W5500 Begin ****

    set_default_MAC(&w5500.eth_mac_obj);
    set_default_PHY(&w5500.eth_phy_obj);
    set_reset_pin_PHY(&w5500.eth_phy_obj, W5500_RST);
    w5500_set_config(&w5500.w5500_obj, eth_spi.handle, W5500_INTR);

    w5500.mac_spi = esp_eth_mac_new_w5500(&w5500.w5500_obj, &w5500.eth_mac_obj);
    w5500.phy_spi = esp_eth_phy_new_w5500(&w5500.eth_phy_obj);
    w5500.eth_obj = (esp_eth_config_t)ETH_DEFAULT_CONFIG(w5500.mac_spi, w5500.phy_spi);
    w5500_eth_driver_install(&w5500.eth_obj, &eth_handle_spi);
    w5500_set_mac_address(eth_handle_spi);
    w5500_netif_glue(eth_netif_spi, eth_handle_spi);
    w5500_register_cbs();
    w5500_start(eth_handle_spi);

    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
}
