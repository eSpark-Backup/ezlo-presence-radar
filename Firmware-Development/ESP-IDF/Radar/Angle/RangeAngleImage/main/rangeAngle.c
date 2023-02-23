/**
 * @file    app_presence_detection.c
 *
 * @brief   Presence detection application main program source file.
 *
 * This example illustrates how to fetch time-domain data from an Avian radar sensor
 * and process it utilizing a presence sensing algorithm of the Radar SDK.
 *
 */


/*
==============================================================================
   1. INCLUDE FILES
==============================================================================
*/

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
// #include "esp_spi_flash.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "debug.h"

#include "xensiv_bgt60trxx_esp32.h"
#include "xensiv_bgt60trxx.h"
#define XENSIV_BGT60TRXX_CONF_IMPL 1
#include "presence_radar_default.h"

#include "Avian.h"
#include "Base.h"
#include "RadarSegmentation.h"

// ***************** RangeAngleImage Headers ***************** //

#include "RangeAngleImage.h"
#include "RDM_Defaults.h"

// ***************** Range Angle Image Configurations: *****************

ifx_RAI_t* raiHandler; 

// * Range Doppler Map Config: * //
#define RANGE_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * 4 // Zero padding of 4 gives good range resolution in Range spectrum
#define DOPPLER_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * 4 // Zero padding of 4 gives good range resolution in Doppler spectrum

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
    .num_beams = 27,
    .num_antennas = 2,
    .min_angle = -40.0,
    .max_angle = 40.0,
    .d_by_lambda =  0.5
};

// * Range Angle Image Config: * //
ifx_RAI_Config_t raiConfig = 
{
    .rdm_config = rdmConfig,
    .alpha_mti_filter = 0.8,
    .dbf_config = dbfConfig,
    .num_of_images = 27,
    .num_antenna_array = 2,
};

#define NUM_SAMPLES_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS *\
                                             XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME *\
                                             XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP)

xensiv_bgt60trxx_t * xnsiv_bgt60tr13c = NULL;
spi_device_handle_t xnsiv_bgt60_spi;

/* Allocate enough memory for the radar dara frame. */
// static uint16_t samples[NUM_SAMPLES_PER_FRAME];

/*
==============================================================================
   5. LOCAL FUNCTION PROTOTYPES
==============================================================================
*/
void init_gpio(void);
void bgt60_hard_reset(void);
void esp32_spi_init(void);
static void print_device_config(const ifx_Avian_Config_t* config);

/*
==============================================================================
   7. MAIN METHOD
==============================================================================
 */

int app_main(int argc, char* argv[]) {
    
    ifx_Error_t error = IFX_OK;
    ifx_Avian_Config_t device_config = {0};
    ifx_Avian_Device_t* device_handle = NULL;
    // ifx_Cube_R_t* frame = NULL;

    xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)malloc(sizeof(xensiv_bgt60trxx_t));

    gpio_reset_pin(XNSIV_BGT60_LED_RED);
    gpio_reset_pin(XNSIV_BGT60_LED_GREEN);

    gpio_set_direction(XNSIV_BGT60_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(XNSIV_BGT60_LED_GREEN, GPIO_MODE_OUTPUT);

    init_gpio();
    esp32_spi_init();
    bgt60_hard_reset();
    vTaskDelay(500 / portTICK_RATE_MS);

    // * Range Angle Image Algorithm * //
    raiHandler = ifx_rai_create(&raiConfig);

    /* Open the device: Connect to the first radar sensor found. */
    device_handle = ifx_avian_create_dummy_from_xensiv(xnsiv_bgt60tr13c);
    
    if ((error = ifx_error_get()) != IFX_OK)
    {
        TRACE_B("Failed to open device: %s\n", ifx_error_to_string(error));
        goto out;
    }

    xensiv_bgt60trxx_init(device_handle->m_xensiv_device_handle, (void *)&xnsiv_bgt60_spi, false);

    xensiv_bgt60trxx_config(device_handle->m_xensiv_device_handle, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
    
    // * Disable Header * //
    // xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, &read_reg);
    // read_reg |= XENSIV_BGT60TRXX_REG_SFCTL_PREFIX_EN_MSK;  
    //xensiv_bgt60trxx_set_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, read_reg);

    /* Fetch NUM_FETCHED_FRAMES number of frames. */
    // frame = (ifx_Avian_Device_t *)malloc(sizeof(ifx_Avian_Device_t));    
    
    xensiv_bgt60trxx_set_fifo_limit(device_handle->m_xensiv_device_handle, NUM_SAMPLES_PER_FRAME);

    if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK) {
        ESP_LOGE("StartFrame","Error stopping the frame.");
    } else 
    {
        ESP_LOGE("StartFrame","Frame started !!");
    }

    uint32_t reg_read = 0;
    uint16_t FillStatus =0;
    uint8_t CREF = 0;
    while(1) 
    {
        if (xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_FSTAT_TR13C, &reg_read) == XENSIV_BGT60TRXX_STATUS_OK)
        {
            FillStatus = reg_read & 0x3FFF;
            CREF = (reg_read >> 21) & 0x0001;
        }
        if(CREF == 1 && FillStatus >= 64)
        {
            FillStatus = 0;
            CREF = 0;

            uint16_t samples[NUM_SAMPLES_PER_FRAME];
            ESP_LOGI("FIFO","get-data");
            int ret = xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples, NUM_SAMPLES_PER_FRAME);                                
              if ( ret == XENSIV_BGT60TRXX_STATUS_OK) {}
  
        }    
}

out:

    ifx_avian_destroy(device_handle);
    ifx_rai_destroy(raiHandler);
    // ifx_cube_destroy_r(frame);
    ESP_LOGI("Out","Occurs");
    return 0;  
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
        .clock_speed_hz= 25*1000*1000,   
        .mode=0,                                //SPI mode 0
        #ifndef XNSIV_CS_HANDLING
        // .spics_io_num = XNSIV_BGT60_CS_GPIO,      //CS pin
        #endif
        .queue_size=500,                          //We want to be able to queue 10 transactions at a time
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
    // printf("sample_rate_Hz:          %" PRIu32 "\n", config->sample_rate_Hz);
    // printf("rx_mask:                 %" PRIu32 "\n", config->rx_mask);
    // printf("tx_mask:                 %" PRIu32 "\n", config->tx_mask);
    printf("tx_power_level:          %" PRIu32 "\n", config->tx_power_level);
    printf("if_gain_dB:              %" PRIu32 "\n", config->if_gain_dB);
    printf("start_frequency_Hz:      %" PRIu64 "\n", config->start_frequency_Hz);
    printf("end_frequency_Hz:        %" PRIu64 "\n", config->end_frequency_Hz);
    printf("num_samples_per_chirp:   %" PRIu32 "\n", config->num_samples_per_chirp);
    printf("num_chirps_per_frame:    %" PRIu32 "\n", config->num_chirps_per_frame);
    printf("chirp_repetition_time_s: %g\n", config->chirp_repetition_time_s);
    printf("frame_repetition_time_s: %g\n", config->frame_repetition_time_s);
    // printf("hp_cutoff_Hz:            %" PRIu32 "\n", config->hp_cutoff_Hz);
    // printf("aaf_cutoff_Hz:           %" PRIu32 "\n", config->aaf_cutoff_Hz);
    // printf("mimo_mode:               %s\n", config->mimo_mode == IFX_MIMO_TDM ? "time-domain multiplexed" : "off");
    printf("\n");
}

