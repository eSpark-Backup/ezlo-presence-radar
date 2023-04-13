#pragma once

/* Ambient Tracker Pro : W5500 Pins & Configurations */
#define W5500_EN           GPIO_NUM_13
#define W5500_CS           GPIO_NUM_5
#define W5500_MOSI         GPIO_NUM_2
#define W5500_MISO         GPIO_NUM_1
#define W5500_SCLK         GPIO_NUM_4
#define W5500_INTR         GPIO_NUM_12
#define W5500_RST      GPIO_NUM_11
#define W5500_PHY_ADDR     -1
#define W5500_HOST         (spi_host_device_t)2
#define W5500_CLOCK_MHZ    12
#define W5500_CMD_BITS     16        
#define W5500_ADDR_BITS    8