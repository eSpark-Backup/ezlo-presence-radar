#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "BME680.hpp"

#define GAS_SENSOR_TEST 1

extern "C" 
{ 
	void app_main(); 
} 

GasSensor* gasSensor;

void app_main()
{
  gasSensor = new BME680;
  gasSensor->fields.precision = disabled;
  gasSensor->setup(gasSensor->fields.precision);
  while (1)
  {
    #if GAS_SENSOR_TEST
      gasSensor->test();
    #else
      if(gasSensor->read(&(gasSensor->fields)) == ESP_OK)
      {
        printf("Temperature = %0.2f *C\n",gasSensor->fields.temperature);
        printf("Pressure = %0.2f hPa\n",gasSensor->fields.pressure);
        printf("Humidity = %0.2f %%\n",gasSensor->fields.humidity);
        printf("Gas = %0.2f KOhms\n",gasSensor->fields.gas_resistance);
        printf("Approx. Altitude = %0.2f m\n\n",gasSensor->fields.altitude);
      }
    #endif
      vTaskDelay(1000/portTICK_RATE_MS);
  }  
}
