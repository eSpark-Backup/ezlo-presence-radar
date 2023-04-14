#include "pms5003.hpp"

static const char* TAG = "PMS";
uint8_t volatile pmsStatusReg = 0x00; 
/*
  * pmsStatusReg =  | X | X | X | X | X | EN_READ | SLEEP_MODE | ACTIVE_MODE |
  *                 | 7 | 6 | 5 | 4 | 3 |   2     |      1     |      0     |
*/                


/*
     @brief  Setups the hardware and detects a valid UART PM2.5
*/ 
void pms_init(PM25_AQI_Data* data)
{
    pms_uart_setup(data);
    pms_gpio_config();
    pms_startup();
    pms_create_sleep_timer(data);
}

/*
    @brief Install UART Driver for PMS5003.
*/ 
void pms_uart_setup(PM25_AQI_Data* data)
{
    // uart_config_t uartConfig = 
    // {
    //         .baud_rate = PMS_UART_BUAD_RATE,
    //         .data_bits = UART_DATA_8_BITS,
    //         .parity = UART_PARITY_DISABLE,
    //         .stop_bits = UART_STOP_BITS_1,
    //         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    // };

    // uart_param_config(PMS_UART_PORT, &uartConfig);
    // uart_set_pin(PMS_UART_PORT, PMS_TX_PIN, PMS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // uart_driver_install(PMS_UART_PORT, PMS_UART_BUF_SIZE * 2, 0, 0, NULL, 0);

    //EZLO:
    ezlopi_uart_init(PMS_UART_BUAD_RATE, PMS_TX_PIN, PMS_RX_PIN, ezlopi_pms5003_upcall, data);
}

/*
    @brief Configures GPIO pins (SET & RESET) for PMS5003.
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
}

/*
    @brief Starts the PMS5003 in sleep-mode.
*/
void pms_startup()
{
    pms_sleep_mode(PMS_SET_PIN);
    gpio_set_level(PMS_RESET_PIN, 1);
}
/*!
 *  @brief  Setup timer for PMS5003 Sleep Mode.
            [Purpose: To extend the life span of the PMS5003 sensor.] 
            [Approach: Activate the sensor for 30s after every 2-Mintues, do the reading and then let’s it go back to sleep.]
            [source : https://forum.airgradient.com/t/extending-the-life-span-of-the-pms5003-sensor/114/9]

 *  @return ESP_OK on success, ESP_FAIL if failed to create the timer.
 */

void pms_create_sleep_timer(PM25_AQI_Data* data)
{
    const esp_timer_create_args_t pms_sleep_timer_args = {
            .callback = &pms_timer_callback,
            .name = "pms_sleep_mode_timer",
            .arg = data
    };

    esp_timer_handle_t pms_sleep_timer;
    ESP_ERROR_CHECK(esp_timer_create(&pms_sleep_timer_args, &pms_sleep_timer));
    /* The timer has been created but is not running yet */

    /* Start the timer of 1sec*/
    ESP_ERROR_CHECK(esp_timer_start_periodic(pms_sleep_timer, 1000000));
}

static void pms_timer_callback(void* arg)
{
    static int sec;
    sec++;
    PM25_AQI_Data* data = (PM25_AQI_Data *)arg;
    
    if(sec <= PMS_TOTAL_ACTIVE_TIME )
    {
      if( !(pmsStatusReg & 0x01) )
      {
        pms_active_mode(PMS_SET_PIN);
        pmsStatusReg |= 0x01; //Active Mode
      }

      if(sec == PMS_STABILITY_TIME)
      {
        pmsStatusReg |= 0x04; //Enable Read Status   
        data->available = true;   
        ESP_LOGI("data->available","%d", data->available); 
      }
    }

    else if( sec <= PMS_TOTAL_TIME )
    {        
      if( !(pmsStatusReg & 0x02) )
      { 
          pmsStatusReg |= 0x02;
          pms_sleep_mode(PMS_SET_PIN);
      }
    }
    
    else if(sec > PMS_TOTAL_TIME)
    {
      sec = 0;
      pmsStatusReg = 0;
    }
}

/*!
 *  @brief  Setups the hardware and detects a valid UART PM2.5
 *  @param  data
 *          Pointer to PM25_AQI_Data that will be filled by read()ing
 *  @return ESP_OK on successful read, ESP_FAIL if timed out or bad data
 */
  esp_err_t pms_read(PM25_AQI_Data *data) 
{
  
  /*  pmsStatusReg : Update from the "pms_timer_callback". 
      If Active Mode and EN_Read is HIGH, this function will read the PMS_Sensor Data
  */
    uint8_t buffer[128];
    uint16_t sum = 0;
    memset(data, 0, sizeof(PM25_AQI_Data));

    if (!data) {
      return ESP_FAIL;
    }

    int length = 0;
    length = uart_read_bytes(PMS_UART_PORT, buffer, 128, 1); 

    /* Length test */
    if (length < 32)
        return ESP_FAIL;

    /* Check that start byte is correct! */
    if (buffer[0] != 0x42) {
      return false;
    }

    /* Get checksum ready */
    for (uint8_t i = 0; i < 30; i++) {
      sum += buffer[i];
    }

    /* The data comes in endian'd, this solves it so it works on all platforms */
    uint16_t buffer_u16[15];
    for (uint8_t i = 0; i < 15; i++) {
      buffer_u16[i] = buffer[2 + i * 2 + 1];
      buffer_u16[i] += (buffer[2 + i * 2] << 8);
    }

    /* put it into a nice struct :) */
    memcpy((void *)data, (void *)buffer_u16, 30);

    if (sum != data->checksum) {
      return ESP_FAIL;
    }
    /* success! */
    return ESP_OK;
}

/*!
 *  @brief  Set PMS5003 in Sleep-mode and stop the fan.
 */
void pms_sleep_mode(gpio_num_t set_pin)
{
  gpio_set_level(set_pin, 0);
}
/*!
 *  @brief  Set PMS5003 in Active-mode and start the fan.
 */
void pms_active_mode(gpio_num_t set_pin)
{
  gpio_set_level(set_pin, 1);
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

static void ezlopi_pms5003_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t uart_object_handle, void* user_args)
{
  uint16_t sum = 0;
  PM25_AQI_Data* data = (PM25_AQI_Data *)user_args;
  memset(data, 0, sizeof(PM25_AQI_Data));

  /* Check that start byte is correct! */
    if (buffer[0] != 0x42) {
      return;
    }

    /* Get checksum ready */
    for (uint8_t i = 0; i < 30; i++) {
      sum += buffer[i];
    }

    /* The data comes in endian'd, this solves it so it works on all platforms */
    uint16_t buffer_u16[15];
    for (uint8_t i = 0; i < 15; i++) {
      buffer_u16[i] = buffer[2 + i * 2 + 1];
      buffer_u16[i] += (buffer[2 + i * 2] << 8);
    }

    /* put it into a nice struct :) */
    memcpy((void *)data, (void *)buffer_u16, 30);

    if (sum != data->checksum) {
      return;
    }
    // pms_print_data(data);
    // ESP_LOGI(TAG,"Particles > 5.0um / 0.1L air: %d", data->particles_50um); 
    // ESP_LOGI(TAG,"Particles > 10 um / 0.1L air: %d", data->particles_100um); 
    // ESP_LOGI("Upcall data->available","%d", data->available); 
    /* success! */
    return;
}