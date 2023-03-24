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
GasSensorOut gasSensorOut;
void app_main()
{
  gasSensor = new BME680;
  gasSensor->setup();
  while (1)
  {
    #if GAS_SENSOR_TEST
      gasSensor->test();
    #else
      if(gasSensor->read(&gasSensorOut) == ESP_OK)
      {
        printf("Temperature = %0.2f *C\n",gasSensorOut.temperature);
        printf("Pressure = %0.2f hPa\n",gasSensorOut.pressure);
        printf("Humidity = %0.2f %%\n",gasSensorOut.humidity);
        printf("Gas = %0.2f KOhms\n",gasSensorOut.gas_resistance);
        printf("Approx. Altitude = %0.2f m\n\n",gasSensorOut.altitude);
      }
    #endif
      vTaskDelay(1000/portTICK_RATE_MS);
  }  
}
