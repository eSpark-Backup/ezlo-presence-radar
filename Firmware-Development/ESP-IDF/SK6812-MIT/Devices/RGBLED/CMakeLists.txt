set(SOURCES src/RGBLED.cpp)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include"
		    			REQUIRES led_strip 
                        PRIV_REQUIRES lib8tion color esp_idf_lib_helpers)