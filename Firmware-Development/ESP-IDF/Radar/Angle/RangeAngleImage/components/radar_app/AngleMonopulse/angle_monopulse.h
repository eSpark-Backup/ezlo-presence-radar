#pragma once

#include "Types.h"
#include "Cube.h"
#include "DeviceConfig.h"
#include "RangeDopplerMap.h"
#include "AngleMonopulse.h"

#define RANGE_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * 4// Zero padding of 4 gives good range resolution in Range spectrum
#define DOPPLER_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * 4 // Zero padding of 4 gives good range resolution in Doppler spectrum

typedef struct ifx_Angle_s
{
    ifx_RDM_t*            rdm_handle;           /**< Range doppler map handle for all rx antennas.*/
    ifx_2DMTI_C_t**       mti_handle_array;     /**< 2D MTI filter coefficient.*/
    uint32_t              num_antenna_array;    /**< Number of antennas.*/
    ifx_Cube_C_t*         rdm_cube;             /**< 2D complex range doppler maps over rx antennas as a cube.*/
    ifx_Cube_C_t*         rx_spectrum_cube;     /**< ... */
    ifx_AngleMonopulse_t* angle_monopulse_handle;
    ifx_Math_Axis_Spec_t range_spec;
    ifx_Math_Axis_Spec_t speed_spec;
    ifx_Matrix_R_t* complex_to_real;
    ifx_Matrix_C_t *target_matrix, *calibration_matrix, *calibration_result;
} ifx_Angle_t;

/**
 * @brief Defines the structure for Range Angle Image module related settings.
 */
typedef struct
{
    ifx_RDM_Config_t  rdm_config;         /**< Range doppler map configurations.*/
    ifx_Float_t       alpha_mti_filter;   /**< 2D MTI filter coefficient.*/
    uint32_t          num_antenna_array;  /**< Number of virtual antennas.*/
    ifx_Float_t wavelength;
    ifx_Float_t antenna_spacing;
} ifx_Angle_Config_t;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

ifx_Error_t angle_monopulse_init(ifx_Angle_Config_t* angle_config);
ifx_Error_t angle_monopulse_config(ifx_Angle_Config_t* angle_config, ifx_Angle_t** handle, ifx_Avian_Config_t* ifx_config);
ifx_Error_t angle_monopulse_process(ifx_Angle_t* handle, const ifx_Cube_R_t* input, ifx_Float_t* azimuth_angle, ifx_Float_t* elevation_angle);
void angle_monopulse_cleanup(ifx_Angle_t* handle);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

