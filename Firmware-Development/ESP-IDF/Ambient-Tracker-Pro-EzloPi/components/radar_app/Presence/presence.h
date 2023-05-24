#pragma once 

#include "xensiv_bgt60trxx_esp32.h"
#include "PresenceSensing.h"
#include "esp_log.h"

#define RX_ANTENNA_ID   0   // This implementation uses only 1 Rx Antenna
#define PRESENCE_LED 0

typedef struct {
    ifx_PresenceSensing_t* presencesensing;
    bool presence_status;
} presence_t;

#ifdef __cplusplus
extern "C" {
#endif
ifx_Error_t presence_init(presence_t* ctx);
ifx_Error_t presence_config(presence_t* ctx, ifx_Avian_Device_t* device, ifx_Avian_Config_t* dev_config);
ifx_Error_t presence_process(presence_t* presence_context, ifx_Cube_R_t* frame);
ifx_Error_t presence_cleanup(presence_t* ctx);
#ifdef __cplusplus
}
#endif