#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bgt60tr13c.h"

static char* TAG = "radar_app";

int app_main(int argc, char* argv[])
{ 
  ESP_ERROR_CHECK(radar_init());

  while(1) 
  {   
      vTaskDelay(1000/ portTICK_RATE_MS);  
  }

  return 0;
}

