#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "radar_app.h"
#include "bme680_bsec.h"
#include "ALS_LTR303.h"
#include "SK6812.h"
#include "pms5003.h"

static const char *TAG = "ambient_tracker_pro";

int app_main(int argc, char *argv[])
{
  ESP_ERROR_CHECK(radar_init());
  ESP_ERROR_CHECK(ltr303_init());
  ESP_ERROR_CHECK(bme680_init());
  ESP_ERROR_CHECK(sk6812_init());
  ESP_ERROR_CHECK(pms5003_init());

  return 0;
}
