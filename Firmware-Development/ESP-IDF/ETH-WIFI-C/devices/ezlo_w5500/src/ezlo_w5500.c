#include "ezlo_w5500.h"

static const char* TAG = "ezlo_w5500";

esp_err_t w5500_setup(w5500_t* w5500)
{
    w5500->power_pin = W5500_EN;
    w5500->reset_pin = W5500_RST;
    esp_err_t ret = w5500_power_pin_config(w5500->power_pin);
    if(ret != ESP_OK)
        return ret;

    ret = w5500_reset_pin_config(w5500->reset_pin);
    if(ret != ESP_OK)
        return ret;

    w5500_startup_reset(w5500);
    return ESP_OK;
}

/*!
 *  @brief  Startup reset sequence for W5500 module.
 *  @param  Pointer of w5500_t.
 */
void w5500_startup_reset(w5500_t* w5500)
{
    /* 1.Enable PWR to W5500 via mosfet */
    gpio_set_level(w5500->power_pin, 1);
    /* 2. Wait 500mS */
    vTaskDelay(500 / portTICK_RATE_MS);
    /* 3. Pull reset low, wait 5mS */
    gpio_set_level(w5500->reset_pin, 0);
    vTaskDelay(5 / portTICK_RATE_MS);
    /* 4. Pull reset high, wait 10mS */
    gpio_set_level(w5500->reset_pin, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
}

esp_err_t w5500_power_pin_config(gpio_num_t power_pin)
{
    esp_err_t ret = w5500_gpio_config(power_pin);
    return ret;
}

esp_err_t w5500_reset_pin_config(gpio_num_t reset_pin)
{
    esp_err_t ret = w5500_gpio_config(reset_pin);
    return ret;
}

esp_err_t w5500_gpio_config(gpio_num_t gpio_num)
{
    esp_err_t status = ESP_OK;
    gpio_config_t gpio_conf = 
    {
        .pin_bit_mask = ( 1UL << gpio_num),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    status = gpio_config(&gpio_conf);
    return status;
}

/**
 * @brief       Set Default Configurations for Ethernet MAC object
 * @param[in]   eth_mac_config_t Pointer of Ethernet MAC object
 */
void set_default_MAC(eth_mac_config_t * eth_mac_obj)
{
    *eth_mac_obj = (eth_mac_config_t)ETH_MAC_DEFAULT_CONFIG();
}

/**
 * @brief       Set Default Configurations for Ethernet PHY object
 * @param[in]   eth_phy_config_t* Pointer of Ethernet PHY object
 */
void set_default_PHY(eth_phy_config_t* eth_phy_obj)
{
    *eth_phy_obj = (eth_phy_config_t)ETH_PHY_DEFAULT_CONFIG();
}

/**
 * @brief       Configure reset-pin of Ethernet PHY object
 * @param[in]   eth_phy_config_t* Pointer of Ethernet PHY object
 * @param[in]   gpio_num_t        Reset Pin of W5500 Module
 */

void set_reset_pin_PHY(eth_phy_config_t* eth_phy_obj, gpio_num_t reset_pin)
{
    eth_phy_obj->phy_addr = -1;
    eth_phy_obj->reset_gpio_num = reset_pin;
}

/**
 * @brief       Configure the Ethernet Object.
 * @param[in]   w5500_t*    Pointer of W5500 Instance
 * @param[in]   spi_device_handle_t  Pointer of SPI Instance
 * @param[in]   gpio_num_t  interrupt pin of W5500 Module
 */
void w5500_set_config(eth_w5500_config_t* w5500_obj, spi_device_handle_t spi_handle, gpio_num_t intr_pin)
{
    *w5500_obj = (eth_w5500_config_t){
        .spi_hdl = spi_handle,        
        .int_gpio_num = intr_pin,
    };
}

/**
 * @brief       Set Reset Pin Member of Ethernet PHY object
 * @param[in]   esp_eth_config_t* Pointer of Ethernet object
 * @param[in]   esp_eth_handle_t* Pointer of handle of Ethernet driver 
 * @return      ESP_OK on success & esp_err_t if any error occurs.
 */
esp_err_t w5500_eth_driver_install(esp_eth_config_t* eth_obj, esp_eth_handle_t* eth_handle_spi)
{
    esp_err_t status = ESP_OK;
    status |= esp_eth_driver_install(eth_obj, eth_handle_spi);
    return status;
}

/**
 *  @brief  The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
            02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    @return ESP_OK on success & esp_err_t if any error occurs.
 */
esp_err_t w5500_set_mac_address(esp_eth_handle_t eth_handle_spi)
{
    esp_err_t status = ESP_OK; 
    uint8_t macAddr[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + 0x00};
    status |= esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, macAddr);
    return status;
};

/**
 *  @brief  Create a netif glue for Ethernet driver.
            - netif glue is used to attach io driver to TCP/IP netif.
            - Glue IO layer: Adapts the input/output functions to use ESP-NETIF transmit, receive and free receive buffer.
 *  @param esp_netif_t* - Pointer to Netif Object
    @param eth_handler  - Ethernet driver handle 
*/
esp_err_t w5500_netif_glue(esp_netif_t* esp_netif , esp_eth_handle_t eth_handler)
{
    esp_err_t status = ESP_OK; 
    // attach Ethernet driver to TCP/IP stack
    status = esp_netif_attach(esp_netif, esp_eth_new_netif_glue(eth_handler));
    return status;
}

/**
 *  @brief  Register user defined event handers.
*/
void w5500_register_cbs()
{
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
}

/**
 *  @brief  Start Ethernet driver state machine. 
*/
void w5500_start(esp_eth_handle_t eth_handle_spi)
{
    ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi));
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

