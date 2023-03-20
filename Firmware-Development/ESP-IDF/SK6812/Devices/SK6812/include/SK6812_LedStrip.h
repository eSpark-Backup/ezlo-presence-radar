#ifndef SK6812_LEDSTRIP_H
#define SK6812_LEDSTRIP_H

#include "driver/rmt.h"
#include "led_strip.h"

#define RMT_TX_CHANNEL      RMT_CHANNEL_0
#define RMT_TX_GPIO         GPIO_NUM_47
#define STRIP_LED_NUMBER    1
#define CHASE_SPEED_MS (50)

led_strip_t *led_strip_new_rmt_sk6812(const led_strip_config_t *config);
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

#endif