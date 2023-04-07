#pragma once
#include "esp_system.h"
#include "Adafruit_BME680.h"

#define I2C_MASTER_SCL_IO (gpio_num_t)17
#define I2C_MASTER_SDA_IO (gpio_num_t)18
#define I2C_MASTER_NUM I2C_NUM_0
#define SEALEVELPRESSURE_HPA (1013.25)

typedef enum 
{
    enabled = 1,
    disabled = 0,
} precision_t;

typedef struct 
{
    // Inputs
    precision_t precision;

    // Outputs
    float temperature;
    float pressure;
    float humidity;
    float gas_resistance;
    float altitude;
} bme680_fields_t;

// Fields fields;
esp_err_t bme680_setup(precision_t precision); 
esp_err_t bme680_test(void);
esp_err_t bme680_read(bme680_fields_t* bme680_fields) ;  
esp_err_t bme680_set_precision(precision_t precision);