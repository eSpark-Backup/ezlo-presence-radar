# set(config_src "config.cpp")

file(GLOB_RECURSE config_src "*.c")

idf_component_register(SRCS "ezlopi_devices.c" "ezlopi_device_value_updated.c" "ezlopi_devices_list.c" "${config_src}"
    INCLUDE_DIRS "."
    REQUIRES trace json ezlopi_nvs ezlopi_hal ezlopi_cloud
    digital_io dimmable_bulb 0034_sens_bme680_sensor 0035_SK6812 0036_sens_ltr303_ambient 0037_sens_pms5003_sensor)


