#include "ezlo_spi.h"

/*!
    @brief  Initializes SPI Master bus.
    @param  (host) : (VSPI/HSPI) , (mosi,miso,sclk) : gpio numbers
    @return ESP_OK on success & esp_err_t if any error occurs.
 */
esp_err_t spi_bus_init(spi_host_device_t host, gpio_num_t mosi, gpio_num_t miso, gpio_num_t sclk)
{    
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    esp_err_t ret = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if( ret != ESP_OK)
        return ret;
    
    return ESP_OK;
}