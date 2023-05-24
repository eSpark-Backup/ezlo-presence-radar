#pragma once

#include <string.h>
#include "RangeAngleImage.h"
#include "Error.h"
#include "../RangeDopplerMap/rdm_defaults.h"
#include "presence_radar_default.h"

#define RANGE_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * 4// Zero padding of 4 gives good range resolution in Range spectrum
#define DOPPLER_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * 4 // Zero padding of 4 gives good range resolution in Doppler spectrum

#define NUM_OF_IMAGES 1
#define NUM_OF_BEAMS 31
#define NUM_OF_ANTENNAS 2
#define MIN_ANGLE -40.0
#define MAX_ANGLE 40.0
#define D_BY_LAMBDA 0.5

#ifdef __cplusplus
extern "C" {
#endif
ifx_Error_t rai_init(ifx_RAI_Config_t* rai_config);
ifx_Error_t rai_config(ifx_RAI_Config_t* rai_config, ifx_RAI_t** handle);
ifx_Error_t rai_process(ifx_RAI_t* handle, const ifx_Cube_R_t* input, ifx_Cube_R_t* output);
#ifdef __cplusplus
}
#endif
