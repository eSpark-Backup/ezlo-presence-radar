#include <stdio.h>
#include "ezlo_w5500.h"

w5500_t w5500;
spi_t spi;

void app_main(void)
{
    ezlo_w5500_setup(&w5500, &spi);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
