#include "ezlo_netif.h"

esp_err_t netif_eth_spi_setup(esp_netif_t **eth_netif_spi)
{
    netif_begin();
    netif_create_spi_instance(eth_netif_spi);
    return ESP_OK;
}

/*!
 *  @brief  Initialize TCP/IP network interface (should be called only once in application).
            Create default event loop that running in background.

 *  @return ESP_OK on success & esp_err_t if any error occurs.
 */
esp_err_t netif_begin()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    return ESP_OK;
}

/*!
 *  @brief  Create instance(s) of esp-netif for SPI Ethernet(s).
 *  @param  Pointer of netif_spi_eth_t (defined in ezlo_netif.h) for SPI Ethernet(s) 
 *  @return ESP_OK on success, ESP_FAIL if failed.
 */

esp_err_t netif_create_spi_instance(esp_netif_t** eth_netif_spi)
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
    *eth_netif_spi = esp_netif_new(&cfg_spi); 

    return ESP_OK;
}

