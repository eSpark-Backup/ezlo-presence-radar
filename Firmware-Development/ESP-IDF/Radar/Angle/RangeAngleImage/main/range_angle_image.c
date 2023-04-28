#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "debug.h"

#include "xensiv_bgt60trxx_esp32.h"
#include "xensiv_bgt60trxx.h"
#define XENSIV_BGT60TRXX_CONF_IMPL 1
// #include "presence_radar_default.h"
#include "presence_radar_default.h"

#include "Avian.h"
#include "Base.h"
#include "2DMTI.h"
#include "SpectrumAxis.h"
#include "RadarSegmentation.h"
#include "RangeDopplerMap.h"
#include "Algo.h"

// ***************** Angle Capon Headers ***************** //

#include "AngleCapon.h"

// ***************** RangeAngleImage Headers ***************** //

#include "RangeAngleImage.h"

// ***************** RangeDopplerMap Headers ***************** //

#include "RangeDopplerMap/range_doppler_map.h"

#define SAMP_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS) 
#define TOTAL_DATA (SAMP_PER_FRAME + (6 * XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME))
uint16_t* samples = NULL;
static uint16_t samples1[TOTAL_DATA];
static uint16_t samples2[TOTAL_DATA];
// extern int64_t timeduration;
uint32_t timeduration=0;
uint32_t duration=0;
uint32_t fillStartTime=0;
uint32_t fillduration=0;
/*
==============================================================================
   2. LOCAL DEFINITIONS
==============================================================================
*/
#define RX_ANTENNA_ID   0   // This implementation uses only 1 Rx Antenna

#define ESP_INTR_FLAG_DEFAULT 0

/*
==============================================================================
   3. LOCAL TYPES
==============================================================================
*/
rdm_t *rdm_context =NULL;
ifx_AngleCapon_t* angle_capon_handle=NULL;
/*
==============================================================================
   4. LOCAL DATA
==============================================================================
*/
xensiv_bgt60trxx_t * xnsiv_bgt60tr13c = NULL;
spi_device_handle_t xnsiv_bgt60_spi;

static xQueueHandle gpio_evt_queue = NULL;
TaskHandle_t getFIFOHandle = NULL;
bool frameProcFlag = 0;
bgt60trxx_fifo_status_t radar_fifo_status = 0;
uint8_t fifoRead = 0;
uint16_t no_of_bytes = 0;
extern spi_transaction_t transaction; 
extern uint32_t sDur;
// Functions
void getFifoData(void);
void processFrame(ifx_Avian_Device_t* device_handle, ifx_RAI_t* rai_handle, ifx_Cube_R_t* frame);
void blinky(void);
void getFifoErr(void);
void getArrData(uint16_t* dest,uint16_t* src);

// ISR
static void IRAM_ATTR queue_ready (spi_transaction_t *trans);
// Tasks
void radar(void* pvParameter);

/*
==============================================================================
   6. LOCAL FUNCTIONS
==============================================================================
*/
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num == XNSIV_BGT60_IRQ_GPIO) {
        // gpio_intr_disable(XNSIV_BGT60_IRQ_GPIO);
        // fillduration = (int)(esp_timer_get_time() - fillStartTime);
        // timeduration = esp_timer_get_time();
        gpio_set_level(XNSIV_BGT60_CS_GPIO, 0);
        // timeduration = esp_timer_get_time();
        getFifoData();
        // fillStartTime = esp_timer_get_time();
        // duration = (int)(esp_timer_get_time() - timeduration);
        // gpio_intr_enable(XNSIV_BGT60_IRQ_GPIO);
    } 
    // xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void init_gpio(void) {

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
  gpio_set_intr_type(XNSIV_BGT60_BUTTON_GPIO, GPIO_INTR_POSEDGE);
  

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

void bgt60_hard_reset(void) {
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

void esp32_spi_init(void) {

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
}

static void print_device_config(const ifx_Avian_Config_t* config)
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

// ***************** Range Angle Image Configurations: *****************

// * Range Doppler Map Config: * //
#define RANGE_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * 1// Zero padding of 4 gives good range resolution in Range spectrum
#define DOPPLER_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * 1 // Zero padding of 4 gives good range resolution in Doppler spectrum

const ifx_PPFFT_Config_t range_fft_config = {
      .fft_type = IFX_FFT_TYPE_R2C,
      .fft_size = RANGE_FFT_SIZE,
      .mean_removal_enabled = true,
      .window_config = { IFX_WINDOW_BLACKMANHARRIS, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP, 0, 1 },
      .is_normalized_window = 1
  };
const ifx_PPFFT_Config_t doppler_fft_config = {
      .fft_type = IFX_FFT_TYPE_C2C,
      .fft_size = DOPPLER_FFT_SIZE,
      .mean_removal_enabled = true,
      .window_config = { IFX_WINDOW_CHEBYSHEV, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME, 100, 1 },
      .is_normalized_window = 1
  };

const ifx_RDM_Config_t rdmConfig =
{
    .spect_threshold = IFX_SPECT_THRESHOLD,
    .output_scale_type = IFX_SCALE_TYPE_LINEAR,
    .range_fft_config = range_fft_config,
    .doppler_fft_config = doppler_fft_config
};
  

// * Digital Beam Forming Config: * //
const ifx_DBF_Config_t dbfConfig =
{
    .num_beams = 31,
    .num_antennas = 2,
    .min_angle = -60.0,
    .max_angle = 60.0,
    .d_by_lambda =  0.5
};

// * Range Angle Image Config: * //
ifx_RAI_Config_t raiConfig = 
{
    .rdm_config = rdmConfig,
    .alpha_mti_filter = 0.8,
    .dbf_config = dbfConfig,
    .num_of_images = 32,
    .num_antenna_array = 2,
};

ifx_Error_t rai_init(ifx_RAI_Config_t* rai_config)
{
    memset(rai_config, 0, sizeof(ifx_RAI_Config_t));
    return ifx_error_get();
}

ifx_Error_t rai_config(ifx_RAI_Config_t* rai_config, ifx_RAI_t** handle)
{
    *handle = ifx_rai_create(rai_config);
    return ifx_error_get();
}

ifx_Error_t rai_process(ifx_RAI_t* handle, const ifx_Cube_R_t* input, ifx_Cube_R_t* output)
{
    ifx_rai_run_r(handle, input, output);
    return ifx_error_get();
}

/* Angle Capon */
ifx_AngleCapon_Config_t ang_cap_config = 
{
    .range_win_size = 5,
    .selected_rx  = 0,
    .chirps_per_frame = XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME,
    .phase_offset_degrees = 0,
    .num_virtual_antennas = dbfConfig.num_antennas,
    .num_beams = dbfConfig.num_beams,
    .min_angle_degrees = dbfConfig.min_angle,
    .max_angle_degrees = dbfConfig.max_angle,
    .d_by_lambda = dbfConfig.d_by_lambda
};

ifx_Error_t angle_capon_config(ifx_AngleCapon_Config_t* angle_capon_config, ifx_AngleCapon_t** handle)
{
    *handle = ifx_anglecapon_create(angle_capon_config);
    return ifx_error_get();
}

ifx_Error_t angle_capon_process(const ifx_AngleCapon_t* handle, uint32_t range_bin, const ifx_Cube_C_t* rx_spectrum)	
{
    ifx_Float_t angle=0;
    angle = ifx_anglecapon_run(handle, range_bin, rx_spectrum);
    printf("Angle : %f\n",angle);
    return ifx_error_get();
}

/* OS CFAR */
ifx_OSCFAR_Config_t os_cfar_conf = 
{
    .win_rank = 5,       /**< Rank of CFAR reference window.*/
    .guard_band = 4,     /**< Rank of CFAR guard band.*/
    .sample = 0.05,         /**< Constant used for setting CFAR threshold.*/
    .pfa = 0.001,            /**< Probability of false alarm.*/
    .coarse_scalar = 0.3  /**< Used for coarse thresholding 2D feature map.*/
};
/*
==============================================================================
   7. MAIN METHOD
==============================================================================
 */
ifx_Avian_Device_t* device_handle = NULL;
// extern uint16_t* data;
// ifx_Cube_R_t* frame = NULL;

int app_main(int argc, char* argv[])
{
    gpio_reset_pin(XNSIV_BGT60_LED_RED);
    gpio_reset_pin(XNSIV_BGT60_LED_GREEN);

    gpio_set_direction(XNSIV_BGT60_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(XNSIV_BGT60_LED_GREEN, GPIO_MODE_OUTPUT);


    init_gpio();
    xTaskCreatePinnedToCore(radar,"radar", 2048*3, NULL, 10, &getFIFOHandle, 1);
 
    while(1) 
    {   
        vTaskDelay(1000/ portTICK_RATE_MS);  
    }
    
    return 0;
}

    ifx_OSCFAR_t* cfar_handle;
    ifx_Matrix_R_t *cfar_mat;

void radar(void* pvParameter)
{
    ifx_Error_t error = IFX_OK;
    ifx_Avian_Config_t device_config = {0};
  
    xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)malloc(sizeof(xensiv_bgt60trxx_t));

    esp32_spi_init();
    bgt60_hard_reset();

    /* Open the device: Connect to the first radar sensor found. */
    device_handle = ifx_avian_create_dummy_from_xensiv(xnsiv_bgt60tr13c);
    
    if ((error = ifx_error_get()) != IFX_OK)
    {
        TRACE_B("Failed to open device: %s\n", ifx_error_to_string(error));
        goto out;
    }

    xensiv_bgt60trxx_init(device_handle->m_xensiv_device_handle, (void *)&xnsiv_bgt60_spi, false);

    xensiv_bgt60trxx_config(device_handle->m_xensiv_device_handle, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
    
    uint32_t read_reg = 0;
    xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, &read_reg);
    read_reg |= XENSIV_BGT60TRXX_REG_SFCTL_PREFIX_EN_MSK | XENSIV_BGT60TRXX_REG_SFCTL_MISO_HS_READ_MSK;  
    xensiv_bgt60trxx_set_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, read_reg);

    ifx_avian_get_config(device_handle, &device_config);
    print_device_config(&device_config);

    // rdm_t *rdm_context = (rdm_t *)malloc(sizeof(rdm_t));
    ifx_RAI_t* rai_handle = NULL;
    ifx_Avian_Config_t* dev_config = (ifx_Avian_Config_t*)malloc(sizeof(ifx_Avian_Config_t));
    ifx_avian_get_config(device_handle, dev_config);

    // if (rai_init(&raiConfig) != IFX_OK)
    // {
    //     printf("Failed to Init RAI object!\n");
    // }
    // printf("RAI init : %s\n",ifx_error_to_string(ifx_error_get()));

    if (rai_config(&raiConfig, &rai_handle) != IFX_OK)
    {
        printf("Failed to config rai object!\n");
    }
    printf("RAI handle addr : %p\n", rai_handle);
    printf("RAI config : %s\n",ifx_error_to_string(ifx_error_get()));
    
    if (angle_capon_config(&ang_cap_config, &angle_capon_handle) != IFX_OK)
    {
        printf("Failed to config angle capon object!\n");
    }
    printf("Angle Capon handle addr : %p\n", angle_capon_handle);
    printf("Angle Capon config : %s\n",ifx_error_to_string(ifx_error_get()));

    cfar_handle = ifx_oscfar_create(&os_cfar_conf);
    cfar_mat = ifx_mat_create_r(XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME/2, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP); // for real input, range dimension has info only in positive half
    if (ifx_error_get())
    {
        printf("Here cfar\n");
    }

    // rdm_context = (rdm_t *)malloc(sizeof(rdm_t));
    // if (rdm_init(rdm_context) != IFX_OK)
    // {
    //     TRACE_B("Failed to Init RDM object!\n");
    // }

    // if (rdm_config(rdm_context, device_handle, dev_config) != IFX_OK)
    // {
    //     TRACE_B("Failed to config rdm object!\n");
    // }
    // printf("RDM config : %s\n",ifx_error_to_string(ifx_error_get()));

      

    xensiv_bgt60trxx_set_fifo_limit(device_handle->m_xensiv_device_handle, (SAMP_PER_FRAME/MAX_FIFO_BREAK_COUNT)+200);

    uint32_t reg_read = 0;
    if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK) {
        TRACE_B("Error stopping the frame.");
    } 
    else {
        TRACE_B("Frame started !!");
    }
    // fillStartTime = esp_timer_get_time();
    ifx_Cube_R_t* frame = NULL;
    // frame = (ifx_Cube_R_t *)malloc(sizeof(ifx_Cube_R_t));
    frame = ifx_cube_create_r(XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP);       
    // frame->d = (ifx_Float_t *)malloc(sizeof(ifx_Float_t) * SAMP_PER_FRAME);
  
    // char str[100];
    ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
    while(1)
    {
        // ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
        // getFifoErr();
        processFrame(device_handle, rai_handle, frame);        
        // vTaskGetRunTimeStats(str);
        // printf("%s\n",str);
                        // printf("time = %d us\n",(uint32_t)fillduration);  
        // blinky();
        // vTaskDelay(100/portTICK_RATE_MS);
    }

    out:
    ifx_avian_destroy(device_handle);
    // ifx_cube_destroy_r(frame);
    // presence_cleanup();
}

void getFifoData(void) 
{
    xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples1, TOTAL_DATA/MAX_FIFO_BREAK_COUNT);
}

void processFrame(ifx_Avian_Device_t* device_handle, ifx_RAI_t* rai_handle, ifx_Cube_R_t* frame)
{
    if (frameProcFlag)
    {
        // uint32_t timeduration = esp_timer_get_time();
        ifx_Cube_R_t* output = ifx_cube_create_r(raiConfig.num_of_images, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP, dbfConfig.num_beams);
        frame = ifx_avian_get_next_frame(device_handle ,frame ,samples);
        uint32_t range_idx=0;

        if(rai_process(rai_handle, frame, output) != IFX_OK) 
        {
            printf("Failed rai processing!\n"); 
        }
        // printf("rows : %d, cols : %d, slices : %d\n",output->rows, output->cols, output->slices); 
        // printf("RAI processing : %s\n",ifx_error_to_string(ifx_error_get()));
        
        // if(rdm_range_idx(rdm_context, frame, &range_idx) != IFX_OK) 
        // {
        //     printf("Failed rdm processing!\n");
        // }
        // printf("RDM processing : %s\n",ifx_error_to_string(ifx_error_get()));

        ifx_Matrix_R_t ridxMat;
        ifx_cube_get_slice_r(output, 10 , &ridxMat);
        // ifx_oscfar_run(cfar_handle, &ridxMat ,cfar_mat);
        // printf("\nrows : %d, cols %d\n",ridxMat.rows, ridxMat.cols);
        // ifx_oscfar_run(rdm_context->cfar_handle, rdm_context->rdm ,rdm_context->cfar_mat);
        // // do peak search
        uint32_t rmax;
        uint32_t cmax;
        rdm_peak_search(&ridxMat, &rmax, &cmax);
        printf("idx : %d\n",cmax);
        // printf("cols %d\n", ifx_rai_get_rx_spectrum(rai_handle)->cols);
        // range_idx=6;
        if (cmax < ifx_rai_get_rx_spectrum(rai_handle)->cols) 
        {
            if(angle_capon_process(angle_capon_handle, cmax, ifx_rai_get_rx_spectrum(rai_handle)))
            {
                printf("Failed angle capon processing!\n"); 
            }
//             // printf("Angle capon processing : %s\n",ifx_error_to_string(ifx_error_get()));
        }
        // for (uint16_t i = 0; i < SAMP_PER_FRAME; i++)
        // {
        //     printf("%d,",(uint16_t)frame->d[i]);
        //     /* code */
        // }
        // printf("\n");
        // printf("from samp addr = %p\n",samples);
        #if 0
        for (uint16_t i = 0; i < TOTAL_DATA; i++)
        {
            printf("%d,",samples[i]);
            /* code */
        }
        printf("\n");
        vTaskDelay(100/portTICK_RATE_MS);
        // Testing frame separtion of antennas 
        // for (uint16_t i = 0; i < SAMP_PER_FRAME; i+=128)
        // {
        //     printf("0,0,0,0,0,0,");
        //     for (uint16_t y = 0; y < XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP; y++)
        //     {
        //         printf("%d,%d,",(uint16_t)frame->d[y+i], (uint16_t)frame->d[(SAMP_PER_FRAME/2)+y+i]);
        //     }
        // }
        for (uint16_t i = 0; i < SAMP_PER_FRAME/2; i+=64)
        {
            printf("0,0,0,0,0,0,");
            for (uint16_t y = 0; y < XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP; y++)
            {
                printf("%d,0,",(uint16_t)frame->d[y+i]);
            }
            
        }
        printf("\n");
        vTaskDelay(100/portTICK_RATE_MS);
        for (uint16_t i = SAMP_PER_FRAME/2; i < SAMP_PER_FRAME; i+=64)
        {
            printf("0,0,0,0,0,0,");
            for (uint16_t y = 0; y < XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP; y++)
            {
                printf("0,%d,",(uint16_t)frame->d[y+i]);
            }
            
        }
        printf("\n\n");
        vTaskDelay(100/portTICK_RATE_MS);
        #endif
        
        frameProcFlag = 0;
        // ESP_LOGI("", "%d",(int)(esp_timer_get_time() - timeduration));
        free(output->d);
        output->d =NULL;
        free(output);
        output = NULL;
    }
}

void blinky(void)
{
    static uint16_t delayCounter = 0;
    // ** GPIO TOGGLE
    if(delayCounter < 1000)
     {
        delayCounter++;
        gpio_set_level(XNSIV_BGT60_LED_GREEN, 1);
     }
     else if(delayCounter < 2000){
        delayCounter++;
        gpio_set_level(XNSIV_BGT60_LED_GREEN, 0);
            // ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
     }
     else
     {
        delayCounter =0;
     }
}

void getFifoErr(void)
{
    if (radar_fifo_status == FIFO_READ_FAILED)
    {
        ESP_LOGI("FIFO","failed!");
        radar_fifo_status= FIFO_NO_ERROR;
    }
    else if (radar_fifo_status == FIFO_READ_SUCCESS)
    {
        /* code */
        ESP_LOGI("FIFO","Read Successfull");
        // ESP_LOGI("FIFO", "Fifo Fill Time = %d us",fillduration);
        // ESP_LOGI("FIFO", "SPI Read Time = %d us",duration);
        radar_fifo_status= FIFO_NO_ERROR;
    }
}

uint16_t rdata=0;
extern uint8_t fifo_read_count;
static void IRAM_ATTR queue_ready(spi_transaction_t *trans)
{
    // fillStartTime = esp_timer_get_time();
    if (!fifoRead)
    {
        /* code */
        return;
    }
    
    gpio_set_level(XNSIV_BGT60_CS_GPIO, 1);
    fifoRead = 0;

    if (fifo_read_count < MAX_FIFO_BREAK_COUNT)
    {
        return;
    }
    fifo_read_count = FIFO_RESET_COUNT;
    radar_fifo_status = FIFO_READ_SUCCESS;

    if (samples != samples1)
    {
        getArrData(samples1, get_data_ptr());
        if (!frameProcFlag)
        {
            samples = samples1;
            frameProcFlag = 1;
        }   
    }
    else
    {
        getArrData(samples2, get_data_ptr());
        if (!frameProcFlag)
        {
            samples = samples2;
            frameProcFlag = 1;
        }  
    }
    // fillduration =esp_timer_get_time() - fillStartTime;
}

void getArrData(uint16_t* dest,uint16_t* src)
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