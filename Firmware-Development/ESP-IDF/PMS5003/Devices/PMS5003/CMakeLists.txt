set(SOURCES src/pms5003.c)

idf_component_register( SRCS ${SOURCES}
                        INCLUDE_DIRS "include" 
                        REQUIRES driver )