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
#include "RadarSegmentation.h"
#include "PresenceSensing.h"

#define SAMP_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP)
#define TOTAL_DATA (SAMP_PER_FRAME + (6 * XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME))
static uint16_t samples[TOTAL_DATA];
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
TaskHandle_t getFIFOHandle = NULL;
void getFIFO(void* pvParameter);
bool frameProcFlag = 0;
volatile bool data_available = false;

/*
==============================================================================
   6. LOCAL FUNCTIONS
==============================================================================
*/
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    if(gpio_num == 13) {
        data_available = true;
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

ifx_Error_t presence_init(presence_t* ctx)
{
    ctx->presencesensing = NULL;

    return IFX_OK;
}

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

ifx_Error_t presence_cleanup(presence_t* ctx)
{
    ifx_presence_sensing_destroy(ctx->presencesensing);

    return ifx_error_get();
}

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
        if (!isnan(ps_result.target_distance_m) && !isnan(ps_result.target_speed_m_s)) 
        {   
            ESP_LOGI("PresenceSensing","Number of targests found: %d", ps_result.num_targets);
            ESP_LOGI("PresenceSensing","target_distance_m: %g, target_speed_m_s: %g, target_signal_strength: %g",
            ps_result.target_distance_m, ps_result.target_speed_m_s, ps_result.target_signal_strength);    
        }
    }

    else{ 
        // printf("is_present: false\r\n");
        gpio_set_level(XNSIV_BGT60_LED_RED, 0);
    }

    // printf("Number of targests found: %d\r\n", ps_result.num_targets);

    return ifx_error_get();
}

/*
==============================================================================
   7. MAIN METHOD
==============================================================================
 */
ifx_Avian_Device_t* device_handle = NULL;
ifx_Cube_R_t* frame = NULL;
extern int64_t timeduration;
int app_main(int argc, char* argv[])
{
    
    ifx_Error_t error = IFX_OK;
    ifx_Avian_Config_t device_config = {0};
  
    xnsiv_bgt60tr13c = (xensiv_bgt60trxx_t *)malloc(sizeof(xensiv_bgt60trxx_t));

    gpio_reset_pin(XNSIV_BGT60_LED_RED);
    gpio_reset_pin(XNSIV_BGT60_LED_GREEN);

    gpio_set_direction(XNSIV_BGT60_LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(XNSIV_BGT60_LED_GREEN, GPIO_MODE_OUTPUT);

    xTaskCreatePinnedToCore(getFIFO,"getFIFO", 2048*3, NULL, 10, &getFIFOHandle, 1);
    init_gpio();
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

    ifx_avian_get_config(device_handle, &device_config);

    // presence_t* Presence = (presence_t*)malloc(sizeof(presence_t));
    // ifx_Avian_Config_t* dev_config = (ifx_Avian_Config_t*)malloc(sizeof(ifx_Avian_Config_t));

    // if (presence_init(Presence) != IFX_OK) {
    //     TRACE_B("Failed to Init Presence object!\n");     
    // }

    // if (presence_config(Presence, device_handle, dev_config) != IFX_OK) {
    //     TRACE_B("Failed to config presene sensor!\n");        
    // }
    
    xensiv_bgt60trxx_set_fifo_limit(device_handle->m_xensiv_device_handle, SAMP_PER_FRAME);

    uint32_t reg_read = 0;
    if (xensiv_bgt60trxx_start_frame(device_handle->m_xensiv_device_handle, true) != XENSIV_BGT60TRXX_STATUS_OK) {
        TRACE_B("Error stopping the frame.");
    } 
    else {
        TRACE_B("Frame started !!");
    }
    // uint32_t delayCounter=0;
    while(1) 
    {
        // if(delayCounter < 10)
        //  {
        //     delayCounter++;
        //     gpio_set_level(XNSIV_BGT60_LED_GREEN, 1);
        //  }
        //  else if(delayCounter < 20){
        //     delayCounter++;
        //     gpio_set_level(XNSIV_BGT60_LED_GREEN, 0);
        //     // ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
        //  }
        //  else
        //  {
        //     delayCounter =0;
        //  }
         
        // while(frameProcFlag == false);
        
        // if(presence_process(Presence, frame) != IFX_OK) 
        // {
        //     TRACE_B("Failed processing presence!\n");    
        //     break;    
        // }
        // frameProcFlag = false;   
        vTaskDelay(10/ portTICK_RATE_MS);     
    }

out:

    ifx_avian_destroy(device_handle);
    // ifx_cube_destroy_r(frame);
    // presence_cleanup();
    return 0;
}

void getFIFO(void* pvParameter)
{
    esp32_spi_init();

    ifx_Error_t error = IFX_OK;

    // ifx_Cube_R_t* frame1 = NULL;
    // frame1 = (ifx_Avian_Device_t *)malloc(sizeof(ifx_Avian_Device_t));      
    // frame1->d = (ifx_Float_t *)malloc(sizeof(ifx_Float_t) * SAMP_PER_FRAME);

    // ifx_Cube_R_t* frame2 = NULL;
    // frame2 = (ifx_Avian_Device_t *)malloc(sizeof(ifx_Avian_Device_t));
    // frame2->d = (ifx_Float_t *)malloc(sizeof(ifx_Float_t) * SAMP_PER_FRAME);
    // uint16_t samples[SAMP_PER_FRAME];
    // bool frameNum = 1;
    // vTaskDelay(100/portTICK_RATE_MS);
    // uint16_t delayCounter = 0;
    while(1)
    {
        // ** GPIO TOGGLE
        // if(delayCounter < 1000)
        //  {
        //     delayCounter++;
        //     gpio_set_level(XNSIV_BGT60_LED_GREEN, 1);
        //  }
        //  else if(delayCounter < 2000){
        //     delayCounter++;
        //     gpio_set_level(XNSIV_BGT60_LED_GREEN, 0);
        //     // ESP_LOGI("HeapMemory : ", "%X", xPortGetFreeHeapSize());
        //  }
        //  else
        //  {
        //     delayCounter =0;
        //  }
        
        while(data_available == false);
        // timeduration = esp_timer_get_time();
        data_available = false;
        
            // ESP_LOGI("getFifo","CREF INT");
        
        // if(frameNum)
        // {
            // memset(frame1->d, 0, sizeof(float) * SAMP_PER_FRAME);
            // frame1 = ifx_avian_get_next_frame(device_handle, frame1);
        int ret = xensiv_bgt60trxx_get_fifo_data(device_handle->m_xensiv_device_handle, samples, TOTAL_DATA);
        // ESP_LOGI("", "%d",(int)(esp_timer_get_time() - timeduration));
        if(ret != 0)
        {
            ESP_LOGI("FIFO","failed!");
        }
        
        // ESP_LOGI("", "%d",(int)(esp_timer_get_time() - timeduration));
            // while(frameProcFlag == 1);
            // frame = frame1;
    //         frameNum = 0;
    //     }
    //     else
    //     {
    //         // memset(frame2->d, 0, sizeof(float) * SAMP_PER_FRAME);
    //         frame2 = ifx_avian_get_next_frame(device_handle, frame2);
    //         // while(frameProcFlag == 1);
    //         // frame = frame2;
    //         frameNum = 1;
    //     }

    //     if ((error = ifx_error_get()) != IFX_OK)
    //     {
    //         // ESP_LOGI("getFIFOTask","Failed to get next frame: %s\n", ifx_error_to_string(error));
    //         continue;
    //     }
    //     // frameProcFlag = true;
        
    }
}
