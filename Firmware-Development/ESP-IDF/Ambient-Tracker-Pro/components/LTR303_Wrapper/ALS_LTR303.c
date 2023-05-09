#include "ALS_LTR303.h"

s_ezlopi_i2c_master_t ltr303_i2c_master_conf =  {
                                                    .enable = true,
                                                    .address = LTR303_ADDR,
                                                    .channel = ACTIVE_I2C,
                                                    .sda = LTR303_SDA,
                                                    .scl = LTR303_SCL,
                                                    .clock_speed = I2C_MASTER_FREQ_HZ,
                                                };

unsigned char gain = LTR3XX_GAIN_1;     // Gain setting, values = 0-7 
unsigned char integration_time = LTR3XX_INTEGTIME_200;  // Integration ("shutter") time in milliseconds
unsigned char measurement_rate = LTR3XX_MEASRATE_200;  // Interval between DATA_REGISTERS update
bool valid=0, intr_status=0, data_status=0;
byte error;

bool ltr303_is_data_available(void)
{
    ltr303_get_status(&valid, &gain, &intr_status, &data_status); 
    return data_status;
}

esp_err_t ltr303_setup(bool initialize_i2c)
{  
    esp_err_t ret;
    if (!initialize_i2c)
    {
        ltr303_i2c_master_conf.enable = false;
    }
    ret = ltr303_begin(&ltr303_i2c_master_conf);
    unsigned char ID;
  
    if (ltr303_get_part_id(&ID)) {
        printf("Got Sensor Part ID: %02X\n", ID);
    }
    if (ltr303_get_manufac_id(&ID)) {
        printf("Got Manuf Part ID: %02X\n", ID);
    }

    printf("Setting Gain...\n");
    ret &= ltr303_set_control(gain, false, false);
    
    printf("Set timing...\n");
    ret &= ltr303_set_measurement_rate(integration_time, measurement_rate);
    
    printf("Powerup...\n");
    ret &= ltr303_set_powerup();

    if (ret)
    {
        return ESP_OK;
    }
    
    return ESP_FAIL;
}

esp_err_t ltr303_loop(void)
{
    
    return ESP_OK;
}

esp_err_t ltr303_get_val(ltr303_data_t* ltr303_data)
{
    if (ltr303_is_data_available())
    {        
        unsigned int data0, data1;
        if (ltr303_get_data(&data0,&data1)) {
            
            // getData() returned true, communication was successful
            #if DEBUG
                printf("data0: %d\n",data0);
                printf("data1: %d\n",data1);
            #endif
            // To calculate lux, pass all your settings and readings 
            // to the getLux() function.
            
            // The getLux() function will return 1 if the calculation
            // was successful, or 0 if one or both of the sensors was
            // saturated (too much light). If this happens, you can
            // reduce the integration time and/or gain.

            // Perform lux calculation:
            if (ltr303_get_lux(gain,integration_time,data0,data1,&(ltr303_data->lux)))
            {
                return ESP_OK;
            }
            
        }
    }
    return ESP_FAIL;
}

void ltr303(void* pvParameter)
{
    ltr303_setup(true); // true : initialize i2c, false : skips i2c initialization
    
    ltr303_data_t light_sensor_data;
    while (1)
    {
        if(ltr303_get_val(&light_sensor_data) == ESP_OK)
        { 
            printf(" lux: %f\n",light_sensor_data.lux);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

TaskHandle_t ltr303_task_handle = NULL;
esp_err_t ltr303_init(void)
{
    BaseType_t ret;
    ret = xTaskCreatePinnedToCore(ltr303,"ltr303", 2048, NULL, 10, &ltr303_task_handle, 1);
    if (pdPASS == ret) 
    {
        return ESP_OK;
    }
    return ESP_FAIL;
}

void ltr303_deinit(void)
{
    if( ltr303_task_handle != NULL )
    {
        vTaskDelete(ltr303_task_handle);
        ltr303_task_handle = NULL;
    }
}
