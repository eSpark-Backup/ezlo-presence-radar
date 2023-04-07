#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "BME680.h"

#define GAS_SENSOR_TEST 0

bme680_fields_t gas_sensor_fields;
void app_main()
{
  gas_sensor_fields.precision = disabled;
  bme680_setup(gas_sensor_fields.precision);
  while (1)
  {
    #if GAS_SENSOR_TEST
      bme680_test();
    #else
      if(bme680_read(&gas_sensor_fields) == ESP_OK)
      {
        printf("Temperature = %0.2f *C\n",gas_sensor_fields.temperature);
        printf("Pressure = %0.2f hPa\n",gas_sensor_fields.pressure);
        printf("Humidity = %0.2f %%\n",gas_sensor_fields.humidity);
        printf("Gas = %0.2f KOhms\n",gas_sensor_fields.gas_resistance);
        printf("Approx. Altitude = %0.2f m\n\n",gas_sensor_fields.altitude);
      }
    #endif
      vTaskDelay(1000/portTICK_RATE_MS);
  }  
}
