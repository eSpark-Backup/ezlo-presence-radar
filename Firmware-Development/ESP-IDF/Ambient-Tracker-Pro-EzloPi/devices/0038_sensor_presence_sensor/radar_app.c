#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "trace.h"
#include "radar_app.h"
#include "gpio_isr_service.h"
// #include "0038_sensor_presence_sensor.h"

#define TAG "radar_app.c"

static esp_err_t __radar_gpio_init(l_ezlopi_item_t *item);
static esp_err_t __radar_spi_init(s_device_args_t *device_args);
static void __print_device_config(const ifx_Avian_Config_t *config);
static ifx_Error_t __radar_process_frame(s_device_args_t *device_args);
static void __split_uint16_by_12bits(uint16_t *dest, uint16_t *src);
static int32_t __radar_get_fifo_data(s_device_args_t *device_args);

static void IRAM_ATTR __gpio_isr_handler(void *arg);
static void IRAM_ATTR __queue_ready_spi_post_cb(spi_transaction_t *trans);

void __radar_app_hard_reset(void)
{
    ESP_ERROR_CHECK(gpio_set_level(XNSIV_BGT60_CS_GPIO, 0));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(XNSIV_BGT60_RST_GPIO, 1));
    ESP_ERROR_CHECK(gpio_set_level(XNSIV_BGT60_CS_GPIO, 1));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(XNSIV_BGT60_RST_GPIO, 0));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(XNSIV_BGT60_RST_GPIO, 1));
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

esp_err_t radar_reset_radar_fifo(s_device_args_t *device_args)
{
    esp_err_t error = ESP_OK;
    if (NULL != device_args->device_handle)
    {
        int32_t ret = xensiv_bgt60trxx_soft_reset(device_args->device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_RESET_FIFO, NULL);
        if (XENSIV_BGT60TRXX_STATUS_OK == ret)
        {
            error = ESP_OK;
        }
        else
        {
            error = ESP_FAIL;
        }
    }
    else
    {
        error = ESP_FAIL;
    }
    return error;
}

esp_err_t radar_set_radar_fifo(s_device_args_t *device_args)
{
    esp_err_t error = ESP_OK;
    if (NULL != device_args->device_handle)
    {
        int32_t ret = xensiv_bgt60trxx_start_frame(device_args->device_handle->m_xensiv_device_handle, true, device_args);
        if (XENSIV_BGT60TRXX_STATUS_OK == ret)
        {
            error = ESP_OK;
        }
        else
        {
            error = ESP_FAIL;
        }
    }
    else
    {
        error = ESP_FAIL;
    }
    return error;
}

esp_err_t radar_get_data(s_device_args_t *device_args)
{
    if (IFX_OK == __radar_process_frame(device_args))
    {
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t radar_app_init(l_ezlopi_item_t *item)
{
    if (!item || !item->user_arg)
    {
        TRACE_E("Invalid args");
        return ESP_FAIL;
    }

    int status = 0;
    esp_err_t ret = ESP_OK;
    ifx_Error_t error = IFX_OK;
    ifx_Avian_Config_t device_config;
    memset(&device_config, 0, sizeof(ifx_Avian_Config_t));
    s_device_args_t *device_args = (s_device_args_t *)item->user_arg;

    device_args->xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)ifx_mem_alloc(sizeof(xensiv_bgt60trxx_t));
    ESP_GOTO_ON_FALSE(device_args->xnsiv_bgt60tr13c, ESP_ERR_NO_MEM, exit_out, TAG, "xensiv bgt60tr13c creation failed");

    ret = __radar_gpio_init(item);
    ESP_GOTO_ON_ERROR(ret, exit_out, TAG, "radar gpio init failed");

    ret = __radar_spi_init(device_args);
    ESP_GOTO_ON_ERROR(ret, exit_out, TAG, "radar spi init failed");

    __radar_app_hard_reset();
    TRACE_I("Perpheral init OK");

    /* Open the device: Connect to the first radar sensor found. */
    device_args->device_handle = ifx_avian_create_dummy_from_xensiv(device_args->xnsiv_bgt60tr13c);
    ESP_GOTO_ON_FALSE(device_args->device_handle, ESP_ERR_NO_MEM, exit_out, TAG, "failed to open device");
    TRACE_I("'ifx_avian_create_dummy_from_xensiv' OK");

    status = xensiv_bgt60trxx_init(device_args->device_handle->m_xensiv_device_handle,
                                   (void *)&device_args->xnsiv_bgt60_spi, false, device_args);
    ESP_GOTO_ON_ERROR(status, exit_out, TAG, "xensiv bgt60tr13c initialization failed");
    TRACE_I("'xensiv_bgt60trxx_init' OK");

    status = xensiv_bgt60trxx_config(device_args->device_handle->m_xensiv_device_handle, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS, device_args);
    ESP_GOTO_ON_ERROR(status, exit_out, TAG, "xensiv bgt60tr13c config failed");
    TRACE_I("'xensiv_bgt60trxx_config' OK");

    uint32_t read_reg = 0;
    status = xensiv_bgt60trxx_get_reg(device_args->device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, &read_reg, device_args);
    if (XENSIV_BGT60TRXX_STATUS_OK == status)
    {
        TRACE_I("'xensiv_bgt60trxx_get_reg' OK");
        read_reg |= XENSIV_BGT60TRXX_REG_SFCTL_PREFIX_EN_MSK;// | XENSIV_BGT60TRXX_REG_SFCTL_MISO_HS_READ_MSK;
        status = xensiv_bgt60trxx_set_reg(device_args->device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, read_reg, device_args);
        if (XENSIV_BGT60TRXX_STATUS_OK != status)
        {
            TRACE_E("unable to set XENSIV_BGT60TRXX_REG_SFCTL value, ERROR: %s", ifx_error_to_string(status));
        }
        else
        {
            TRACE_I("'xensiv_bgt60trxx_set_reg' OK");
        }
    }
    else
    {
        TRACE_E("'xensiv_bgt60trxx_get_reg' failed, ERROR: %s", ifx_error_to_string(status));
    }

    ifx_avian_get_config(device_args->device_handle, &device_config);
    TRACE_I("'ifx_avian_get_config' OK");
    __print_device_config(&device_config);
    TRACE_I("'__print_device_config' OK");

    status = xensiv_bgt60trxx_set_fifo_limit(device_args->device_handle->m_xensiv_device_handle, (SAMP_PER_FRAME / MAX_FIFO_BREAK_COUNT), device_args);
    if (XENSIV_BGT60TRXX_STATUS_OK != status)
    {
        TRACE_E("unable to set fifo limit");
    }
    else
    {
        TRACE_I("'xensiv_bgt60trxx_set_fifo_limit' OK");
    }

    device_args->frame = ifx_cube_create_r(XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME,
                                           XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP);
    ESP_GOTO_ON_FALSE(device_args->frame, ESP_ERR_NO_MEM, exit_out, TAG, "frame creation failed");
    TRACE_I("'ifx_cube_create_r' OK");

    device_args->p_samples = (uint16_t *)ifx_mem_alloc(sizeof(uint16_t) * TOTAL_DATA);
    ESP_GOTO_ON_FALSE(device_args->p_samples, ESP_ERR_NO_MEM, exit_out, TAG, "samples buffer memory allocation failed");
    TRACE_I("'ifx_mem_alloc' OK");

    status = xensiv_bgt60trxx_start_frame(device_args->device_handle->m_xensiv_device_handle, true, device_args);
    if (XENSIV_BGT60TRXX_STATUS_OK != status)
    {
        TRACE_E("unable to start frame");
    }
    else
    {
        TRACE_I("'xensiv_bgt60trxx_start_frame' OK");
    }

#if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
    error = presence_init(&device_args->presence_handle);
    ESP_GOTO_ON_ERROR(error, exit_out, TAG, "failed to init presence object!"); // IFX_OK = ESP_OK

    error = presence_config(&device_args->presence_handle, device_args->device_handle, &device_config);
    ESP_GOTO_ON_ERROR(error, exit_out, TAG, "failed to config presene object!"); // IFX_OK = ESP_OK
#endif

#if VELOCITY_APP
    device_args->rdm_context = (rdm_t *)ifx_mem_alloc(sizeof(rdm_t));
    ESP_GOTO_ON_FALSE(device_args->rdm_context, ESP_ERR_NO_MEM, exit_out, TAG, "rdm context creation failed");

    error = rdm_init(device_args->rdm_context);
    ESP_GOTO_ON_ERROR(error, exit_out, TAG, "failed to init rdm object!"); // IFX_OK = ESP_OK

    error = rdm_config(device_args->rdm_context, device_args->device_handle, &device_config);
    ESP_GOTO_ON_ERROR(error, exit_out, TAG, "failed to config rdm object!"); // IFX_OK = ESP_OK
#endif

#if MOTION_ANGLE_APP
    error = ma_init(&device_args->ma_context);
    ESP_GOTO_ON_ERROR(error, exit_out, TAG, "failed to init motion angle object!"); // IFX_OK = ESP_OK

    error = ma_config(&device_args->ma_context, &device_config);
    ESP_GOTO_ON_ERROR(error, exit_out, TAG, "failed to config motion angle object!"); // IFX_OK = ESP_OK
#endif
    TRACE_I("HeapMemory : %d", xPortGetFreeHeapSize());
    return ESP_OK;

exit_out:
    TRACE_E("error: %s\n", ifx_error_to_string(ifx_error_get()));
    ifx_avian_destroy(device_args->device_handle);
    ifx_cube_destroy_r(device_args->frame);
#if MOTION_ANGLE_APP
    ma_cleanup(&device_args->ma_context);
#endif
#if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
    presence_cleanup(&device_args->presence_handle);
#endif
    return ESP_FAIL;
}

static ifx_Error_t __radar_process_frame(s_device_args_t *device_args)
{
    ifx_Error_t ret = IFX_ERROR;
    // TRACE_D("frame_process_status is %d\n", device_args->frame_process_status);
    if (device_args->frame_process_status)
    {
        if (device_args->dma_spi_buffer[0] == device_args->p_samples)
        {
            __split_uint16_by_12bits(device_args->p_samples, device_args->dma_spi_buffer[1]);
        }
        else
        {
            __split_uint16_by_12bits(device_args->p_samples, device_args->dma_spi_buffer[0]);
        }
        
        device_args->frame = ifx_avian_get_next_frame(device_args->device_handle, device_args->frame, device_args->p_samples);

#if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
        ret = presence_process(&device_args->presence_handle, device_args->frame);
        // TRACE_W("ret: %d", ret);
        ESP_GOTO_ON_ERROR(ret, algo_err, TAG, "failed processing presence!"); // IFX_OK = ESP_OK

        if (device_args->presence_handle.presence_status == IFX_PRESENCE_SENSING_PRESENT)
        {
            device_args->target_result.presence = true;
#if VELOCITY_APP
            ret = rdm_process(rdm_context, device_args->frame);
            ESP_GOTO_ON_ERROR(ret, algo_err, TAG, "failed rdm processing!"); // IFX_OK = ESP_OK
#endif

#if MOTION_ANGLE_APP
            vTaskDelay(20 / portTICK_RATE_MS); // To avoid wdt
            ret = ma_process(&device_args->ma_context, &device_args->target_result, device_args->frame);
            // TRACE_W("ret: %d", ret);
            ESP_GOTO_ON_ERROR(ret, algo_err, TAG, "failed motion angle processing!"); // IFX_OK = ESP_OK
#endif
        }
        else
        {
            device_args->target_result.presence = false;
        }
#endif
    algo_err:
        device_args->frame_process_status = false;
    }
    return ret;
}

static void IRAM_ATTR __gpio_isr_handler(void *user_arg)
{
    l_ezlopi_item_t *item = (l_ezlopi_item_t *)user_arg;
    if (item)
    {
        s_device_args_t *device_args = item->user_arg;
        if (device_args)
        {
            esp_err_t ret;
            xensiv_bgt60trxx_esp32_spi_cs_set(device_args->device_handle->m_xensiv_device_handle->iface, GPIO_LEVEL_LOW);
            if (XENSIV_BGT60TRXX_STATUS_OK !=  (ret = __radar_get_fifo_data(device_args))) // initiate spi dma read request (no computation involved )
            {
                // TRACE_E("'radar_get_fifo_data' error[%d]: %s", ret, ifx_error_to_string(ret));
            }          
        }
    }
}

static esp_err_t __radar_gpio_init(l_ezlopi_item_t *item)
{
    esp_err_t ret = ESP_OK;
    gpio_config_t gpio_reset_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = XNSIV_BGT60_RST_PIN_SEL,
    };

    ret = gpio_config(&gpio_reset_conf);
    if (ret)
        TRACE_E("reset gpio not configured - invalid argument, ERROR: %s", esp_err_to_name(ret));

    gpio_config_t gpio_cs_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = XNSIV_BGT60_CS_PIN_SEL,
    };

    ret = gpio_config(&gpio_cs_conf);
    if (ret)
        TRACE_E("chip select gpio not configured - invalid argument, ERROR: %s", esp_err_to_name(ret));

    gpio_config_t gpio_irq_confg = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_POSEDGE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pin_bit_mask = XNSIV_BGT60_IRQ_PIN_SEL,
    };

    ret = gpio_config(&gpio_irq_confg);
    if (ret)
        TRACE_E("bgt60 irq and button gpio's not configured - invalid argument, ERROR: %s", esp_err_to_name(ret));

    gpio_set_intr_type(XNSIV_BGT60_IRQ_GPIO, GPIO_INTR_POSEDGE);
    gpio_intr_enable(XNSIV_BGT60_IRQ_GPIO);
    gpio_isr_handler_add(XNSIV_BGT60_IRQ_GPIO, __gpio_isr_handler, (void *)item);

    // Disable Reset
    gpio_set_level(XNSIV_BGT60_RST_GPIO, 1);

    return ret;
}

static esp_err_t __radar_spi_init(s_device_args_t *device_args)
{
    esp_err_t ret = ESP_FAIL;

    if (device_args)
    {
        spi_bus_config_t buscfg = {
            .miso_io_num = XNSIV_BGT60_MISO_GPIO,
            .mosi_io_num = XNSIV_BGT60_MOSI_GPIO,
            .sclk_io_num = XNSIV_BGT60_CLK_GPIO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 12 * TOTAL_DATA,
            .intr_flags = 1,
        };

        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = XNSIV_BGT60_SPI_CLK,
            .mode = 0, // SPI mode 0
#ifndef XNSIV_CS_HANDLING
// .spics_io_num = XNSIV_BGT60_CS_GPIO,      // CS pin
#endif
            .queue_size = 2, // We want to be able to queue 10 transactions at a time
            .post_cb = __queue_ready_spi_post_cb,
        };

        // Initialize the SPI bus
        ret = spi_bus_initialize(XNSIV_BGT60_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO); // SPI_DMA_DISABLED);
        if (ret)
            TRACE_E("spi bus not initialized, ERROR: %s", esp_err_to_name(ret));

        ret = spi_bus_add_device(XNSIV_BGT60_SPI_HOST, &devcfg, &device_args->xnsiv_bgt60_spi);
        if (ret)
            TRACE_E("spi bus not added, ERROR: %s", esp_err_to_name(ret));
    }

    return ret;
}

static void __split_uint16_by_12bits(uint16_t *dest, uint16_t *src)
{
    uint16_t index = 0;
    for (uint16_t i = 0; i < TOTAL_DATA * 12 / 16; i += 3)
    {
        dest[index] = src[i] & 0xF0FF;
        dest[index] = (dest[index] << 4) | (dest[index] >> 12);

        dest[index + 1] = (src[i + 1] & 0x00FF) | (src[i] & 0x0F00);

        dest[index + 2] = ((src[i + 1] & 0xFF00) >> 4) | ((src[i + 2] & 0x00F0) >> 4);

        dest[index + 3] = (src[i + 2] & 0xFF0F);
        dest[index + 3] = (dest[index + 3] << 8) | (dest[index + 3] >> 8);

        index += 4;
    }
}

static void IRAM_ATTR __queue_ready_spi_post_cb(spi_transaction_t *trans)
{
    // TRACE_D("Queue ready");

    if (trans && trans->user)
    {
        s_device_args_t *device_args = (s_device_args_t *)trans->user;
        if (device_args->fifo_settings.spi_fifo_read)
        {
            xensiv_bgt60trxx_esp32_spi_cs_set(device_args->device_handle->m_xensiv_device_handle->iface, GPIO_LEVEL_HIGH);
            device_args->fifo_settings.spi_fifo_read = false;

            if (device_args->fifo_settings.fifo_read_count >= MAX_FIFO_BREAK_COUNT)
            {
                device_args->fifo_settings.fifo_read_count = FIFO_RESET_COUNT;
                device_args->fifo_settings.radar_fifo_status = FIFO_READ_SUCCESS;
                
                for (uint8_t i = 0; i < 2; i++)
                {
                    if (device_args->samples_spi_buf != device_args->dma_spi_buffer[i])
                    {
                        if (!device_args->frame_process_status)
                        {
                            device_args->samples_spi_buf = device_args->dma_spi_buffer[i];
                            device_args->frame_process_status = true;
                        }
                        break;
                    }
                }
            }
        }
    }
}

static void __print_device_config(const ifx_Avian_Config_t *config)
{
    TRACE_D("Device configuration:\n");
    TRACE_D("sample_rate_Hz:          %" PRIu32, config->sample_rate_Hz);
    TRACE_D("rx_mask:                 %" PRIu32, config->rx_mask);
    TRACE_D("tx_mask:                 %" PRIu32, config->tx_mask);
    TRACE_D("tx_power_level:          %" PRIu32, config->tx_power_level);
    TRACE_D("if_gain_dB:              %" PRIu32, config->if_gain_dB);
    // TRACE_D("start_frequency_Hz:      %" PRIu64 , config->start_frequency_Hz);
    // TRACE_D("end_frequency_Hz:        %" PRIu64 , config->end_frequency_Hz);
    TRACE_D("num_samples_per_chirp:   %" PRIu32, config->num_samples_per_chirp);
    TRACE_D("num_chirps_per_frame:    %" PRIu32, config->num_chirps_per_frame);
    TRACE_D("chirp_repetition_time_s: %g", config->chirp_repetition_time_s);
    TRACE_D("frame_repetition_time_s: %g", config->frame_repetition_time_s);
    TRACE_D("hp_cutoff_Hz:            %" PRIu32, config->hp_cutoff_Hz);
    TRACE_D("aaf_cutoff_Hz:           %" PRIu32, config->aaf_cutoff_Hz);
    TRACE_D("mimo_mode:               %s", config->mimo_mode == IFX_MIMO_TDM ? "time-domain multiplexed" : "off");
    TRACE_D("\n");
}

static int32_t __radar_get_fifo_data(s_device_args_t *device_args)
{
    return xensiv_bgt60trxx_get_fifo_data(device_args->device_handle->m_xensiv_device_handle,
                                          device_args->samples_spi_buf,
                                          TOTAL_DATA / MAX_FIFO_BREAK_COUNT,
                                          device_args,
                                          &(device_args->fifo_settings));
}

#if 0
static void __presence_sensor_task(void *pv)
{
    l_ezlopi_item_t *item = (l_ezlopi_item_t *)pv;

    if (item && item->user_arg)
    {
        s_device_args_t *device_args = (s_device_args_t *)item->user_arg;
        TRACE_D("task handle: %p", device_args->task_handle);
        TRACE_W("Wait complete!");

        while (1)
        {
            esp_err_t err = ESP_OK;

            if (ulTaskNotifyTake(false, portMAX_DELAY))
            {
                // TRACE_I("Got event");
                static int event_count = 0;
                event_count ++;
                TRACE_E("event_count : %d", event_count);
            }
        }
    }

    vTaskDelete(NULL);
}
#endif

esp_err_t initializeRadarTask(l_ezlopi_item_t *item)
{
    if (!item || !item->user_arg)
    {
        TRACE_E("Invalid args");
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_FAIL;

    if(ESP_OK == (ret = radar_app_init(item)))
    {
        TRACE_B("'radar_app_init' OK");
        #if 0
        s_device_args_t *device_args = item->user_arg;
        xTaskCreatePinnedToCore(__presence_sensor_task, "presense_sensor_task", 2 * 2048, item, 10, &device_args->task_handle, 0);
        TRACE_D("task handle: %p", device_args->task_handle);
        #endif
    }

    return ret;
}