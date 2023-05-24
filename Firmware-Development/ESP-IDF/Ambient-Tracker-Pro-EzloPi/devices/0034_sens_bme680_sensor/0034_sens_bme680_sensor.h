


#ifndef __0034_SENS_BME680_SENSOR_H_
#define __0034_SENS_BME680_SENSOR_H_


#include "ezlopi_actions.h"
#include "ezlopi_devices.h"


#define BME680_I2C_CHANNEL I2C_NUM_0



int bme680_sensor(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *properties, void *arg, void *user_arg);


#endif // __0034_SENS_BME680_SENSOR_H_

