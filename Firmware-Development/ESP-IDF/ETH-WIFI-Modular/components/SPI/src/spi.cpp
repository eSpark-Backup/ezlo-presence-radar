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