set(SOURCES src/ezlo_spi.c)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include" )
