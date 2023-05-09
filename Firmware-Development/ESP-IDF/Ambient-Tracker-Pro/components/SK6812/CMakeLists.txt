set(SOURCES src/SK6812.c)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include"
		    			REQUIRES led_strip )