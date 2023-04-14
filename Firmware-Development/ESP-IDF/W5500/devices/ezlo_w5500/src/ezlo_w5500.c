#include "ezlo_w5500.h"
static const char* TAG = "ezlo_w5500";

void ezlo_w5500_setup(w5500_t* w5500, s_ezlopi_spi_master_t* spi)
{
    w5500_spi_setup(spi);
    esp_netif_t* eth_netif_spi = NULL;
    w5500_netif_setup(&eth_netif_spi);
    w5500_setup(w5500, spi, eth_netif_spi);
}

esp_err_t w5500_spi_setup(s_ezlopi_spi_master_t* spi)
{
    // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
    gpio_install_isr_service(0);

    esp_err_t ret = ESP_OK;
    *spi = (s_ezlopi_spi_master_t){
            .enable = true,
            .cs = W5500_CS,
            .miso = W5500_MISO,
            .mosi = W5500_MOSI,
            .sck = W5500_SCLK,
            .mode = 0,
            .clock_speed_mhz = W5500_CLOCK_MHZ,
            .addr_length = W5500_ADDR_BITS, 
            .command_length = W5500_CMD_BITS,
            .transfer_sz = 4096,
            .flags = SPICOMMON_BUSFLAG_MASTER,
            .channel = EZLOPI_SPI_3
    };
    ezlopi_spi_master_init(spi);
    return ret;
}

esp_err_t w5500_netif_setup(esp_netif_t** eth_netif_spi)
{
    return netif_eth_spi_setup(eth_netif_spi);
}

void w5500_setup(w5500_t* w5500, s_ezlopi_spi_master_t* spi, esp_netif_t* eth_netif_spi)
{
    eth_mac_config_t eth_mac_obj; 
    eth_phy_config_t eth_phy_obj; 
    
    w5500_gpio_setup(W5500_EN, W5500_RST);
    w5500_startup_reset(W5500_EN, W5500_RST);
    w5500_set_config(&w5500->w5500_obj, spi->handle, W5500_INTR);
    w5500_eth_mac_config(&eth_mac_obj);
    w5500_eth_phy_config(&eth_phy_obj);
    w5500->mac_spi = esp_eth_mac_new_w5500(&w5500->w5500_obj, &eth_mac_obj);
    w5500->phy_spi = esp_eth_phy_new_w5500(&eth_phy_obj);
    w5500->eth_obj = (esp_eth_config_t)ETH_DEFAULT_CONFIG(w5500->mac_spi, w5500->phy_spi);
    w5500_eth_driver_install(&w5500->eth_obj, &w5500->eth_handle_spi);
    w5500_set_mac_address(w5500->eth_handle_spi);
    w5500_netif_glue(eth_netif_spi, w5500->eth_handle_spi);
    w5500_register_cbs();
    w5500_start(w5500->eth_handle_spi);
}

void w5500_eth_mac_config(eth_mac_config_t * eth_mac_obj)
{
    *eth_mac_obj = (eth_mac_config_t)
        {
            .sw_reset_timeout_ms = 100,                       
            .rx_task_stack_size = 2048,                       
            .rx_task_prio = 15,                               
            .smi_mdc_gpio_num = 23,                           
            .smi_mdio_gpio_num = 18,                          
            .flags = 0,                                       
            .interface = EMAC_DATA_INTERFACE_RMII,            
            .clock_config =                                   
            {                                                 
                .rmii =                                       
                {                                             
                    .clock_mode = EMAC_CLK_DEFAULT,           
                    .clock_gpio = EMAC_CLK_IN_GPIO            
                }                                             
            }                                                 
        };
}

void w5500_eth_phy_config(eth_phy_config_t* eth_phy_obj)
{
    *eth_phy_obj = (eth_phy_config_t) {                                      
                    .phy_addr = W5500_PHY_ADDR, 
                    .reset_timeout_ms = 100,           
                    .autonego_timeout_ms = 4000,       
                    .reset_gpio_num = W5500_RST,               
                };
}

esp_err_t w5500_gpio_setup(gpio_num_t power_pin, gpio_num_t reset_pin)
{
    esp_err_t ret = ESP_OK; 
    ret |= w5500_gpio_config(power_pin);
    ret |= w5500_gpio_config(reset_pin);
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

/*!
 *  @brief  Startup reset sequence for W5500 module.
 *  @param  Pointer of w5500_t.
 */
void w5500_startup_reset(gpio_num_t power_pin, gpio_num_t reset_pin)
{
    /* 1.Enable PWR to W5500 via mosfet */
    gpio_set_level(power_pin, 1);
    /* 2. Wait 500mS */
    vTaskDelay(500 / portTICK_RATE_MS);
    /* 3. Pull reset low, wait 5mS */
    gpio_set_level(reset_pin, 0);
    vTaskDelay(5 / portTICK_RATE_MS);
    /* 4. Pull reset high, wait 10mS */
    gpio_set_level(reset_pin, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
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

