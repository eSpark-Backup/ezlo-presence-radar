#include "Stream.hpp"

int Stream::available(void)
{  

    return ESP_OK;
}

size_t Stream::write(uint8_t byte)
{

    return ESP_OK;
}

uint8_t Stream::read(void)  
{

    return ESP_OK;
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