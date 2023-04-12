#pragma once

#include "bme68xLibrary.h"
#include "bsec2.h"
#include <math.h>

#define BME680_SDA              GPIO_NUM_18
#define BME680_SCL              GPIO_NUM_17
#define ACTIVE_I2C              I2C_NUM_0

#define BME680_TEST 0
#define SEALEVELPRESSURE_HPA (1013.25)

typedef struct 
{
    float iaq;
    bool iaq_accuracy;
    float co2_equivalent;
    float voc_equivalent;
    float temperature;
    float pressure;
    float humidity;
    float gas_resistance;
    float altitude;
    bool stabilization_status;
    bool run_in_status;
}   bme680_data_t;
/**
 * @brief : This function checks the BSEC status, prints the respective error code. Halts in case of error
 * @param[in] bsec  : Bsec2 class object
 */
void check_bsec_status();

/**
 * @brief : This function is called by the BSEC library when a new output is available
 * @param[in] input     : BME68X sensor data before processing
 * @param[in] outputs   : Processed BSEC BSEC output data
 * @param[in] bsec      : Instance of BSEC2 calling the callback
 */
void bme680_data_callback(const bme68x_data data, const bsec_outputs outputs);

void bme680_setup(bool initialize_i2c, gpio_num_t sda, gpio_num_t scl, i2c_port_t i2c_num, uint8_t bme68x_address);

bool bme680_get_data(bme680_data_t* data);

bool bme680_print_data(bme680_data_t* data);

float bme680_read_altitude(float pressure, float seaLevel);