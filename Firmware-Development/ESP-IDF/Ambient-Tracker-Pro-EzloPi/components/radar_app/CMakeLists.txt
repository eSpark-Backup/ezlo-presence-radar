idf_component_register(SRCS "radar_app.c"
                            "RangeDopplerMap/range_doppler_map.c"
                            "RangeAngleImage/range_angle_image.c"
                            "AngleCapon/angle_capon.c"
                            "Presence/presence.c"
                            "MotionAngle/motion_angle.c"
                        INCLUDE_DIRS "RangeDopplerMap" "RangeAngleImage" "AngleCapon" "MotionAngle" "Presence" "."
                        REQUIRES ifxAvian ifxRadar gpio_isr_service esp_system )


set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 3)