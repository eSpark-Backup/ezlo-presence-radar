#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "Adafruit_BME680.h"

#define I2C_MASTER_SCL_IO (gpio_num_t)17
#define I2C_MASTER_SDA_IO (gpio_num_t)18
#define SEALEVELPRESSURE_HPA (1013.25)

extern "C" 
{ 
	void app_main(); 
} 
void bme_setup();
void bme_loop();

Adafruit_BME680 bme(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
void app_main()
{
    bme_setup();
    while (1)
    { 
        bme_loop();
        vTaskDelay(1000/portTICK_RATE_MS);
    }  
}

void bme_setup() {

  printf("BME680 test\n");

  if (!bme.begin()) {
    printf("Could not find a valid BME680 sensor, check wiring!\n");
    while (1);
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void bme_loop() {
  if (! bme.performReading()) {
    printf("Failed to perform reading :(\n");
    return;
  }
  printf("Temperature = %0.2f *C\n",bme.temperature);

  printf("Pressure = %0.2f hPa\n",bme.pressure / 100.0);

  printf("Humidity = %0.2f %%\n",bme.humidity);

  printf("Gas = %0.2f KOhms\n",(bme.gas_resistance / 1000.0));

  printf("Approx. Altitude = %0.2f m\n\n",bme.readAltitude(SEALEVELPRESSURE_HPA));
}