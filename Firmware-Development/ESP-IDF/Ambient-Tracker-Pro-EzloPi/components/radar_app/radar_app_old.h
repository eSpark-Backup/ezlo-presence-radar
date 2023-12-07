
#ifndef __RADAR_APP_H__
#define __RADAR_APP_H__

#include <inttypes.h>
#include "esp_check.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "xensiv_bgt60trxx.h"
#include "xensiv_bgt60trxx_esp32.h"

#define XENSIV_BGT60TRXX_CONF_IMPL 1

#include "presence_radar_default.h"
#include "RangeAngleImage/range_angle_image.h"
#include "RangeDopplerMap/range_doppler_map.h"
#include "AngleCapon/angle_capon.h"
#include "Presence/presence.h"
#include "motion_angle.h"

#define MOTION_ANGLE_APP 1
#define VELOCITY_APP 0
#define PRESENCE_APP 0

#define ESP_INTR_FLAG_DEFAULT 0

#define GPIO_LEVEL_LOW 0
#define GPIO_LEVEL_HIGH 1

/**
 * @brief Setup radar parameters
 *
 * @return esp_err_t
 *  - ESP_OK    : radar setup successfully
 *  - ESP_FAIL  : failed to setup radar
 */
esp_err_t radar_app_init();

/**
 * @brief Get target's data
 *
 * @param[in] target_result pointer to target data structure
 * @return esp_err_t
 *  - ESP_OK    : frame processed succcessfully
 *  - ESP_FAIL  : in case of failure in any of the stages of processing
 */
esp_err_t radar_get_data(ifx_Target_Result_t *target_result);

/**
 * @brief Initialize radar task for test
 *
 * @return esp_err_t
 *  - ESP_OK    : task created successfully
 *  - ESP_FAIL  : failed to create radar task
 */
esp_err_t radar_init(void);

esp_err_t radar_reset_radar_fifo();
esp_err_t radar_set_radar_fifo();

#endif // __RADAR_APP_H__
