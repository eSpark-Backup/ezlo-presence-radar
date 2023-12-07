

idf_component_register(SRCS "radar_app.c" "0038_sensor_presence_sensor.c" "presence_sensor_dev_item_preps.c"
                        INCLUDE_DIRS .
                        REQUIRES trace library ezlopi_actions ezlopi_hal ezlopi_timer ezlopi_devices ezlopi_cloud_constants gpio_isr_service radar_app ifxRadar ifxAvian)


