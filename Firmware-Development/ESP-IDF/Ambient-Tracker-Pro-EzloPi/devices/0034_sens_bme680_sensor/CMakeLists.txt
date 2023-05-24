file(GLOB_RECURSE config_src "*.c")
# set(config_src)

idf_component_register(SRCS "bme680_bsec.c" "0034_sens_bme680_sensor.c" "${config_src}"
    INCLUDE_DIRS "."
    REQUIRES trace library ezlopi_actions ezlopi_hal ezlopi_timer ezlopi_devices ezlopi_cloud_constants gpio_isr_service ezlopi_cloud
    BME68x_Sensor_library Bosch_BSEC2)