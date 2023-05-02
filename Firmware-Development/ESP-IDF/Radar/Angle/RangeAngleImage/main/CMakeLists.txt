idf_component_register(SRCS "radar_app.c" 
                            "Radar/RangeDopplerMap/range_doppler_map.c"
                            "Radar/RangeAngleImage/range_angle_image.c"
                            "Radar/AngleCapon/angle_capon.c"
                            "Radar/radar.c"
                        REQUIRES ifxAvian ifxRadarSegmentation ifxRadar)