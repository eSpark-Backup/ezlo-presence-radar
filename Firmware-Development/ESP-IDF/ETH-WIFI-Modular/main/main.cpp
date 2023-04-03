#include "W5500.hpp"

NETIF netif;
SPI spi;
W5500 w5500;
static const char* TAG = "MAIN-APP";

void resetSeq();

#define PWR_EN  GPIO_NUM_13
#define RST_PIN GPIO_NUM_11

extern "C" void app_main(void)
{
    resetSeq();
    // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
    gpio_install_isr_service(0);

    netif.setup();

    spi.setPins(W5500_ETH_SPI_CS, W5500_ETH_SPI_MISO, W5500_ETH_SPI_MOSI, W5500_ETH_SPI_SCLK, W5500_ETH_SPI_INT, W5500_ETH_SPI_PHY_RST);
    spi.setHost(W5500_ETH_SPI_HOST);
    spi.setSpeed(W5500_ETH_SPI_CLOCK_MHZ);
    // Initiallize SPI Bus and Add Device.
    spi.initBus();
    spi.setDeviceConfig(w5500.spi_command_bits, w5500.spi_address_bits);
    spi.busAddDevice();

    w5500.configMAC();
    w5500.configPHY();
    w5500.setPHYConfig(W5500_ETH_SPI_PHY_ADDR, W5500_ETH_SPI_PHY_RST);
    w5500.setConfig(spi.spi_handle, W5500_ETH_SPI_INT);

    w5500.mac_spi = esp_eth_mac_new_w5500(&w5500.w5500_config, &w5500.mac_config_spi);
    w5500.phy_spi = esp_eth_phy_new_w5500(&w5500.phy_config_spi);

    w5500.eth_config_spi = ETH_DEFAULT_CONFIG(w5500.mac_spi, w5500.phy_spi);
    ESP_ERROR_CHECK(esp_eth_driver_install(&w5500.eth_config_spi, &w5500.eth_handle_spi));

    /* 
        The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
        02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    uint8_t macAddr[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + 0x00};
    ESP_ERROR_CHECK(esp_eth_ioctl(w5500.eth_handle_spi, ETH_CMD_S_MAC_ADDR, macAddr));

    netif.setETHGlue(w5500.eth_handle_spi);

    w5500.registerCallBacks();
    w5500.start();

}


void resetSeq()
{
     // RST PIN GPIO:
    gpio_config_t rstPinConfig = 
    {
        .pin_bit_mask = (1UL<<RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if(gpio_config(&rstPinConfig) == ESP_OK) 
    { ESP_LOGI(TAG, "RST_PIN OutputMode Done!"); }
    else
    { ESP_LOGE(TAG, "RST_PIN OutputMode Failed"); }

    // Setting PWRN_EN High to active W5500 IC
    gpio_config_t pwrEnConfig = 
    {
        .pin_bit_mask = (1UL<<PWR_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if(gpio_config(&pwrEnConfig) == ESP_OK) 
    { ESP_LOGI(TAG, "PWR_EN OutputMode Done!"); }
    else
    { ESP_LOGE(TAG, "PWR_EN OutputMode Failed"); }
    
    // 1.Enable PWR to W5500 via mosfet
    gpio_set_level(PWR_EN, 1);
    // 2. Wait 500mS
    vTaskDelay(500 / portTICK_RATE_MS);
    // 3. pull reset low, wait 5mS
    gpio_set_level(RST_PIN, 0);
    vTaskDelay(5 / portTICK_RATE_MS);
    // 4. pull reset high, wait 10mS
    gpio_set_level(RST_PIN, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
}
