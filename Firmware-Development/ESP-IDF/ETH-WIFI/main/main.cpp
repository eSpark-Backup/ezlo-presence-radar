#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"
#include "wifinetwork.hpp"

WIFINETWORK WIFI(WIFI_SSID, WIFI_PW);
uint8_t ETH_EVENT_FLAG = 2;

// ************** WEBSERVER ************** 
#include <esp_http_server.h>
esp_err_t test_handler(httpd_req_t *req);
// ************** WEBSERVER **************

static const char *TAG = "eth_example";

#define INIT_SPI_ETH_MODULE_CONFIG(eth_module_config, num)                                      \
    do {                                                                                        \
        eth_module_config[num].spi_cs_gpio = CONFIG_EXAMPLE_ETH_SPI_CS ##num## _GPIO;           \
        eth_module_config[num].int_gpio = CONFIG_EXAMPLE_ETH_SPI_INT ##num## _GPIO;             \
        eth_module_config[num].phy_reset_gpio = CONFIG_EXAMPLE_ETH_SPI_PHY_RST ##num## _GPIO;   \
        eth_module_config[num].phy_addr = CONFIG_EXAMPLE_ETH_SPI_PHY_ADDR ##num;                \
    } while(0)

typedef struct {
    uint8_t spi_cs_gpio;
    uint8_t int_gpio;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
}spi_eth_module_config_t;

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ETH_EVENT_FLAG = 1;
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ETH_EVENT_FLAG = 0;
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

#define PWR_EN  GPIO_NUM_13
#define RST_PIN GPIO_NUM_11

extern "C" void app_main(void)
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

    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // *********** STATIC IP ***********
    char* ip= "192.168.1.249";
    char* gateway = "192.168.1.1";
    char* netmask = "255.255.255.0";
    esp_netif_ip_info_t info_t;
    memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
    ip4addr_aton((const char *)ip, (ip4_addr_t*)&info_t.ip);
    ip4addr_aton((const char *)gateway, (ip4_addr_t*)&info_t.gw);
    ip4addr_aton((const char *)netmask, (ip4_addr_t*)&info_t.netmask);
    // *********** STATIC IP ***********

    // Create instance(s) of esp-netif for SPI Ethernet(s)
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    // *********** STATIC IP ***********
    esp_netif_config.ip_info = &info_t;
    esp_netif_config.if_key = "ETH_SPI_2";
    esp_netif_config.if_desc = "eth0";
    esp_netif_config.route_prio = 30;
    // *********** STATIC IP ***********
    esp_netif_config_t cfg_spi = {
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };
    esp_netif_t *eth_netif_spi = esp_netif_new(&cfg_spi);

    // *********** STATIC IP ***********
    esp_netif_dhcpc_stop(eth_netif_spi);
    esp_netif_set_ip_info(eth_netif_spi, &info_t);
    // *********** STATIC IP ***********

    // Init MAC and PHY configs to default
    eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

    // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
    gpio_install_isr_service(0);

    // Init SPI bus
    spi_device_handle_t spi_handle[CONFIG_EXAMPLE_SPI_ETHERNETS_NUM] = { NULL };
    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_EXAMPLE_ETH_SPI_MOSI_GPIO,
        .miso_io_num = CONFIG_EXAMPLE_ETH_SPI_MISO_GPIO,
        .sclk_io_num = CONFIG_EXAMPLE_ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)CONFIG_EXAMPLE_ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
    spi_eth_module_config_t spi_eth_module_config[CONFIG_EXAMPLE_SPI_ETHERNETS_NUM];
    INIT_SPI_ETH_MODULE_CONFIG(spi_eth_module_config, 0);

    // Configure SPI interface and Ethernet driver for specific SPI module
    esp_eth_mac_t *mac_spi[CONFIG_EXAMPLE_SPI_ETHERNETS_NUM];
    esp_eth_phy_t *phy_spi[CONFIG_EXAMPLE_SPI_ETHERNETS_NUM];
    esp_eth_handle_t eth_handle_spi[CONFIG_EXAMPLE_SPI_ETHERNETS_NUM] = { NULL };

    spi_device_interface_config_t devcfg = {
        .command_bits = 16, // Actually it's the address phase in W5500 SPI frame
        .address_bits = 8,  // Actually it's the control phase in W5500 SPI frame
        .mode = 0,
        .clock_speed_hz = CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .queue_size = 20
    };

    for (int i = 0; i < CONFIG_EXAMPLE_SPI_ETHERNETS_NUM; i++) 
    {
        // Set SPI module Chip Select GPIO
        devcfg.spics_io_num = spi_eth_module_config[i].spi_cs_gpio;

        ESP_ERROR_CHECK(spi_bus_add_device((spi_host_device_t)CONFIG_EXAMPLE_ETH_SPI_HOST, &devcfg, &spi_handle[i]));
        // w5500 ethernet driver is based on spi driver
        eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle[i]);

        // Set remaining GPIO numbers and configuration used by the SPI module
        w5500_config.int_gpio_num = spi_eth_module_config[i].int_gpio;
        phy_config_spi.phy_addr = spi_eth_module_config[i].phy_addr;
        phy_config_spi.reset_gpio_num = spi_eth_module_config[i].phy_reset_gpio;

        mac_spi[i] = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
        phy_spi[i] = esp_eth_phy_new_w5500(&phy_config_spi);
    }

    for (int i = 0; i < CONFIG_EXAMPLE_SPI_ETHERNETS_NUM; i++) {
        esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi[i], phy_spi[i]);
        ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi[i]));

        /* The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
       02:00:00 is a Locally Administered OUI range so should not be used except when testing on a LAN under your control.
        */
       uint8_t macAddr[6] = { 0x02, 0x00, 0x00, 0x12, 0x34, 0x56 + (uint8_t)i};
        ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi[i], ETH_CMD_S_MAC_ADDR, macAddr));

        // attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi[i])));
    }

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    /* start Ethernet driver state machine */
    for (int i = 0; i < CONFIG_EXAMPLE_SPI_ETHERNETS_NUM; i++) {
        ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi[i]));
    }

    while(1)
    {
        if(ETH_EVENT_FLAG == 0)
        {
            ETH_EVENT_FLAG = 2;
            WIFI.connect();
        }
        else if(ETH_EVENT_FLAG == 1)
        {
            ETH_EVENT_FLAG = 2;
            WIFI.disconnect();
        }
        else{}

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

esp_err_t test_handler(httpd_req_t *req)
{
    const char resp[] = "<h1>Hello World</h1>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}