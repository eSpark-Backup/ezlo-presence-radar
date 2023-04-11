#include <stdio.h>
#include "bme680_bsec.h"

void app_main(void)
{
    bme680_setup();
    while (1)
    {
       bme680_test();
    } 
}
