set(SOURCES src/netif.cpp)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include"
		    REQUIRES nvs_flash )
