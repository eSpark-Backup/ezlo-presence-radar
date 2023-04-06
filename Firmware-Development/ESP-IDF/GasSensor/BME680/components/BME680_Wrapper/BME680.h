#pragma once
#include "esp_system.h"
#include "Adafruit_BME680.h"

#define I2C_MASTER_SCL_IO (gpio_num_t)17
#define I2C_MASTER_SDA_IO (gpio_num_t)18
#define SEALEVELPRESSURE_HPA (1013.25)

typedef enum 
{
    enabled = 1,
    disabled = 0,
} Precision;

typedef struct 
{
    // Inputs
    Precision precision;

    // Outputs
    float temperature;
    float pressure;
    float humidity;
    float gas_resistance;
    float altitude;
} Fields;

// Fields fields;
esp_err_t bme680_setup(Precision precision); 
esp_err_t bme680_test(void);
esp_err_t bme680_read(Fields*) ;  
esp_err_t bme680_set_precision(Precision precision);