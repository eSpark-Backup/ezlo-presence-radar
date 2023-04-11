#include "ezlo_w5500.h"

esp_err_t w5500_setup(w5500_t* w5500)
{
    esp_err_t ret = w5500_power_pin_config(w5500->power_pin);
    if(ret != ESP_OK)
        return ret;

    ret = w5500_reset_pin_config(w5500->reset_pin);
    if(ret != ESP_OK)
        return ret;

    w5500_startup_reset(w5500);
    return ESP_OK;
}

/*!
 *  @brief  Startup reset sequence for W5500 module.
 *  @param  Pointer of w5500_t.
 */
void w5500_startup_reset(w5500_t* w5500)
{
    /* 1.Enable PWR to W5500 via mosfet */
    gpio_set_level(w5500->power_pin, 1);
    /* 2. Wait 500mS */
    vTaskDelay(500 / portTICK_RATE_MS);
    /* 3. Pull reset low, wait 5mS */
    gpio_set_level(w5500->reset_pin, 0);
    vTaskDelay(5 / portTICK_RATE_MS);
    /* 4. Pull reset high, wait 10mS */
    gpio_set_level(w5500->reset_pin, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
}

esp_err_t w5500_power_pin_config(gpio_num_t power_pin)
{
    esp_err_t ret = w5500_gpio_config(power_pin);
    if(ret != ESP_OK) 
        return ret;
        
    return ESP_OK;
}

esp_err_t w5500_reset_pin_config(gpio_num_t reset_pin)
{
    esp_err_t ret = w5500_gpio_config(reset_pin);
    if(ret != ESP_OK) 
        return ret;
        
    return ESP_OK;
}

esp_err_t w5500_gpio_config(gpio_num_t gpio_num)
{
    gpio_config_t gpio_conf = 
    {
        .pin_bit_mask = ( 1UL << gpio_num),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&gpio_conf);
    if(ret != ESP_OK) 
        return ret;
        
    return ESP_OK;
}