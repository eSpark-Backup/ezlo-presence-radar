#pragma once
#include <string.h>
#include "Avian.h"
#include "Base.h"
#include "2DMTI.h"
#include "SpectrumAxis.h"
#include "RadarSegmentation.h"
#include "RangeDopplerMap.h"
#include "rdm_defaults.h"

typedef struct
{
    ifx_Avian_Device_t *device_handle;
    ifx_Matrix_R_t *rdm;
    ifx_RDM_t *rdm_handle;
    ifx_2DMTI_R_t *mti_handle;
    ifx_Math_Axis_Spec_t range_spec;
    ifx_Math_Axis_Spec_t speed_spec;
} rdm_t;

ifx_Error_t rdm_init(rdm_t *rdm_context);
ifx_Error_t rdm_config(rdm_t *rdm_context, ifx_Avian_Device_t *device, ifx_Avian_Config_t *dev_config);
ifx_Error_t rdm_cleanup(rdm_t *rdm_context);

ifx_Error_t rdm_range_idx(rdm_t *rdm_context, ifx_Cube_R_t *frame, uint32_t* range_idx);