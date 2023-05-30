#include "ld2410.h"

static uint32_t last_reading = 0;

esp_err_t ld2410_setup(void)
{
    printf("\nConnect LD2410 radar TX to GPIO: %d\n", RADAR_RX_PIN);
    printf("Connect LD2410 radar RX to GPIO: %d\n", RADAR_TX_PIN);
    printf("LD2410 radar sensor initialising:\n");
    if(ld2410_begin(true))
    {
        printf("OK\n");
        ld2410_set_max_values(distance_600cm, distance_525cm, 5); // (max_moving_distance, max_still_distance, no_one_duration in seconds)

        uint8_t mov_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20}; // Gate 0 to 8 sensitivity : sensitivity range 0-100
        uint8_t still_sensitivity[9] = {20, 20, 20, 20, 20, 20, 20, 20, 20}; // Gate 0 to 8 sensitivity : sensitivity range 0-100
        ld2410_set_gates_sensitivity_threshold(mov_sensitivity, still_sensitivity);
        return ESP_OK;
    }
    else
    {
        printf("not connected\n");
    }
    return ESP_FAIL;
}
esp_err_t ld2410_get_data(void)
{
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
            if(ld2410_stationary_target_detected())
            {
                printf("Stationary target: %d ", ld2410_stationary_target_distance());
                printf("cm energy: %d ", ld2410_stationary_target_energy());
            }
            if(ld2410_moving_target_detected())
            {
                printf("Moving target: %d ", ld2410_moving_target_distance());
                printf("cm energy: %d ", ld2410_moving_target_energy());
            }
            printf("\n");
        }
        else
        {
            printf("No target\n");
        }
    }
    return ret;
}
