set(SOURCES ezlopi_spi_master.c)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "." )
