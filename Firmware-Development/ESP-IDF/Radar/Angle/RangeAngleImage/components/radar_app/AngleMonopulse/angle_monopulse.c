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
#include "angle_monopulse.h"
#include "range_doppler_map.c"

static const ifx_PPFFT_Config_t range_fft_config = 
{
      .fft_type = IFX_FFT_TYPE_R2C,
      .fft_size = RANGE_FFT_SIZE,
      .mean_removal_enabled = true,
      .window_config = { IFX_WINDOW_BLACKMANHARRIS, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP, 0, 1 },
      .is_normalized_window = 1
  };
static const ifx_PPFFT_Config_t doppler_fft_config = 
{
    .fft_type = IFX_FFT_TYPE_C2C,
    .fft_size = DOPPLER_FFT_SIZE,
    .mean_removal_enabled = true,
    .window_config = { IFX_WINDOW_CHEBYSHEV, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME, 100, 1 },
    .is_normalized_window = 1
};

static const ifx_RDM_Config_t rdm_config_obj =
{
    .spect_threshold = IFX_SPECT_THRESHOLD,
    .output_scale_type = IFX_SCALE_TYPE_LINEAR,
    .range_fft_config = range_fft_config,
    .doppler_fft_config = doppler_fft_config
};

ifx_Angle_t* ifx_angle_create(ifx_Angle_Config_t* config);
void ifx_angle_destroy(ifx_Angle_t* handle);
void ifx_angle_run_r(ifx_Angle_t* handle, const ifx_Cube_R_t* input, ifx_Float_t* azimuth_angle, ifx_Float_t* elevation_angle);


ifx_Angle_t* ifx_angle_create(ifx_Angle_Config_t* config)
{
    IFX_ERR_BRN_NULL(config);

    ifx_Angle_t* h = ifx_mem_alloc(sizeof(struct ifx_Angle_s));
    IFX_ERR_BRN_MEMALLOC(h);

    //----------------------- Range Doppler Map Handle -----------------------
    IFX_ERR_HANDLE_N(h->rdm_handle = ifx_rdm_create(&config->rdm_config),
                     ifx_angle_destroy(h));

    uint32_t range_fft_size = config->rdm_config.range_fft_config.fft_size;
    uint32_t doppler_fft_size = config->rdm_config.doppler_fft_config.fft_size;

    if (config->rdm_config.range_fft_config.fft_type == IFX_FFT_TYPE_R2C)
    {
        range_fft_size /= 2;
    }

    IFX_ERR_HANDLE_N(h->rdm_cube = ifx_cube_create_c(range_fft_size, doppler_fft_size, config->num_antenna_array),
                     ifx_angle_destroy(h));
    IFX_ERR_HANDLE_N(h->rx_spectrum_cube = ifx_cube_create_c(range_fft_size, doppler_fft_size, config->num_antenna_array),
                     ifx_angle_destroy(h));

    //----------------------- 2D MTI Handle ----------------------------------
    h->mti_handle_array = ifx_mem_alloc(sizeof(ifx_2DMTI_C_t*) * config->num_antenna_array);
    IFX_ERR_BRN_MEMALLOC(h);

    for (uint32_t i = 0; i < config->num_antenna_array; ++i)
    {
        IFX_ERR_HANDLE_N(h->mti_handle_array[i] = ifx_2dmti_create_c(config->alpha_mti_filter,
                         range_fft_size, doppler_fft_size),
                         ifx_angle_destroy(h));
    }

    //----------------------- Angle Monopulse ----------------------------------
    IFX_ERR_HANDLE_N(h->angle_monopulse_handle = ifx_anglemonopulse_create(config->wavelength, config->antenna_spacing), ifx_angle_destroy(h));
    IFX_ERR_HANDLE_N(h->complex_to_real = ifx_mat_create_r(range_fft_size, doppler_fft_size), ifx_angle_destroy(h));
    IFX_ERR_HANDLE_N(h->target_matrix = ifx_mat_create_c(2, 1), ifx_angle_destroy(h));
    IFX_ERR_HANDLE_N(h->calibration_matrix = ifx_mat_create_c(2, 2), ifx_angle_destroy(h));
    IFX_ERR_HANDLE_N(h->calibration_result = ifx_mat_create_c(2, 1), ifx_angle_destroy(h));

    printf("stride[0] %d stride[1] %d\n",h->calibration_matrix->stride[0], h->calibration_matrix->stride[1]);
    h->calibration_matrix->d[0].data[0] = 1.0;
    h->calibration_matrix->d[0].data[1] = 0.0;

    h->calibration_matrix->d[1].data[0] = -0.1409;
    h->calibration_matrix->d[1].data[1] = 0.0604;

    h->calibration_matrix->d[2].data[0] = 0.0;
    h->calibration_matrix->d[2].data[1] = 0.0;

    h->calibration_matrix->d[3].data[0] = 0.4868;
    h->calibration_matrix->d[3].data[1] = 0.7859;

    h->num_antenna_array = config->num_antenna_array;
    return h;
}

//----------------------------------------------------------------------------

void ifx_angle_destroy(ifx_Angle_t* handle)
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
void ifx_angle_run_r(ifx_Angle_t* handle,
                   const ifx_Cube_R_t* input, ifx_Float_t* azimuth_angle, ifx_Float_t* elevation_angle)
{
    IFX_ERR_BRK_NULL(handle);
    IFX_ERR_BRK_NULL(input);

    // rdm_view: range_fft_size x doppler_fft_size
    ifx_Matrix_C_t rdm_view = { 0 };

    for (uint32_t rx = 0; rx < handle->num_antenna_array; ++rx)
    {
        // rawdata_view: num_chirps_per_frame x num_samples_per_frame
        ifx_Matrix_R_t rawdata_view = { 0 };

        // rx_spectrum_view: range_fft_size x doppler_fft_size
        ifx_Matrix_C_t rx_spectrum_view = { 0 };

        ifx_cube_get_row_r(input, rx, &rawdata_view); // set view to the rx antenna for raw data matrix
        ifx_cube_get_slice_c(handle->rdm_cube, rx, &rdm_view);  // set view to the rx antenna for range doppler map
        ifx_cube_get_slice_c(handle->rx_spectrum_cube, rx, &rx_spectrum_view);
        ifx_rdm_run_rc(handle->rdm_handle, &rawdata_view, &rdm_view);
        ifx_2dmti_run_c(handle->mti_handle_array[rx], &rdm_view, &rx_spectrum_view);
    }
    
    ifx_Matrix_C_t rx1;
    ifx_Matrix_C_t rx2;
    ifx_Matrix_C_t rx3;
    ifx_cube_get_slice_c(handle->rx_spectrum_cube, 0 , &rx1);
    ifx_cube_get_slice_c(handle->rx_spectrum_cube, 1 , &rx2);
    ifx_cube_get_slice_c(handle->rx_spectrum_cube, 2 , &rx3);

    ifx_mat_abs_c(&rx1, handle->complex_to_real);

    // // do peak search
    uint32_t rmax;
    uint32_t cmax;
    rdm_peak_search(handle->complex_to_real, &rmax, &cmax);

    ifx_Complex_t rxcomp[3];

    memcpy(&rxcomp[0], &(IFX_MAT_AT(&rx1, rmax, cmax)), sizeof(ifx_Complex_t));
    memcpy(&rxcomp[1], &(IFX_MAT_AT(&rx2, rmax, cmax)), sizeof(ifx_Complex_t));
    memcpy(&rxcomp[2], &(IFX_MAT_AT(&rx3, rmax, cmax)), sizeof(ifx_Complex_t));

    *azimuth_angle = ifx_anglemonopulse_scalar_run(handle->angle_monopulse_handle, rxcomp[0], rxcomp[2]);

    memcpy(&handle->target_matrix->d[0], &rxcomp[2], sizeof(ifx_Complex_t));
    memcpy(&handle->target_matrix->d[1], &rxcomp[1], sizeof(ifx_Complex_t));

    ifx_mat_mul_c(handle->calibration_matrix, handle->target_matrix, handle->calibration_result);

    #if MATRIX_MULTIPLICATION_DEBUG
    // Prints calibration matrix
    printf("Calibration Matrix\n");
    for (size_t i = 0; i < 4; i+=2)
    {
        printf("%0.4f%+0.4fi\t",handle->calibration_matrix->d[i].data[0],handle->calibration_matrix->d[i].data[1]);
        printf("%0.4f%+0.4fi\n",handle->calibration_matrix->d[i+1].data[0],handle->calibration_matrix->d[i+1].data[1]);
    }
    printf("\n\n");

    // Prints (RX2 and RX3) data matrix
    printf("RX2 and RX3 Data Matrix\n");
    for (size_t i = 0; i < 2; i++)
    {
        printf("%0.4f%+0.4fi\n",handle->target_matrix->d[i].data[0],handle->target_matrix->d[i].data[1]);
    }
    printf("\n\n");

    // Prints multiplication resultant matrix
    printf("Resultant Matrix\n");
    for (size_t i = 0; i < 2; i++)
    {
         printf("%0.4f%+0.4fi\n",handle->calibration_result->d[i].data[0],handle->calibration_result->d[i].data[1]);
    }
    printf("\n\n");
    #endif
    
    // *elevation_angle = ifx_anglemonopulse_scalar_run(handle->angle_monopulse_handle, rxcomp[1], rxcomp[2]); // Without calibration 
    // printf("Elevation without calibration %0.2f\n" , *elevation_angle);
    *elevation_angle = ifx_anglemonopulse_scalar_run(handle->angle_monopulse_handle, handle->calibration_result->d[0], handle->calibration_result->d[1]);
}

ifx_Error_t angle_monopulse_init(ifx_Angle_Config_t* angle_config)
{
    memset(angle_config, 0, sizeof(ifx_Angle_Config_t));

    angle_config->rdm_config = rdm_config_obj;
    angle_config->alpha_mti_filter = 0.8;
    angle_config->num_antenna_array = XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS;
    ifx_Float_t center_freq = (XENSIV_BGT60TRXX_CONF_START_FREQ_HZ + XENSIV_BGT60TRXX_CONF_END_FREQ_HZ)/2;
    angle_config->wavelength = 299792458 / center_freq;
    angle_config->antenna_spacing = 0.0025;

    return ifx_error_get();
}

ifx_Error_t angle_monopulse_config(ifx_Angle_Config_t* angle_config, ifx_Angle_t** handle)
{
    *handle = ifx_angle_create(angle_config);
    return ifx_error_get();
}

ifx_Error_t angle_monopulse_process(ifx_Angle_t* handle, const ifx_Cube_R_t* input, ifx_Float_t* azimuth_angle, ifx_Float_t* elevation_angle)
{
    ifx_angle_run_r(handle, input, azimuth_angle, elevation_angle);
    return ifx_error_get();
}

void angle_monopulse_cleanup(ifx_Angle_t* handle)
{
    ifx_angle_destroy(handle);
}