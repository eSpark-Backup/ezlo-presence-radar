#pragma once

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

#include "Avian.h"
#include "Base.h"
#include "2DMTI.h"
#include "SpectrumAxis.h"
#include "RadarSegmentation.h"
#include "RangeDopplerMap.h"
#include "Algo.h"

#include <inttypes.h>
#include <math.h>

#include "RangeAngleImage/range_angle_image.h"
#include "RangeDopplerMap/range_doppler_map.h"
#include "AngleCapon/angle_capon.h"

#define SAMP_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS) 
#define TOTAL_DATA (SAMP_PER_FRAME + (6 * XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME))

#define GPIO_LEVEL_LOW 0
#define GPIO_LEVEL_HIGH 1

// Tasks
void radar(void* pvParameter);

// Functions
void radar_gpio_init(void);
esp_err_t radar_configure_led(void);
esp_err_t radar_spi_init(void);
void radar_led_blinky(void);
int32_t radar_get_fifo_data(void);
void radar_process_frame(ifx_Avian_Device_t* device_handle, ifx_RAI_t* rai_handle, ifx_Cube_R_t* frame, ifx_Cube_R_t* output);
void radar_print_fifo_err(void);
void radar_print_device_config(const ifx_Avian_Config_t* config);

void getArrData(uint16_t* dest,uint16_t* src);
void getEspRuntimeStatus(void);
