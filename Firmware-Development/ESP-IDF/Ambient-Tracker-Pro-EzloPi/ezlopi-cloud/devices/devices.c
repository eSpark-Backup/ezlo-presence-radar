#include <string.h>

#include "devices.h"
#include "trace.h"
#include "ezlopi_factory_info.h"
#include "ezlopi_devices.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_cloud_keywords.h"
#include "ezlopi_cloud_methods_str.h"
#include "cJSON.h"

int __added_already(uint32_t device_id, cJSON *properties_array)
{
    int ret = 0;
    int device_array_size = cJSON_GetArraySize(properties_array);
    char* device_array_id = NULL;
    if(device_array_size > 0)
    {
        for(int i = 0; i < device_array_size; i++)
        {
            cJSON* device_array_item = cJSON_GetArrayItem(properties_array, i);
            CJSON_GET_VALUE_STRING(device_array_item, "_id", device_array_id);
            uint32_t obtained_id = (uint32_t)strtol(device_array_id, NULL, 16);
            if(obtained_id == device_id)
            {
                ret = 1;
            }
        }
    }
    return ret;
}

void devices_list(cJSON *cj_request, cJSON *cj_response)
{
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_id_str, cJSON_GetObjectItem(cj_request, ezlopi_id_str));
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_key_method_str, cJSON_GetObjectItem(cj_request, ezlopi_key_method_str));
    cJSON *cjson_result = cJSON_AddObjectToObject(cj_response, ezlopi_result);
    if (cjson_result)
    {
        cJSON *cjson_devices_array = cJSON_AddArrayToObject(cjson_result, "devices");
        if (cjson_devices_array)
        {
            l_ezlopi_configured_devices_t *registered_devices = ezlopi_devices_list_get_configured_items();

            while (NULL != registered_devices)
            {
                if (NULL != registered_devices->properties)
                {
                    if (!__added_already(registered_devices->properties->ezlopi_cloud.device_id, cjson_devices_array))
                    {
                        cJSON *cj_properties = cJSON_CreateObject();
                        if (cj_properties)
                        {
                            char tmp_string[64];
                            snprintf(tmp_string, sizeof(tmp_string), "%08x", registered_devices->properties->ezlopi_cloud.device_id);
                            cJSON_AddStringToObject(cj_properties, "_id", tmp_string);
                            cJSON_AddStringToObject(cj_properties, "deviceTypeId", "ezlopi");
                            cJSON_AddStringToObject(cj_properties, "parentDeviceId", "");
                            cJSON_AddStringToObject(cj_properties, "category", registered_devices->properties->ezlopi_cloud.category);
                            cJSON_AddStringToObject(cj_properties, "subcategory", registered_devices->properties->ezlopi_cloud.subcategory);
                            cJSON_AddStringToObject(cj_properties, "gatewayId", "3124201377");
                            cJSON_AddBoolToObject(cj_properties, "batteryPowered", registered_devices->properties->ezlopi_cloud.battery_powered);
                            cJSON_AddStringToObject(cj_properties, "name", registered_devices->properties->ezlopi_cloud.device_name);
                            cJSON_AddStringToObject(cj_properties, "type", registered_devices->properties->ezlopi_cloud.device_type);
                            cJSON_AddBoolToObject(cj_properties, "reachable", registered_devices->properties->ezlopi_cloud.reachable);
                            cJSON_AddBoolToObject(cj_properties, "persistent", true);
                            cJSON_AddBoolToObject(cj_properties, "serviceNotification", false);
                            cJSON_AddBoolToObject(cj_properties, "armed", false);
                            cJSON_AddStringToObject(cj_properties, "roomId", "");
                            cJSON_AddStringToObject(cj_properties, "security", "");
                            cJSON_AddBoolToObject(cj_properties, "ready", true);
                            cJSON_AddStringToObject(cj_properties, "status", "idle");
                            cJSON_AddObjectToObject(cj_properties, "info");

                            if (!cJSON_AddItemToArray(cjson_devices_array, cj_properties))
                            {
                                cJSON_Delete(cj_properties);
                            }
                        }
                    }
                }

                registered_devices = registered_devices->next;
            }
        }
        else
        {
            TRACE_E("Failed to create devices-array");
        }
    }
    else
    {
        TRACE_E("Failed to create 'result'");
    }
}

#if 0
char *devices_name_set(const char *payload, uint32_t len, struct json_token *method, uint32_t msg_count)
{
    uint32_t buf_len = 300;
    char *send_buf = (char *)malloc(buf_len);

    if (send_buf)
    {
        struct json_token msg_id = JSON_INVALID_TOKEN;
        struct json_token sender = JSON_INVALID_TOKEN;
        memset(send_buf, 0, buf_len);

        json_scanf(payload, len, "{id: %T}", &msg_id);
        json_scanf(payload, len, "{sender: %T}", &sender);

        snprintf(send_buf, buf_len, "{\"method\": \"%.*s\",\"msg_id\":%d,\"error\": null,\"id\": \"%.*s\",\"result\": {},\"sender\":%.*s}",
                 method->len, method->ptr,
                 msg_count,
                 msg_id.len, msg_id.ptr,
                 sender.len ? sender.len : 2, sender.len ? sender.ptr : "{}");

        TRACE_B(">> WS Tx - '%.*s' [%d]\r\n%s", method->len, method->ptr, strlen(send_buf), send_buf);
    }

    return send_buf;
}
#endif
