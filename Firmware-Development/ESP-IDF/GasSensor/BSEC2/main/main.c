#include <stdio.h>
#include "bme680_bsec.h"

bme680_data_t gas_sesnor_data;


void app_main(void)
{   
    bme680_setup(true); // True : Initialize I2C, False : Skips I2C Initialization
    while (1)
    {
       if (bme680_get_data(&gas_sesnor_data))
       {
            bme680_print_data(&gas_sesnor_data);
       }
    } 
}
