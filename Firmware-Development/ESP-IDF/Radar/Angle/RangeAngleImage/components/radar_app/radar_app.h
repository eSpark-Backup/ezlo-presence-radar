#pragma once

#include <inttypes.h>
#include "xensiv_bgt60trxx_esp32.h"
#include "xensiv_bgt60trxx.h"
#include "presence_radar_default.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "debug.h"

#include "RangeAngleImage/range_angle_image.h"
#include "RangeDopplerMap/range_doppler_map.h"
#include "AngleCapon/angle_capon.h"
#include "Presence/presence.h"

#define ANGLE_APP 0
#define VELOCITY_APP 1
#define PRESENCE_APP 0

#define ESP_INTR_FLAG_DEFAULT 0

#define SAMP_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS) 
#define TOTAL_DATA (SAMP_PER_FRAME + (6 * XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME))

#define GPIO_LEVEL_LOW 0
#define GPIO_LEVEL_HIGH 1

#ifdef __cplusplus
extern "C" {
#endif

// Functions
esp_err_t radar_init(void);

#ifdef __cplusplus
}
#endif