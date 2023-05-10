#pragma once
#include <string.h>
#include "esp_log.h"
#include "Avian.h"
#include "Base.h"
#include "2DMTI.h"
#include "SpectrumAxis.h"
#include "RangeDopplerMap.h"
#include "rdm_defaults.h"
#include "Algo.h"

#define RX_ANTENNA_ID   0   // This implementation uses only 1 Rx Antenna

typedef struct
{
    ifx_Avian_Device_t *device_handle;
    ifx_Matrix_R_t *rdm;
    ifx_Matrix_R_t *cfar_mat;
    ifx_RDM_t *rdm_handle;
    ifx_2DMTI_R_t *mti_handle;
    ifx_Math_Axis_Spec_t range_spec;
    ifx_Math_Axis_Spec_t speed_spec;
    ifx_OSCFAR_t* cfar_handle;
} rdm_t;

#ifdef __cplusplus
extern "C" {
#endif
ifx_Error_t rdm_init(rdm_t *rdm_context);
ifx_Error_t rdm_config(rdm_t *rdm_context, ifx_Avian_Device_t *device, ifx_Avian_Config_t *dev_config);
ifx_Error_t rdm_cleanup(rdm_t *rdm_context);

ifx_Error_t rdm_process(rdm_t *rdm_context, ifx_Cube_R_t *frame);
void rdm_peak_search(const ifx_Matrix_R_t *matrix, uint32_t *rmax, uint32_t *cmax);
#ifdef __cplusplus
}
#endif