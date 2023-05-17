#pragma once

#include "Types.h"
#include "Cube.h"
#include "DeviceConfig.h"
#include "RangeDopplerMap.h"
#include "AngleMonopulse.h"

#define RANGE_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * 4// Zero padding of 4 gives good range resolution in Range spectrum
#define DOPPLER_FFT_SIZE XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * 4 // Zero padding of 4 gives good range resolution in Doppler spectrum

typedef struct ifx_MotionAngle_Result_s{
   ifx_Float_t range;               /*!< Distance of object in [m], -1 if undefined/absence */
   ifx_Float_t angle_azimuth;       /*!< Azimuth angle of the closest object in degrees, 0 if undefined/absence */
   ifx_Float_t angle_elevation;     /*!< Elevation angle of the closest object in degrees, 0 if undefined/absence */
   ifx_Float_t speed;               /*!< Speed of object in [m/s], 0 if undefined/absence */
} ifx_MotionAngle_Result_t;

typedef struct ifx_MotionAngle_s
{
    ifx_RDM_t*              rdm_handle;           /**< Range doppler map handle for all rx antennas.*/
    ifx_2DMTI_C_t**         mti_handle_array;     /**< 2D MTI filter coefficient.*/
    uint32_t                num_antenna_array;    /**< Number of antennas.*/
    ifx_Cube_C_t*           rdm_cube;             /**< 2D complex range doppler maps over rx antennas as a cube.*/
    ifx_Cube_C_t*           rx_spectrum_cube;     /**< ... */
    ifx_AngleMonopulse_t*   angle_monopulse_handle;
    ifx_Math_Axis_Spec_t    range_spec;
    ifx_Math_Axis_Spec_t    speed_spec;
    ifx_Matrix_R_t*         complex_to_real;
    ifx_Matrix_C_t          *target_matrix, *calibration_matrix, *calibration_result;
} ifx_MotionAngle_t;

/**
 * @brief Defines the structure for Motion Angle module related settings.
 */
typedef struct
{
    ifx_RDM_Config_t    rdm_config;         /**< Range doppler map configurations.*/
    ifx_Float_t         alpha_mti_filter;   /**< 2D MTI filter coefficient.*/
    uint8_t             num_antenna_array;  /**< Number of antennas.*/
    ifx_Float_t         wavelength;
    ifx_Float_t         antenna_spacing;
} ifx_MotionAngle_Config_t;

typedef struct ma_s
{
    ifx_MotionAngle_t* handle;
    ifx_MotionAngle_Config_t config;
    ifx_MotionAngle_Result_t result;
} ma_t;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

ifx_Error_t ma_init(ma_t* ma_context);
ifx_Error_t ma_config(ma_t* ma_context, ifx_Avian_Config_t* ifx_config);
ifx_Error_t ma_process(ma_t* ma_context, const ifx_Cube_R_t* frame);
void ma_cleanup(ma_t* ma_context);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

