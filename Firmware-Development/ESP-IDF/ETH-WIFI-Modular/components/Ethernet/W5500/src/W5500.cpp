#include "W5500.hpp"

const char* TAG = "W5500";

void W5500::setConfig(void* spiHandle, gpio_num_t intr)
{
    this->w5500_config = (eth_w5500_config_t){
        .spi_hdl = spiHandle,        
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
