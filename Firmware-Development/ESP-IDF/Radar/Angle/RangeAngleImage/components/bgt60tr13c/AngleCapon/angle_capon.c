#include "angle_capon.h"

/* Angle Capon */
// {
//     .range_win_size = 5,
//     .selected_rx  = 0,
//     .chirps_per_frame = XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME,
//     .phase_offset_degrees = 0,
//     .num_virtual_antennas = NUM_OF_ANTENNAS,
//     .num_beams = NUM_OF_BEAMS,
//     .min_angle_degrees = MIN_ANGLE,
//     .max_angle_degrees = MAX_ANGLE,
//     .d_by_lambda = D_BY_LAMBDA
// };

ifx_Error_t angle_capon_init(ifx_AngleCapon_Config_t* angle_capon_config)
{
    memset(angle_capon_config, 0, sizeof(ifx_AngleCapon_Config_t));

    angle_capon_config->range_win_size = 5;
    angle_capon_config->selected_rx  = 0;
    angle_capon_config->chirps_per_frame = XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME;
    angle_capon_config->phase_offset_degrees = 0;
    angle_capon_config->num_virtual_antennas = NUM_OF_ANTENNAS;
    angle_capon_config->num_beams = NUM_OF_BEAMS;
    angle_capon_config->min_angle_degrees = MIN_ANGLE;
    angle_capon_config->max_angle_degrees = MAX_ANGLE;
    angle_capon_config->d_by_lambda = D_BY_LAMBDA;

    return ifx_error_get();
}

ifx_Error_t angle_capon_config(ifx_AngleCapon_Config_t* angle_capon_config, ifx_AngleCapon_t** handle)
{
    *handle = ifx_anglecapon_create(angle_capon_config);
    return ifx_error_get();
}

ifx_Error_t angle_capon_process(const ifx_AngleCapon_t* handle, uint32_t range_bin, const ifx_Cube_C_t* rx_spectrum)	
{
    ifx_Float_t angle=0;
    angle = ifx_anglecapon_run(handle, range_bin, rx_spectrum);
    printf("Angle : %f\n",angle);
    return ifx_error_get();
}