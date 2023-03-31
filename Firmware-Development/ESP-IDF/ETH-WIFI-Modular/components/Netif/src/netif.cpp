#include "netif.hpp"

void NETIF::setup()
{
    begin();
    createSpiInstance();
}

/*
    Initialize TCP/IP network interface (should be called only once in application).
    Create default event loop that running in background.
*/
void NETIF::begin()
{
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default()); 
}

/*
    Create instance(s) of esp-netif for SPI Ethernet(s)
*/
void NETIF::createSpiInstance()
{
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    esp_netif_config_t cfg_spi = {
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };

    const char* if_key_str = "ETH_SPI_0";
    const char* if_desc_str = "eth";

    esp_netif_config.if_key = if_key_str;
    esp_netif_config.if_desc = if_desc_str;
    esp_netif_config.route_prio = 30;
    eth_netif_spi = esp_netif_new(&cfg_spi); 
}

esp_netif_t* NETIF::getInstance()
{
    return eth_netif_spi;
}