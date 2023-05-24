


#ifndef __0036_SENS_LTR303_AMBIENT_H_
#define __0036_SENS_LTR303_AMBIENT_H_


#include "ezlopi_actions.h"
#include "ezlopi_devices.h"


#define LTR303_I2C_CHANNEL I2C_NUM_0


int ltr303_ambient_sensor(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *properties, void *arg, void *user_arg);


#endif // __0036_SENS_LTR303_AMBIENT_H_

