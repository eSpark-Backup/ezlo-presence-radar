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
#include "rdm_defaults.h"

#define SAMP_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP)
#define TOTAL_DATA (SAMP_PER_FRAME + (6 * XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME))
uint16_t* samples = NULL;
static uint16_t samples1[TOTAL_DATA];
static uint16_t samples2[TOTAL_DATA];
// extern int64_t timeduration;

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
typedef struct
{
    ifx_Avian_Device_t *device_handle;
    ifx_Matrix_R_t *rdm;
    ifx_RDM_t *rdm_handle;
    ifx_2DMTI_R_t *mti_handle;
    ifx_Math_Axis_Spec_t range_spec;
    ifx_Math_Axis_Spec_t speed_spec;
} rdm_t;

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
volatile bool data_available = false;
bool fifoFailed = false;
// Functions
void getFifoData(void);
void processFrame(ifx_Avian_Device_t* device_handle, rdm_t *rdm_context, ifx_Cube_R_t* frame);
void blinky(void);
void getFifoErr(void);

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
        gpio_set_level(XNSIV_BGT60_CS_GPIO, 0);
        // timeduration = esp_timer_get_time();
        data_available = true;
        getFifoData();
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
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz= 50*1000*1000,   
        .mode=0,                                //SPI mode 0
        #ifndef XNSIV_CS_HANDLING
        // .spics_io_num = XNSIV_BGT60_CS_GPIO,      //CS pin
        #endif
        .queue_size=100,                          //We want to be able to queue 10 transactions at a time
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

//----------------------------------------------------------------------------

/**
 * @brief Application specific initialization function
 *
 * Initializes the presence sensing context.
 *
 * @param ctx           context of the application
 */
ifx_Error_t rdm_init(rdm_t *rdm_context)
{
    memset(rdm_context, 0, sizeof(rdm_t));
    return ifx_error_get();
}

/**
 * @brief Application specific configuration function
 *
 * Configures the presence sensing with specified in json or default configuration.
 * Defines a state change callback and creates an instance of presence sensing algorithm.
 *
 * @param ctx           context of the application
 * @param device        connected device handle
 * @param json          json handle with configuration
 * @param dev_config    device configuration handle
 */
ifx_Error_t rdm_config(rdm_t *rdm_context, ifx_Avian_Device_t *device, ifx_Avian_Config_t *dev_config)
{
    ifx_Error_t ret = 0;

    const uint32_t range_fft_size = dev_config->num_samples_per_chirp * 4;  // Zero padding of 4 gives good range resolution in Range spectrum
    const uint32_t doppler_fft_size = dev_config->num_chirps_per_frame * 4; // Zero padding of 4 gives good range resolution in Doppler spectrum

    ifx_PPFFT_Config_t range_fft_config = {
        .fft_type = IFX_FFT_TYPE_R2C,
        .fft_size = range_fft_size,
        .mean_removal_enabled = true,
        .window_config = {IFX_WINDOW_BLACKMANHARRIS, dev_config->num_samples_per_chirp, 0, 1},
        .is_normalized_window = 1};

    ifx_PPFFT_Config_t doppler_fft_config = {
        .fft_type = IFX_FFT_TYPE_C2C,
        .fft_size = doppler_fft_size,
        .mean_removal_enabled = true,
        .window_config = {IFX_WINDOW_CHEBYSHEV, dev_config->num_chirps_per_frame, 100, 1},
        .is_normalized_window = 1};

    ifx_RDM_Config_t rdm_config = (ifx_RDM_Config_t){
        .spect_threshold = IFX_SPECT_THRESHOLD,
        .output_scale_type = IFX_SCALE_TYPE_LINEAR,
        .range_fft_config = range_fft_config,
        .doppler_fft_config = doppler_fft_config};

    rdm_context->rdm_handle = ifx_rdm_create(&rdm_config);
    if ((ret = ifx_error_get()))
    {
        printf("Here 1\n");
        return ret;
    }

    rdm_context->rdm = ifx_mat_create_r(range_fft_size / 2, doppler_fft_size); // for real input, range dimension has info only in positive half
    if ((ret = ifx_error_get()))
    {
        printf("Here 2\n");
        return ret;
    }

    // MTI Filter for static target cancellation
    rdm_context->mti_handle = ifx_2dmti_create_r(IFX_ALPHA_MTI_FILTER,
                                                 IFX_MAT_ROWS(rdm_context->rdm),
                                                 IFX_MAT_COLS(rdm_context->rdm));
    if ((ret = ifx_error_get()))
    {
        printf("Here 3\n");
        return ret;
    }

    const ifx_Float_t bandwidth_Hz = ifx_devconf_get_bandwidth(dev_config);
    rdm_context->range_spec = ifx_spectrum_axis_calc_range_axis(IFX_FFT_TYPE_R2C, range_fft_size, dev_config->num_samples_per_chirp, bandwidth_Hz);
    if ((ret = ifx_error_get()))
    {
        printf("Here 4");
        return ret;
    }

    const ifx_Float_t center_freq_Hz = ifx_devconf_get_center_frequency(dev_config);
    const ifx_Float_t chirptime_s = ifx_devconf_get_chirp_time(dev_config);
    rdm_context->speed_spec = ifx_spectrum_axis_calc_speed_axis(IFX_FFT_TYPE_C2C, doppler_fft_size, center_freq_Hz, chirptime_s);

    return (ifx_error_get());
}

/**
 * @brief Application specific cleanup function
 *
 * Destroys the presence sensing instances and cleans up allocated memory.
 *
 * @param ctx           context of the application
 */
ifx_Error_t rdm_cleanup(rdm_t *rdm_context)
{
    ifx_rdm_destroy(rdm_context->rdm_handle);
    ifx_mat_destroy_r(rdm_context->rdm);
    ifx_2dmti_destroy_r(rdm_context->mti_handle);
    return ifx_error_get();
}

/**
 * @brief Application specific processing function
 *
 * Runs a presence sensing algorithm on fetched data and informs about the results.
 *
 * @param ctx           context of the application
 * @param frame         collected frame
 */
void rdm_peak_search(const ifx_Matrix_R_t *matrix, uint32_t *rmax, uint32_t *cmax)
{
    *rmax = 0, *cmax = 0;
    ifx_Float_t max_value = IFX_MAT_AT(matrix, 0, 0);

    for (uint32_t r = 0; r < IFX_MAT_ROWS(matrix); r++)
    {
        for (uint32_t c = 0; c < IFX_MAT_COLS(matrix); c++)
        {
            ifx_Float_t value = IFX_MAT_AT(matrix, r, c);
            if (value > max_value)
            {
                *rmax = r;
                *cmax = c;
                max_value = value;
            }
        }
    }
}

ifx_Error_t rdm_process(rdm_t *rdm_context, ifx_Cube_R_t *frame)
{
    ifx_Error_t ret = 0;
    ifx_Matrix_R_t antenna_data;
    ifx_cube_get_row_r(frame, 0, &antenna_data);
    ifx_rdm_run_r(rdm_context->rdm_handle, &antenna_data, rdm_context->rdm);
    if ((ret = ifx_error_get()))
    {
        return ret;
    }

    ifx_2dmti_run_r(rdm_context->mti_handle, rdm_context->rdm, rdm_context->rdm);
    if ((ret = ifx_error_get()))
    {
        return ret;
    }

    // do peak search
    uint32_t rmax;
    uint32_t cmax;
    rdm_peak_search(rdm_context->rdm, &rmax, &cmax);
    // range
    const ifx_Float_t range = rmax * rdm_context->range_spec.value_bin_per_step;
    // speed
    const ifx_Float_t speed = (((ifx_Float_t)IFX_MAT_COLS(rdm_context->rdm) / 2) - (ifx_Float_t)cmax) * rdm_context->speed_spec.value_bin_per_step;

    // printf("range_m:%g, speed_m_s:%g\n", range, speed);
    if (abs(speed) < 7  )
    {
        if (speed > 1.5)
        {
            printf("Range = %g , Moving Forward\n", range);
        }
        else if (speed < -1.5)
        {
            printf("Range = %g ,Moving Backward\n", range);
        }
        // else 
        // {
        //     printf("Range = %g \n", range);
        // }
    }

    return (ifx_error_get());
}

/*
==============================================================================
   7. MAIN METHOD
==============================================================================
 */
ifx_Avian_Device_t* device_handle = NULL;
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
        getFifoErr();        
        vTaskDelay(500/ portTICK_RATE_MS);  
    }
    
    return 0;
}

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

    rdm_t *rdm_context = (rdm_t *)malloc(sizeof(rdm_t));
    ifx_Avian_Config_t* dev_config = (ifx_Avian_Config_t*)malloc(sizeof(ifx_Avian_Config_t));
    ifx_avian_get_config(device_handle, dev_config);

    if (rdm_init(rdm_context) != IFX_OK)
    {
        TRACE_B("Failed to Init RDM object!\n");
    }

    if (rdm_config(rdm_context, device_handle, dev_config) != IFX_OK)
    {
        TRACE_B("Failed to config rdm object!\n");
    }
    printf("%s\n",ifx_error_to_string(ifx_error_get()));
    
    xensiv_bgt60trxx_set_fifo_limit(device_handle->m_xensiv_device_handle, SAMP_PER_FRAME);

    uint32_t reg_read = 0;
    if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK) {
        TRACE_B("Error stopping the frame.");
    } 
    else {
        TRACE_B("Frame started !!");
    }

    ifx_Cube_R_t* frame = NULL;
    frame = (ifx_Avian_Device_t *)malloc(sizeof(ifx_Avian_Device_t));      
    frame->d = (ifx_Float_t *)malloc(sizeof(ifx_Float_t) * SAMP_PER_FRAME);
  
    while(1)
    {
       processFrame(device_handle, rdm_context, frame);
    }

    out:
    ifx_avian_destroy(device_handle);
    // ifx_cube_destroy_r(frame);
    // presence_cleanup();
}

void getFifoData(void) 
{
    if (!data_available)
    {
        return;
    }
        // timeduration = esp_timer_get_time();
        data_available = false;
        // ESP_LOGI("", "%d",(int)(esp_timer_get_time() - timeduration));
        int ret=0;
        if (samples != samples1)
        {
            ret = xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples1, TOTAL_DATA);
            if(ret != XENSIV_BGT60TRXX_STATUS_OK)
            {
                // ESP_LOGI("FIFO","failed!");
                fifoFailed = true;
                return;
            }
            if (!frameProcFlag)
            {
                samples = samples1;
                frameProcFlag = 1;
            } 
            
        }
        else
        {
            ret = xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples2, TOTAL_DATA);
            if(ret != XENSIV_BGT60TRXX_STATUS_OK)
            {
                // ESP_LOGI("FIFO","failed!");
                fifoFailed = true;
                return;
            }
            if (!frameProcFlag)
            {
                samples = samples2;
                frameProcFlag = 1;
            } 
        }
}

void processFrame(ifx_Avian_Device_t* device_handle, rdm_t *rdm_context, ifx_Cube_R_t* frame)
{
    if (frameProcFlag)
    {
        frame = ifx_avian_get_next_frame(device_handle ,frame ,samples);
        if(rdm_process(rdm_context, frame) != IFX_OK) 
        {
            TRACE_B("Failed rdm processing!\n");    
            return;    
        }
        // printf("from samp addr = %p\n",samples);
        // for (uint16_t i = 0; i < TOTAL_DATA; i++)
        // {
        //     printf("%d,",samples[i]);
        //     /* code */
        // }
        // printf("\n");
        frameProcFlag = 0;
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
    if (fifoFailed)
    {
        ESP_LOGI("FIFO","failed!");
        fifoFailed= false;
    }
}