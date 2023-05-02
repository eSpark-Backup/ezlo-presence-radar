#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Radar/radar.h"

TaskHandle_t getFIFOHandle = NULL;

int app_main(int argc, char* argv[])
{ 
  xTaskCreatePinnedToCore(radar,"radar", 2048*3, NULL, 10, &getFIFOHandle, 1);

  while(1) 
  {   
      vTaskDelay(1000/ portTICK_RATE_MS);  
  }

  return 0;
}

