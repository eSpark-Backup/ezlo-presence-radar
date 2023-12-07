
#ifndef __0038_SENSOR_PRESENCE_SENSOR_H__
#define __0038_SENSOR_PRESENCE_SENSOR_H__

#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

#include "presence_radar_default.h"
#include "RangeAngleImage/range_angle_image.h"
#include "RangeDopplerMap/range_doppler_map.h"
#include "AngleCapon/angle_capon.h"
#include "Presence/presence.h"
#include "motion_angle.h"

#include "xensiv_bgt60trxx_esp32.h"
#include "xensiv_bgt60trxx.h"

#include "radar_app.h"

int presence_sensor_dev_item_prep(s_ezlopi_prep_arg_t *prep_args, s_device_args_t *device_args);
int presence_sensor_v3(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif // __0038_SENSOR_PRESENCE_SENSOR_H__
