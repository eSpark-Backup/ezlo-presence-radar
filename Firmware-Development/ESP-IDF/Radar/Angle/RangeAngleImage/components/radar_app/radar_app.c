#define XENSIV_BGT60TRXX_CONF_IMPL 1
#include "radar_app.h"

// Function declaration 
static void radar_gpio_init(void);
static esp_err_t radar_configure_led(void);
static esp_err_t radar_spi_init(void);
static void radar_led_blinky(void);
static int32_t radar_get_fifo_data(void);
static void radar_process_frame(ifx_Avian_Device_t* device_handle, ifx_Cube_R_t* frame);
static void radar_print_fifo_err(void);
static void radar_print_device_config(const ifx_Avian_Config_t* config);
static void split_uint16_by_12bits(uint16_t* dest,uint16_t* src);
static void getEspRuntimeStatus(void);

// tasks declaration 
void radar(void* pvParameter);

// radar task 
static xQueueHandle gpio_evt_queue = NULL;
static void IRAM_ATTR queue_ready(spi_transaction_t *trans);

// bgt60tr13c
spi_device_handle_t xnsiv_bgt60_spi;
xensiv_bgt60trxx_t* xnsiv_bgt60tr13c = NULL;
ifx_Avian_Device_t* device_handle = NULL;
extern spi_transaction_t transaction; 
bgt60trxx_fifo_status_t radar_fifo_status = 0;
uint8_t fifoRead = 0;
uint16_t no_of_bytes = 0;
bool frame_process_status = 0;

// frame buffer
uint16_t* samples = NULL;
static uint16_t samples1[TOTAL_DATA];
static uint16_t samples2[TOTAL_DATA];
ifx_Cube_R_t* frame = NULL;

// cfar
ifx_OSCFAR_t* cfar_handle;
ifx_Matrix_R_t *cfar_mat;

#if ANGLE_APP
// range_angle_image
ifx_RAI_t* rai_handle = NULL;
ifx_Cube_R_t* rai_output = NULL;
ifx_RAI_Config_t raiConfig;

// angle_capon
ifx_AngleCapon_Config_t ang_cap_config;
ifx_AngleCapon_t* angle_capon_handle=NULL;
#endif

#if VELOCITY_APP
// rdm 
rdm_t *rdm_context =NULL;
#endif

#if PRESENCE_APP | VELOCITY_APP
// presence
presence_t presence_handle;
#endif

// logs
static const char* TAG = "radar bgt60tr13c";

// radar interrupt handler
static void IRAM_ATTR gpio_isr_handler(void* arg) 
{
    uint32_t gpio_num = (uint32_t) arg;

    if(gpio_num == XNSIV_BGT60_IRQ_GPIO) 
    {
        xensiv_bgt60trxx_esp32_spi_cs_set(device_handle->m_xensiv_device_handle->iface, GPIO_LEVEL_LOW);
        radar_get_fifo_data();
    } 
}

static void radar_gpio_init(void) {

  gpio_config_t io_conf_bgt60_rst = {};
  gpio_config_t io_conf_bgt60_cs = {};
  gpio_config_t io_conf_bgt60_LEDs = {};
  gpio_config_t io_conf_inputs = {};

  io_conf_bgt60_rst.pin_bit_mask = XNSIV_BGT60_RST_PIN_SEL;
  io_conf_bgt60_rst.intr_type = GPIO_INTR_DISABLE;
  io_conf_bgt60_rst.mode = GPIO_MODE_OUTPUT;
  io_conf_bgt60_rst.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf_bgt60_rst.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf_bgt60_rst);
  
  io_conf_bgt60_cs.pin_bit_mask = XNSIV_BGT60_CS_PIN_SEL;
  io_conf_bgt60_cs.intr_type = GPIO_INTR_DISABLE;
  io_conf_bgt60_cs.mode = GPIO_MODE_OUTPUT;
  io_conf_bgt60_cs.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf_bgt60_cs.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf_bgt60_cs);
  
  io_conf_bgt60_LEDs.pin_bit_mask = XNSIV_BGT60_LEDS_PIN_SEL;
  io_conf_bgt60_LEDs.intr_type = GPIO_INTR_DISABLE;
  io_conf_bgt60_LEDs.mode = GPIO_MODE_OUTPUT;
  io_conf_bgt60_LEDs.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf_bgt60_LEDs.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf_bgt60_LEDs);

  io_conf_inputs.pin_bit_mask = XNSIV_BGT60_IRQ_PIN_SEL | XNSIV_BGT60_BUTTON_PIN_SEL;
  io_conf_inputs.intr_type = GPIO_INTR_POSEDGE;
  io_conf_inputs.mode = GPIO_MODE_INPUT;
  io_conf_inputs.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf_inputs.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpio_config(&io_conf_inputs);  

  //change gpio intrrupt type for one pin
  gpio_set_intr_type(XNSIV_BGT60_IRQ_GPIO, GPIO_INTR_POSEDGE);
//   gpio_set_intr_type(XNSIV_BGT60_BUTTON_GPIO, GPIO_INTR_POSEDGE);

  //create a queue to handle gpio event from isr
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

  //install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(XNSIV_BGT60_IRQ_GPIO, gpio_isr_handler, (void*) XNSIV_BGT60_IRQ_GPIO);
//   gpio_isr_handler_add(XNSIV_BGT60_BUTTON_GPIO, gpio_isr_handler, (void*) XNSIV_BGT60_BUTTON_GPIO);

  // Disable Reset
  gpio_set_level(XNSIV_BGT60_RST_GPIO, 1);
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
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO); //SPI_DMA_CH_AUTO); //SPI_DMA_DISABLED);
    ESP_ERROR_CHECK(ret);
    
    ret=spi_bus_add_device(SPI2_HOST, &devcfg, &xnsiv_bgt60_spi);
    ESP_ERROR_CHECK(ret);

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
    ifx_Avian_Config_t device_config = {0};
    xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)malloc(sizeof(xensiv_bgt60trxx_t));

    frame = ifx_cube_create_r(XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP);    

    radar_gpio_init();
    // radar_configure_led();
    radar_spi_init();
    bgt60_hard_reset();

    /* Open the device: Connect to the first radar sensor found. */
    device_handle = ifx_avian_create_dummy_from_xensiv(xnsiv_bgt60tr13c);
    
    if ((error = ifx_error_get()) != IFX_OK)
    {
        ESP_LOGE(TAG, "failed to open device: %s\n", ifx_error_to_string(error));
        goto out;
    }

    xensiv_bgt60trxx_init(device_handle->m_xensiv_device_handle, (void *)&xnsiv_bgt60_spi, false);
    xensiv_bgt60trxx_config(device_handle->m_xensiv_device_handle, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
    
    uint32_t read_reg = 0;
    xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, &read_reg);
    read_reg |= XENSIV_BGT60TRXX_REG_SFCTL_PREFIX_EN_MSK | XENSIV_BGT60TRXX_REG_SFCTL_MISO_HS_READ_MSK;  
    xensiv_bgt60trxx_set_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, read_reg);

    ifx_avian_get_config(device_handle, &device_config);
    radar_print_device_config(&device_config);

    xensiv_bgt60trxx_set_fifo_limit(device_handle->m_xensiv_device_handle, (SAMP_PER_FRAME/MAX_FIFO_BREAK_COUNT)+200);

    if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK) 
    {
        ESP_LOGE(TAG, "error stopping the frame.");
    } 
    else 
    {
        ESP_LOGI(TAG, "frame started !!");
    }  

#if PRESENCE_APP | VELOCITY_APP
    if (presence_init(&presence_handle) != IFX_OK) {
        ESP_LOGE(TAG, "failed to init presence object!\n"); 
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));    
    }

    if (presence_config(&presence_handle, device_handle, &device_config) != IFX_OK) {
        ESP_LOGE(TAG, "failed to config presene object!\n");  
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));      
    }
#endif

#if VELOCITY_APP
    rdm_context = (rdm_t *)malloc(sizeof(rdm_t));
    if (rdm_init(rdm_context) != IFX_OK)
    {
        ESP_LOGE(TAG, "failed to init rdm object!\n");
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));
    }

    if (rdm_config(rdm_context, device_handle, &device_config) != IFX_OK)
    {
        ESP_LOGE(TAG, "failed to config rdm object!\n");
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));
    }
    
#endif

#if ANGLE_APP
    ifx_Avian_Config_t* dev_config = (ifx_Avian_Config_t*)malloc(sizeof(ifx_Avian_Config_t));
    ifx_avian_get_config(device_handle, dev_config);

    if (rai_init(&raiConfig) != IFX_OK)
    {
        ESP_LOGE(TAG, "failed to init rai object!\n");
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));
    }

    if (rai_config(&raiConfig, &rai_handle) != IFX_OK)
    {
        ESP_LOGE(TAG, "failed to config rai object!\n");
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));
    }
    
    if (angle_capon_init(&ang_cap_config) != IFX_OK)
    {
        ESP_LOGE(TAG, "failed to init angle capon object!\n");
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));
    }
    if (angle_capon_config(&ang_cap_config, &angle_capon_handle) != IFX_OK)
    {
        ESP_LOGE(TAG, "failed to config angle capon object!\n");
        ESP_LOGE(TAG, "error: %s\n",ifx_error_to_string(ifx_error_get()));
        
    }
        
    rai_output = ifx_cube_create_r(NUM_OF_IMAGES, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP, NUM_OF_BEAMS); 
#endif 
    
    ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
    while(1)
    {
        // radar_print_fifo_err();
        radar_process_frame(device_handle, frame); 
        // radar_led_blinky();
    }

    out:
    ifx_avian_destroy(device_handle);
    ifx_cube_destroy_r(frame);
    #if ANGLE_APP
        ifx_cube_destroy_r(rai_output);
    #endif
    #if PRESENCE_APP | VELOCITY_APP
    presence_cleanup(&presence_handle);
    #endif
}

static void radar_process_frame(ifx_Avian_Device_t* device_handle, ifx_Cube_R_t* frame)
{
    if (frame_process_status)
    {    
        frame = ifx_avian_get_next_frame(device_handle ,frame ,samples);
    #if ANGLE_APP
        if(rai_process(rai_handle, frame, rai_output) != IFX_OK) 
        {
            printf("failed rai processing!\n"); 
        }

        ifx_Matrix_R_t ridxMat;
        ifx_cube_get_slice_r(rai_output, 10 , &ridxMat);

        // // do peak search
        uint32_t rmax;
        uint32_t cmax;
        rdm_peak_search(&ridxMat, &rmax, &cmax);
        printf("idx : %d\n",cmax);

        if (cmax < ifx_rai_get_rx_spectrum(rai_handle)->cols) 
        {
            if(angle_capon_process(angle_capon_handle, cmax, ifx_rai_get_rx_spectrum(rai_handle)))
            {
                printf("Failed angle capon processing!\n"); 
            }
        }
    #endif

    #if PRESENCE_APP | VELOCITY_APP
        if(presence_process(&presence_handle, frame) != IFX_OK) {
            ESP_LOGE(TAG, "failed processing presence!\n"); 
        }  
        else
        {
        #if VELOCITY_APP
            if (presence_handle.presence_status == IFX_PRESENCE_SENSING_PRESENT)
            {
                if(rdm_process(rdm_context, frame) != IFX_OK) 
                {
                    printf("Failed rdm processing!\n");
                }
            }  
        #endif 
        }
    #endif
        frame_process_status = 0;
    }
}

static void IRAM_ATTR queue_ready(spi_transaction_t *trans)
{
    if (!fifoRead)
    {
        return;
    }
    
    xensiv_bgt60trxx_esp32_spi_cs_set(device_handle->m_xensiv_device_handle->iface, GPIO_LEVEL_HIGH);
    fifoRead = 0;

    if (xensiv_bgt60trxx_esp32_get_fifo_read_count() < MAX_FIFO_BREAK_COUNT)
    {
        return;
    }
    xensiv_bgt60trxx_esp32_reset_fifo_count();
    radar_fifo_status = FIFO_READ_SUCCESS;

    if (samples != samples1)
    {
        split_uint16_by_12bits(samples1, xensiv_bgt60trxx_esp32_get_data_ptr());
        if (!frame_process_status)
        {
            samples = samples1;
            frame_process_status = 1;
        }   
    }
    else
    {
        split_uint16_by_12bits(samples2, xensiv_bgt60trxx_esp32_get_data_ptr());
        if (!frame_process_status)
        {
            samples = samples2;
            frame_process_status = 1;
        }  
    }
}

static int32_t radar_get_fifo_data(void) 
{
    return xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples1, TOTAL_DATA/MAX_FIFO_BREAK_COUNT);
}

static void radar_print_fifo_err(void)
{
    switch (radar_fifo_status)
    {  
        case FIFO_NO_ERROR:
            break;

        case FIFO_READ_FAILED:
            ESP_LOGE("FIFO","failed!");
            radar_fifo_status= FIFO_NO_ERROR;
            break;
        
        case FIFO_READ_SUCCESS:
            ESP_LOGI("FIFO","Read Successfull");
            radar_fifo_status= FIFO_NO_ERROR; 
            break;

        default:
            ESP_LOGE("FIFO","Unknown error"); 
            radar_fifo_status= FIFO_NO_ERROR;
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
    esp_err_t err;
    err = gpio_reset_pin(XNSIV_BGT60_LED_RED);
    err = gpio_reset_pin(XNSIV_BGT60_LED_GREEN);
    err = gpio_set_direction(XNSIV_BGT60_LED_RED, GPIO_MODE_OUTPUT);
    err = gpio_set_direction(XNSIV_BGT60_LED_GREEN, GPIO_MODE_OUTPUT);

    return err;
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

char str[100];   
static void getEspRuntimeStatus(void)
{    
    vTaskGetRunTimeStats(str);
    printf("%s\n",str);
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