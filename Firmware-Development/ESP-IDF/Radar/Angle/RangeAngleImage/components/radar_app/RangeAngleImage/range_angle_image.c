#include "range_angle_image.h"

const ifx_PPFFT_Config_t range_fft_config = 
{
      .fft_type = IFX_FFT_TYPE_R2C,
      .fft_size = RANGE_FFT_SIZE,
      .mean_removal_enabled = true,
      .window_config = { IFX_WINDOW_BLACKMANHARRIS, XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP, 0, 1 },
      .is_normalized_window = 1
  };
const ifx_PPFFT_Config_t doppler_fft_config = 
{
    .fft_type = IFX_FFT_TYPE_C2C,
    .fft_size = DOPPLER_FFT_SIZE,
    .mean_removal_enabled = true,
    .window_config = { IFX_WINDOW_CHEBYSHEV, XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME, 100, 1 },
    .is_normalized_window = 1
};

const ifx_RDM_Config_t rdmConfig =
{
    .spect_threshold = IFX_SPECT_THRESHOLD,
    .output_scale_type = IFX_SCALE_TYPE_LINEAR,
    .range_fft_config = range_fft_config,
    .doppler_fft_config = doppler_fft_config
};
  

// * Digital Beam Forming Config: * //
const ifx_DBF_Config_t dbfConfig =
{
    .num_beams = NUM_OF_BEAMS,
    .num_antennas = NUM_OF_ANTENNAS,
    .min_angle = MIN_ANGLE,
    .max_angle = MAX_ANGLE,
    .d_by_lambda = D_BY_LAMBDA
};

ifx_Error_t rai_init(ifx_RAI_Config_t* rai_config)
{
    memset(rai_config, 0, sizeof(ifx_RAI_Config_t));

    rai_config->rdm_config = rdmConfig;
    rai_config->alpha_mti_filter = 0.8;
    rai_config->dbf_config = dbfConfig;
    rai_config->num_of_images = NUM_OF_IMAGES;
    rai_config->num_antenna_array = 2;

    return ifx_error_get();
}

ifx_Error_t rai_config(ifx_RAI_Config_t* rai_config, ifx_RAI_t** handle)
{
    *handle = ifx_rai_create(rai_config);
    return ifx_error_get();
}

ifx_Error_t rai_process(ifx_RAI_t* handle, const ifx_Cube_R_t* input, ifx_Cube_R_t* output)
{
    ifx_rai_run_r(handle, input, output);
    return ifx_error_get();
}