#pragma once

#include "bme68xLibrary.h"
#include "bsec2.h"

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

void bme680_test(void);