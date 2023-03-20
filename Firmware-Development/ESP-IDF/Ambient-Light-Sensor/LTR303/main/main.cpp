#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "LTR303.h"



static const char *TAG_MAIN = "LTR303";
void printError(byte error);

extern "C" 
{ 
	void app_main(); 
} 

unsigned char gain;     // Gain setting, values = 0-7 
unsigned char integrationTime;  // Integration ("shutter") time in milliseconds
unsigned char measurementRate;  // Interval between DATA_REGISTERS update

void app_main()
{
    byte error;

    LTR303 light;
    light.begin();
    unsigned char ID;
  
    if (light.getPartID(ID)) {
        printf("Got Sensor Part ID: %02X\n", ID);
    }
    if (light.getManufacID(ID)) {
        printf("Got Manuf Part ID: %02X\n", ID);
    }

    gain = 0;
    printf("Setting Gain...\n");
    light.setControl(gain, false, false);

    // If integrationTime = 0, integrationTime will be 100ms (default)
    // If integrationTime = 1, integrationTime will be 50ms
    // If integrationTime = 2, integrationTime will be 200ms
    // If integrationTime = 3, integrationTime will be 400ms
    // If integrationTime = 4, integrationTime will be 150ms
    // If integrationTime = 5, integrationTime will be 250ms
    // If integrationTime = 6, integrationTime will be 300ms
    // If integrationTime = 7, integrationTime will be 350ms
    
    integrationTime = 1;
    printf("Set timing...\n");
    light.setMeasurementRate(integrationTime,3);

    // To start taking measurements, power up the sensor:
    
    printf("Powerup...\n");
    light.setPowerUp();
    
    error = light.getError();
    if(error)
    {
        light.printError(error);
    }
    
    while (1)
    {
        unsigned int data0 = 0, data1 = 0;
    
        if (light.getData(data0,data1)) {
            // getData() returned true, communication was successful
            
            printf("data0: %d\n",data0);
            printf("data1: %d\n",data1);
        
            // To calculate lux, pass all your settings and readings
            // to the getLux() function.
            
            // The getLux() function will return 1 if the calculation
            // was successful, or 0 if one or both of the sensors was
            // saturated (too much light). If this happens, you can
            // reduce the integration time and/or gain.
        
            double lux;    // Resulting lux value
            boolean good;  // True if neither sensor is saturated
            
            // Perform lux calculation:

            good = light.getLux(gain,integrationTime,data0,data1,lux);
            
            // Print out the results:
            
            printf(" lux: %f\n",lux);
            if (good) printf(" (good)\n"); else printf(" (BAD)\n");
        }
        vTaskDelay(1000/portTICK_RATE_MS);
    }  
}

