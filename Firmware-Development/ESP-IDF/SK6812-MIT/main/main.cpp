#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "RGBLED.hpp"

extern "C" void app_main()
{
    RGBLED SK6812; /* SK6812 RGB LED Configuration */
    SK6812.setup(); /* Initialize SK6812 */

    while (1)
    {
       SK6812.test();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
