

#include "cJSON.h"
#include "trace.h"
#include "ezlopi_actions.h"
#include "ezlopi_timer.h"
#include "items.h"

#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "stdlib.h"

#include "0037_sens_pms5003_sensor.h"
#include "pms5003.h"

#define ADD_PROPERTIES_DEVICE_LIST(device_id, category, sub_category, item_name, value_type, cjson_device, pms_data)                  \
    {                                                                                                                                 \
        s_ezlopi_device_properties_t *_properties = pms5003_sensor_prepare_properties(device_id, category, sub_category,              \
                                                                                      item_name, value_type, cjson_device, pms_data); \
        if (NULL != _properties)                                                                                                      \
        {                                                                                                                             \
            add_device_to_list(prep_arg, _properties, NULL);                                                                          \
        }                                                                                                                             \
    }

static int counter  = 0;
static bool pms5003_initialized = false;

static int pms5003_sensor_prepare(void *arg);
static s_ezlopi_device_properties_t *pms5003_sensor_prepare_properties(uint32_t device_id, const char *category, const char *subcategory, const char *item_name, const char *value_type, cJSON *cjson_device, PM25_AQI_Data *pms5003_data);
static int add_device_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *properties, void *user_arg);
static int pms5003_sensor_init(s_ezlopi_device_properties_t *properties);
static int pms5003_sensor_get_value_cjson(s_ezlopi_device_properties_t *properties, void *args);
static int pms5003_sensor_check_new_data_available_and_update(s_ezlopi_device_properties_t* properties);

int pms5003_sensor(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *properties, void *arg, void *user_arg)
{
    int ret = 0;

    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        pms5003_sensor_prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        pms5003_sensor_init(properties);
        break;
    }
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        pms5003_sensor_get_value_cjson(properties, arg);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    {
        pms5003_sensor_check_new_data_available_and_update(properties);
        break;
    }
    default:
    {
        break;
    }
    }

    return ret;
}

static int pms5003_sensor_prepare(void *arg)
{
    int ret = 0;

    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    PM25_AQI_Data *pms5003_data = (PM25_AQI_Data *)malloc(sizeof(PM25_AQI_Data));
    if ((NULL != prep_arg) && (NULL != pms5003_data))
    {
        memset(pms5003_data, 0, sizeof(PM25_AQI_Data));
        uint32_t device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_generic_sensor, subcategory_not_defined, ezlopi_item_name_particulate_matter_1, value_type_substance_amount, prep_arg->cjson_device, pms5003_data);
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_generic_sensor, subcategory_not_defined, ezlopi_item_name_particulate_matter_2_dot_5, value_type_substance_amount, prep_arg->cjson_device, pms5003_data);
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_generic_sensor, subcategory_not_defined, ezlopi_item_name_particulate_matter_10, value_type_substance_amount, prep_arg->cjson_device, pms5003_data);
    }   
    return ret;
}

static s_ezlopi_device_properties_t *pms5003_sensor_prepare_properties(uint32_t device_id, const char *category, const char *subcategory, const char *item_name, const char *value_type, cJSON *cjson_device, PM25_AQI_Data *pms5003_data)
{
    s_ezlopi_device_properties_t *pms5003_properties = (s_ezlopi_device_properties_t *)malloc(sizeof(s_ezlopi_device_properties_t));
    if (pms5003_properties)
    {
        memset(pms5003_properties, 0, sizeof(s_ezlopi_device_properties_t));
        pms5003_properties->interface_type = EZLOPI_DEVICE_INTERFACE_UART;

        
        char *device_name = NULL;

        if (ezlopi_item_name_particulate_matter_1 == item_name)
        {
                device_name = "PM 1 ";
        } else if (ezlopi_item_name_particulate_matter_2_dot_5 == item_name)
        {
                device_name = "PM 2.5 ";
        } else if (ezlopi_item_name_particulate_matter_10 == item_name)
        {
                device_name = "PM 10 ";
        } else {
            device_name = "Dust Particles";
        }

        // CJSON_GET_VALUE_STRING(cjson_device, "dev_name", device_name);

        ASSIGN_DEVICE_NAME(pms5003_properties, device_name);
        pms5003_properties->ezlopi_cloud.category = category;
        pms5003_properties->ezlopi_cloud.subcategory = subcategory;
        pms5003_properties->ezlopi_cloud.item_name = item_name;
        pms5003_properties->ezlopi_cloud.device_type = dev_type_sensor;
        pms5003_properties->ezlopi_cloud.value_type = value_type;
        pms5003_properties->ezlopi_cloud.has_getter = true;
        pms5003_properties->ezlopi_cloud.has_setter = false;
        pms5003_properties->ezlopi_cloud.reachable = true;
        pms5003_properties->ezlopi_cloud.battery_powered = false;
        pms5003_properties->ezlopi_cloud.show = true;
        pms5003_properties->ezlopi_cloud.room_name[0] = '\0';
        pms5003_properties->ezlopi_cloud.device_id = device_id;
        pms5003_properties->ezlopi_cloud.room_id = ezlopi_cloud_generate_room_id();
        pms5003_properties->ezlopi_cloud.item_id = ezlopi_cloud_generate_item_id();

        CJSON_GET_VALUE_INT(cjson_device, "baud_rate", pms5003_properties->interface.uart.baudrate);
        CJSON_GET_VALUE_INT(cjson_device, "gpio_tx", pms5003_properties->interface.uart.tx);
        CJSON_GET_VALUE_INT(cjson_device, "gpio_rx", pms5003_properties->interface.uart.rx);

        pms5003_properties->user_arg = pms5003_data;
    }
    return pms5003_properties;
}

static int add_device_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *properties, void *user_arg)
{
    int ret = 0;

    if (properties)
    {
        if (0 == ezlopi_devices_list_add(prep_arg->device, properties, user_arg))
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


static int pms5003_sensor_init(s_ezlopi_device_properties_t *properties)
{
    int ret = 0;

    PM25_AQI_Data *pms5003_data = (PM25_AQI_Data *)properties->user_arg;

    if((NULL != pms5003_data) && (false == pms5003_initialized))
    {
        pms_init(pms5003_data);
        pms5003_initialized = true;
    }

    return ret;
}

static int pms5003_sensor_get_value_cjson(s_ezlopi_device_properties_t *properties, void *args)
{
    int ret = 0;

    cJSON* cjson_properties = (cJSON*)args;
    PM25_AQI_Data* pms5003_data  =(PM25_AQI_Data*)properties->user_arg;
    char formatted_value[20];

    if(cjson_properties && pms5003_data)
    {
        if(ezlopi_item_name_particulate_matter_1 == properties->ezlopi_cloud.item_name)
        {   
            // pms5003_data->pm10_standard = 0.3205;
            snprintf(formatted_value, 20, "%.2f", pms5003_data->pm10_standard);
            TRACE_I("Dust particle 1 : %s", formatted_value);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", pms5003_data->pm10_standard);
            cJSON_AddStringToObject(cjson_properties, "scale", "micro_gram_per_cubic_meter");
        }
        if(ezlopi_item_name_particulate_matter_2_dot_5 == properties->ezlopi_cloud.item_name)
        {   
            // pms5003_data->pm25_standard = 1.5360; // Dummy value
            snprintf(formatted_value, 20, "%.2f", pms5003_data->pm25_standard);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", pms5003_data->pm25_standard);
            cJSON_AddStringToObject(cjson_properties, "scale", "micro_gram_per_cubic_meter");
        }
        if(ezlopi_item_name_particulate_matter_10 == properties->ezlopi_cloud.item_name)
        {
            // pms5003_data->pm25_standard = 7.5360;  Dummy value 
            snprintf(formatted_value, 20, "%.2f", pms5003_data->pm100_standard);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", pms5003_data->pm100_standard);
            cJSON_AddStringToObject(cjson_properties, "scale", "micro_gram_per_cubic_meter");
        }
    }

    return ret;
}


static int pms5003_sensor_check_new_data_available_and_update(s_ezlopi_device_properties_t* properties)
{
    int ret = 0;

    if((true == pms_is_data_available(properties->user_arg)) && (counter == 0))
    {
        pms_set_data_available_to_false(properties->user_arg);
        counter = 1;
    }
    if((counter != 0) && (counter <= 3))
    {
        PM25_AQI_Data *pms5003_data = (PM25_AQI_Data *)properties->user_arg;
        pms_print_data(pms5003_data);
        ezlopi_device_value_updated_from_device(properties);
        counter++;
    }
    else if(counter > 3)
    {
            counter = 0;
    }
    return ret;
}
