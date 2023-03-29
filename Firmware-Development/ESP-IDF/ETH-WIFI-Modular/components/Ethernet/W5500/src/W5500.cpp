#include "W5500.hpp"
/* 
    initBus : Initialize SPI bus
*/ 
void W5500::busInit()
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = spiConfig.mosi,
        .miso_io_num = spiConfig.miso,
        .sclk_io_num = spiConfig.sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)spiConfig.spiHost, &buscfg, SPI_DMA_CH_AUTO));
}

/* 
    addDevice : Allocate a Ethernet device on a SPI bus.
    Initializes the internal structures for a Ethernet device, plus allocates a CS pin on the indicated SPI master peripheral
*/

void W5500::addDevice()
{
    spi_device_interface_config_t devcfg = {
            .command_bits = 16, // Actually it's the address phase in W5500 SPI frame
            .address_bits = 8,  // Actually it's the control phase in W5500 SPI frame
            .mode = 0,
            .clock_speed_hz = W5500_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
            .spics_io_num = W5500_ETH_SPI_CS,
            .queue_size = 20
        };

    ESP_ERROR_CHECK(spi_bus_add_device((spi_host_device_t)W5500_ETH_SPI_HOST, &devcfg, (spi_device_handle_t *)&handle));
}

void W5500::spiBegin()
{
    busInit();
    addDevice();
}
void W5500::setup(){}