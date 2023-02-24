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

#define SAMP_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP)
#define TOTAL_DATA (SAMP_PER_FRAME + (6 * XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME))
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
typedef struct {
    ifx_PresenceSensing_t* presencesensing;
} presence_t;

/*
==============================================================================
   4. LOCAL DATA
==============================================================================
*/
xensiv_bgt60trxx_t * xnsiv_bgt60tr13c = NULL;
spi_device_handle_t xnsiv_bgt60_spi;

static xQueueHandle gpio_evt_queue = NULL;

volatile bool data_available = false;

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



/**
 * @brief Callback function to print the information about presence state
 *
 * @param new_state     new presence sensing state
 * @param context       context of the application
 */
static void state_change_callback(ifx_PresenceSensing_State_t new_state, void* context)
{
    (void)context;
    switch(new_state){
        case IFX_PRESENCE_SENSING_PRESENT:
            TRACE_B("change: present");
            break;
        case IFX_PRESENCE_SENSING_ABSENT:
            TRACE_B("change: absent");
            break;
        default:
            break;
    }
}

//----------------------------------------------------------------------------

/**
 * @brief Application specific initialization function
 *
 * Initializes the presence sensing context.
 *
 * @param ctx           context of the application
 */
ifx_Error_t presence_init(presence_t* ctx)
{
    ctx->presencesensing = NULL;

    return IFX_OK;
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
ifx_Error_t presence_config(presence_t* ctx, ifx_Avian_Device_t* device, ifx_Avian_Config_t* dev_config)
{   
    ifx_Error_t err = IFX_OK;
    ifx_error_clear();
    ifx_PresenceSensing_Config_t ps_config = { 0 };

    ifx_Radar_Sensor_t sensor = IFX_AVIAN_BGT60TR13C;

    err = ifx_error_get();    
    TRACE_B("Error: %d:%s\n", err, ifx_error_to_string(err));    
    
    ifx_presence_sensing_get_config_defaults(sensor, dev_config, &ps_config);

    err = ifx_error_get();    
    TRACE_B("Error: %d:%s\n", err, ifx_error_to_string(err));    
    ps_config.state_change_cb = state_change_callback;
    
    #if 0
    if(device) {
        //We use always last antenna as first one on some devices have leakage from tx (BGT60TR13C)
        const ifx_Radar_Sensor_Info_t* sensor_info = ifx_avian_get_sensor_information(device);
        dev_config->rx_mask = 1<<(sensor_info->num_rx_antennas-1u);
    }
    #endif
    
    dev_config->rx_mask = 1;

    // ctx->presencesensing = ifx_presence_sensing_create(&ps_config);
    ctx->presencesensing = ifx_presence_sensing_create(&ps_config);
    err = ifx_error_get();    
    TRACE_B("Error: %d:%s\n", err, ifx_error_to_string(err));    
  
    return ifx_error_get();
}

/**
 * @brief Application specific cleanup function
 *
 * Destroys the presence sensing instances and cleans up allocated memory.
 *
 * @param ctx           context of the application
 */
ifx_Error_t presence_cleanup(presence_t* ctx)
{
    ifx_presence_sensing_destroy(ctx->presencesensing);

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
ifx_Error_t presence_process(presence_t* presence_context, ifx_Cube_R_t* frame)
{
    ifx_Matrix_R_t antenna_data;
    
    ifx_PresenceSensing_Result_t ps_result;

    ifx_cube_get_row_r(frame, RX_ANTENNA_ID, &antenna_data);

    ifx_presence_sensing_run(presence_context->presencesensing, &antenna_data, &ps_result);
    
    
    if (ifx_error_get() != IFX_OK)
        return ifx_error_get();
    // printf("----------------------------------------------\r\n");
    if (ps_result.cur_presence_state == IFX_PRESENCE_SENSING_PRESENT)
    {
        // printf("is_present: true\r\n");
        gpio_set_level(XNSIV_BGT60_LED_RED, 1);
        // gpio_set_level(XNSIV_BGT60_LED_GREEN, 1);
    }

    else{ //if(cur_state == ABSENCE)
        // printf("is_present: false\r\n");
        gpio_set_level(XNSIV_BGT60_LED_RED, 0);
        // gpio_set_level(XNSIV_BGT60_LED_GREEN, 0);
    }

    // printf("Number of targests found: %d\r\n", ps_result.num_targets);

    if (!isnan(ps_result.target_distance_m) && !isnan(ps_result.target_speed_m_s)) {           
        printf("target_distance_m: %g, target_speed_m_s: %g, target_signal_strength: %g",
            ps_result.target_distance_m, ps_result.target_speed_m_s, ps_result.target_signal_strength);
        printf("\r\n");
    }
    return ifx_error_get();
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
    ifx_Cube_R_t* frame = NULL;

    xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)malloc(sizeof(xensiv_bgt60trxx_t));

    gpio_reset_pin(XNSIV_BGT60_LED_RED);
    gpio_reset_pin(XNSIV_BGT60_LED_GREEN);

    gpio_set_direction(XNSIV_BGT60_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(XNSIV_BGT60_LED_GREEN, GPIO_MODE_OUTPUT);

    init_gpio();
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
    read_reg |= XENSIV_BGT60TRXX_REG_SFCTL_PREFIX_EN_MSK;  
    xensiv_bgt60trxx_set_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, read_reg);

    // xTaskCreate(vTaskLED_Toggle, "LED_TOGGLE", 1024, NULL, tskIDLE_PRIORITY, NULL);

    // printf("Radar SDK Version: %s\n", ifx_sdk_get_version_string_full());


    // ifx_avian_get_config_defaults(device_handle, &device_config);
    // if ((error = ifx_error_get()) != IFX_OK)
    // {
    //     fprintf(stderr, "Failed to get default device config:  %s\n", ifx_error_to_string(error));
    //     goto out;
    // }

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
    // print_device_config(&device_config);

    presence_t* Presence = (presence_t*)malloc(sizeof(presence_t));
    ifx_Avian_Config_t* dev_config = (ifx_Avian_Config_t*)malloc(sizeof(ifx_Avian_Config_t));

    if (presence_init(Presence) != IFX_OK) {
        TRACE_B("Failed to Init Presence object!\n");     
    }

    if (presence_config(Presence, device_handle, dev_config) != IFX_OK) {
        TRACE_B("Failed to config presene sensor!\n");        
    }


    /* Fetch NUM_FETCHED_FRAMES number of frames. */
    frame = (ifx_Avian_Device_t *)malloc(sizeof(ifx_Avian_Device_t));    
    
    xensiv_bgt60trxx_set_fifo_limit(device_handle->m_xensiv_device_handle, SAMP_PER_FRAME);

    uint32_t reg_read = 0;
    if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK) {
        TRACE_B("Error stopping the frame.");
    } 
    else {
        TRACE_B("Frame started !!");
    }
    uint32_t delayCounter=0;
    while(1) {
        if(delayCounter < 50)
         {
            delayCounter++;
            gpio_set_level(XNSIV_BGT60_LED_GREEN, 1);
         }
         else if(delayCounter < 100){
            delayCounter++;
            gpio_set_level(XNSIV_BGT60_LED_GREEN, 0);
            // ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
         }
         else
         {
            delayCounter =0;
         }
         

        frame->d = (ifx_Float_t *)malloc(sizeof(ifx_Float_t) * SAMP_PER_FRAME);
        memset(frame->d, 0, sizeof(float) * SAMP_PER_FRAME);
                

        // if(xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_FSTAT_TR13C, &reg_read) != XENSIV_BGT60TRXX_STATUS_OK) {
        //     TRACE_B("Error reading FSTAT Register");
        // }
        // TRACE_B("FSTAT: 0x%x", reg_read);

        // reg_read = 0;
        // if(xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_STAT1, &reg_read) != XENSIV_BGT60TRXX_STATUS_OK) {
        //     TRACE_B("Error reading STAT 1 Register");
        // }
        // TRACE_B("STAT 1: 0x%x", reg_read);

        

        while(data_available == false);
        data_available = false;   
        TRACE_B("Data available !!!");        
        
        reg_read = 0;
        if(xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_FSTAT_TR13C, &reg_read) != XENSIV_BGT60TRXX_STATUS_OK) {
            TRACE_B("Error reading FSTAT Register");
        }        
        TRACE_B("FSTAT: 0x%x", reg_read); 

        if((reg_read & 0x3FFF) < SAMP_PER_FRAME/2) {
            free(frame->d);
            frame->d = NULL;
            continue; 
        } 

        // reg_read = 0;
        // if(xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_SFCTL, &reg_read) != XENSIV_BGT60TRXX_STATUS_OK) {
        //         TRACE_B("Error reading FSTAT Register");
        // }
        // TRACE_B("SFCTL: 0x%x", reg_read);      

        // reg_read = 0;
        // if(xensiv_bgt60trxx_get_reg(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_REG_STAT1, &reg_read) != XENSIV_BGT60TRXX_STATUS_OK) {
        //     TRACE_B("Error reading STAT 1 Register");
        // }
        // TRACE_B("STAT 1: 0x%x", reg_read);

        // reg_read = 0;
        // xensiv_bgt60trxx_get_fifo_status(device_handle->m_xensiv_device_handle, &reg_read);
        // TRACE_B("FIFO status: 0x%x", reg_read);            
        

        frame = ifx_avian_get_next_frame(device_handle, frame);  
        if ((error = ifx_error_get()) != IFX_OK)
        {
            TRACE_B("Failed to get next frame: %s\n", ifx_error_to_string(error));
            break;
        }

        // if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, false) != XENSIV_BGT60TRXX_STATUS_OK) {
        //     TRACE_B("Error stopping the frame.");
        // }        
        // xensiv_bgt60trxx_soft_reset(device_handle->m_xensiv_device_handle, XENSIV_BGT60TRXX_RESET_FIFO);
        if(presence_process(Presence, frame) != IFX_OK) {
            TRACE_B("Failed processing presence!\n");    
            break;    
        }        
        // vTaskDelay(100 / portTICK_PERIOD_MS);
        free(frame->d);
        frame->d = NULL;
    }

out:

    ifx_avian_destroy(device_handle);
    ifx_cube_destroy_r(frame);
    // presence_cleanup();
    return 0;
}
