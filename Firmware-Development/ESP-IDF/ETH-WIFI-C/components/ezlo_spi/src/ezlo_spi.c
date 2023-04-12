#include "ezlo_spi.h"

static const char* TAG = "ezlo_spi";

/*!
    @brief  Initializes SPI Master bus.
    @param  
    @return ESP_OK on success & esp_err_t if any error occurs.
 */
esp_err_t spi_init(spi_t* spi_obj, const spi_host_device_t spi_peripheral, const int pin_miso, const int pin_mosi, const int pin_sclk)
{
    esp_err_t status = ESP_OK;
    spi_obj->host = spi_peripheral;
    spi_obj->transaction.tx_buffer = NULL;
    spi_obj->transaction.rx_buffer = NULL;
    spi_obj->bus_cfg.mosi_io_num = pin_mosi;
    spi_obj->bus_cfg.miso_io_num = pin_miso;
    spi_obj->bus_cfg.sclk_io_num = pin_sclk;
    spi_obj->bus_cfg.quadwp_io_num = -1;
    spi_obj->bus_cfg.quadhd_io_num = -1;
    status |= spi_bus_initialize(spi_obj->host, &(spi_obj->bus_cfg), SPI_DMA_CH_AUTO);
    return status;
}
/*!
    @brief  Register a device on the SPI bus.
    @param  
    @return ESP_OK on success & esp_err_t if any error occurs.
 */
esp_err_t spi_register_device(spi_t* spi_obj ,const uint8_t mode, const int ss, const int addr_length, const int command_length, const int clock_speed_devider)
{
    esp_err_t status = ESP_OK;
    spi_obj->interface_cfg.command_bits = command_length; 
    spi_obj->interface_cfg.address_bits = addr_length;
    spi_obj->interface_cfg.mode = mode;
    spi_obj->interface_cfg.clock_speed_hz = clock_speed_devider; // 80MHz/10 = 8MHz SPI speed
    spi_obj->interface_cfg.spics_io_num = ss; // IO pin 5 is my chip select
    spi_obj->interface_cfg.queue_size = 20;
    status |= spi_bus_add_device(spi_obj->host, &(spi_obj->interface_cfg), &(spi_obj->handle));
    return status;
}