idf_component_register(SRCS "ELECHOUSE_CC1101_SRC_DRV.c"
                    REQUIRES driver rc-switch json RF_Blaster ezlopi_hal
                    INCLUDE_DIRS "."
)