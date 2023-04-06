#pragma once

#include "LTR303.h"

#define DEBUG 0

esp_err_t ltr303_setup(void); 
esp_err_t ltr303_loop(void); 
esp_err_t ltr303_get_val(double* lux); 
bool ltr303_is_data_available(void);