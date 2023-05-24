
#include <string.h>
#include "sdkconfig.h"

#include "cJSON.h"
#include "trace.h"
#include "ezlopi_timer.h"
#include "items.h"

#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_devices.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_i2c_master.h"
#include "ezlopi_spi_master.h"

#include "0034_sens_bme680_sensor.h"
#include "bme680_bsec.h"

#define ADD_PROPERTIES_DEVICE_LIST(device_id, category, sub_category, item_name, value_type, cjson_device, bme680_sensor_data)                 \
    {                                                                                                                                          \
        s_ezlopi_device_properties_t *_properties = bem680_sensor_prepare_properties(device_id, category, sub_category,                        \
                                                                                     item_name, value_type, cjson_device, bme680_sensor_data); \
        if (NULL != _properties)                                                                                                               \
        {                                                                                                                                      \
            add_device_to_list(prep_arg, _properties, NULL);                                                                                   \
        }                                                                                                                                      \
    }

static bool bme680_sensor_i2c_initialized = false;
static int count = 5;

static int bem680_sensor_prepare(void *arg);
static s_ezlopi_device_properties_t *bem680_sensor_prepare_properties(uint32_t device_id, const char *category,
                                                                      const char *sub_category, const char *item_name,
                                                                      const char *value_type, cJSON *cjson_device, bme680_data_t *bme680_sensor_data);
static int add_device_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *properties, void *user_args);
static int bme680_sensor_init(s_ezlopi_device_properties_t *properties);
static int bme680_sensor_get_value_cjson(s_ezlopi_device_properties_t *properties, void *args);
static int bme680_sensor_update_values(s_ezlopi_device_properties_t *properties);

int bme680_sensor(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *properties, void *arg, void *user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        bem680_sensor_prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        bme680_sensor_init(properties);
        break;
    }
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        bme680_sensor_get_value_cjson(properties, arg);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    {
        if (5 == count)
        {
            bme680_sensor_update_values(properties);
            ezlopi_device_value_updated_from_device(properties);
            count = 0;
        }
        count++;
        break;
    }
    default:
    {
        break;
    }
    }
    return ret;
}

static int bem680_sensor_prepare(void *arg)
{
    int ret = 0;

    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    bme680_data_t *bme680_sensor_data = (bme680_data_t *)malloc(sizeof(bme680_data_t));

    if ((NULL != prep_arg) && (NULL != prep_arg->cjson_device) && (NULL != bme680_sensor_data))
    {
        memset(bme680_sensor_data, 0, sizeof(bme680_data_t));
        uint32_t device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_temperature, subcategory_not_defined, ezlopi_item_name_temp, value_type_temperature, prep_arg->cjson_device, bme680_sensor_data);
        // device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_humidity, subcategory_not_defined, ezlopi_item_name_humidity, value_type_humidity, prep_arg->cjson_device, bme680_sensor_data);
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_generic_sensor, subcategory_not_defined, ezlopi_item_name_atmospheric_pressure, value_type_pressure, prep_arg->cjson_device, bme680_sensor_data);
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_generic_sensor, subcategory_not_defined, ezlopi_item_name_volatile_organic_compound_level, value_type_substance_amount, prep_arg->cjson_device, bme680_sensor_data);
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_generic_sensor, subcategory_not_defined, ezlopi_item_name_distance, value_type_length, prep_arg->cjson_device, bme680_sensor_data);
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_generic_sensor, subcategory_not_defined, ezlopi_item_name_co2_level, value_type_substance_amount, prep_arg->cjson_device, bme680_sensor_data);        
    }
    return ret;
}

static s_ezlopi_device_properties_t *bem680_sensor_prepare_properties(uint32_t device_id, const char *category,
                                                                      const char *sub_category, const char *item_name,
                                                                      const char *value_type, cJSON *cjson_device, bme680_data_t *bme680_sensor_data)
{
    s_ezlopi_device_properties_t *bme680_properties = NULL;

    if (NULL != cjson_device)
    {
        bme680_properties = (s_ezlopi_device_properties_t *)malloc(sizeof(s_ezlopi_device_properties_t));

        if (bme680_properties)
        {
            memset(bme680_properties, 0, sizeof(s_ezlopi_device_properties_t));

            bme680_properties->interface_type = EZLOPI_DEVICE_INTERFACE_I2C_MASTER;

            char *device_name = NULL;

            if (ezlopi_item_name_temp == item_name)
            {
                device_name = "Tmp / Hum ";
            }
            if (ezlopi_item_name_humidity == item_name)
            {
                device_name = "Tmp / Hum ";
            }
            if (ezlopi_item_name_atmospheric_pressure == item_name)
            {
                device_name = "ATM";
            }
            if (ezlopi_item_name_volatile_organic_compound_level == item_name)
            {
                device_name = "AQI";
            }
            if (ezlopi_item_name_distance == item_name)
            {
                device_name = "Altitude";
            }
            if (ezlopi_item_name_co2_level == item_name)
            {
                device_name = "Equivalent CO2";
            }            
            ASSIGN_DEVICE_NAME(bme680_properties, device_name);
            bme680_properties->ezlopi_cloud.category = category;
            bme680_properties->ezlopi_cloud.subcategory = sub_category;
            bme680_properties->ezlopi_cloud.item_name = item_name;
            bme680_properties->ezlopi_cloud.value_type = value_type;
            bme680_properties->ezlopi_cloud.device_type = dev_type_sensor;
            bme680_properties->ezlopi_cloud.reachable = true;
            bme680_properties->ezlopi_cloud.battery_powered = false;
            bme680_properties->ezlopi_cloud.show = true;
            bme680_properties->ezlopi_cloud.has_getter = true;
            bme680_properties->ezlopi_cloud.has_setter = false;
            bme680_properties->ezlopi_cloud.room_name[0] = '\0';
            bme680_properties->ezlopi_cloud.device_id = device_id;
            bme680_properties->ezlopi_cloud.room_id = ezlopi_cloud_generate_room_id();
            bme680_properties->ezlopi_cloud.item_id = ezlopi_cloud_generate_item_id();

            CJSON_GET_VALUE_INT(cjson_device, "gpio_scl", bme680_properties->interface.i2c_master.scl);
            CJSON_GET_VALUE_INT(cjson_device, "gpio_sda", bme680_properties->interface.i2c_master.sda);

            bme680_properties->interface.i2c_master.clock_speed = 100000;
            bme680_properties->interface.i2c_master.address = 0x77;
            bme680_properties->interface.i2c_master.enable = true;
            bme680_properties->interface.i2c_master.channel = BME680_I2C_CHANNEL;
            bme680_properties->user_arg = bme680_sensor_data;
        }
    }
    return bme680_properties;
}

static int add_device_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *properties, void *user_args)
{
    int ret = 0;

    if (properties)
    {
        if (0 == ezlopi_devices_list_add(prep_arg->device, properties, user_args))
        {
            free(properties);
        }
        else
        {
            ret = 1;
        }
    }
    return ret;
}

static int bme680_sensor_init(s_ezlopi_device_properties_t *properties)
{
    int ret = 0;

    if ((!bme680_sensor_i2c_initialized))
    {
        bme680_setup(true);
        bme680_sensor_i2c_initialized = true;
    }
    return ret;
}

static int bme680_sensor_update_values(s_ezlopi_device_properties_t *properties)
{
    int ret = 0;
    bme680_data_t *bme680_sensor_data = (bme680_data_t *)properties->user_arg;
    if (NULL != bme680_sensor_data)
    {
        bme680_get_data(bme680_sensor_data);
        #if 1
        TRACE_I("---------------------------------------");
        TRACE_I("Air Quality Index : %f", bme680_sensor_data->iaq);
        TRACE_I("AIQ accuracy : %s", bme680_sensor_data->iaq_accuracy ? "TRUE" : "FALSE");
        TRACE_I("co2_equivalent : %f", bme680_sensor_data->co2_equivalent);
        TRACE_I("voc_equivalent : %f", bme680_sensor_data->voc_equivalent);
        TRACE_I("Temperature : %f", bme680_sensor_data->temperature);
        TRACE_I("Pressure : %f", bme680_sensor_data->pressure);
        TRACE_I("Humidity : %f", bme680_sensor_data->humidity);
        TRACE_I("Gas resistance : %f", bme680_sensor_data->gas_resistance);
        TRACE_I("Altitude : %f", bme680_sensor_data->altitude);
        TRACE_I("Stabilization_status : %s", bme680_sensor_data->stabilization_status ? "TRUE" : "FALSE");
        TRACE_I("run in status : %s", bme680_sensor_data->run_in_status ? "TRUE" : "FALSE");
        TRACE_I("---------------------------------------");
        #endif
    }
    return ret;
}

static int bme680_sensor_get_value_cjson(s_ezlopi_device_properties_t *properties, void *args)
{
    int ret = 0;

    cJSON *cjson_properties = (cJSON *)args;
    char formatted_value[20];
    bme680_data_t *bme680_sensor_data = (bme680_data_t *)properties->user_arg;

    if (cjson_properties && bme680_sensor_data)
    {
        if (ezlopi_item_name_temp == properties->ezlopi_cloud.item_name)
        {
            snprintf(formatted_value, 20, "%.2f", bme680_sensor_data->temperature);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", bme680_sensor_data->temperature);
            cJSON_AddStringToObject(cjson_properties, "scale", "celsius");
        }
        if (ezlopi_item_name_humidity == properties->ezlopi_cloud.item_name)
        {
            snprintf(formatted_value, 20, "%.2f", bme680_sensor_data->humidity);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", bme680_sensor_data->humidity);
            cJSON_AddStringToObject(cjson_properties, "scale", "percent");
        }
        if (ezlopi_item_name_atmospheric_pressure == properties->ezlopi_cloud.item_name)
        {
            bme680_sensor_data->pressure = bme680_sensor_data->pressure / 1000.0;
            snprintf(formatted_value, 20, "%.2f", bme680_sensor_data->pressure);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", (bme680_sensor_data->pressure));
            cJSON_AddStringToObject(cjson_properties, "scale", "kilo_pascal");
        }
        if (ezlopi_item_name_volatile_organic_compound_level == properties->ezlopi_cloud.item_name)
        {
            snprintf(formatted_value, 20, "%.2f", bme680_sensor_data->iaq);
            TRACE_I("ezlopi_item_name_volatile_organic_compound_level: %s", formatted_value);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            // cJSON_AddStringToObject(cjson_properties, "scale", "parts_per_million");        
            cJSON_AddNumberToObject(cjson_properties, "value", bme680_sensor_data->iaq);
        }
        if (ezlopi_item_name_distance == properties->ezlopi_cloud.item_name)
        {
            snprintf(formatted_value, 20, "%.2f", bme680_sensor_data->altitude);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", bme680_sensor_data->altitude);
            cJSON_AddStringToObject(cjson_properties, "scale", "meter");     
        }     
        if (ezlopi_item_name_co2_level == properties->ezlopi_cloud.item_name)
        {
            snprintf(formatted_value, 20, "%.2f", bme680_sensor_data->co2_equivalent);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", bme680_sensor_data->co2_equivalent);
            // cJSON_AddStringToObject(cjson_properties, "scale", "");    
        }             
    }
    return ret;
}


