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
#include "PresenceSensing.h"

/*
==============================================================================
   2. LOCAL DEFINITIONS
==============================================================================
*/
#define RX_ANTENNA_ID   0   // This implementation uses only 1 Rx Antenna

#define ESP_INTR_FLAG_DEFAULT 0

/*
==============================================================================
   4. LOCAL DATA
==============================================================================
*/
xensiv_bgt60trxx_t * xnsiv_bgt60tr13c = NULL;
spi_device_handle_t xnsiv_bgt60_spi;

static xQueueHandle gpio_evt_queue = NULL;

volatile bool data_available = false;

#define NUM_SAMPLES_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS *\
                                             XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME *\
                                             XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP)
/* Allocate enough memory for the radar dara frame. */
// static uint16_t samples[NUM_SAMPLES_PER_FRAME];

/*
==============================================================================
   5. LOCAL FUNCTION PROTOTYPES
==============================================================================
*/

/*
==============================================================================
   6. LOCAL FUNCTIONS
==============================================================================
*/
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void vTaskLED_Toggle(void * pvParameters) {
  
  bool stat_red_led, stat_green_led;
  
  stat_red_led = true;
  stat_green_led = false;

  while(1) {
    stat_red_led = !stat_red_led;
    stat_green_led = !stat_green_led;
    gpio_set_level(XNSIV_BGT60_LED_RED, stat_red_led);
    gpio_set_level(XNSIV_BGT60_LED_GREEN, stat_green_led);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

static void gpio_irq_task(void* arg)
{
    uint32_t io_num;

    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            // TRACE_B("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            if(io_num == 1) {
              TRACE_B("Software restarting the ESP32 device.");
              esp_restart();
            } else if(io_num == 13) {
              data_available = true;
            }          
        }
    }
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
  //start gpio task
  xTaskCreate(gpio_irq_task, "FIFO Filled", 2048, NULL, 10, NULL);

  //install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(XNSIV_BGT60_IRQ_GPIO, gpio_isr_handler, (void*) XNSIV_BGT60_IRQ_GPIO);
  gpio_isr_handler_add(XNSIV_BGT60_BUTTON_GPIO, gpio_isr_handler, (void*) XNSIV_BGT60_BUTTON_GPIO);

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
    
    // * Test Mode * //
    if (xensiv_bgt60trxx_enable_data_test_mode(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        ESP_LOGE("TestMode","Failed to enable.");
    }
    else
    {
        ESP_LOGE("TestMode","Enabled.");
    }
    uint32_t frame_idx = 0;
    uint16_t test_word = XENSIV_BGT60TRXX_INITIAL_TEST_WORD;

    if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK) {
        TRACE_B("Error stopping the frame.");
    } else 
    {
        TRACE_B("Frame started !!");
    }
    // * Test Mode * //

    uint32_t reg_read = 0;
    while(1) {
        
        while(data_available == false);
        data_available = false;   
        TRACE_B("Data available !!!");        
        
        
        //if ((error = ifx_error_get()) != IFX_OK)
        //{
          //  TRACE_B("Failed to get next frame: %s\n", ifx_error_to_string(error));
            //break;
        //}

        // if(presence_process(Presence, frame) != IFX_OK) {
        //     TRACE_B("Failed processing presence!\n");    
        //     break;    
        // }
        // xensiv_bgt60trxx_soft_reset(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_RESET_FIFO);
        // ESP_LOGI("get_fifo","Before!!!"); 
         
        ESP_LOGI();
        uint16_t samples[NUM_SAMPLES_PER_FRAME];
        int ret = xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples,
                                           NUM_SAMPLES_PER_FRAME);
        if (xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_STAT1, &reg_read) != XENSIV_BGT60TRXX_STATUS_OK)
        {
            TRACE_B("Error reading STAT 1 Register");
        }
        TRACE_B("STAT 1: 0x%x", reg_read);
        // ESP_LOGI("get_fifo","After!!!");
        // ESP_LOGI("fifo,"ret =");                                           
        if ( ret == XENSIV_BGT60TRXX_STATUS_OK)
        {
            ESP_LOGI("FIFO","RECVVVVVVVVVVV");
            /* Check received data */
            for (int32_t sample_idx = 0; sample_idx < NUM_SAMPLES_PER_FRAME; ++sample_idx)
            {
                    if (test_word != samples[sample_idx])
                    {
                        ESP_LOGI("TestMode","Frame %" PRIu32 " error detected. "
                               "Expected: %" PRIu16 ". "
                               "Received: %" PRIu16 "\n",
                               frame_idx, test_word, samples[sample_idx]);
                        // CY_ASSERT(0);
                    }

                    // Generate next test_word
                    test_word = xensiv_bgt60trxx_get_next_test_word(test_word);
            }
        }

        // if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, false) != XENSIV_BGT60TRXX_STATUS_OK) {
        //     TRACE_B("Error stopping the frame.");
        // }        
        else
        {
            ESP_LOGI("fifo","ret =%d", ret);
            ESP_LOGI("fifo","ret =%d", ret);
            ESP_LOGI("fifo","ret =%d", ret);
        }
            // ESP_LOGI("TestMode","Frame %" PRIu32 " received correctly\n", frame_idx);
        frame_idx++;
                        // xensiv_bgt60trxx_soft_reset(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_RESET_FIFO);

        }        

        // vTaskDelay(100 / portTICK_PERIOD_MS);
        // free(frame->d);
        // frame->d = NULL;

out:

    ifx_avian_destroy(device_handle);
    // ifx_cube_destroy_r(frame);
    // presence_cleanup();
    return 0;
}
