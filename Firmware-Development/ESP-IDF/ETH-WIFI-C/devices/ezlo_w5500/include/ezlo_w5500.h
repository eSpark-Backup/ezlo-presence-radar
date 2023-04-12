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
#include "driver/spi_master.h"
#include "w5500_ambient_pro.h"

typedef struct 
{
    eth_mac_config_t eth_mac_obj; 
    eth_phy_config_t eth_phy_obj; 
    esp_eth_mac_t *mac_spi;         
    esp_eth_phy_t *phy_spi;             
    gpio_num_t reset_pin; 
    gpio_num_t power_pin;            
    eth_w5500_config_t w5500_obj;   
    esp_eth_config_t eth_obj;     
}w5500_t;

esp_err_t w5500_setup(w5500_t* w5500);
esp_err_t w5500_gpio_config(gpio_num_t gpio_num);
esp_err_t w5500_power_pin_config(gpio_num_t power_pin);
esp_err_t w5500_reset_pin_config(gpio_num_t reset_pin);
void w5500_startup_reset(w5500_t* w5500);
void set_default_MAC(eth_mac_config_t * eth_mac_obj);
void set_default_PHY(eth_phy_config_t* eth_phy_obj);
void set_reset_pin_PHY(eth_phy_config_t* eth_phy_obj, gpio_num_t reset_pin);
void w5500_set_config(eth_w5500_config_t* w5500_obj, spi_device_handle_t spi_handle, gpio_num_t intr_pin);
esp_err_t w5500_eth_driver_install(esp_eth_config_t* eth_obj, esp_eth_handle_t* eth_handle_spi);
esp_err_t w5500_set_mac_address(esp_eth_handle_t eth_handle_spi);
esp_err_t w5500_netif_glue(esp_netif_t* esp_netif , esp_eth_handle_t eth_handler);
void w5500_register_cbs();
void w5500_start(esp_eth_handle_t eth_handle_spi);


/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

