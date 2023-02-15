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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "debug.h"
#include "Error.h"

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


   config->sample_rate_Hz = 1000000;
   // config.rx_mask
   // config.tx_mask
   config->tx_power_level = 31;
   config->if_gain_dB = 33;
   config->start_frequency_Hz = 60020000000;
   config->end_frequency_Hz = 61020000000;
   config->num_samples_per_chirp = 128;
   config->num_chirps_per_frame = 64;
   config->chirp_repetition_time_s = 0.00050564f;
   config->frame_repetition_time_s = 0.2f;
   // config.hp_cutoff_Hz;
   // config.aaf_cutoff_Hz;  
   // to be defined 
 
}

void ifx_avian_get_config_defaults(ifx_Avian_Device_t* handle, ifx_Avian_Config_t* config) {
   // to be defined
}

void ifx_avian_config_get_default_limits(const ifx_Avian_Device_t* handle, ifx_Avian_Config_t* config_lower, ifx_Avian_Config_t* config_upper) {
   // to be defined
}

ifx_Radar_Sensor_t ifx_avian_get_sensor_type(const ifx_Avian_Device_t* handle) {
   return handle->m_sensor_type;
}

void ifx_avian_get_temperature(ifx_Avian_Device_t* handle, ifx_Float_t* temperature_celsius) {

}

void ifx_avian_configure_adc(ifx_Avian_Device_t* handle,
                              const ifx_Avian_ADC_Config_t* config) {

}

void ifx_avian_destroy(ifx_Avian_Device_t* handle) {
   free(handle);
}

void ifx_avian_start_acquisition(ifx_Avian_Device_t* handle) {
   uint32_t err = xensiv_bgt60trxx_start_frame(handle->m_xensiv_device_handle, true);
   IFX_ERR_BRK_COND(err, IFX_ERROR_FRAME_ACQUISITION_FAILED);
}

void ifx_avian_stop_acquisition(ifx_Avian_Device_t* handle) {
	uint32_t err = xensiv_bgt60trxx_start_frame(handle->m_xensiv_device_handle, false);
	IFX_ERR_BRK_INTERNAL((err != XENSIV_BGT60TRXX_STATUS_OK));
}

ifx_Cube_R_t* ifx_avian_get_next_frame(ifx_Avian_Device_t* handle,
    ifx_Cube_R_t* frame) {

   ifx_Avian_Config_t * cfg = (ifx_Avian_Config_t *)malloc(sizeof(ifx_Avian_Config_t));

   ifx_avian_get_config(handle, cfg);
   const uint32_t num_samples_per_frame = cfg->num_chirps_per_frame * cfg->num_samples_per_chirp;
   const uint32_t num_total_fifo_data = num_samples_per_frame + (6 * cfg->num_chirps_per_frame);
   uint16_t *samples = (uint16_t *)malloc(sizeof(uint16_t) * num_total_fifo_data);

   frame->rows = 1; // No of antennas
   frame->cols = cfg->num_chirps_per_frame;
   frame->slices = cfg->num_samples_per_chirp;
   frame->stride[0] = 1;
   frame->stride[1] = frame->cols;
   
   uint8_t err = xensiv_bgt60trxx_get_fifo_data(handle->m_xensiv_device_handle, samples, num_total_fifo_data);

   if(XENSIV_BGT60TRXX_STATUS_OK != err) {
      
      TRACE_B("Error: %d", err);
      free(samples);
      ifx_error_set(IFX_ERROR_HOST);   
      return frame;

   }
   
   uint32_t chirp_index = 0;
   uint32_t idx = 0;   
   while(1) {
      if(samples[idx] == 0) break;
      idx++;
   }

   while(chirp_index < cfg->num_chirps_per_frame) {
      if((samples[idx+1] == 0) && (samples[idx+2] == 0)) {
         if(samples[idx+3] == chirp_index) {
            if(samples[idx+4] == frame->slices) { // XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP
                  idx += 6;
                  uint32_t j = 0;
                  for(uint32_t i = 0; i < frame->slices; i++) {
                     frame->d[j] =  (ifx_Float_t)(samples[idx] * 1.0);
                     j++;
                     idx++;
                  }
                  chirp_index++;                          
            }
         }
      }
   }
   free(samples);
   return frame;

}

ifx_Cube_R_t* ifx_avian_get_next_frame_timeout(ifx_Avian_Device_t* handle, ifx_Cube_R_t* frame, uint16_t timeout_ms) {
   ifx_Cube_R_t* t = NULL;
   return t;
}

const char* ifx_avian_get_board_uuid(const ifx_Avian_Device_t* handle) {
   char * uuid = NULL;
   return uuid;
}

const ifx_Radar_Sensor_Info_t* ifx_avian_get_sensor_information(const ifx_Avian_Device_t* handle) {
   
   ifx_Radar_Sensor_Info_t * sensor_info = (ifx_Radar_Sensor_Info_t *)malloc(sizeof(ifx_Radar_Sensor_Info_t));
   
   // sensor_info->description = (char *)malloc(50);   
   // sprintf(sensor_info->description, "%s", "EzloPi Presence Sensor");

   sensor_info->min_rf_frequency_Hz = 60020000000;
   sensor_info->max_rf_frequency_Hz = 61020000000;
   sensor_info->num_tx_antennas = 1;
   sensor_info->num_rx_antennas = 1;
   sensor_info->max_tx_power = 31;
   sensor_info->num_temp_sensors = 1;
   // sensor_info->interleaved_rx;
   // sensor_info->device_id;                    

   return sensor_info;
}

const ifx_Firmware_Info_t* ifx_avian_get_firmware_information(const ifx_Avian_Device_t* handle) {
   ifx_Firmware_Info_t* t = NULL;
   return t;
}

void ifx_avian_get_shield_information(const ifx_Avian_Device_t* handle, ifx_RF_Shield_Info_t* shield_info) {

}

ifx_Float_t ifx_avian_get_chirp_time(const ifx_Avian_Device_t* handle, const ifx_Avian_Config_t* config) {
   
   return 0.0;
}

ifx_Float_t ifx_avian_get_tx_power(ifx_Avian_Device_t* handle, uint8_t tx_antenna) {
   return 0.0;
}

ifx_Float_t ifx_avian_config_get_min_chirp_repetition_time(const ifx_Avian_Device_t* handle, const ifx_Avian_Config_t* config) {
   return 0.0;
}

ifx_Float_t ifx_avian_config_get_min_frame_repetition_time(const ifx_Avian_Device_t* handle, const ifx_Avian_Config_t* config) {
   return 0.0;
}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
