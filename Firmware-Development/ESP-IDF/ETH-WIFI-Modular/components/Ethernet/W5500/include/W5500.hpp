#pragma once

#include "Ethernet.hpp"

/* Ambient Tracker Pro : W5500 Pins & Configurations */
#define W5500_ETH_SPI_CS           (gpio_num_t)5
#define W5500_ETH_SPI_MOSI         (gpio_num_t)2
#define W5500_ETH_SPI_MISO         (gpio_num_t)1
#define W5500_ETH_SPI_SCLK         (gpio_num_t)4
#define W5500_ETH_SPI_INT          (gpio_num_t)12
#define W5500_ETH_SPI_PHY_RST      (gpio_num_t)11
#define W5500_ETH_SPI_PHY_ADDR     1
#define W5500_ETH_SPI_HOST         (spi_host_device_t)2
#define W5500_ETH_SPI_CLOCK_MHZ    12

static Ethernet::SPIConfig w5500SPI = 
{
    .cs = W5500_ETH_SPI_CS,
    .miso = W5500_ETH_SPI_MISO,
    .mosi = W5500_ETH_SPI_MOSI,
    .sclk = W5500_ETH_SPI_SCLK,
    .intr = W5500_ETH_SPI_INT,
    .phyReset = W5500_ETH_SPI_PHY_RST,
    .phyAddr = W5500_ETH_SPI_PHY_ADDR,
    .clkSpeed = W5500_ETH_SPI_CLOCK_MHZ,
    .spiHost = W5500_ETH_SPI_HOST
};

class W5500 : public Ethernet
{
    private:
    SPIConfig spiConfig;
    eth_mac_config_t mac_config_spi;
    eth_phy_config_t phy_config_spi;
    esp_eth_mac_t *mac;
    esp_eth_phy_t *phy;
    esp_eth_handle_t handle; /* Handle of Ethernet Driver */

    public:
    W5500(SPIConfig spi = w5500SPI) : Ethernet(spi) {}
    void busInit() override;
    void addDevice() override;
    void spiBegin() override;
    void setup() override;
    
};