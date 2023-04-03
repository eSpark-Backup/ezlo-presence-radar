#include "W5500.hpp"

const char* TAG = "W5500";

void W5500::setup()
{
    reset();
    netif.setup();
    spiBegin();
}

void W5500::setConfig(void* spiHandle, gpio_num_t intr)
{
    this->w5500_config = (eth_w5500_config_t){
        .spi_hdl = spiHandle,        
        .int_gpio_num = intr,
    };
}

void W5500::setConfig(gpio_num_t intr)
{
    this->w5500_config = (eth_w5500_config_t){
        .spi_hdl = this->spi.spi_handle,        
        .int_gpio_num = intr,
    };
}

void W5500::createObjMAC()
{   
    this->mac_spi = esp_eth_mac_new_w5500(&this->w5500_config, &this->mac_config_spi);
}

void W5500::createObjPHY()
{
    this->phy_spi = esp_eth_phy_new_w5500(&this->phy_config_spi);
}

/* 
    The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
    02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
*/
void W5500::setMacAddress()
{
    uint8_t macAddr[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + 0x00};
    ESP_ERROR_CHECK(esp_eth_ioctl(this->eth_handle_spi, ETH_CMD_S_MAC_ADDR, macAddr));
}

void W5500::setETHDriver()
{
    ESP_ERROR_CHECK(esp_eth_driver_install(&this->eth_config_spi, &this->eth_handle_spi));
}

void W5500::setIOLayer()
{
    netif.setETHGlue(this->eth_handle_spi);
}

void W5500::spiBegin()
{
    // Set SPI configurations.
    spi.setPins(W5500_ETH_SPI_CS, W5500_ETH_SPI_MISO, W5500_ETH_SPI_MOSI, W5500_ETH_SPI_SCLK, W5500_ETH_SPI_INT, W5500_ETH_SPI_PHY_RST);
    spi.setHost(W5500_ETH_SPI_HOST);
    spi.setSpeed(W5500_ETH_SPI_CLOCK_MHZ);

    // Initiallize SPI Bus and Add Device.
    spi.initBus();
    spi.setDeviceConfig(spi_command_bits, spi_address_bits);
    spi.busAddDevice();
}

/* 
    Start Ethernet driver state machine. 
*/
void W5500::start()
{
    ESP_ERROR_CHECK(esp_eth_start(this->eth_handle_spi));
}

/*
    Reset Sequence For W5500 Module.
*/

void W5500::reset()
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
