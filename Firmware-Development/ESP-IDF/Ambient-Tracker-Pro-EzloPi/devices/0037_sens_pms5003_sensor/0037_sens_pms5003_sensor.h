

#ifndef _SENSOR_PMS5003_UART_H_
#define _SENSOR_PMS5003_UART_H_

#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

int pms5003_sensor(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *ezlo_device, void *arg, void *user_arg);
int pms5003_sensor_v3(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);
int pms5003_sensor_preapre_devices_and_items(uint32_t parent_id);

#endif // _SENSOR_PMS5003_UART_H_


