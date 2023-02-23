
#include "xensiv_bgt60trxx.h"
#include "xensiv_bgt60trxx_esp32.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void xensiv_bgt60trxx_esp32_rst_set(const void* iface, uint8_t val) {
    gpio_set_level(XNSIV_BGT60_RST_GPIO, (uint32_t)val);
}

void xensiv_bgt60trxx_esp32_spi_cs_set(const void* iface, uint8_t val) {
    #ifdef XNSIV_CS_HANDLING
    gpio_set_level(XNSIV_BGT60_CS_GPIO, (uint32_t)val);
    #endif
}

int32_t xensiv_bgt60trxx_esp32_spi_transfer(void* iface,
                                               uint8_t* tx_data,
                                               uint8_t* rx_data,
                                               uint32_t len) {
    spi_transaction_t transaction; 

    transaction.flags = 0;
    transaction.length = 8 * len;
    transaction.rxlength = 0;
    transaction.user = NULL;
    transaction.tx_buffer = tx_data;
    transaction.rx_buffer = rx_data;    
    
    esp_err_t err = spi_device_polling_transmit(*(spi_device_handle_t *)iface, &transaction);

    if (ESP_OK == err)
        return XENSIV_BGT60TRXX_STATUS_OK;
    else 
        return XENSIV_BGT60TRXX_STATUS_DEV_ERROR;

    return err;
}
uint16_t data ;
int32_t xensiv_bgt60trxx_esp32_spi_fifo_read(void* iface,
                                                uint16_t* rx_data,
                                                uint32_t len) {
    int32_t err = XENSIV_BGT60TRXX_STATUS_OK;

    xensiv_bgt60trxx_esp32_assert(iface != NULL);
    xensiv_bgt60trxx_esp32_assert(rx_data != NULL);

    
    
    spi_transaction_t transaction; 

    transaction.flags = 0;
    transaction.length = 12;
    transaction.rxlength = 12;
    transaction.user = NULL;
    transaction.tx_buffer = &data;
    transaction.rx_buffer = &data;
    
    
    for(uint32_t i = 0; i < len; i++) {        
        esp_err_t err = spi_device_polling_transmit(*(spi_device_handle_t *)iface, &transaction);        
        rx_data[i] =  ((data << 4) | (data >> 12 )) & 0x0FFF;
    }
    if (ESP_OK == err)
        return XENSIV_BGT60TRXX_STATUS_OK;
    else 
        return XENSIV_BGT60TRXX_STATUS_DEV_ERROR;
}

uint32_t xensiv_bgt60trxx_esp32_word_reverse(uint32_t x) {
    return (((x & 0x000000ffUL) << 24) |
        ((x & 0x0000ff00UL) <<  8) |
        ((x & 0x00ff0000UL) >>  8) |
        ((x & 0xff000000UL) >> 24));
}

void xensiv_bgt60trxx_esp32_assert(bool expr) {
    
    esp_err_t err_expr;

    if(false == expr) {
        err_expr = ESP_FAIL;
    } else {
        err_expr = ESP_OK;
    }
    
    ESP_ERROR_CHECK(err_expr);

}

void xensiv_bgt60trxx_esp32_delay(uint32_t ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
}
