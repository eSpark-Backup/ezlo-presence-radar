#include "Stream.hpp"

esp_err_t Stream::setup(void)
{
    // Configure the UART parameters
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_PORT_NUM, &uart_config);

    // Set UART pins
    uart_set_pin(UART_PORT_NUM, RADAR_TX_PIN, RADAR_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Install UART driver using an event queue here
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    uart_driver_install(UART_PORT_NUM, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0);

    return ESP_OK;
}
int Stream::available(void)
{  
    size_t available;
    uart_get_buffered_data_len(UART_PORT_NUM, &available);
    return available;
}

size_t Stream::write(uint8_t byte)
{
    uart_write_bytes(UART_PORT_NUM, &byte, 1);
    return 1;
}

size_t Stream::write(const uint8_t *buffer, size_t size)
{
    uart_write_bytes(UART_PORT_NUM, buffer, size);
    return size;
}

int Stream::read(void)  
{
    uint8_t data = 0;
    if (uart_read_bytes(UART_PORT_NUM, &data, 1, pdMS_TO_TICKS(0))) 
    {
        return data;
    } 
    else
    {
        return -1;
    }
}

size_t Stream::print(const char* buf)
{
    return 1;
}

size_t Stream::print(const char buf)
{
    return 1;
}

size_t Stream::print(float buf)
{
    return 1;
}

size_t Stream::print(double buf)
{
    return 1;
}

size_t Stream::print(uint32_t buf)
{
    return 1;
}

size_t Stream::print(uint16_t buf)
{
    return 1;
}

size_t Stream::print(uint8_t buf)
{
    return 1;
}

size_t Stream::print(uint8_t buf, uint8_t base)
{
    return 1;
}

size_t Stream::println(const char* buf)
{
    return 1;
}

size_t Stream::println(const char buf)
{
    return 1;
}

size_t Stream::println(float buf)
{
    return 1;
}

size_t Stream::println(uint32_t buf)
{
    return 1;
}

size_t Stream::println(uint8_t buf)
{
    return 1;
}