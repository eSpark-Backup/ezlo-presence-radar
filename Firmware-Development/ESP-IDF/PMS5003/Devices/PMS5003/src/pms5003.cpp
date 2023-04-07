#include "pms5003.hpp"

static const char* TAG = "PMS";

/*
     @brief  Setups the hardware and detects a valid UART PM2.5
*/ 
void pms_init()
{
    pms_uart_setup();
    pms_gpio_config();
}

/*
    Install UART Driver
*/ 
void pms_uart_setup()
{
    uart_config_t uartConfig = 
    {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(PMS_UART_PORT, &uartConfig);
    uart_set_pin(PMS_UART_PORT, PMS_TX_PIN, PMS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(PMS_UART_PORT, PMS_UART_BUF_SIZE * 2, 0, 0, NULL, 0);
}

/*
    Initializing GPIO pins
*/
void pms_gpio_config()
{
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PMS_SET_PIN) | (1ULL << PMS_RESET_PIN);
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    gpio_set_level(PMS_SET_PIN, 1);
    gpio_set_level(PMS_RESET_PIN, 1);
}
/*!
 *  @brief  Setups the hardware and detects a valid UART PM2.5
 *  @param  data
 *          Pointer to PM25_AQI_Data that will be filled by read()ing
 *  @return ESP_OK on successful read, ESP_FAIL if timed out or bad data
 */
esp_err_t pms_read(PM25_AQI_Data *data) 
{
  
  uint8_t buffer[128];
  uint16_t sum = 0;
  memset(data, 0, sizeof(PM25_AQI_Data));

  if (!data) {
    return ESP_FAIL;
  }

  int length = 0;
  length = uart_read_bytes(PMS_UART_PORT, buffer, 128, 1); 

  // Length test
  if (length < 32)
      return ESP_FAIL;

  // Check that start byte is correct!
  if (buffer[0] != 0x42) {
    return false;
  }

  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)data, (void *)buffer_u16, 30);

  if (sum != data->checksum) {
    return ESP_FAIL;
  }

  // success!
  return ESP_OK;
}

/*!
 *  @brief  Prints the PMS Sensor Data
 *  @param  data
 *          Pointer to PM25_AQI_Data that was filled by pms_read()
 */

void pms_print_data(PM25_AQI_Data* data)
{
  ESP_LOGI(TAG ,"AQI reading success");

  ESP_LOGI(TAG,"---------------------------------------");
  ESP_LOGI(TAG,"Concentration Units (standard)");
  ESP_LOGI(TAG,"---------------------------------------");
  ESP_LOGI(TAG,"PM 1.0: %d", data->pm10_standard); 
  ESP_LOGI(TAG,"PM 2.5: %d", data->pm25_standard); 
  ESP_LOGI(TAG,"PM 10:  %d", data->pm100_standard); 

  ESP_LOGI(TAG,"---------------------------------------");
  ESP_LOGI(TAG,"Concentration Units (environmental)");
  ESP_LOGI(TAG,"---------------------------------------");
  ESP_LOGI(TAG,"PM 1.0: %d", data->pm10_env); 
  ESP_LOGI(TAG,"PM 2.5: %d", data->pm25_env); 
  ESP_LOGI(TAG,"PM 10:  %d", data->pm100_env); 

  ESP_LOGI(TAG,"---------------------------------------");
  ESP_LOGI(TAG,"Particles > 0.3um / 0.1L air: %d", data->particles_03um); 
  ESP_LOGI(TAG,"Particles > 0.5um / 0.1L air: %d", data->particles_05um); 
  ESP_LOGI(TAG,"Particles > 1.0um / 0.1L air: %d", data->particles_10um); 
  ESP_LOGI(TAG,"Particles > 2.5um / 0.1L air: %d", data->particles_25um); 
  ESP_LOGI(TAG,"Particles > 5.0um / 0.1L air: %d", data->particles_50um); 
  ESP_LOGI(TAG,"Particles > 10 um / 0.1L air: %d", data->particles_100um); 
  ESP_LOGI(TAG,"---------------------------------------");
}