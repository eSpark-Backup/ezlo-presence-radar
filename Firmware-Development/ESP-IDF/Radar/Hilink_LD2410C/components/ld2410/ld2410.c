#include "ld2410.h"

static const char* TAG = "ld2410";
static uint32_t last_reading = 0;

esp_err_t ld2410_setup(void)
{
    printf("\nConnect LD2410 radar TX to GPIO: %d\n", RADAR_RX_PIN);
    printf("Connect LD2410 radar RX to GPIO: %d\n", RADAR_TX_PIN);
    printf("LD2410 radar sensor initialising:\n");
    if(ld2410_begin(true))
    {
        printf("OK\n");
        // ld2410_set_max_values(DISTANCE_600CM, DISTANCE_525CM, 5); // (max_moving_distance, max_still_distance, no_one_duration in seconds)

        // uint8_t mov_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20}; // Gate 0 to 8 sensitivity : sensitivity range 0-100
        // uint8_t still_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20}; // Gate 0 to 8 sensitivity : sensitivity range 0-100
        // ld2410_set_gates_sensitivity_threshold(mov_sensitivity, still_sensitivity);
        ld2410_settings_t settings = 
        {
            .max_still_distance = DISTANCE_525CM,
            .max_move_distance = DISTANCE_525CM,
            .min_still_distance = DISTANCE_300CM,
            .min_move_distance = DISTANCE_300CM,
            .no_one_duration = 5
        };
        ld2410_set_template(CUSTOM_TEMPLATE, &settings);
        return ESP_OK;
    }
    else
    {
        printf("not connected\n");
    }
    return ESP_FAIL;
}

esp_err_t ld2410_set_template(ld2410_template_t template, ld2410_settings_t* p_settings)
{
    ld2410_settings_t settings = 
    {
        .max_still_distance = DISTANCE_600CM,
        .max_move_distance = DISTANCE_600CM,
        .no_one_duration = 5
    };
    uint8_t move_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20};    // Gate 0 to 8 sensitivity : sensitivity range 0-100
    uint8_t still_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20};  // Gate 0 to 8 sensitivity : sensitivity range 0-100
    
    switch (template)
    {
        case SLEEP_TEMPLATE_CLOSE_RANGE:
            settings.no_one_duration = 180; // 180 seconds
            settings.max_still_distance = DISTANCE_225CM;
            settings.max_move_distance = DISTANCE_225CM;
            break;
        
        case SLEEP_TEMPLATE_MID_RANGE:
            settings.no_one_duration = 180; // 180 seconds
            settings.max_still_distance = DISTANCE_375CM;
            settings.max_move_distance = DISTANCE_375CM;
            break;
        
        case SLEEP_TEMPLATE_LONG_RANGE:
            settings.no_one_duration = 180; // 180 seconds
            settings.max_still_distance = DISTANCE_600CM;
            settings.max_move_distance = DISTANCE_600CM;
            break;
        
        case MOVEMENT_TEMPLATE_CLOSE_RANGE:
            settings.no_one_duration = 2; // 2 seconds
            settings.max_still_distance = DISTANCE_75CM;
            settings.max_move_distance = DISTANCE_225CM;
            break;
        
        case MOVEMENT_TEMPLATE_MID_RANGE:
            settings.no_one_duration = 2; // 2 seconds
            settings.max_still_distance = DISTANCE_75CM;
            settings.max_move_distance = DISTANCE_375CM;
            break;
        
        case MOVEMENT_TEMPLATE_LONG_RANGE:
            settings.no_one_duration = 2; // 2 seconds
            settings.max_still_distance = DISTANCE_75CM;
            settings.max_move_distance = DISTANCE_600CM;
            break;
        
        case BASIC_TEMPLATE_CLOSE_RANGE:
            settings.no_one_duration = 5; // 5 seconds
            settings.max_still_distance = DISTANCE_225CM;
            settings.max_move_distance = DISTANCE_225CM;
            break;
        
        case BASIC_TEMPLATE_MID_RANGE:
            settings.no_one_duration = 5; // 5 seconds
            settings.max_still_distance = DISTANCE_375CM;
            settings.max_move_distance = DISTANCE_375CM;
            break;
        
        case BASIC_TEMPLATE_LONG_RANGE:
            settings.no_one_duration = 5; // 5 seconds
            settings.max_still_distance = DISTANCE_600CM;
            settings.max_move_distance = DISTANCE_600CM;
            break;
        
        case CUSTOM_TEMPLATE:
            if (p_settings)
            {
                settings.no_one_duration = p_settings->no_one_duration;
                settings.max_still_distance = p_settings->max_still_distance;
                settings.max_move_distance = p_settings->max_move_distance;
                for (uint8_t i = 0; i < p_settings->min_still_distance; i++)
                {
                    still_sensitivity[i] = 100;
                }
                for (uint8_t i = 8; i > p_settings->max_still_distance; i--)
                {
                    still_sensitivity[i] = 100;
                }
                for (uint8_t i = 0; i < p_settings->min_move_distance; i++)
                {
                    move_sensitivity[i] = 100;
                }
                for (uint8_t i = 8; i > p_settings->max_move_distance; i--)
                {
                    move_sensitivity[i] = 100;
                }
            }
            
            break;
        
        default:
            ESP_LOGE(TAG, "unknown template");
            return ESP_FAIL;
            break;
    } 

    #if TEST_SETTINGS
    printf("no_one_duration : %d max_still_distance : %d max_move_distance : %d\n", settings.no_one_duration, settings.max_still_distance, settings.max_move_distance);
    printf("still_sensitivity : ");
    for (uint8_t i = 0; i < 9; i++)
    {
        printf("%d ", still_sensitivity[i]);
    }
    printf("\nmove_sensitivity : ");
    for (uint8_t i = 0; i < 9; i++)
    {
        printf("%d ", still_sensitivity[i]);
    }
    printf("\n");
    #endif
    
    if (!ld2410_set_max_values(settings.max_move_distance, settings.max_still_distance, settings.no_one_duration)) // (max_moving_distance, max_still_distance, no_one_duration in seconds)
    {
        ESP_LOGE(TAG, "unable to set configuations");
        return ESP_FAIL;
    }
    if (!ld2410_set_gates_sensitivity_threshold(move_sensitivity, still_sensitivity))
    {
        ESP_LOGE(TAG, "unable to set gates sensitivity");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t ld2410_get_data(ld2410_outputs_t* output)
{
    if (NULL == output)
    {
        return ESP_FAIL;
    }
    
    esp_err_t ret = ESP_OK;
    if (!ld2410_read())
    {
        ret = ESP_FAIL;
    }

    if(ld2410_is_connected() && millis() - last_reading > 300)  //Report every 300ms
    {
        last_reading = millis();
        if(ld2410_presence_detected())
        {
            output->presence = true;
            if(ld2410_stationary_target_detected())
            {
                output->stationary_target_distance = ld2410_stationary_target_distance();
                printf("Stationary target: %d ", output->stationary_target_distance);
                printf("cm energy: %d ", ld2410_stationary_target_energy());
            }
            if(ld2410_moving_target_detected())
            {
                output->moving_target_distance = ld2410_moving_target_distance();
                printf("Moving target: %d ", output->moving_target_distance);
                printf("cm energy: %d ", ld2410_moving_target_energy());
            }
            printf("\n");
        }
        else
        {
            output->presence = false;
            printf("No target\n");
        }
    }
    return ret;
}
