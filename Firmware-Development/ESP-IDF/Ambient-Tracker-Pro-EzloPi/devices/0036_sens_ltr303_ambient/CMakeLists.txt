file(GLOB_RECURSE config_src "*.c")
# set(config_src)

idf_component_register(SRCS "0036_sens_ltr303_ambient.c" "${config_src}"
    INCLUDE_DIRS "."
    REQUIRES items trace library ezlopi_actions ezlopi_hal ezlopi_timer ezlopi_devices ezlopi_cloud_constants gpio_isr_service ezlopi_cloud)