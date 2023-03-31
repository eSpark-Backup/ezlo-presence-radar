set(SOURCES src/Ethernet.cpp W5500/src/W5500.cpp)

idf_component_register(	SRCS ${SOURCES}
                    	INCLUDE_DIRS "W5500/include" "include" 
						REQUIRES Netif SPI)