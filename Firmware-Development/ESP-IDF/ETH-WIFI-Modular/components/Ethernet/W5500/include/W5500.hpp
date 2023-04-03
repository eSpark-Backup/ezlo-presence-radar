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

#define PWR_EN  GPIO_NUM_13
#define RST_PIN GPIO_NUM_11

class W5500 : public Ethernet
{
    private:
    NETIF netif;
    SPI spi;

    public:
    uint8_t spi_command_bits = 16;  /* Actually it's the address phase in W5500 SPI frame */
    uint8_t spi_address_bits = 8;   /* Actually it's the control phase in W5500 SPI frame */
    eth_w5500_config_t w5500_config;
    esp_eth_mac_t *mac_spi = NULL;
    esp_eth_phy_t *phy_spi = NULL;
    esp_eth_handle_t eth_handle_spi;
    esp_eth_config_t eth_config_spi;

    W5500(){}
    ~W5500(){}
    void setConfig(void* spiHandle, gpio_num_t intr);
    void setConfig(gpio_num_t intr);
    void createObjMAC();
    void createObjPHY();
    void setMacAddress();
    void setETHDriver();
    void setIOLayer(); 
    void spiBegin();
    void reset();
    void setup();
    void start();
};