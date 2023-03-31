#pragma once

#include "Ethernet.hpp"

/* Ambient Tracker Pro : W5500 Pins & Configurations */
#define W5500_ETH_PWR_EN           GPIO_NUM_13
#define W5500_ETH_SPI_CS           GPIO_NUM_5
#define W5500_ETH_SPI_MOSI         GPIO_NUM_2
#define W5500_ETH_SPI_MISO         GPIO_NUM_1
#define W5500_ETH_SPI_SCLK         GPIO_NUM_4
#define W5500_ETH_SPI_INT          GPIO_NUM_12
#define W5500_ETH_SPI_PHY_RST      GPIO_NUM_11
#define W5500_ETH_SPI_PHY_ADDR     1
#define W5500_ETH_SPI_HOST         (spi_host_device_t)2
#define W5500_ETH_SPI_CLOCK_MHZ    12


class W5500 : public Ethernet
{
    private:
    esp_eth_mac_t *mac_spi = NULL;
    esp_eth_phy_t *phy_spi = NULL;
    eth_w5500_config_t w5500_config;

    public:
    W5500(){}
    ~W5500(){}
};