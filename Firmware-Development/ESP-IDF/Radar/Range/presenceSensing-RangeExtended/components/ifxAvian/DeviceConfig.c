#include "DeviceConfig.h"
#include "Util.h"

ifx_Float_t ifx_devconf_get_center_frequency(const ifx_Avian_Config_t* config) {
    return (config->start_frequency_Hz + config->end_frequency_Hz) / 2;
}

ifx_Float_t ifx_devconf_get_bandwidth(const ifx_Avian_Config_t* config) {
    if (config->start_frequency_Hz > config->end_frequency_Hz)
    {
        return (config->start_frequency_Hz - config->end_frequency_Hz);
    }
    else
    {
        return (config->end_frequency_Hz - config->start_frequency_Hz);
    }
}

ifx_Float_t ifx_devconf_get_chirp_repetition_time(const ifx_Avian_Config_t* config) {
    return config->chirp_repetition_time_s;
}

ifx_Float_t ifx_devconf_get_chirp_time(const ifx_Avian_Config_t* config) {
    return ((ifx_Float_t)(config->num_samples_per_chirp) / (ifx_Float_t)config->sample_rate_Hz);
}

uint8_t ifx_devconf_count_rx_antennas(const ifx_Avian_Config_t* config) {
    const uint8_t tx_antennas = config->mimo_mode == IFX_MIMO_TDM ? 2 : 1;
    return ((uint8_t)(ifx_util_popcount(config->rx_mask)) * tx_antennas); // multiplying with tx antenna count to get virtual antenna count
}
