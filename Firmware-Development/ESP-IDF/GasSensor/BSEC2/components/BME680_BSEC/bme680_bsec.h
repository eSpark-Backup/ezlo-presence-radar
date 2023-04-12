#pragma once

#include "bme68xLibrary.h"
#include "bsec2.h"

#define BME680_TEST 0

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

void bme680_setup(void);

bool bme680_get_data(bme680_data_t* data);

bool bme680_print_data(bme680_data_t* data);