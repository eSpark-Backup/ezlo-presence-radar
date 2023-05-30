#include "ld2410_uart.h"

static const char* TAG = "ld2410 uart";

esp_err_t ld2410_uart_setup(void)
{
    esp_err_t ret = ESP_OK;
    // Configure the UART parameters
    uart_config_t uart_config = {
        .baud_rate = 256000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    ret = uart_param_config(UART_PORT_NUM, &uart_config);
    ESP_RETURN_ON_ERROR(ret, TAG, "uart param config - parameter error");

    // Set UART pins
    ret = uart_set_pin(UART_PORT_NUM, RADAR_TX_PIN, RADAR_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ESP_RETURN_ON_ERROR(ret, TAG, "uart set pin - parameter error");

    // Install UART driver using an event queue here
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    ret = uart_driver_install(UART_PORT_NUM, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0);
    ESP_RETURN_ON_ERROR(ret, TAG, "uart driver not initialized");

    return ret;
}
int ld2410_uart_available(void)
{  
    size_t available;
    uart_get_buffered_data_len(UART_PORT_NUM, &available); // returns true always if there is no parameter error
    return available;
}

size_t ld2410_uart_write_byte(uint8_t byte)
{
    uart_write_bytes(UART_PORT_NUM, &byte, 1); // returns true always if there is no parameter error
    return 1;
}

int ld2410_uart_read(void)  
{
    uint8_t data = 0;
    if (uart_read_bytes(UART_PORT_NUM, &data, 1, pdMS_TO_TICKS(0)) == 1) 
    {
        return data;
    } 
    else
    {
        return -1;
    }
}