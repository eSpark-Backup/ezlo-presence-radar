#ifndef __GPIO_ISR_SERVICE_H__
#define __GPIO_ISR_SERVICE_H__

#include "ezlopi_devices_list.h"

typedef void (*f_interrupt_upcall_t)(void *);
typedef union u_param
{
    gpio_num_t gpio_num;
    l_ezlopi_item_t *item;
} u_param_t;

void gpio_isr_service_register_v3(l_ezlopi_item_t *item, f_interrupt_upcall_t __upcall, TickType_t debounce_ms);

void gpio_isr_service_init(void);
void gpio_isr_service_register_via_gpio_num(gpio_num_t gpio_num, f_interrupt_upcall_t __upcall, TickType_t debounce_ms, void *user_arg);
void gpio_isr_service_register(s_ezlopi_device_properties_t *properties, f_interrupt_upcall_t __upcall, TickType_t debounce_ms);

#endif // __GPIO_ISR_SERVICE_H__
