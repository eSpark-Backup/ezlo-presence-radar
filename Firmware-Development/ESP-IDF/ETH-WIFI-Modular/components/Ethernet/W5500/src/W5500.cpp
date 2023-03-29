#include "W5500.hpp"

const char* TAG = "W5500";
/* 
    initBus : Initialize SPI bus
*/ 
void W5500::busInit()
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = spiConfig.mosi,
        .miso_io_num = spiConfig.miso,
        .sclk_io_num = spiConfig.sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(W5500_ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

/* 
    addDevice : Allocate a Ethernet device on a SPI bus.
    Initializes the internal structures for a Ethernet device, plus allocates a CS pin on the indicated SPI master peripheral
*/

void W5500::addDevice()
{
    spi_device_interface_config_t devcfg = {
            .command_bits = 16, // Actually it's the address phase in W5500 SPI frame
            .address_bits = 8,  // Actually it's the control phase in W5500 SPI frame
            .mode = 0,
            .clock_speed_hz = W5500_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
            .spics_io_num = W5500_ETH_SPI_CS,
            .queue_size = 20
        };

    ESP_ERROR_CHECK(spi_bus_add_device((spi_host_device_t)W5500_ETH_SPI_HOST, &devcfg, &spi_handle));
}

void W5500::spiBegin()
{
    busInit();
    addDevice();
}

void W5500::setupRSTPin()
{
    // RST PIN GPIO:
    gpio_config_t rstPinConfig = 
    {
        .pin_bit_mask = ( 1UL << W5500_ETH_SPI_PHY_RST ),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if(gpio_config(&rstPinConfig) == ESP_OK) 
    { ESP_LOGI(TAG, "RST_PIN OutputMode Done!"); }
    else
    { ESP_LOGE(TAG, "RST_PIN OutputMode Failed"); }
}

void W5500::setupPWRPin()
{
    // Setting PWRN_EN High to active W5500 IC
    gpio_config_t pwrEnConfig = 
    {
        .pin_bit_mask = (1UL<< W5500_ETH_PWR_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if(gpio_config(&pwrEnConfig) == ESP_OK) 
    { ESP_LOGI(TAG, "PWR_EN OutputMode Done!"); }
    else
    { ESP_LOGE(TAG, "PWR_EN OutputMode Failed"); }
}

void W5500::resetSeq()
{
    setupRSTPin();
    setupPWRPin();

    // 1.Enable PWR to W5500 via mosfet
    gpio_set_level(W5500_ETH_PWR_EN, 1);
    // 2. Wait 500mS
    vTaskDelay(500 / portTICK_RATE_MS);
    // 3. pull reset low, wait 5mS
    gpio_set_level(RST_PIN, 0);
    vTaskDelay(5 / portTICK_RATE_MS);
    // 4. pull reset high, wait 10mS
    gpio_set_level(W5500_ETH_SPI_PHY_RST, 1);
    vTaskDelay(10 / portTICK_RATE_MS);

}

void W5500::beginNetif()
{
    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void W5500::createNetifInstance()
{
    // Create instance(s) of esp-netif for SPI Ethernet(s)
    esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    cfg_spi = (esp_netif_config_t){
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };
    char if_key_str[10];
    char if_desc_str[10];
    char num_str[3];

    itoa(i, num_str, 10);
    strcat(strcpy(if_key_str, "ETH_SPI_"), num_str);
    strcat(strcpy(if_desc_str, "eth"), num_str);
    esp_netif_config.if_key = if_key_str;
    esp_netif_config.if_desc = if_desc_str;
    esp_netif_config.route_prio = 30 - i;
    eth_netif_spi[i] = esp_netif_new(&cfg_spi);
}   

void W5500::initMAC()
{
    // Init MAC configs to default
    mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
}
void W5500::initPHY()
{
    // Init PHY configs to default
    phy_config_spi = ETH_PHY_DEFAULT_CONFIG();
}

void W5500::installGpioISR()
{
    // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
    gpio_install_isr_service(0);
}

void W5500::setConfig()
{
    // w5500 ethernet driver is based on spi driver
    w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle);
}

void W5500::setGPIO()
{
    // Set remaining GPIO numbers and configuration used by the SPI module
        w5500_config.int_gpio_num = spiConfig.intr;
        phy_config_spi.phy_addr = spiConfig.phyAddr;
        phy_config_spi.reset_gpio_num = spiConfig.phyReset;
}

void W5500::setSpiMAC()
{
    mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);  
}

void W5500::setSpiPHY()
{
    phy_spi = esp_eth_phy_new_w5500(&phy_config_spi);
}

void W5500::setupDriver()
{
    eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi[i], phy_spi[i]);
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi[i]));

    /* The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
    02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
    */
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi[i], ETH_CMD_S_MAC_ADDR, (uint8_t[]) {
        0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + i
    }));

    // attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi[i], esp_eth_new_netif_glue(eth_handle_spi[i])));
}

void W5500::registerCallback()
{
     // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
}

void W5500::start()
{
     ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi));
}
void W5500::setup()
{
    resetSeq();
    netifBegin();
    createNetifInstance();
    initMAC();
    initPHY();
    installGpioISR();
    setConfig();
    setGPIO();
    setSpiMAC();
    setSpiPHY();
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
        ETH_EVENT_FLAG = ETHERNET_EVENT_CONNECTED;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ETH_EVENT_FLAG = ETHERNET_EVENT_DISCONNECTED;
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ETH_EVENT_FLAG = ETHERNET_EVENT_START;
        ethStartTime = esp_timer_get_time();
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
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