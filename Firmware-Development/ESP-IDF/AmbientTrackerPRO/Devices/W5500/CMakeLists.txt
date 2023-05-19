set(SOURCES src/w5500.c)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include"
		    			REQUIRES nvs_flash esp_eth ezlopi_hal)
