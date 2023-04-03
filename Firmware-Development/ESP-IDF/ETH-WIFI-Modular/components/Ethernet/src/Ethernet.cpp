#include "Ethernet.hpp"

static const char* TAG = "Ethernet";

void Ethernet::setup()
{
    // netif.setup();
}

/* Initialize MAC configs to default */
void Ethernet::configMAC()
{
    mac_config_spi = (eth_mac_config_t)ETH_MAC_DEFAULT_CONFIG();
}

/* Initialize PHY configs to default */
void Ethernet::configPHY()
{
    phy_config_spi = (eth_phy_config_t)ETH_PHY_DEFAULT_CONFIG();
}

void Ethernet::setPHYConfig(uint8_t phy_addr, gpio_num_t resetPin)
{
    this->phy_config_spi.phy_addr = phy_addr;
    this->phy_config_spi.reset_gpio_num = resetPin;
}

void Ethernet::setEthConfigSPI()
{
    eth_config_spi = ETH_DEFAULT_CONFIG(this->mac_spi, this->phy_spi);
}

void Ethernet::installDriver()
{
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &this->eth_handle_spi));
}

void Ethernet::setMacAddress()
{
    /* 
        The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
        02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    uint8_t macAddr[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + 0x00};
    ESP_ERROR_CHECK(esp_eth_ioctl(this->eth_handle_spi, ETH_CMD_S_MAC_ADDR, macAddr));
}

void Ethernet::attachToNetif(esp_netif_t* eth_netif_spi)
{
    // attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(this->eth_handle_spi)));
}

void Ethernet::registerCallBacks()
{
    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
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