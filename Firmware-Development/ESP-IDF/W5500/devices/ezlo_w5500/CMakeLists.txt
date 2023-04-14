set(SOURCES src/ezlo_w5500.c)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include"
		    			REQUIRES nvs_flash esp_eth ezlo_netif ezlopi_hal)
