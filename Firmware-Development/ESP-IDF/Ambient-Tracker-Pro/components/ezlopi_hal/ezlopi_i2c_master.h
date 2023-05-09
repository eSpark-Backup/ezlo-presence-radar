#ifndef __EZLOPI_HAL_H__
#define __EZLOPI_HAL_H__

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"

#define EZLOPI_I2C_NUM_MAX I2C_NUM_MAX

#define EZLOPI_I2C_ERROR_CHANNEL_NOT_AVAILABLE -1
#define ACK_CHECK_EN 0x1



typedef struct s_ezlopi_i2c_master
{
    bool enable;
    uint32_t sda;
    uint32_t scl;
    uint32_t clock_speed;
    uint32_t channel;
    uint32_t address;
} s_ezlopi_i2c_master_t;



#define EZLOPI_I2C_MASTER_DEFAULT_CONF                                   \
    {                                                                    \
        .sda = 9, .scl = 10, .clock_speed = 100000, .channel = I2C_NUM_0 \
    }


int ezlopi_i2c_master_init(s_ezlopi_i2c_master_t *i2c_master_conf);
int ezlopi_i2c_master_deinit(s_ezlopi_i2c_master_t *i2c_master_conf);

esp_err_t ezlopi_i2c_master_read_byte_from_device(uint32_t channel, uint8_t slave_addr, uint8_t *data, uint8_t register_addr);
esp_err_t ezlopi_i2c_master_write_byte_to_device(uint32_t channel, uint8_t slave_addr, uint8_t data, uint8_t register_addr);

esp_err_t ezlopi_i2c_master_read_len_from_device(uint32_t channel, uint8_t slave_addr, uint8_t* data, uint32_t read_len);
esp_err_t ezlopi_i2c_master_write_len_to_device(uint32_t channel, uint8_t slave_addr, uint8_t* data, uint32_t write_len);

esp_err_t ezlopi_i2c_master_write_read_device(uint32_t channel, uint8_t slave_addr, 
                                                    uint8_t* write_data, uint8_t write_len, 
                                                    uint8_t* read_data, uint32_t read_len);

#endif // __EZLOPI_HAL_H__