idf_component_register(SRCS  "RangeDopplerMap/range_doppler_map.c"
                            "RangeAngleImage/range_angle_image.c"
                            "AngleCapon/angle_capon.c"
                            "Presence/presence.c"
                            "radar_app.c"
                        INCLUDE_DIRS "RangeDopplerMap" "RangeAngleImage" "AngleCapon" "."
                        REQUIRES ifxAvian ifxRadar)