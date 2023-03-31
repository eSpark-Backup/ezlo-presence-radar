set(SOURCES src/spi.cpp)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "include" )
