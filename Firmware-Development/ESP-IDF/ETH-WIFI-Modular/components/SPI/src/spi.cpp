#include "spi.hpp"

void SPI::setPins(gpio_num_t cs, gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t intr, gpio_num_t rst)
{
    this->cs = cs;
    this-> miso = miso;
    this->mosi = mosi;
    this->sclk = sclk;
    this->intr = intr;
    this->rst = rst;
}

void SPI::setSpeed(uint8_t clkSpeed)
{
    this->clkSpeed = clkSpeed;
}
void SPI::setHost(spi_host_device_t host)
{
    this->host = host;
}

void SPI::initBus()
{
    /* Initialize SPI bus */
    spi_bus_config_t buscfg = {
        .mosi_io_num = this->mosi,
        .miso_io_num = this->miso,
        .sclk_io_num = this->sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(this->host, &buscfg, SPI_DMA_CH_AUTO));
}

/*
    Sets configuration for a SPI slave device that is connected to one of the SPI buses.
*/

void SPI::setDeviceConfig(uint8_t commandBits, uint8_t addressBits)
{ 
    this->devcfg = (spi_device_interface_config_t){
        .command_bits = commandBits, 
        .address_bits = addressBits,  
        .mode = 0,
        .clock_speed_hz = this->clkSpeed * 1000 * 1000,
        .queue_size = 20
    };
}

/*
    Allocate a device on a SPI bus.
*/
void SPI::busAddDevice()
{
    // Set SPI module Chip Select GPIO
    this->devcfg.spics_io_num = this->cs;   
    ESP_ERROR_CHECK(spi_bus_add_device(this->host, &devcfg, &spi_handle));   
}

