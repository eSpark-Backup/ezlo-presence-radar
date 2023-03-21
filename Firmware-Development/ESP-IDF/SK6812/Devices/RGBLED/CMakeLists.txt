set(SOURCES SK6812/src/SK6812.cpp)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "SK6812/include" "include"
		    			REQUIRES led_strip 
                        PRIV_REQUIRES lib8tion color esp_idf_lib_helpers)