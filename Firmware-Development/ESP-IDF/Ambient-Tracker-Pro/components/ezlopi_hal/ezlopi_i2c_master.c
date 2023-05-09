
#include "stdint.h"
#include "driver/i2c.h"
#include "ezlopi_i2c_master.h"
// #include "trace.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

xSemaphoreHandle xMutex = NULL;

static int ezlopi_i2c_channel_not_available_array[EZLOPI_I2C_NUM_MAX];

static int ezlopi_i2c_master_get_available_channel();
static int ezlopi_i2c_mark_channel_as_unavailable(int ezlopi_i2c_channel_number);
static int ezlopi_i2c_mark_channel_as_available(int ezlopi_i2c_channel_number);
static void ezlopi_i2c_channel_status();

static int ezlopi_i2c_hold_task()
{
    int ret = -1;
    while (1)
    {

        int channel_number = ezlopi_i2c_master_get_available_channel();
        if (EZLOPI_I2C_ERROR_CHANNEL_NOT_AVAILABLE != channel_number)
        {
            ret = channel_number;
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    return ret;
}

int ezlopi_i2c_master_init(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    if (NULL == xMutex)
    {
        xMutex = xSemaphoreCreateMutex();
    }

    int ret = -1;
    
    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {    
        if (NULL != i2c_master_conf)
        {
            int ezlopi_channel_number = ezlopi_i2c_hold_task();
            // i2c_master_conf->channel = ezlopi_channel_number;

            if (EZLOPI_I2C_ERROR_CHANNEL_NOT_AVAILABLE != i2c_master_conf->channel)
            {
                ezlopi_i2c_mark_channel_as_unavailable(ezlopi_channel_number);
                if (true == i2c_master_conf->enable)
                {
                    i2c_config_t i2c_config = {
                        .mode = I2C_MODE_MASTER,
                        .sda_io_num = i2c_master_conf->sda,
                        .scl_io_num = i2c_master_conf->scl,
                        .sda_pullup_en = GPIO_PULLUP_DISABLE,
                        .scl_pullup_en = GPIO_PULLUP_DISABLE,
                        .master.clk_speed = i2c_master_conf->clock_speed,
                    };
                    i2c_param_config(i2c_master_conf->channel, &i2c_config);
                    i2c_driver_install(i2c_master_conf->channel, I2C_MODE_MASTER, 0, 0, 0);
                    ret = 0;
                }    
            } 
        }
        xSemaphoreGive(xMutex);
    }
    return ret;
}

esp_err_t ezlopi_i2c_master_read_byte_from_device(uint32_t channel, uint8_t slave_addr, uint8_t *data, uint8_t register_addr)
{
    esp_err_t ret = ESP_OK;

    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        ret = i2c_master_start(cmd);
        ret = i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        ret = i2c_master_write_byte(cmd, register_addr, ACK_CHECK_EN);
        ret = i2c_master_start(cmd);
        ret = i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        ret = i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
        ret = i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(channel, cmd, 50 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        xSemaphoreGive(xMutex);
    }
    
    return ret;
}

esp_err_t ezlopi_i2c_master_write_byte_to_device(uint32_t channel, uint8_t slave_addr, uint8_t data, uint8_t register_addr)
{
    esp_err_t ret = ESP_OK;

    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        ret = i2c_master_start(cmd);
        ret = i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        ret = i2c_master_write_byte(cmd, register_addr, ACK_CHECK_EN);
        ret = i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
        ret = i2c_master_stop(cmd);
        i2c_master_cmd_begin(channel, cmd, 50 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        xSemaphoreGive(xMutex);
    }

    return ret;
}

esp_err_t ezlopi_i2c_master_read_len_from_device(uint32_t channel, uint8_t slave_addr, uint8_t* data, uint32_t read_len)
{
    esp_err_t error = ESP_OK;

    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        error = i2c_master_read_from_device(channel, slave_addr, data, read_len, 200);
        xSemaphoreGive(xMutex);
    }
    
    return error;
}
esp_err_t ezlopi_i2c_master_write_len_to_device(uint32_t channel, uint8_t slave_addr, uint8_t* data, uint32_t write_len)
{
    esp_err_t error = ESP_OK;
    
    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        error = i2c_master_write_to_device(channel, slave_addr, data, write_len, 200);
        xSemaphoreGive(xMutex);        
    }

    return error;
}

esp_err_t ezlopi_i2c_master_write_read_device(uint32_t channel, uint8_t slave_addr, 
                                                    uint8_t* write_data, uint8_t write_len, 
                                                    uint8_t* read_data, uint32_t read_len)
{
    esp_err_t error = ESP_OK;

    if (xSemaphoreTake(xMutex, portMAX_DELAY))
    {
        error = i2c_master_write_read_device(channel, slave_addr, write_data, write_len, read_data, read_len, 200);
        xSemaphoreGive(xMutex);
    }
    
    return error;
}

int ezlopi_i2c_master_deinit(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    int ret = 0;
    if (NULL != i2c_master_conf)
    {
        ezlopi_i2c_mark_channel_as_available(i2c_master_conf->channel);
        i2c_driver_delete(i2c_master_conf->channel);
    }
    return ret;
}

static int ezlopi_i2c_master_get_available_channel()
{
    for (int i = 0; i < EZLOPI_I2C_NUM_MAX; i++)
    {
        if (false == ezlopi_i2c_channel_not_available_array[i])
        {
            return i;
        }
    }
    return EZLOPI_I2C_ERROR_CHANNEL_NOT_AVAILABLE;
}

static int ezlopi_i2c_mark_channel_as_unavailable(int ezlopi_i2c_channel_number)
{
    int ret = 0;
    ezlopi_i2c_channel_not_available_array[ezlopi_i2c_channel_number] = true;
    return ret;
}

static int ezlopi_i2c_mark_channel_as_available(int ezlopi_i2c_channel_number)
{
    int ret = 0;
    ezlopi_i2c_channel_not_available_array[ezlopi_i2c_channel_number] = false;
    return ret;
}

static void ezlopi_i2c_channel_status()
{
    for (int i = 0; i < EZLOPI_I2C_NUM_MAX; i++)
    {
        // TRACE_B("Channel[%d] is %s", i, ((ezlopi_i2c_channel_not_available_array[i] == false) ? "available" : "unavailable"));
    }
}