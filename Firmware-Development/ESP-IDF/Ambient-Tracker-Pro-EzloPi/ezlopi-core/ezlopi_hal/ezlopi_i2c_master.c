
#include "stdint.h"
#include "driver/i2c.h"
#include "trace.h"
#include "ezlopi_i2c_master.h"

static s_ezlopi_i2c_master_t *i2c_master_conf_ptr[I2C_NUM_MAX] = {NULL, NULL};
static SemaphoreHandle_t i2c_mutex = NULL;
static bool mutex_initialized = false;

static int ezlopi_i2c_check_channel(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    int ret = 0;

    for (int idx = 0; idx < I2C_NUM_MAX; idx++)
    {
        if (NULL != i2c_master_conf_ptr)
        {
        }
    }

    return ret;
}

int ezlopi_i2c_master_init(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    int ret = 0;

    if(!mutex_initialized)
    {
        i2c_mutex = xSemaphoreCreateMutex();
        mutex_initialized = true;
    }

    if (NULL != i2c_master_conf)
    {
        if (true == i2c_master_conf->enable)
        {
            if (NULL == i2c_master_conf_ptr[i2c_master_conf->channel])
            {
                i2c_config_t i2c_config = {
                    .mode = I2C_MODE_MASTER,
                    .sda_io_num = i2c_master_conf->sda,
                    .scl_io_num = i2c_master_conf->scl,
                    .sda_pullup_en = GPIO_PULLUP_ENABLE,
                    .scl_pullup_en = GPIO_PULLUP_ENABLE,
                    .master.clk_speed = i2c_master_conf->clock_speed,
                };

                esp_err_t error = i2c_param_config(i2c_master_conf->channel, &i2c_config);
                TRACE_B("config error is %s", esp_err_to_name(error));
                error = i2c_driver_install(i2c_master_conf->channel, I2C_MODE_MASTER, 0, 0, 0);
                TRACE_B("driver installation error is %s", esp_err_to_name(error));
                i2c_master_conf_ptr[i2c_master_conf->channel] = i2c_master_conf;
            }
            else 
            {
                ret = 1;
            }
        }
    }
    return ret;
}

esp_err_t ezlopi_i2c_master_read_from_device(s_ezlopi_i2c_master_t *i2c_master_conf, uint8_t *read_buffer, uint32_t read_len)
{
    esp_err_t error = ESP_OK;
    // 1000 / portTICK_PERIOD_MS
    if(xSemaphoreTake(i2c_mutex, portMAX_DELAY))
    {
        error = i2c_master_read_from_device(i2c_master_conf->channel, i2c_master_conf->address, read_buffer, read_len, 200);
    }
    xSemaphoreGive(i2c_mutex);
    return error;
}

esp_err_t ezlopi_i2c_master_write_to_device(s_ezlopi_i2c_master_t *i2c_master_conf, uint8_t *write_buffer, uint32_t write_len)
{
    esp_err_t error = ESP_OK;
    if(xSemaphoreTake(i2c_mutex, portMAX_DELAY))
    {
        error = i2c_master_write_to_device(i2c_master_conf->channel, i2c_master_conf->address, write_buffer, write_len, 200);
    }
    xSemaphoreGive(i2c_mutex);
    return error;
}

esp_err_t ezlopi_i2c_master_deinit(s_ezlopi_i2c_master_t *i2c_master_conf)
{
    esp_err_t error = ESP_OK;
    if (NULL != i2c_master_conf)
    {
        if (1 == i2c_master_conf_ptr[i2c_master_conf->channel])
        {
            error = i2c_driver_delete(i2c_master_conf->channel);
            i2c_master_conf_ptr[i2c_master_conf->channel] = NULL;
        }
    }
    return error;
}