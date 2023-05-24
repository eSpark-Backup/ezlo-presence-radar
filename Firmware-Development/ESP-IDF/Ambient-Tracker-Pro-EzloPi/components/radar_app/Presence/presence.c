#include "presence.h"

static const char* TAG = "Presence";

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
            ESP_LOGI(TAG, "change: present");
            break;
        case IFX_PRESENCE_SENSING_ABSENT:
            ESP_LOGI(TAG, "change: absent");
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
    ifx_PresenceSensing_Config_t ps_config = { 0 };
    ifx_Radar_Sensor_t sensor = IFX_AVIAN_BGT60TR13C;
   
    ifx_presence_sensing_get_config_defaults(sensor, dev_config, &ps_config); 
    ps_config.state_change_cb = state_change_callback;
    dev_config->rx_mask = 1;

    // ctx->presencesensing = ifx_presence_sensing_create(&ps_config);
    ctx->presencesensing = ifx_presence_sensing_create(&ps_config); 

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
    {
        return ifx_error_get();
    }        

    #if PRESENCE_LED
    if (ps_result.cur_presence_state == IFX_PRESENCE_SENSING_PRESENT)
    {
        gpio_set_level(XNSIV_BGT60_LED_RED, 1);
        gpio_set_level(XNSIV_BGT60_LED_GREEN, 1);
    }
    else
    { //if(cur_state == ABSENCE)
        gpio_set_level(XNSIV_BGT60_LED_RED, 0);
        gpio_set_level(XNSIV_BGT60_LED_GREEN, 0);
    }
    #endif

    // printf("Number of targests found: %d\r\n", ps_result.num_targets);

    // if (!isnan(ps_result.target_distance_m) && !isnan(ps_result.target_speed_m_s)) {           
    //     printf("target_distance_m: %g, target_speed_m_s: %g, target_signal_strength: %g",
    //         ps_result.target_distance_m, ps_result.target_speed_m_s, ps_result.target_signal_strength);
    //     printf("\r\n");
    // }

    presence_context->presence_status = ps_result.cur_presence_state;
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