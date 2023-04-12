#include "ezlo_netif.h"

esp_err_t netif_eth_spi_setup(netif_spi_eth_t* netif_obj)
{
    esp_err_t ret = netif_begin();
    if(ret != ESP_OK)
        return ret;

    ret = netif_create_spi_instance(netif_obj);
    if( ret != ESP_OK )
        return ret;
    
    return ESP_OK;
}

/*!
 *  @brief  Initialize TCP/IP network interface (should be called only once in application).
            Create default event loop that running in background.

 *  @return ESP_OK on success & esp_err_t if any error occurs.
 */
esp_err_t netif_begin()
{
    esp_err_t ret = esp_netif_init();
    if(ret != ESP_OK)
        return ret;

    ret = esp_event_loop_create_default();
    if( ret != ESP_OK )
        return ret;

    return ESP_OK;
}

/*!
 *  @brief  Create instance(s) of esp-netif for SPI Ethernet(s).
 *  @param  Pointer of netif_spi_eth_t (defined in ezlo_netif.h) for SPI Ethernet(s) 
 *  @return ESP_OK on success, ESP_FAIL if failed.
 */

esp_err_t netif_create_spi_instance(netif_spi_eth_t* netif_obj)
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
    
    netif_obj->eth_netif_spi = esp_netif_new(&cfg_spi); 
    if(netif_obj->eth_netif_spi == NULL)
        return ESP_FAIL;
    else 
        return ESP_OK;
}

/*!
* @brief Create a netif glue for Ethernet driver.
    - netif glue is used to attach io driver to TCP/IP netif.
    - Glue IO layer: Adapts the input/output functions to use ESP-NETIF transmit, receive and free receive buffer.

    @params :
        netif_spi_eth_t* netif_obj
*/
void netif_set_eth_glue(netif_spi_eth_t* netif_obj)
{
    // attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK( esp_netif_attach( &netif_obj->eth_netif_spi, esp_eth_new_netif_glue(netif_obj->eth_netif_spi) ) );
}

