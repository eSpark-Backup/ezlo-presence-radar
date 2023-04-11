set(SOURCES src/ezlo_netif.c)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include"
		    			REQUIRES nvs_flash )
