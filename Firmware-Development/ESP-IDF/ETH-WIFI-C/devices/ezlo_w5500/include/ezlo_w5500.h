#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "ezlo_netif.h"
#include "ezlo_spi.h"
#include "w5500_ambient_pro.h"

typedef struct 
{
    esp_eth_mac_t *mac_spi;         
    esp_eth_phy_t *phy_spi;            
    eth_w5500_config_t w5500_obj;   
    esp_eth_config_t eth_obj;   
    esp_eth_handle_t eth_handle_spi;
}w5500_t;

void ezlo_w5500_setup(w5500_t* w5500, spi_t* spi);
esp_err_t w5500_spi_setup(spi_t* spi);
esp_err_t w5500_netif_setup(esp_netif_t** eth_netif_spi);
void w5500_setup(w5500_t* w5500, spi_t* spi, esp_netif_t* eth_netif_spi);
esp_err_t w5500_gpio_setup(gpio_num_t power_pin, gpio_num_t reset_pin);
esp_err_t w5500_gpio_config(gpio_num_t gpio_num);
void w5500_set_config(eth_w5500_config_t* w5500_obj, spi_device_handle_t spi_handle, gpio_num_t intr_pin);
void w5500_eth_mac_config(eth_mac_config_t * eth_mac_obj);
void w5500_eth_phy_config(eth_phy_config_t* eth_phy_obj);
void w5500_startup_reset(gpio_num_t power_pin, gpio_num_t reset_pin);
esp_err_t w5500_eth_driver_install(esp_eth_config_t* eth_obj, esp_eth_handle_t* eth_handle_spi);
esp_err_t w5500_set_mac_address(esp_eth_handle_t eth_handle_spi);
esp_err_t w5500_netif_glue(esp_netif_t* esp_netif , esp_eth_handle_t eth_handler);
void w5500_register_cbs();
void w5500_start(esp_eth_handle_t eth_handle_spi);

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);