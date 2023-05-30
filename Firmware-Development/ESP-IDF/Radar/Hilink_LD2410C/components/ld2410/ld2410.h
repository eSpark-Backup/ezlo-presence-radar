#pragma once

#include "ld2410_driver.h"

typedef enum ld2410_template
{
    SLEEP_TEMPLATE_CLOSE_RANGE,
    SLEEP_TEMPLATE_MID_RANGE,
    SLEEP_TEMPLATE_LONG_RANGE,
    MOVEMENT_TEMPLATE_CLOSE_RANGE,
    MOVEMENT_TEMPLATE_MID_RANGE,
    MOVEMENT_TEMPLATE_LONG_RANGE,
    BASIC_TEMPLATE_CLOSE_RANGE,
    BASIC_TEMPLATE_MID_RANGE,
    BASIC_TEMPLATE_LONG_RANGE,
    CUSTOM_TEMPLATE
} ld2410_template_t;

typedef struct ld2410_settings
{
    uint16_t no_one_duration;
    distance max_move_distance;
    distance max_still_distance;
    uint8_t mov_sensitivity[9];
    uint8_t still_sensitivity[9];
} ld2410_settings_t;

esp_err_t ld2410_setup(void);
esp_err_t ld2410_get_data(void);
esp_err_t ld2410_set_template(ld2410_template_t template);
