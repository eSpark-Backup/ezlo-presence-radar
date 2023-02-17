#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
==============================================================================
   1. INCLUDE FILES
==============================================================================
*/

#include "DeviceControl.h"
#include <stdio.h>
#include <string.h>
#include "Error.h"

#include "presence_radar_default.h"

ifx_Avian_Device_t* ifx_avian_create_dummy_from_xensiv(xensiv_bgt60trxx_t *bgt60) {
   
	IFX_ERR_BRK_NULL(bgt60);
	ifx_Avian_Device_t * ifx_device = ifx_avian_create();

	IFX_ERR_BRK_NULL(ifx_device);

	// Assign the SPI handel in here
	ifx_device->m_xensiv_device_handle = bgt60;

	// Initialize with the BGT60TR13C sensor at the beginning
	ifx_device->m_sensor_type =  IFX_AVIAN_BGT60TR13C;

	return ifx_device;
}

ifx_Avian_Device_t* ifx_avian_create(void) {

   ifx_Avian_Device_t * ifx_device = malloc(sizeof(* ifx_device));

   IFX_ERR_BRK_MEMALLOC(ifx_device);

   memset(ifx_device, 0, sizeof(* ifx_device));

   ifx_device->m_firmware_info.description = (char *)malloc(sizeof(IFX_SIZE_STR_FMW_DESCRIPTION));
   IFX_ERR_BRK_MEMALLOC(ifx_device->m_firmware_info.description);

   ifx_device->m_sensor_info.description = (char *)malloc(sizeof(IFX_SIZE_STR_SENSOR_DESCRIPTION));
   IFX_ERR_BRK_MEMALLOC(ifx_device->m_sensor_info.description);

   return ifx_device;
}

void ifx_avian_set_config(ifx_Avian_Device_t* handle, const ifx_Avian_Config_t* config) {

      // TODO: To be defined later with these getter and setter functions
   // int32_t xensiv_bgt60trxx_set_reg(const xensiv_bgt60trxx_t* dev, uint32_t reg_addr, uint32_t data)
   // int32_t xensiv_bgt60trxx_get_reg(const xensiv_bgt60trxx_t* dev, uint32_t reg_addr, uint32_t* data)

}

void ifx_avian_get_config(ifx_Avian_Device_t* handle, ifx_Avian_Config_t* config) {
   
   /**
    * TODO : Config read from device itself
    * Feeding known config at the moment.
    */
       /* default device configuration for presence sensing */
   //  device_config->sample_rate_Hz = 1000000;         // 1 MHz
   //  device_config->rx_mask = (num_rx >= 3) ? 4 : 1;  // Use RX1-3 when device has at least 3 antennas, RX1 otherwise
   //  device_config->tx_mask = 1;
   //  device_config->tx_power_level = TX_POWER_LEVEL_UPPER;
   //  device_config->if_gain_dB = 33;                  // 33 dB
   //  device_config->start_frequency_Hz = 60020000000; // 60.02 GHz
   //  device_config->end_frequency_Hz = 61020000000; // 61.02 GHz
   //  device_config->num_samples_per_chirp = 128;
   //  device_config->num_chirps_per_frame = 64;
   //  device_config->chirp_repetition_time_s = 0.00050564f; // 505.64 µs
   //  device_config->frame_repetition_time_s = 0.2f;    // frame rate of 5 Hz
   //  device_config->hp_cutoff_Hz = 80000;
   //  device_config->aaf_cutoff_Hz = 500000;
   //  device_config->mimo_mode = IFX_MIMO_OFF;


   config->sample_rate_Hz = XENSIV_BGT60TRXX_CONF_SAMPLE_RATE;
   config->rx_mask = 1;
   config->tx_mask =  1;
   config->tx_power_level = 31;
   config->if_gain_dB = 50;
   config->start_frequency_Hz = XENSIV_BGT60TRXX_CONF_START_FREQ_HZ;
   config->end_frequency_Hz = XENSIV_BGT60TRXX_CONF_END_FREQ_HZ;
   config->num_samples_per_chirp = XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP;
   config->num_chirps_per_frame = XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME;
   config->chirp_repetition_time_s = XENSIV_BGT60TRXX_CONF_CHIRP_REPETITION_TIME_S;
   config->frame_repetition_time_s = XENSIV_BGT60TRXX_CONF_FRAME_REPETITION_TIME_S;
   config->hp_cutoff_Hz = 80000;
   config->aaf_cutoff_Hz = 500000;  
   // to be defined 
 
}


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
