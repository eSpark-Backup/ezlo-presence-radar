#define XENSIV_BGT60TRXX_CONF_IMPL 1
#include "radar_app.h"

// Function declaration 
static esp_err_t radar_gpio_init(void);
static esp_err_t radar_configure_led(void);
static esp_err_t radar_spi_init(void);
static void radar_led_blinky(void);
static int32_t radar_get_fifo_data(void);
static ifx_Error_t radar_process_frame(ifx_Avian_Device_t* device_handle, ifx_Cube_R_t* frame);
static void radar_print_fifo_err(void);
static void radar_print_device_config(const ifx_Avian_Config_t* config);
static void split_uint16_by_12bits(uint16_t* dest,uint16_t* src);

// tasks declaration 
void radar(void* pvParameter);

// radar task 
static void IRAM_ATTR queue_ready(spi_transaction_t *trans);

// bgt60tr13c
spi_device_handle_t xnsiv_bgt60_spi;
xensiv_bgt60trxx_t* xnsiv_bgt60tr13c = NULL;
ifx_Avian_Device_t* device_handle = NULL;
extern spi_transaction_t transaction; 
bgt60trxx_fifo_status_t radar_fifo_status = 0;
uint16_t no_of_bytes = 0;
bool frame_process_status = false;

// samples spi buffer
DMA_ATTR uint16_t samples_spi_buf[BUFF_SIZE];
// samples buffer pointers
uint16_t* p_samples = NULL;
uint16_t* samples_buf1 = NULL;
uint16_t* samples_buf2 = NULL;
// frame buffer pointer
ifx_Cube_R_t* frame = NULL;

#if MOTION_ANGLE_APP
ma_t ma_context;
#endif

#if VELOCITY_APP
// rdm 
rdm_t *rdm_context =NULL;
#endif

#if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
// presence
presence_t presence_handle;
#endif

// logs
static const char* TAG = "radar_app.c";

// radar interrupt handler
static void IRAM_ATTR gpio_isr_handler(void* arg) 
{
    uint32_t gpio_num = (uint32_t)arg;

    switch(gpio_num)
    {
        case XNSIV_BGT60_IRQ_GPIO:
            xensiv_bgt60trxx_esp32_spi_cs_set(device_handle->m_xensiv_device_handle->iface, GPIO_LEVEL_LOW);
            if (XENSIV_BGT60TRXX_STATUS_OK != radar_get_fifo_data())
            {
                radar_fifo_status = FIFO_READ_FAILED;
            }
            break;

        default:
            break;
    }
}

static esp_err_t radar_gpio_init(void) 
{
    esp_err_t ret = ESP_OK;
    gpio_config_t io_conf_bgt60_rst = 
    {
        .pin_bit_mask = XNSIV_BGT60_RST_PIN_SEL,
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    gpio_config_t io_conf_bgt60_cs = 
    {
        .pin_bit_mask = XNSIV_BGT60_CS_PIN_SEL,
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    gpio_config_t io_conf_bgt60_LEDs = 
    {
        .pin_bit_mask = XNSIV_BGT60_LEDS_PIN_SEL,
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    gpio_config_t io_conf_inputs = 
    {
        .pin_bit_mask = XNSIV_BGT60_IRQ_PIN_SEL, // | XNSIV_BGT60_BUTTON_PIN_SEL,
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE
    };

    ret = gpio_config(&io_conf_bgt60_rst);
    ESP_RETURN_ON_ERROR(ret, TAG, "reset gpio not configured - invalid argument");
    ret = gpio_config(&io_conf_bgt60_cs);
    ESP_RETURN_ON_ERROR(ret, TAG, "chip select gpio not configured - invalid argument");
    ret = gpio_config(&io_conf_bgt60_LEDs);
    ESP_RETURN_ON_ERROR(ret, TAG, "led gpio's not configured - invalid argument");
    ret = gpio_config(&io_conf_inputs);
    ESP_RETURN_ON_ERROR(ret, TAG, "bgt60 irq and button gpio's not configured - invalid argument");  

    gpio_set_intr_type(XNSIV_BGT60_IRQ_GPIO, GPIO_INTR_POSEDGE);  
    // gpio_set_intr_type(XNSIV_BGT60_BUTTON_GPIO, GPIO_INTR_POSEDGE); 

    //install gpio isr service
    ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    ESP_RETURN_ON_ERROR(ret, TAG, "unable to install isr service");

    //hook isr handler for specific gpio pin
    ret = gpio_isr_handler_add(XNSIV_BGT60_IRQ_GPIO, gpio_isr_handler, (void*) XNSIV_BGT60_IRQ_GPIO);
    ESP_RETURN_ON_ERROR(ret, TAG, "unable to assign isr handler to bgt60 irq gpio");
    // ret = gpio_isr_handler_add(XNSIV_BGT60_BUTTON_GPIO, gpio_isr_handler, (void*) XNSIV_BGT60_BUTTON_GPIO);
    // ESP_RETURN_ON_ERROR(ret, TAG, "unable to assign isr handler to button gpio");

    // Disable Reset
    gpio_set_level(XNSIV_BGT60_RST_GPIO, 1);

    return ret;
}

static esp_err_t radar_spi_init(void) 
{
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .miso_io_num=XNSIV_BGT60_MISO_GPIO,
        .mosi_io_num=XNSIV_BGT60_MOSI_GPIO,
        .sclk_io_num=XNSIV_BGT60_CLK_GPIO,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz = 12 * TOTAL_DATA,
        .intr_flags = 1
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz= 50*1000*1000,   
        .mode=0,                                //SPI mode 0
        #ifndef XNSIV_CS_HANDLING
        // .spics_io_num = XNSIV_BGT60_CS_GPIO,      //CS pin
        #endif
        .queue_size=100,                          //We want to be able to queue 10 transactions at a time
        .post_cb = queue_ready
    };    

    //Initialize the SPI bus
    ret = spi_bus_initialize(XNSIV_BGT60_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO); //SPI_DMA_CH_AUTO); //SPI_DMA_DISABLED);
    ESP_RETURN_ON_ERROR(ret, TAG, "spi bus not initialized");
    
    ret = spi_bus_add_device(XNSIV_BGT60_SPI_HOST, &devcfg, &xnsiv_bgt60_spi);
    ESP_RETURN_ON_ERROR(ret, TAG, "spi bus not added");

    return ret;
}

void bgt60_hard_reset(void) 
{
    gpio_set_level(XNSIV_BGT60_CS_GPIO, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(XNSIV_BGT60_RST_GPIO, 1);
    gpio_set_level(XNSIV_BGT60_CS_GPIO, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(XNSIV_BGT60_RST_GPIO, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(XNSIV_BGT60_RST_GPIO, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void radar(void* pvParameter)
{
    ifx_Error_t error = IFX_OK;
    esp_err_t ret = ESP_OK; 
    int status = 0;
    ifx_Avian_Config_t device_config = {0};

    xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)ifx_mem_alloc(sizeof(xensiv_bgt60trxx_t));
    ESP_GOTO_ON_FALSE(xnsiv_bgt60tr13c, ESP_ERR_NO_MEM, out, TAG, "xensiv bgt60tr13c creation failed"); 

    ret = radar_gpio_init();
    ESP_GOTO_ON_ERROR(ret, out, TAG, "radar gpio init failed");
    
    ret = radar_configure_led();
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);

    ret = radar_spi_init();
    ESP_GOTO_ON_ERROR(ret, out, TAG, "radar spi init failed");

    bgt60_hard_reset();

    /* Open the device: Connect to the first radar sensor found. */
    device_handle = ifx_avian_create_dummy_from_xensiv(xnsiv_bgt60tr13c);
    ESP_GOTO_ON_FALSE(device_handle, ESP_ERR_NO_MEM, out, TAG, "failed to open device");

    status = xensiv_bgt60trxx_init(device_handle->m_xensiv_device_handle, (void *)&xnsiv_bgt60_spi, false); 
    ESP_GOTO_ON_ERROR(status, out, TAG, "xensiv bgt60tr13c initialization failed"); // XENSIV_BGT60TRXX_STATUS_OK = ESP_OK

    status = xensiv_bgt60trxx_config(device_handle->m_xensiv_device_handle, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
    ESP_GOTO_ON_ERROR(status, out, TAG, "xensiv bgt60tr13c config failed"); // XENSIV_BGT60TRXX_STATUS_OK = ESP_OK

    uint32_t read_reg = 0;
    status = xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, &read_reg);
    if(XENSIV_BGT60TRXX_STATUS_OK == status)
    {
        read_reg |= XENSIV_BGT60TRXX_REG_SFCTL_PREFIX_EN_MSK | XENSIV_BGT60TRXX_REG_SFCTL_MISO_HS_READ_MSK;  
        status = xensiv_bgt60trxx_set_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, read_reg);
        if(XENSIV_BGT60TRXX_STATUS_OK != status)
        {
            ESP_LOGE(TAG, "unable to set XENSIV_BGT60TRXX_REG_SFCTL value");
        }
    } 

    ifx_avian_get_config(device_handle, &device_config);
    radar_print_device_config(&device_config);

    status = xensiv_bgt60trxx_set_fifo_limit(device_handle->m_xensiv_device_handle, (SAMP_PER_FRAME/MAX_FIFO_BREAK_COUNT)+200);
    if(XENSIV_BGT60TRXX_STATUS_OK != status)
    {
        ESP_LOGE(TAG, "unable to set fifo limit");
    }
    

    frame = ifx_cube_create_r(XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP); 
    ESP_GOTO_ON_FALSE(frame, ESP_ERR_NO_MEM, out, TAG, "frame creation failed");

    samples_buf1 = (uint16_t*) ifx_mem_alloc(sizeof(uint16_t) * TOTAL_DATA);
    ESP_GOTO_ON_FALSE(samples_buf1, ESP_ERR_NO_MEM, out, TAG, "samples buffer 1 memory allocation failed");
    samples_buf2 = (uint16_t*) ifx_mem_alloc(sizeof(uint16_t) * TOTAL_DATA);
    ESP_GOTO_ON_FALSE(samples_buf2, ESP_ERR_NO_MEM, out, TAG, "samples buffer 2 memory allocation failed");

    status = xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true);
    if(XENSIV_BGT60TRXX_STATUS_OK != status)
    {
        ESP_LOGE(TAG, "unable to start frame");
    }

#if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
    error = presence_init(&presence_handle);
    ESP_GOTO_ON_ERROR(error, out, TAG, "failed to init presence object!"); // IFX_OK = ESP_OK

    error = presence_config(&presence_handle, device_handle, &device_config);
    ESP_GOTO_ON_ERROR(error, out, TAG, "failed to config presene object!"); // IFX_OK = ESP_OK
#endif

#if VELOCITY_APP
    rdm_context = (rdm_t *)ifx_mem_alloc(sizeof(rdm_t));
    ESP_GOTO_ON_FALSE(rdm_context, ESP_ERR_NO_MEM, out, TAG, "rdm context creation failed"); 

    error = rdm_init(rdm_context);
    ESP_GOTO_ON_ERROR(error, out, TAG, "failed to init rdm object!"); // IFX_OK = ESP_OK

    error = rdm_config(rdm_context, device_handle, &device_config);
    ESP_GOTO_ON_ERROR(error, out, TAG, "failed to config rdm object!"); // IFX_OK = ESP_OK 
#endif

#if MOTION_ANGLE_APP
    error = ma_init(&ma_context);
    ESP_GOTO_ON_ERROR(error, out, TAG, "failed to init motion angle object!"); // IFX_OK = ESP_OK

    error = ma_config(&ma_context, &device_config);
    ESP_GOTO_ON_ERROR(error, out, TAG, "failed to config motion angle object!"); // IFX_OK = ESP_OK
#endif 
    ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
    
    while(1)
    {
        // radar_print_fifo_err();
        if (radar_process_frame(device_handle, frame) != IFX_OK)
        {
            ESP_LOGE(TAG, "failed to process radar frame");
        }
        // radar_led_blinky();
    }

    out:
    ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));
    ifx_avian_destroy(device_handle);
    ifx_cube_destroy_r(frame);
    #if MOTION_ANGLE_APP
    ma_cleanup(&ma_context);
    #endif
    #if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
    presence_cleanup(&presence_handle);
    #endif
    vTaskDelete(NULL);
}

static ifx_Error_t radar_process_frame(ifx_Avian_Device_t* device_handle, ifx_Cube_R_t* frame)
{
    ifx_Error_t ret = IFX_OK;
    if (frame_process_status)
    {    
        frame = ifx_avian_get_next_frame(device_handle ,frame ,p_samples);

    #if PRESENCE_APP | VELOCITY_APP | MOTION_ANGLE_APP
        ret = presence_process(&presence_handle, frame);
        ESP_GOTO_ON_ERROR(ret, algo_err, TAG, "failed processing presence!"); // IFX_OK = ESP_OK 

        if (presence_handle.presence_status == IFX_PRESENCE_SENSING_PRESENT)
        {
            #if VELOCITY_APP
            ret = rdm_process(rdm_context, frame);
            ESP_GOTO_ON_ERROR(ret, algo_err, TAG, "failed rdm processing!"); // IFX_OK = ESP_OK 
            #endif 


            #if MOTION_ANGLE_APP
            ret = ma_process(&ma_context, frame);
            ESP_GOTO_ON_ERROR(ret, algo_err, TAG, "failed motion angle processing!"); // IFX_OK = ESP_OK 

            printf("Range %0.2f ", ma_context.result.range);
            printf("Speed %0.2f ", ma_context.result.speed);
            printf("Azimuth %0.2f ", ma_context.result.angle_azimuth);
            printf("Elevation %0.2f\n", ma_context.result.angle_elevation);
            #endif
        }      
    #endif
        algo_err:
        frame_process_status = false;
    }
    return ret;
}

static void IRAM_ATTR queue_ready(spi_transaction_t *trans)
{
    if (!xensiv_bgt60trxx_esp32_is_reading_fifo())
    {
        return;
    }
    
    xensiv_bgt60trxx_esp32_spi_cs_set(device_handle->m_xensiv_device_handle->iface, GPIO_LEVEL_HIGH);
    xensiv_bgt60trxx_esp32_disable_reading_fifo();

    if (xensiv_bgt60trxx_esp32_get_fifo_read_count() < MAX_FIFO_BREAK_COUNT)
    {
        return;
    }
    xensiv_bgt60trxx_esp32_reset_fifo_count();
    radar_fifo_status = FIFO_READ_SUCCESS;

    if (p_samples != samples_buf1)
    {
        split_uint16_by_12bits(samples_buf1, samples_spi_buf);
        if (!frame_process_status)
        {
            p_samples = samples_buf1;
            frame_process_status = true;
        }   
    }
    else
    {
        split_uint16_by_12bits(samples_buf2, samples_spi_buf);
        if (!frame_process_status)
        {
            p_samples = samples_buf2;
            frame_process_status = true;
        }  
    }
}

static int32_t radar_get_fifo_data(void) 
{
    return xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples_spi_buf, TOTAL_DATA/MAX_FIFO_BREAK_COUNT);
}

static void radar_print_fifo_err(void)
{
    switch (radar_fifo_status)
    {  
        case FIFO_NO_ERROR:
            break;

        case FIFO_READ_FAILED:
            ESP_LOGE("fifo","read failed!");
            radar_fifo_status = FIFO_NO_ERROR;
            break;
        
        case FIFO_READ_SUCCESS:
            ESP_LOGI("fifo","read successfull");
            radar_fifo_status = FIFO_NO_ERROR; 
            break;

        default:
            ESP_LOGE("fifo","unknown error"); 
            radar_fifo_status = FIFO_NO_ERROR;
            break;
    }
}

void split_uint16_by_12bits(uint16_t* dest, uint16_t* src)
{
    uint16_t index=0;
    for (uint16_t i = 0; i < TOTAL_DATA*12/16; i+=3)
    {
        dest[index] = src[i] & 0xF0FF;
        dest[index] = (dest[index]<<4) | (dest[index]>>12);

        dest[index+1] = (src[i+1] & 0x00FF) | (src[i] & 0x0F00);

        dest[index+2] = ((src[i+1] & 0xFF00) >> 4)  | ((src[i+2] & 0x00F0) >> 4);

        dest[index+3] = (src[i+2] & 0xFF0F);
        dest[index+3] = (dest[index+3]<<8) | (dest[index+3]>>8);

        index+=4;
    }
}

static void radar_print_device_config(const ifx_Avian_Config_t* config)
{
    printf("Device configuration:\n");
    printf("sample_rate_Hz:          %" PRIu32 "\n", config->sample_rate_Hz);
    printf("rx_mask:                 %" PRIu32 "\n", config->rx_mask);
    printf("tx_mask:                 %" PRIu32 "\n", config->tx_mask);
    printf("tx_power_level:          %" PRIu32 "\n", config->tx_power_level);
    printf("if_gain_dB:              %" PRIu32 "\n", config->if_gain_dB);
    // printf("start_frequency_Hz:      %" PRIu64 "\n", config->start_frequency_Hz);
    // printf("end_frequency_Hz:        %" PRIu64 "\n", config->end_frequency_Hz);
    printf("num_samples_per_chirp:   %" PRIu32 "\n", config->num_samples_per_chirp);
    printf("num_chirps_per_frame:    %" PRIu32 "\n", config->num_chirps_per_frame);
    printf("chirp_repetition_time_s: %g\n", config->chirp_repetition_time_s);
    printf("frame_repetition_time_s: %g\n", config->frame_repetition_time_s);
    printf("hp_cutoff_Hz:            %" PRIu32 "\n", config->hp_cutoff_Hz);
    printf("aaf_cutoff_Hz:           %" PRIu32 "\n", config->aaf_cutoff_Hz);
    printf("mimo_mode:               %s\n", config->mimo_mode == IFX_MIMO_TDM ? "time-domain multiplexed" : "off");
    printf("\n");
}

static esp_err_t radar_configure_led(void)
{
    esp_err_t ret;
    ret = gpio_reset_pin(XNSIV_BGT60_LED_RED);
    ESP_RETURN_ON_ERROR(ret, TAG, "reset pin - invalid gpio number for led red");
    ret = gpio_reset_pin(XNSIV_BGT60_LED_GREEN);
    ESP_RETURN_ON_ERROR(ret, TAG, "reset pin - invalid gpio number for led green");

    gpio_set_direction(XNSIV_BGT60_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(XNSIV_BGT60_LED_GREEN, GPIO_MODE_OUTPUT);

    return ret;
}

static void radar_led_blinky(void)
{
    static uint16_t delayCounter = 0;
    // ** GPIO TOGGLE
    if(delayCounter < 1000)
    {
        delayCounter++;
        gpio_set_level(XNSIV_BGT60_LED_GREEN, 1);
    }
    else if(delayCounter < 2000)
    {
        delayCounter++;
        gpio_set_level(XNSIV_BGT60_LED_GREEN, 0);
    }
    else
    {
        delayCounter =0;
    }
}

TaskHandle_t radar_task_handle = NULL;
esp_err_t radar_init(void)
{
    BaseType_t ret;
    ret = xTaskCreatePinnedToCore(radar,"radar", 2048*3, NULL, 10, &radar_task_handle, 1);
    if (pdPASS == ret) 
    {
        return ESP_OK;
    }
    return ESP_FAIL;
}