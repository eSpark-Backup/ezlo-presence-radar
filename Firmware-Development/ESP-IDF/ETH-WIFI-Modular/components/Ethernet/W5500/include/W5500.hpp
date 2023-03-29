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
    esp_netif_inherent_config_t esp_netif_config;
    esp_netif_config_t cfg_spi;
    esp_netif_t *eth_netif_spi = NULL;
    eth_mac_config_t mac_config_spi;
    eth_phy_config_t phy_config_spi;
    esp_eth_mac_t *mac_spi;
    esp_eth_phy_t *phy_spi;
    esp_eth_handle_t eth_handle_spi = NULL;
    spi_device_handle_t spi_handle = NULL; /* Handle of Ethernet Driver */
    eth_w5500_config_t w5500_config;
    esp_eth_config_t eth_config_spi;

    public:
    W5500(SPIConfig spi = w5500SPI) : Ethernet(spi) {}
    void setupRSTPin();
    void setupPWRPin();
    void resetSeq();
    void beginNetif();
    void createNetifInstance();
    void initMAC();
    void initPHY();
    void installGpioISR();
    void busInit() override;
    void addDevice() override;
    void spiBegin() override;
    void setConfig();
    void setGPIO();
    void setSpiMAC();
    void setSpiPHY();
    void setupDriver();
    void registerCallback();
    void start();
    void setup() override;
    
};

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data);

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data);