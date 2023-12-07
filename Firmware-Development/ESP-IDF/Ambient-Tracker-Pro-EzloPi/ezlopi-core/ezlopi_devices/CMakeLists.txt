# set(config_src "config.cpp")

file(GLOB_RECURSE config_src "*.c")

idf_component_register(SRCS "ezlopi_devices.c" "ezlopi_device_value_updated.c" "ezlopi_devices_list.c" "${config_src}"
    INCLUDE_DIRS "."
    REQUIRES trace json ezlopi_nvs ezlopi_hal ezlopi_cloud ezlopi_actions 0008_sensor_I2C_LTR303ALS 0009_other_RMT_SK6812 0010_I2C_BME680 0037_sens_pms5003_sensor 0038_sensor_presence_sensor)


