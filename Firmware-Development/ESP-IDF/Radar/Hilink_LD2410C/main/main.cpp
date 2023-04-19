#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "ld2410.h"

extern "C" 
{ 
	void app_main(); 
} 

ld2410 radar;
Stream radarUART;

uint32_t lastReading = 0;
bool radarConnected = false;

void app_main(void)
{
    delay(500);
    printf("\nConnect LD2410 radar TX to GPIO: %d\n", RADAR_RX_PIN);
    printf("Connect LD2410 radar RX to GPIO: %d\n", RADAR_TX_PIN);
    printf("LD2410 radar sensor initialising:\n");
    if(radar.begin(&radarUART))
    {
        printf("OK\n");
        printf("LD2410 firmware version: %02X.%02X.%02X\n",radar.firmware_major_version, radar.firmware_minor_version, radar.firmware_bugfix_version);
    }
    else
    {
        printf("not connected\n");
    }
    while (1) 
    {
        radar.read();
        if(radar.isConnected() && millis() - lastReading > 300)  //Report every 300ms
        {
            lastReading = millis();
            if(radar.presenceDetected())
            {
                if(radar.stationaryTargetDetected())
                {
                    printf("Stationary target: %d ",radar.stationaryTargetDistance());
                    printf("cm energy: %d ",radar.stationaryTargetEnergy());
                }
                if(radar.movingTargetDetected())
                {
                    printf("Moving target: %d ",radar.movingTargetDistance());
                    printf("cm energy: %d ",radar.movingTargetEnergy());
                }
                printf("\n");
            }
            else
            {
                printf("No target\n");
            }
        }
        // vTaskDelay(10/ portTICK_RATE_MS);
    }
}
