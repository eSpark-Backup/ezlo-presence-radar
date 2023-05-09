#pragma once

#include "AngleCapon.h"
#include "Error.h"
#include "../RangeAngleImage/range_angle_image.h"

ifx_Error_t angle_capon_init(ifx_AngleCapon_Config_t* angle_capon_config);
ifx_Error_t angle_capon_config(ifx_AngleCapon_Config_t* angle_capon_config, ifx_AngleCapon_t** handle);
ifx_Error_t angle_capon_process(const ifx_AngleCapon_t* handle, uint32_t range_bin, const ifx_Cube_C_t* rx_spectrum);