/* ===========================================================================
** Copyright (C) 2021-2022 Infineon Technologies AG
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** ===========================================================================
*/

/**
 * @file    app_raw_data.c
 *
 * @brief   Raw data application main program source file.
 *
 * This example illustrates how to fetch time-domain data from an Avian FMCW
 * radar sensor like BGT60TR13, BGT60UTR11AIP or BGT60ATR24 using the Radar SDK.
 */

/*
==============================================================================
   1. INCLUDE FILES
==============================================================================
*/

#if 0

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Base.h"
#include "Avian.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "debug.h"

#include "xensiv_bgt60trxx_esp32.h"
#include "xensiv_bgt60trxx.h"

#include "presence_radar_settings.h"

#define ESP_INTR_FLAG_DEFAULT 0

xensiv_bgt60trxx_t * xnsiv_bgt60tr13c = NULL;
spi_device_handle_t xnsiv_bgt60_spi;

static xQueueHandle gpio_evt_queue = NULL;

volatile bool data_available = false;

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

/*
==============================================================================
   2. LOCAL DEFINITIONS
==============================================================================
*/

#define NUM_FETCHED_FRAMES  10  /**< Number of frames to fetch */

/*
==============================================================================
   6. LOCAL FUNCTIONS
==============================================================================
*/

/**
 * @brief Helper function to process data
 *
 * This function is an example showing a possible way
 * of processing antenna signal. The goal in this example is to
 * sum up all chirps into one vector.
 *
 * @param antenna_data data from one antenna containing multiple chirps
 */
static void process_antenna_data(const ifx_Matrix_R_t* antenna_data)
{
    ifx_Vector_R_t chirp = {0};
    // Create the sum vector
    ifx_Vector_R_t* sum = ifx_vec_create_r(antenna_data->cols);

    // Iterate through all chirps
    for(uint32_t i = 0; i < IFX_MAT_ROWS(antenna_data); i++)
    {
        // Fetch a chirp from the antenna data matrix
        ifx_mat_get_rowview_r(antenna_data, i, &chirp);
        // add it to the sum vector
        ifx_vec_add_r(&chirp, sum, sum);
    }
    
    // Divide the sum vector element wise by number of chirps in the antenna data
    ifx_vec_scale_r(sum, 1.0f / IFX_MAT_ROWS(antenna_data), sum);

    for(uint32_t i = 0; i < IFX_VEC_LEN(sum); i++)
    {
        printf("%.4f ", IFX_VEC_AT(sum, i));
    }
    printf("\n\n");
    ifx_vec_destroy_r(sum);
}

//----------------------------------------------------------------------------

/**
 * @brief Helper function to separate signals
 *
 * Separates different antenna signals
 * and pass them for further processing.
 *
 * @param frame The frame may contain multiple antenna signals,
 *              depending on the device configuration.
 *              Each antenna signal can contain multiple chirps.
 */
static void process_frame(ifx_Cube_R_t* frame)
{
    ifx_Matrix_R_t antenna_data;
    
    for (uint32_t i = 0; i < IFX_CUBE_ROWS(frame); i++)
    {
        ifx_cube_get_row_r(frame, i, &antenna_data);
        process_antenna_data(&antenna_data);
    }
}

/**
 * @brief Helper function to print device configuration
 *
 * Print the device configuration to stdout.
 *
 * @param config        device configuration
 */
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

int app_main(int argc, char** argv)
{
    ifx_Error_t error = IFX_OK;
    ifx_Avian_Config_t device_config = {0};
    ifx_Avian_Device_t* device_handle = NULL;
    ifx_Cube_R_t* frame = NULL;

    xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)malloc(sizeof(xensiv_bgt60trxx_t));

    gpio_reset_pin(XNSIV_BGT60_LED_RED);
    gpio_reset_pin(XNSIV_BGT60_LED_GREEN);

    gpio_set_direction(XNSIV_BGT60_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(XNSIV_BGT60_LED_GREEN, GPIO_MODE_OUTPUT);

    init_gpio();
    esp32_spi_init();
    bgt60_hard_reset();

    xensiv_bgt60trxx_init(xnsiv_bgt60tr13c, (void *)&xnsiv_bgt60_spi, true);

    xensiv_bgt60trxx_config(xnsiv_bgt60tr13c, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
    
    uint32_t read_reg = 0;
    xensiv_bgt60trxx_get_reg(xnsiv_bgt60tr13c, XENSIV_BGT60TRXX_REG_SFCTL, &read_reg);
    read_reg |= XENSIV_BGT60TRXX_REG_SFCTL_PREFIX_EN_MSK;  
    xensiv_bgt60trxx_set_reg(xnsiv_bgt60tr13c, XENSIV_BGT60TRXX_REG_SFCTL, read_reg);

    xTaskCreate(vTaskLED_Toggle, "LED_TOGGLE", 1024, NULL, tskIDLE_PRIORITY, NULL);

    printf("Radar SDK Version: %s\n", ifx_sdk_get_version_string_full());

    /* Open the device: Connect to the first radar sensor found. */
    device_handle = ifx_avian_create_dummy_from_xensiv(xnsiv_bgt60tr13c);
    if ((error = ifx_error_get()) != IFX_OK)
    {
        fprintf(stderr, "Failed to open device: %s\n", ifx_error_to_string(error));
        goto out;
    }

    /* Get default device configuration for connected radar sensor.
     * Here, we use the default configuration that depends on the connected
     * radar sensor.
     * Typically, the radar sensor is either configured by setting all members
     * of the device configuration (ifx_Avian_Config_t) or by setting the
     * members of a structure of type ifx_Avian_Metrics_t and converting the
     * structure to a device configuration using the function
     * ifx_avian_metrics_to_config. The latter approach using the
     * ifx_Avian_Metrics_t structure provides an easy way to configure the
     * radar sensor in terms of high-level parameters, while the device
     * configuration structure ifx_Avian_Config_t allows to configure the
     * sensor in a more detailed way.
     */
    ifx_avian_get_config_defaults(device_handle, &device_config);
    if ((error = ifx_error_get()) != IFX_OK)
    {
        fprintf(stderr, "Failed to get default device config:  %s\n", ifx_error_to_string(error));
        goto out;
    }

    /* Apply the device settings based on the device configuration structure. */
    // ifx_avian_set_config(device_handle, &device_config);
    // if ((error = ifx_error_get()) != IFX_OK)
    // {
    //     fprintf(stderr, "Failed to set device config:  %s\n", ifx_error_to_string(error));
    //     goto out;
    // }

    /* Print the current device configuration to stdout:
     * Read back the current device configuration. Due to discrete register
     * values in the radar sensors, the device configuration passed to
     * ifx_avian_set_config and the actual device configuration set might
     * differ slightly.
     * We then print the read-back device configuration to stdout.
     */
    ifx_avian_get_config(device_handle, &device_config);
    print_device_config(&device_config);
    
    /* Fetch NUM_FETCHED_FRAMES number of frames. */
    frame = (ifx_Avian_Device_t *)malloc(sizeof(ifx_Avian_Device_t));    
    
    xensiv_bgt60trxx_set_fifo_limit(xnsiv_bgt60tr13c, 4096);

    for (int frame_number = 0; frame_number < NUM_FETCHED_FRAMES; frame_number++)
    {
        /* Get the time-domain data for the next frame. The function will block
         * until the full frame is available and copy the data into the frame
         * handle.
         * This function also creates a frame structure for time domain data
         * acquisition, if not created already. It is the responsibility of
         * the caller to free the returned frame in this scope.
         */
        frame->d = (ifx_Float_t *)malloc(sizeof(ifx_Float_t) * 8192);        
        
        xensiv_bgt60trxx_soft_reset(xnsiv_bgt60tr13c, XENSIV_BGT60TRXX_RESET_FIFO);
        
        if (xensiv_bgt60trxx_start_frame(xnsiv_bgt60tr13c, true) != XENSIV_BGT60TRXX_STATUS_OK) {
            TRACE_B("Error stopping the frame.");
        }

        while(data_available == false);
        data_available = false;
        // TRACE_B("Data available !!!");

        frame = ifx_avian_get_next_frame(device_handle, frame);  

        if ((error = ifx_error_get()) != IFX_OK)
        {
            fprintf(stderr, "Failed to get next frame: %s\n", ifx_error_to_string(error));
            goto out;
        }

        if (xensiv_bgt60trxx_start_frame(xnsiv_bgt60tr13c, false) != XENSIV_BGT60TRXX_STATUS_OK) {
            TRACE_B("Error stopping the frame.");
        }
        /* Process the frame. */
        process_frame(frame);        
        free(frame->d);
        frame->d = NULL;
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }

out:
    /* Close the device after processing all frames. It is valid to pass NULL
     * to destroy functions.
     */
    if(frame->d != NULL) {
        free(frame->d);
    }
    ifx_avian_destroy(device_handle);
    ifx_cube_destroy_r(frame);
    printf("Program exit: %s, %d\r\n", __FILE__, __LINE__);

    return error == IFX_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif