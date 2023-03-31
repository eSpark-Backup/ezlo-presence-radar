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