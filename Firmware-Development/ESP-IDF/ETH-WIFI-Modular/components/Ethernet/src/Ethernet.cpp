#include "Ethernet.hpp"

static const char* TAG = "Ethernet";

void Ethernet::setup()
{
    // netif.setup();
}

/* Initialize MAC configs to default */
void Ethernet::configMAC()
{
    mac_config_spi = (eth_mac_config_t)ETH_MAC_DEFAULT_CONFIG();
}

/* Initialize PHY configs to default */
void Ethernet::configPHY()
{
    phy_config_spi = (eth_phy_config_t)ETH_PHY_DEFAULT_CONFIG();
}

void Ethernet::setPHYConfig(uint8_t phy_addr, gpio_num_t resetPin)
{
    this->phy_config_spi.phy_addr = phy_addr;
    this->phy_config_spi.reset_gpio_num = resetPin;
}

void Ethernet::setEthConfigSPI()
{
    eth_config_spi = ETH_DEFAULT_CONFIG(this->mac_spi, this->phy_spi);
}

void Ethernet::installDriver()
{
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi));
}

void Ethernet::setMacAddress()
{
    /* 
        The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
        02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    uint8_t macAddr[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + 0x00};
    ESP_ERROR_CHECK(esp_eth_ioctl(this->eth_handle_spi, ETH_CMD_S_MAC_ADDR, macAddr));
}

void Ethernet::attachToNetif(esp_netif_t* eth_netif_spi)
{
    // attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(this->eth_handle_spi)));
}