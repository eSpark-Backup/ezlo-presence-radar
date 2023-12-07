#include <stdlib.h>
#include <cJSON.h>
#include <sdkconfig.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "trace.h"
#include "items.h"

#include "ezlopi_timer.h"
#include "ezlopi_cloud.h"
#include "ezlopi_actions.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_device_value_updated.h"

#include "radar_app.h"
#include "gpio_isr_service.h"
#include "0038_sensor_presence_sensor.h"

#define PRESENCE_SENSOR_INTERRUPT_PIN 10

static const char *presence_token_vals[3] = {
    "unknown",
    "not_detected",
    "presence",
};

static const char *motion_direction_token_vals[3] = {
    "unknown",
    "incoming",
    "outgoing",
};

// DMA_ATTR static uint16_t samples_spi_buf_1[BUFF_SIZE];
// DMA_ATTR static uint16_t samples_buf1[TOTAL_DATA * 2];
// DMA_ATTR static uint16_t samples_buf2[TOTAL_DATA * 2];

static int __prepare(void *arg);
static int __init(l_ezlopi_item_t *item);
static int __notify(l_ezlopi_item_t *item);
static int __get_hub_item(l_ezlopi_item_t *item, void *args);
static int __get_cjson_ezlopi_value(l_ezlopi_item_t *item, void *arg);

static int32_t radar_get_fifo_data(s_device_args_t *device_args);

int presence_sensor_v3(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg)
{
    int ret = 0;
    // TRACE_B("%s", ezlopi_actions_to_string(action));
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        __prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        __init(item);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    {
        __get_hub_item(item, arg);
        break;
    }
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        __get_cjson_ezlopi_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    {
        static int count = 0;
        if(++count > 4)
        {
            __notify(item);
            count = 0;
        }
        break;
    }
    default:
    {
        break;
    }
    }

    return ret;
}

static int __notify(l_ezlopi_item_t *item)
{
    int ret = 0;

    s_device_args_t *device_args = (s_device_args_t *) item->user_arg;
    esp_err_t error = radar_get_data(device_args);
    if (ESP_OK == error)
    {
        TRACE_I("%s", esp_err_to_name(error));
        ezlopi_device_value_updated_from_device_v3(item);
        TRACE_W("range %f, angle_azimuth %f, angle_elevation %f, speed, %f", device_args->target_result.range, device_args->target_result.angle_azimuth, device_args->target_result.angle_elevation, device_args->target_result.speed);
    }

    return ret;
}

static void __add_array_to_object(cJSON *cj_params, const char *const *arr, const int count)
{
    if (cj_params && arr)
    {
        cJSON *enum_array = cJSON_CreateStringArray(arr, count);
        cJSON_AddItemToObject(cj_params, "enum", enum_array);
        TRACE_B("%s", cJSON_Print(cj_params));
    }
}

static void __add_values_to_cjson(l_ezlopi_item_t *item, cJSON *cj_params, bool is_hub_request)
{
    s_device_args_t *device_args = (s_device_args_t *) item->user_arg;

    char valueFormatted[20];
    if (ezlopi_item_name_velocity == item->cloud_properties.item_name)
    {
        snprintf(valueFormatted, 20, "%0.3f", device_args->target_result.speed);
        cJSON_AddStringToObject(cj_params, "valueFormatted", valueFormatted);
        cJSON_AddNumberToObject(cj_params, "value", device_args->target_result.speed);
    }
    if (ezlopi_item_name_presence == item->cloud_properties.item_name)
    {
        if (is_hub_request)
        {
            __add_array_to_object(cj_params, presence_token_vals, 3);
        }
        cJSON_AddStringToObject(cj_params, "valueFormatted", ((true == device_args->target_result.presence) ? "presence" : "not_detected"));
        cJSON_AddStringToObject(cj_params, "value", ((true == device_args->target_result.presence) ? "presence" : "not_detected"));
    }
    if (ezlopi_item_name_distance == item->cloud_properties.item_name)
    {
        snprintf(valueFormatted, 20, "%0.5f", device_args->target_result.range);
        cJSON_AddStringToObject(cj_params, "valueFormatted", valueFormatted);
        cJSON_AddNumberToObject(cj_params, "value", device_args->target_result.range);
    }
    if (ezlopi_item_name_motion_direction == item->cloud_properties.item_name)
    {
        if (is_hub_request)
        {
            __add_array_to_object(cj_params, motion_direction_token_vals, 3);
        }
        cJSON_AddStringToObject(cj_params, "valueFormatted", ((device_args->target_result.speed < 0) ? "incoming" : "outgoing"));
        cJSON_AddStringToObject(cj_params, "value", ((device_args->target_result.speed < 0) ? "incoming" : "outgoing"));
    }
    if ((ezlopi_item_name_angle_position == item->cloud_properties.item_name) && (scales_north_pole_degress == item->cloud_properties.scale))
    {
        snprintf(valueFormatted, 20, "%0.3f", device_args->target_result.angle_azimuth);
        cJSON_AddStringToObject(cj_params, "valueFormatted", valueFormatted);
        cJSON_AddNumberToObject(cj_params, "value", device_args->target_result.angle_azimuth);
    }
    if ((ezlopi_item_name_angle_position == item->cloud_properties.item_name) && (scales_elevation_degrees == item->cloud_properties.scale))
    {
        snprintf(valueFormatted, 20, "%0.3f", device_args->target_result.angle_elevation);
        cJSON_AddStringToObject(cj_params, "valueFormatted", valueFormatted);
        cJSON_AddNumberToObject(cj_params, "value", device_args->target_result.angle_elevation);
    }
}

static int __get_hub_item(l_ezlopi_item_t *item, void *args)
{
    int ret = 0;

    cJSON *cj_params = (cJSON *)args;
    if (cj_params)
    {
        __add_values_to_cjson(item, cj_params, true);
    }

    return ret;
}

static int __get_cjson_ezlopi_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    cJSON *cj_params = (cJSON *)arg;
    if (cj_params)
    {
        __add_values_to_cjson(item, cj_params, false);
    }

    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    int ret = 0;

    if (item && item->user_arg)
    {
        s_device_args_t *device_args = (s_device_args_t *)item->user_arg;
        if (NULL == device_args->task_handle)
        {
            device_args->task_handle = 3;
            if(ESP_OK == initializeRadarTask(item))
            {
                TRACE_B("'radar_app_init' OK");
                ret = 1;
            }
        }
        else
        {
            TRACE_W("Presence sensor already initiated!");
        }
    }

    return ret;
}

static int __prepare(void *arg)
{
    int ret = 0;

    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        s_device_args_t *device_args = malloc(sizeof(s_device_args_t));
        if (device_args)
        {
            memset(device_args, 0, sizeof(s_device_args_t));
            dump("device_args", &device_args, 0, sizeof(s_device_args_t));

            device_args->dma_spi_buffer[0] = heap_caps_malloc(BUFF_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA); // samples_spi_buf_1;
            device_args->dma_spi_buffer[1] = heap_caps_malloc(BUFF_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA); // samples_spi_buf_2;
            device_args->samples_spi_buf = device_args->dma_spi_buffer[0];

            presence_sensor_dev_item_prep(prep_arg, device_args);
        }
    }

    return ret;
}
