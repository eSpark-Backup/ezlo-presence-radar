#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "SK6812.hpp"

SK6812 *rgb;

extern "C" void app_main()
{
    rgb = new SK6812; /* SK6812 RGB LED Configuration */
    rgb->setup(); /* Initialize SK6812 */

    while (1)
    {
        rgb->test();
    }
}
