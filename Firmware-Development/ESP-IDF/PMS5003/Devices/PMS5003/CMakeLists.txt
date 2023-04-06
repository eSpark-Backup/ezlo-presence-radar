set(SOURCES src/pms5003.cpp)

idf_component_register( SRCS ${SOURCES}
                        INCLUDE_DIRS "include" 
                        REQUIRES driver)