#include "W5500.hpp"

W5500 w5500;
static const char* TAG = "MAIN-APP";

extern "C" void app_main(void)
{
    // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
    gpio_install_isr_service(0);

    w5500.setup();
    w5500.setDefaultMAC();
    w5500.setDefaultPHY();
    w5500.setPHYConfig(W5500_ETH_SPI_PHY_ADDR, W5500_ETH_SPI_PHY_RST);
    w5500.setConfig(W5500_ETH_SPI_INT);
    w5500.mac_spi = esp_eth_mac_new_w5500(&w5500.w5500_config, &w5500.mac_config_spi);
    w5500.phy_spi = esp_eth_phy_new_w5500(&w5500.phy_config_spi);
    w5500.eth_config_spi = ETH_DEFAULT_CONFIG(w5500.mac_spi, w5500.phy_spi);
    w5500.setETHDriver();
    w5500.setMacAddress();
    w5500.setIOLayer();
    w5500.registerCallBacks();
    w5500.start();
}