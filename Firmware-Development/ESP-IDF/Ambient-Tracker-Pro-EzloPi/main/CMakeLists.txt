file(GLOB_RECURSE SOURCES "main.c")

idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES trace driver ezlopi trace web_provisioning ezlopi_ble timer_service gpio_isr_service ezlopi_ble_service mac_uuid radar_app)

