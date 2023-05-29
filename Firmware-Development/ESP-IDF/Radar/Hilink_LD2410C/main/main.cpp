#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "ld2410.h"

#define BUF_SIZE (1024)

extern "C" 
{ 
	void app_main(); 
} 

// ld2410 radar;
// Stream radarUART;

uint32_t lastReading = 0;
bool radarConnected = false;

void app_main(void)
{
    // delay(500);
    printf("\nConnect LD2410 radar TX to GPIO: %d\n", RADAR_RX_PIN);
    printf("Connect LD2410 radar RX to GPIO: %d\n", RADAR_TX_PIN);
    printf("LD2410 radar sensor initialising:\n");
    if(ld2410_begin(true))
    {
        printf("OK\n");
        // printf("LD2410 firmware version: %02X.%02X.%02X\n",radar.firmware_major_version, radar.firmware_minor_version, radar.firmware_bugfix_version);

        ld2410_setMaxValues(distance_600cm, distance_525cm, 5); // (max_moving_distance, max_still_distance, no_one_duration in seconds)

        // uint8_t mov_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20}; // Gate 0 to 8 sensitivity : sensitivity range 0-100
        // uint8_t still_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20}; // Gate 0 to 8 sensitivity : sensitivity range 0-100
        // ld2410_setGateSensitivityThreshold(mov_sensitivity, still_sensitivity);
    }
    else
    {
        printf("not connected\n");
    }
    
    while (1) 
    {
        ld2410_read();
        if(ld2410_isConnected() && millis() - lastReading > 300)  //Report every 300ms
        {
            lastReading = millis();
            if(ld2410_presenceDetected())
            {
                // if(ld2410_stationaryTargetDetected())
                // {
                //     printf("Stationary target: %d ",radar.stationaryTargetDistance());
                //     printf("cm energy: %d ",radar.stationaryTargetEnergy());
                // }
                // if(ld2410_movingTargetDetected())
                // {
                //     printf("Moving target: %d ",radar.movingTargetDistance());
                //     printf("cm energy: %d ",radar.movingTargetEnergy());
                // }
                printf("\n");
            }
            else
            {
                printf("No target\n");
            }
        }
    }
}
