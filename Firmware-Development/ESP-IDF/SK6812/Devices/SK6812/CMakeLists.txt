set(SOURCES src/SK6812_LedStrip.c)

idf_component_register( SRCS ${SOURCES}
                        INCLUDE_DIRS "include")
                        
