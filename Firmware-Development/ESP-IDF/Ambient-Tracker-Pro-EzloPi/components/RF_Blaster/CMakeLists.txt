idf_component_register(SRCS "RF_blaster.c" 
REQUIRES driver trace SmartRC-CC1101-Driver-Lib ezlopi_hal
                    INCLUDE_DIRS "."
)