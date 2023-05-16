idf_component_register(SRCS  "RangeDopplerMap/range_doppler_map.c"
                            "RangeAngleImage/range_angle_image.c"
                            "AngleCapon/angle_capon.c"
                            "Presence/presence.c"
                            "AngleMonopulse/angle_monopulse.c"
                            "radar_app.c"
                        INCLUDE_DIRS "RangeDopplerMap" "RangeAngleImage" "AngleCapon" "AngleMonopulse" "."
                        REQUIRES ifxAvian ifxRadar)