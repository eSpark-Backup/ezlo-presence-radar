#pragma once 

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"

// Ambient Pro - Constants
#define PMS_SET_PIN     GPIO_NUM_39
#define PMS_RESET_PIN   GPIO_NUM_42
#define PMS_RX_PIN      GPIO_NUM_41
#define PMS_TX_PIN      GPIO_NUM_40
#define PMS_UART_PORT   UART_NUM_1
#define PMS_UART_BUF_SIZE 128

/**! Structure holding Plantower's standard packet **/
typedef struct PMSAQIdata {
  uint16_t framelen;       ///< How long this data chunk is
  uint16_t pm10_standard,  ///< Standard PM1.0
      pm25_standard,       ///< Standard PM2.5
      pm100_standard;      ///< Standard PM10.0
  uint16_t pm10_env,       ///< Environmental PM1.0
      pm25_env,            ///< Environmental PM2.5
      pm100_env;           ///< Environmental PM10.0
  uint16_t particles_03um, ///< 0.3um Particle Count
      particles_05um,      ///< 0.5um Particle Count
      particles_10um,      ///< 1.0um Particle Count
      particles_25um,      ///< 2.5um Particle Count
      particles_50um,      ///< 5.0um Particle Count
      particles_100um;     ///< 10.0um Particle Count
  uint16_t unused;         ///< Unused
  uint16_t checksum;       ///< Packet checksum
} PM25_AQI_Data;

void pms_init();
void pms_uart_setup();
void pms_gpio_config();
esp_err_t pms_read(PM25_AQI_Data *data);
void pms_print_data(PM25_AQI_Data* data);
void pms_sleep_mode(gpio_num_t set_pin);
void pms_active_mode(gpio_num_t set_pin);
void pms_create_sleep_timer();
static void pms_timer_callback(void* arg);
