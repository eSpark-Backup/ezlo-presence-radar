
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
#include "ezlopi_devices.h"

#define VELOCITY_APP 0
#define PRESENCE_APP 0
#define MOTION_ANGLE_APP 1

#define GPIO_LEVEL_LOW 0
#define GPIO_LEVEL_HIGH 1

#define ESP_INTR_FLAG_DEFAULT 0

typedef struct s_device_args
{
    TaskHandle_t task_handle; // used

    ifx_Avian_Device_t *device_handle;
    spi_device_handle_t xnsiv_bgt60_spi;
    xensiv_bgt60trxx_t *xnsiv_bgt60tr13c;

    uint16_t no_of_bytes;
    bool fifo_read_status;
    uint8_t fifo_read_count;
    bool frame_process_status;
    bgt60trxx_fifo_status_t radar_fifo_status;

    uint16_t *samples_spi_buf;
    spi_transaction_t transaction;

    uint16_t *p_samples;
    uint16_t *samples_buf1;
    uint16_t *samples_buf2;
    ifx_Cube_R_t *frame;

#if MOTION_ANGLE_APP
    ma_t ma_context;
#endif

#if VELOCITY_APP
    // rdm
    rdm_t *rdm_context = NULL;
#endif

#if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
    // presence
    presence_t presence_handle;
    ifx_Target_Result_t target_result;
#endif

} s_device_args_t;

/**
 * @brief Setup radar parameters
 *
 * @return esp_err_t
 *  - ESP_OK    : radar setup successfully
 *  - ESP_FAIL  : failed to setup radar
 */
esp_err_t radar_app_init(l_ezlopi_item_t *item);

esp_err_t radar_set_radar_fifo(s_device_args_t *device_args);
esp_err_t radar_reset_radar_fifo(s_device_args_t *device_args);

/**
 * @brief Get target's data
 *
 * @param[in] target_result pointer to target data structure
 * @return esp_err_t
 *  - ESP_OK    : frame processed succcessfully
 *  - ESP_FAIL  : in case of failure in any of the stages of processing
 */

esp_err_t radar_get_data(s_device_args_t *device_args);

#endif // __RADAR_APP_H__
