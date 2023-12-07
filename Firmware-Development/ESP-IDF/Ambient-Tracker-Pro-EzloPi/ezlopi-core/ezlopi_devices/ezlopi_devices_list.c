#include "ezlopi_actions.h"
#include "ezlopi_devices_list.h"

#include "0008_sensor_I2C_LTR303ALS.h"
#include "0009_other_RMT_SK6812.h"
#include "0010_I2C_BME680.h"
#include "0037_sens_pms5003_sensor.h"
#include "0038_sensor_presence_sensor.h"

static s_ezlopi_device_v3_t device_array_v3[] = {

#ifdef EZLOPI_SENSOR_0008_ALS_LTR303_I2C_SENSOR
    {
        .id = EZLOPI_SENSOR_0008_ALS_LTR303_I2C_SENSOR,
        .func = sensor_0008_I2C_LTR303ALS_v3,
    },
#endif

#ifdef EZLOPI_DEVICE_0009_SK6812_LED_STRIP
    {
        .id = EZLOPI_DEVICE_0009_SK6812_LED_STRIP,
        .func = device_0009_other_RMT_SK6812_v3,
    },
#endif

#ifdef EZLOPI_SENSOR_0010_BME680_I2C
    {
        .id = EZLOPI_SENSOR_0010_BME680_I2C,
        .func = sensor_0010_I2C_BME680_v3,
    },
#endif

#ifdef EZLOPI_SENSOR_0037_PMS_UART
    {
        .id = EZLOPI_SENSOR_0037_PMS_UART,
        .func = pms5003_sensor_v3,
    },
#endif

#ifdef EZLOPI_SENSOR_0038_RADAR_SENSOR
    {
        .id = EZLOPI_SENSOR_0038_RADAR_SENSOR,
        .func = presence_sensor_v3,
    },
#endif
    /**
     * @brief 'EZLOPI_SENSOR_NONE' must not be removed from this array.
     * This is essential for terminating the loop termination of loop.
     */
    {
        .id = EZLOPI_SENSOR_NONE,
        .func = NULL,
    },
};

#if 1 // version 3.x.x
s_ezlopi_device_v3_t *ezlopi_devices_list_get_list_v3(void)
{
    return device_array_v3;
}
#endif

#if 1 // version 1.x.x to 2.x.x
s_ezlopi_device_t *ezlopi_devices_list_get_list(void)
{
    return NULL;
    // device_array;
}

static l_ezlopi_configured_devices_t *configured_device = NULL;
static l_ezlopi_configured_devices_t *ezlopi_device_list_create(s_ezlopi_device_t *device, s_ezlopi_device_properties_t *properties, void *user_arg);

l_ezlopi_configured_devices_t *ezlopi_devices_list_get_configured_items(void)
{
    return configured_device;
}

int ezlopi_devices_list_add(s_ezlopi_device_t *device, s_ezlopi_device_properties_t *properties, void *user_arg)
{
    int ret = 0;
    if (configured_device)
    {
        l_ezlopi_configured_devices_t *current_dev = configured_device;

        while (NULL != current_dev->next)
        {
            current_dev = current_dev->next;
        }

        current_dev->next = ezlopi_device_list_create(device, properties, user_arg);
        if (current_dev->next)
        {
            ret = 1;
        }
    }
    else
    {
        configured_device = ezlopi_device_list_create(device, properties, user_arg);
        if (configured_device)
        {
            ret = 1;
        }
    }

    return ret;
}

static l_ezlopi_configured_devices_t *ezlopi_device_list_create(s_ezlopi_device_t *device, s_ezlopi_device_properties_t *properties, void *user_arg)
{
    l_ezlopi_configured_devices_t *device_list_element = (l_ezlopi_configured_devices_t *)malloc(sizeof(l_ezlopi_configured_devices_t));
    if (device_list_element)
    {
        memset(device_list_element, 0, sizeof(l_ezlopi_configured_devices_t));
        device_list_element->device = device;
        device_list_element->properties = properties;
        device_list_element->next = NULL;
        if (user_arg)
        {
            device_list_element->user_arg = user_arg;
        }
    }
    return device_list_element;
}
#endif
