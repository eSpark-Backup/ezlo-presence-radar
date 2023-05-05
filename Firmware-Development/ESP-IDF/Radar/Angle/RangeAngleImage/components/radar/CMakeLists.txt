idf_component_register(SRCS  "RangeDopplerMap/range_doppler_map.c"
                            "RangeAngleImage/range_angle_image.c"
                            "AngleCapon/angle_capon.c"
                            "radar.c"
                        INCLUDE_DIRS "RangeDopplerMap" "RangeAngleImage" "AngleCapon" "."
                        REQUIRES ifxAvian ifxRadarSegmentation ifxRadar)