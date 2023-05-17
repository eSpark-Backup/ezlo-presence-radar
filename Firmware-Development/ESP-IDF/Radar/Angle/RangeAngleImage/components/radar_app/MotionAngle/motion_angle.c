#include <stdlib.h>
#include <string.h>

#include "2DMTI.h"

#include "Defines.h"
#include "Mem.h"
#include "Vector.h"
#include "Matrix.h"
#include "Cube.h"
#include "Error.h"
#include "Macros.h"

#include "presence_radar_default.h"
#include "../RangeDopplerMap/rdm_defaults.h"
#include "AngleMonopulse.h"
#include "range_doppler_map.c"
#include "motion_angle.h"

static const ifx_PPFFT_Config_t range_fft_config =
    {
        .fft_type = IFX_FFT_TYPE_R2C,
        .fft_size = RANGE_FFT_SIZE,
        .mean_removal_enabled = true,
        .window_config = {IFX_WINDOW_BLACKMANHARRIS, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP, 0, 1},
        .is_normalized_window = 1};
static const ifx_PPFFT_Config_t doppler_fft_config =
    {
        .fft_type = IFX_FFT_TYPE_C2C,
        .fft_size = DOPPLER_FFT_SIZE,
        .mean_removal_enabled = true,
        .window_config = {IFX_WINDOW_CHEBYSHEV, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME, 100, 1},
        .is_normalized_window = 1};

static const ifx_RDM_Config_t rdm_configurations =
    {
        .spect_threshold = IFX_SPECT_THRESHOLD,
        .output_scale_type = IFX_SCALE_TYPE_LINEAR,
        .range_fft_config = range_fft_config,
        .doppler_fft_config = doppler_fft_config};

static ifx_MotionAngle_t *ifx_motionangle_create(ifx_MotionAngle_Config_t *config, ifx_Avian_Config_t *ifx_config);
static void ifx_motionangle_destroy(ifx_MotionAngle_t *handle);
static void ifx_motionangle_run(ifx_MotionAngle_t *handle, const ifx_Cube_R_t *input, ifx_MotionAngle_Result_t *result);

static ifx_MotionAngle_t *ifx_motionangle_create(ifx_MotionAngle_Config_t *config, ifx_Avian_Config_t *ifx_config)
{
    IFX_ERR_BRN_NULL(config);
    IFX_ERR_BRN_NULL(ifx_config);

    ifx_MotionAngle_t *h = ifx_mem_alloc(sizeof(struct ifx_MotionAngle_s));
    IFX_ERR_BRN_MEMALLOC(h);

    //----------------------- Range Doppler Map Handle -----------------------
    IFX_ERR_HANDLE_N(h->rdm_handle = ifx_rdm_create(&config->rdm_config),
                     ifx_motionangle_destroy(h));

    uint32_t range_fft_size = config->rdm_config.range_fft_config.fft_size;
    uint32_t doppler_fft_size = config->rdm_config.doppler_fft_config.fft_size;

    if (config->rdm_config.range_fft_config.fft_type == IFX_FFT_TYPE_R2C)
    {
        range_fft_size /= 2;
    }

    IFX_ERR_HANDLE_N(h->rdm_cube = ifx_cube_create_c(range_fft_size, doppler_fft_size, config->num_antenna_array),
                     ifx_motionangle_destroy(h));
    IFX_ERR_HANDLE_N(h->rx_spectrum_cube = ifx_cube_create_c(range_fft_size, doppler_fft_size, config->num_antenna_array),
                     ifx_motionangle_destroy(h));

    //----------------------- 2D MTI Handle ----------------------------------
    h->mti_handle_array = ifx_mem_alloc(sizeof(ifx_2DMTI_C_t *) * config->num_antenna_array);
    IFX_ERR_BRN_MEMALLOC(h);

    for (uint32_t i = 0; i < config->num_antenna_array; ++i)
    {
        IFX_ERR_HANDLE_N(h->mti_handle_array[i] = ifx_2dmti_create_c(config->alpha_mti_filter,
                                                                     range_fft_size, doppler_fft_size),
                         ifx_motionangle_destroy(h));
    }

    //----------------------- Angle Monopulse ----------------------------------
    IFX_ERR_HANDLE_N(h->angle_monopulse_handle = ifx_anglemonopulse_create(config->wavelength, config->antenna_spacing), ifx_motionangle_destroy(h));
    IFX_ERR_HANDLE_N(h->complex_to_real = ifx_mat_create_r(range_fft_size, doppler_fft_size), ifx_motionangle_destroy(h));
    IFX_ERR_HANDLE_N(h->target_matrix = ifx_mat_create_c(2, 1), ifx_motionangle_destroy(h));
    IFX_ERR_HANDLE_N(h->calibration_matrix = ifx_mat_create_c(2, 2), ifx_motionangle_destroy(h));
    IFX_ERR_HANDLE_N(h->calibration_result = ifx_mat_create_c(2, 1), ifx_motionangle_destroy(h));

    const ifx_Float_t bandwidth_Hz = ifx_devconf_get_bandwidth(ifx_config);
    h->range_spec = ifx_spectrum_axis_calc_range_axis(IFX_FFT_TYPE_R2C, range_fft_size * 2, ifx_config->num_samples_per_chirp, bandwidth_Hz);
    if (ifx_error_get())
    {
        ESP_LOGE(TAG, "Unable to calculate axis of spectrum");
    }

    const ifx_Float_t center_freq_Hz = ifx_devconf_get_center_frequency(ifx_config);
    const ifx_Float_t chirptime_s = ifx_devconf_get_chirp_time(ifx_config);
    h->speed_spec = ifx_spectrum_axis_calc_speed_axis(IFX_FFT_TYPE_C2C, doppler_fft_size, center_freq_Hz, chirptime_s);

    // Calibration Matrix R1C1
    h->calibration_matrix->d[0].data[0] = 1.0;
    h->calibration_matrix->d[0].data[1] = 0.0;

    // Calibration Matrix R1C2
    h->calibration_matrix->d[1].data[0] = -0.1409;
    h->calibration_matrix->d[1].data[1] = 0.0604;

    // Calibration Matrix R2C1
    h->calibration_matrix->d[2].data[0] = 0.0;
    h->calibration_matrix->d[2].data[1] = 0.0;

    // Calibration Matrix R2C2
    h->calibration_matrix->d[3].data[0] = 0.4868;
    h->calibration_matrix->d[3].data[1] = 0.7859;

    h->num_antenna_array = config->num_antenna_array;
    return h;
}

//----------------------------------------------------------------------------

static void ifx_motionangle_destroy(ifx_MotionAngle_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    ifx_cube_destroy_c(handle->rdm_cube);
    ifx_cube_destroy_c(handle->rx_spectrum_cube);
    ifx_mat_destroy_r(handle->complex_to_real);
    ifx_rdm_destroy(handle->rdm_handle);
    ifx_anglemonopulse_destroy(handle->angle_monopulse_handle);

    for (uint32_t i = 0; i < handle->num_antenna_array; ++i)
    {
        ifx_2dmti_destroy_c(handle->mti_handle_array[i]);
    }

    ifx_mem_free(handle->mti_handle_array);

    ifx_mem_free(handle);
}

//---------------------------------------------------------------------------
static void ifx_motionangle_run(ifx_MotionAngle_t *handle, const ifx_Cube_R_t *input, ifx_MotionAngle_Result_t *result)
{
    IFX_ERR_BRK_NULL(handle);
    IFX_ERR_BRK_NULL(input);

    // rdm_view: range_fft_size x doppler_fft_size
    ifx_Matrix_C_t rdm_view = {0};

    for (uint32_t rx = 0; rx < handle->num_antenna_array; ++rx)
    {
        // rawdata_view: num_chirps_per_frame x num_samples_per_frame
        ifx_Matrix_R_t rawdata_view = {0};

        // rx_spectrum_view: range_fft_size x doppler_fft_size
        ifx_Matrix_C_t rx_spectrum_view = {0};

        ifx_cube_get_row_r(input, rx, &rawdata_view);          // set view to the rx antenna for raw data matrix
        ifx_cube_get_slice_c(handle->rdm_cube, rx, &rdm_view); // set view to the rx antenna for range doppler map
        ifx_cube_get_slice_c(handle->rx_spectrum_cube, rx, &rx_spectrum_view);
        ifx_rdm_run_rc(handle->rdm_handle, &rawdata_view, &rdm_view);
        ifx_2dmti_run_c(handle->mti_handle_array[rx], &rdm_view, &rx_spectrum_view);
    }

    ifx_Matrix_C_t rx1, rx2, rx3;
    ifx_cube_get_slice_c(handle->rx_spectrum_cube, 0, &rx1);
    ifx_cube_get_slice_c(handle->rx_spectrum_cube, 1, &rx2);
    ifx_cube_get_slice_c(handle->rx_spectrum_cube, 2, &rx3);

    ifx_mat_abs_c(&rx1, handle->complex_to_real);

    // // do peak search
    uint32_t rmax;
    uint32_t cmax;
    rdm_peak_search(handle->complex_to_real, &rmax, &cmax);

    ifx_Complex_t rxcomp[3];
    memcpy(&rxcomp[0], &(IFX_MAT_AT(&rx1, rmax, cmax)), sizeof(ifx_Complex_t));
    memcpy(&rxcomp[1], &(IFX_MAT_AT(&rx2, rmax, cmax)), sizeof(ifx_Complex_t));
    memcpy(&rxcomp[2], &(IFX_MAT_AT(&rx3, rmax, cmax)), sizeof(ifx_Complex_t));

    // RX2 and RX3 target matrix
    memcpy(&handle->target_matrix->d[0], &rxcomp[2], sizeof(ifx_Complex_t));
    memcpy(&handle->target_matrix->d[1], &rxcomp[1], sizeof(ifx_Complex_t));

    // RX2 and RX3 calibrated matrix
    ifx_mat_mul_c(handle->calibration_matrix, handle->target_matrix, handle->calibration_result);

#if MATRIX_MULTIPLICATION_DEBUG
    // Prints calibration matrix
    printf("Calibration Matrix\n");
    for (size_t i = 0; i < 4; i += 2)
    {
        printf("%0.4f%+0.4fi\t", handle->calibration_matrix->d[i].data[0], handle->calibration_matrix->d[i].data[1]);
        printf("%0.4f%+0.4fi\n", handle->calibration_matrix->d[i + 1].data[0], handle->calibration_matrix->d[i + 1].data[1]);
    }
    printf("\n\n");

    // Prints (RX2 and RX3) data matrix
    printf("RX2 and RX3 Data Matrix\n");
    for (size_t i = 0; i < 2; i++)
    {
        printf("%0.4f%+0.4fi\n", handle->target_matrix->d[i].data[0], handle->target_matrix->d[i].data[1]);
    }
    printf("\n\n");

    // Prints multiplication resultant matrix
    printf("Resultant Matrix\n");
    for (size_t i = 0; i < 2; i++)
    {
        printf("%0.4f%+0.4fi\n", handle->calibration_result->d[i].data[0], handle->calibration_result->d[i].data[1]);
    }
    printf("\n\n");
#endif

    // *elevation_angle = ifx_anglemonopulse_scalar_run(handle->angle_monopulse_handle, rxcomp[1], rxcomp[2]); // Without calibration
    // printf("Elevation without calibration %0.2f\n" , *elevation_angle);

    // angle_azimuth
    result->angle_azimuth = ifx_anglemonopulse_scalar_run(handle->angle_monopulse_handle, rxcomp[0], rxcomp[2]);
    // angle_elevation
    result->angle_elevation = ifx_anglemonopulse_scalar_run(handle->angle_monopulse_handle, handle->calibration_result->d[0], handle->calibration_result->d[1]);
    // range
    result->range = rmax * handle->range_spec.value_bin_per_step;
    // speed
    result->speed = (((ifx_Float_t)IFX_MAT_COLS(&rx1) / 2) - (ifx_Float_t)cmax) * handle->speed_spec.value_bin_per_step;
}

ifx_Error_t ma_init(ma_t *ma_context)
{
    memset(&ma_context->config, 0, sizeof(ifx_MotionAngle_Config_t));

    ma_context->config.rdm_config = rdm_configurations;
    ma_context->config.alpha_mti_filter = 0.8;
    ma_context->config.num_antenna_array = XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS;
    ifx_Float_t center_freq = (XENSIV_BGT60TRXX_CONF_START_FREQ_HZ + XENSIV_BGT60TRXX_CONF_END_FREQ_HZ) / 2;
    ma_context->config.wavelength = 299792458 / center_freq;
    ma_context->config.antenna_spacing = 0.0025;

    return ifx_error_get();
}

ifx_Error_t ma_config(ma_t *ma_context, ifx_Avian_Config_t *ifx_config)
{
    ma_context->handle = ifx_motionangle_create(&ma_context->config, ifx_config);
    return ifx_error_get();
}

ifx_Error_t ma_process(ma_t *ma_context, const ifx_Cube_R_t *frame)
{
    ifx_motionangle_run(ma_context->handle, frame, &ma_context->result);
    return ifx_error_get();
}

void ma_cleanup(ma_t *ma_context)
{
    ifx_motionangle_destroy(ma_context->handle);
}