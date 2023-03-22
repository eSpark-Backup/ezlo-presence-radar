#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pms5003.h"

void delay_sec(int time)
{
    vTaskDelay(time * 1000 / portTICK_PERIOD_MS);
}

void app_main()
{
    pms5003_config_t pms0 = {
            .set_pin = GPIO_NUM_39,
            .reset_pin = GPIO_NUM_42,
            .rxd_pin = GPIO_NUM_41,
            .txd_pin = GPIO_NUM_40,
            .uart_instance = UART_NUM_1,
            .uart_buffer_size = 128
    };
    pms5003_setup(&pms0);

    while(1)
    {
        pms5003_measurement_t reading;
        pms5003_make_measurement(&pms0, &reading);
        pms5003_print_measurement(&reading);

        delay_sec(5);
    }
}
