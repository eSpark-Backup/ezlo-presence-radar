#include "range_doppler_map.h"

//----------------------------------------------------------------------------

/**
 * @brief Application specific initialization function
 *
 * Initializes the presence sensing context.
 *
 * @param ctx           context of the application
 */
ifx_Error_t rdm_init(rdm_t *rdm_context)
{
    memset(rdm_context, 0, sizeof(rdm_t));
    return ifx_error_get();
}

/**
 * @brief Application specific configuration function
 *
 * Configures the presence sensing with specified in json or default configuration.
 * Defines a state change callback and creates an instance of presence sensing algorithm.
 *
 * @param ctx           context of the application
 * @param device        connected device handle
 * @param json          json handle with configuration
 * @param dev_config    device configuration handle
 */
ifx_Error_t rdm_config(rdm_t *rdm_context, ifx_Avian_Device_t *device, ifx_Avian_Config_t *dev_config)
{
    ifx_Error_t ret = 0;

    const uint32_t range_fft_size = dev_config->num_samples_per_chirp * 1;  // Zero padding of 4 gives good range resolution in Range spectrum
    const uint32_t doppler_fft_size = dev_config->num_chirps_per_frame * 1; // Zero padding of 4 gives good range resolution in Doppler spectrum

    ifx_PPFFT_Config_t range_fft_config = {
        .fft_type = IFX_FFT_TYPE_R2C,
        .fft_size = range_fft_size,
        .mean_removal_enabled = true,
        .window_config = {IFX_WINDOW_BLACKMANHARRIS, dev_config->num_samples_per_chirp, 0, 1},
        .is_normalized_window = 1};

    ifx_PPFFT_Config_t doppler_fft_config = {
        .fft_type = IFX_FFT_TYPE_C2C,
        .fft_size = doppler_fft_size,
        .mean_removal_enabled = true,
        .window_config = {IFX_WINDOW_CHEBYSHEV, dev_config->num_chirps_per_frame, 100, 1},
        .is_normalized_window = 1};

    ifx_RDM_Config_t rdm_config = (ifx_RDM_Config_t){
        .spect_threshold = IFX_SPECT_THRESHOLD,
        .output_scale_type = IFX_SCALE_TYPE_LINEAR,
        .range_fft_config = range_fft_config,
        .doppler_fft_config = doppler_fft_config};

    rdm_context->rdm_handle = ifx_rdm_create(&rdm_config);
    if ((ret = ifx_error_get()))
    {
        printf("Here 1\n");
        return ret;
    }

    rdm_context->rdm = ifx_mat_create_r(range_fft_size / 2, doppler_fft_size); // for real input, range dimension has info only in positive half
    if ((ret = ifx_error_get()))
    {
        printf("Here 2\n");
        return ret;
    }

    // MTI Filter for static target cancellation
    rdm_context->mti_handle = ifx_2dmti_create_r(IFX_ALPHA_MTI_FILTER,
                                                 IFX_MAT_ROWS(rdm_context->rdm),
                                                 IFX_MAT_COLS(rdm_context->rdm));
    if ((ret = ifx_error_get()))
    {
        printf("Here 3\n");
        return ret;
    }

    const ifx_Float_t bandwidth_Hz = ifx_devconf_get_bandwidth(dev_config);
    rdm_context->range_spec = ifx_spectrum_axis_calc_range_axis(IFX_FFT_TYPE_R2C, range_fft_size, dev_config->num_samples_per_chirp, bandwidth_Hz);
    if ((ret = ifx_error_get()))
    {
        printf("Here 4");
        return ret;
    }

    const ifx_Float_t center_freq_Hz = ifx_devconf_get_center_frequency(dev_config);
    const ifx_Float_t chirptime_s = ifx_devconf_get_chirp_time(dev_config);
    rdm_context->speed_spec = ifx_spectrum_axis_calc_speed_axis(IFX_FFT_TYPE_C2C, doppler_fft_size, center_freq_Hz, chirptime_s);

    return (ifx_error_get());
}

/**
 * @brief Application specific cleanup function
 *
 * Destroys the presence sensing instances and cleans up allocated memory.
 *
 * @param ctx           context of the application
 */
ifx_Error_t rdm_cleanup(rdm_t *rdm_context)
{
    ifx_rdm_destroy(rdm_context->rdm_handle);
    ifx_mat_destroy_r(rdm_context->rdm);
    ifx_2dmti_destroy_r(rdm_context->mti_handle);
    return ifx_error_get();
}

/**
 * @brief Application specific processing function
 *
 * Runs a presence sensing algorithm on fetched data and informs about the results.
 *
 * @param ctx           context of the application
 * @param frame         collected frame
 */
void rdm_peak_search(const ifx_Matrix_R_t *matrix, uint32_t *rmax, uint32_t *cmax)
{
    *rmax = 0, *cmax = 0;
    ifx_Float_t max_value = IFX_MAT_AT(matrix, 0, 0);

    for (uint32_t r = 0; r < IFX_MAT_ROWS(matrix); r++)
    {
        for (uint32_t c = 0; c < IFX_MAT_COLS(matrix); c++)
        {
            ifx_Float_t value = IFX_MAT_AT(matrix, r, c);
            if (value > max_value)
            {
                *rmax = r;
                *cmax = c;
                max_value = value;
            }
        }
    }
}

ifx_Error_t rdm_range_idx(rdm_t *rdm_context, ifx_Cube_R_t *frame, uint32_t* range_idx)
{
    ifx_Error_t ret = 0;
    ifx_Matrix_R_t antenna_data;
    ifx_cube_get_row_r(frame, 1, &antenna_data);
    ifx_rdm_run_r(rdm_context->rdm_handle, &antenna_data, rdm_context->rdm);
    if ((ret = ifx_error_get()))
    {
        return ret;
    }

    ifx_2dmti_run_r(rdm_context->mti_handle, rdm_context->rdm, rdm_context->rdm);
    if ((ret = ifx_error_get()))
    {
        return ret;
    }

    // do peak search
    uint32_t rmax;
    uint32_t cmax;
    rdm_peak_search(rdm_context->rdm, &rmax, &cmax);
    // range
    const ifx_Float_t range = rmax * rdm_context->range_spec.value_bin_per_step;
    // speed
    const ifx_Float_t speed = (((ifx_Float_t)IFX_MAT_COLS(rdm_context->rdm) / 2) - (ifx_Float_t)cmax) * rdm_context->speed_spec.value_bin_per_step;

    *range_idx = rmax;
    if (abs(speed)>= 4) 
    {
        *range_idx = 100;
        return (ifx_error_get());
    }

        printf("RangeIDX : %d, range_m:%0.3f, speed_m_s:%0.2f\n",rmax, range, speed);
    return (ifx_error_get());
}