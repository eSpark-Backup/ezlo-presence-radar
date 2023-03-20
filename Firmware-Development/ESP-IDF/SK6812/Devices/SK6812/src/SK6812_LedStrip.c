#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "esp_log.h"
#include "esp_attr.h"
#include "led_strip.h"
#include "driver/rmt.h"

static const char *TAG = "SK6812";
#define STRIP_CHECK(a, str, goto_tag, ret_value, ...)                             \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)

#define SK6812_T0H_NS (300)
#define SK6812_T0L_NS (600)
#define SK6812_T1H_NS (900)
#define SK6812_T1L_NS (600)
#define SK6812_RESET_US (80)

static uint32_t sk6812_t0h_ticks = 0;
static uint32_t sk6812_t1h_ticks = 0;
static uint32_t sk6812_t0l_ticks = 0;
static uint32_t sk6812_t1l_ticks = 0;

typedef struct {
    led_strip_t parent;
    rmt_channel_t rmt_channel;
    uint32_t strip_len;
    uint8_t buffer[0];
} sk6812_t;

/**
 * @brief Conver RGB data to RMT format.
 *
 * @note For sk6812, R,G,B each contains 256 different choices (i.e. uint8_t)
 *
 * @param[in] src: source data, to converted to RMT format
 * @param[in] dest: place where to store the convert result
 * @param[in] src_size: size of source data
 * @param[in] wanted_num: number of RMT items that want to get
 * @param[out] translated_size: number of source data that got converted
 * @param[out] item_num: number of RMT items which are converted from source data
 */
static void IRAM_ATTR sk6812_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    const rmt_item32_t bit0 = {{{ sk6812_t0h_ticks, 1, sk6812_t0l_ticks, 0 }}}; //Logical 0
    const rmt_item32_t bit1 = {{{ sk6812_t1h_ticks, 1, sk6812_t1l_ticks, 0 }}}; //Logical 1
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            // MSB first
            if (*psrc & (1 << (7 - i))) {
                pdest->val =  bit1.val;
            } else {
                pdest->val =  bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

static esp_err_t sk6812_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{
    esp_err_t ret = ESP_OK;
    sk6812_t *sk6812 = __containerof(strip, sk6812_t, parent);
    STRIP_CHECK(index < sk6812->strip_len, "index out of the maximum number of leds", err, ESP_ERR_INVALID_ARG);
    uint32_t start = index * 3;
    // In thr order of GRB
    sk6812->buffer[start + 0] = green & 0xFF;
    sk6812->buffer[start + 1] = red & 0xFF;
    sk6812->buffer[start + 2] = blue & 0xFF;
    return ESP_OK;
err:
    return ret;
}

static esp_err_t sk6812_refresh(led_strip_t *strip, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    sk6812_t *sk6812 = __containerof(strip, sk6812_t, parent);
    STRIP_CHECK(rmt_write_sample(sk6812->rmt_channel, sk6812->buffer, sk6812->strip_len * 3, true) == ESP_OK,
                "transmit RMT samples failed", err, ESP_FAIL);
    return rmt_wait_tx_done(sk6812->rmt_channel, pdMS_TO_TICKS(timeout_ms));
err:
    return ret;
}

static esp_err_t sk6812_clear(led_strip_t *strip, uint32_t timeout_ms)
{
    sk6812_t *sk6812 = __containerof(strip, sk6812_t, parent);
    // Write zero to turn off all leds
    memset(sk6812->buffer, 0, sk6812->strip_len * 3);
    return sk6812_refresh(strip, timeout_ms);
}

static esp_err_t sk6812_del(led_strip_t *strip)
{
    sk6812_t *sk6812 = __containerof(strip, sk6812_t, parent);
    free(sk6812);
    return ESP_OK;
}

led_strip_t *led_strip_new_rmt_sk6812(const led_strip_config_t *config)
{
    led_strip_t *ret = NULL;
    STRIP_CHECK(config, "configuration can't be null", err, NULL);

    // 24 bits per led
    uint32_t sk6812_size = sizeof(sk6812_t) + config->max_leds * 3;
    sk6812_t *sk6812 = calloc(1, sk6812_size);
    STRIP_CHECK(sk6812, "request memory for sk6812 failed", err, NULL);

    uint32_t counter_clk_hz = 0;
    STRIP_CHECK(rmt_get_counter_clock((rmt_channel_t)config->dev, &counter_clk_hz) == ESP_OK,
                "get rmt counter clock failed", err, NULL);
    // ns -> ticks
    float ratio = (float)counter_clk_hz / 1e9;
    sk6812_t0h_ticks = (uint32_t)(ratio * SK6812_T0H_NS);
    sk6812_t0l_ticks = (uint32_t)(ratio * SK6812_T0L_NS);
    sk6812_t1h_ticks = (uint32_t)(ratio * SK6812_T1H_NS);
    sk6812_t1l_ticks = (uint32_t)(ratio * SK6812_T1L_NS);

    // set sk6812 to rmt adapter
    rmt_translator_init((rmt_channel_t)config->dev, sk6812_rmt_adapter);

    sk6812->rmt_channel = (rmt_channel_t)config->dev;
    sk6812->strip_len = config->max_leds;

    sk6812->parent.set_pixel = sk6812_set_pixel;
    sk6812->parent.refresh = sk6812_refresh;
    sk6812->parent.clear = sk6812_clear;
    sk6812->parent.del = sk6812_del;

    return &sk6812->parent;
err:
    return ret;
}

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}
