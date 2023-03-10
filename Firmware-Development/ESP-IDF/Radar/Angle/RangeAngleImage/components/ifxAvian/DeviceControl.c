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
   config->rx_mask = 5;
   config->tx_mask =  1;
   config->tx_power_level = 31;
   config->if_gain_dB = IF_DB_GAIN;
   config->start_frequency_Hz = XENSIV_BGT60TRXX_CONF_START_FREQ_HZ;
   config->end_frequency_Hz = XENSIV_BGT60TRXX_CONF_END_FREQ_HZ;
   config->num_samples_per_chirp = XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP;
   config->num_chirps_per_frame = XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME;
   config->chirp_repetition_time_s = XENSIV_BGT60TRXX_CONF_CHIRP_REPETITION_TIME_S;
   config->frame_repetition_time_s = XENSIV_BGT60TRXX_CONF_FRAME_REPETITION_TIME_S;
   config->hp_cutoff_Hz = 80000;
   config->aaf_cutoff_Hz = 500000;  
   config->mimo_mode = IFX_MIMO_OFF;
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

#define HEADER_OFFSET 6
#define SAMP_PER_FRAME (XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP * XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)
#define TOTAL_DATA (SAMP_PER_FRAME + (6 * XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME))
ifx_Avian_Config_t cfg;// = (ifx_Avian_Config_t *)malloc(sizeof(ifx_Avian_Config_t));

ifx_Cube_R_t* ifx_avian_get_next_frame(ifx_Avian_Device_t* handle,
    ifx_Cube_R_t* frame, uint16_t* samples) {

   

   ifx_avian_get_config(handle, &cfg);
   // const uint32_t num_samples_per_frame = cfg->num_chirps_per_frame * cfg->num_samples_per_chirp;
   // const uint32_t num_total_fifo_data = num_samples_per_frame + (6 * cfg->num_chirps_per_frame);
   // = (uint16_t *)malloc(sizeof(uint16_t) * num_total_fifo_data);

   frame->rows = XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS; // No of antennas
   frame->cols = cfg.num_chirps_per_frame;
   frame->slices = cfg.num_samples_per_chirp;
   frame->stride[0] = 1;
   frame->stride[1] = frame->slices;
   // frame->stride[2] = frame->cols * frame->slices;
   
   uint32_t samp_no=0;
   uint32_t frame_idx=0;
   uint32_t samp_idx=0;
   for (uint16_t chirp_index=0; chirp_index< cfg.num_chirps_per_frame; chirp_index++) {
      if((samples[samp_no] != 0) && (samples[samp_no+1] != 0)) 
      {
         ESP_LOGI("FIFO","Sync Word Error");
         return frame;
      }

      if(samples[samp_no+3] != chirp_index) {
         ESP_LOGI("FIFO","Chirp missing : %d , Received : %d",chirp_index,samples[3]);
         return frame;
      }

      if(samples[samp_no+4] != frame->slices) // XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP
      {
         ESP_LOGI("FIFO","Chirp Samples Mismatched, Expected : %d , Received : %d", frame->slices, samples[4]);
         return frame;
      }

      frame_idx = samp_no-(chirp_index*HEADER_OFFSET);
      samp_idx = samp_no + HEADER_OFFSET;


      for(uint32_t x = 0; x < frame->slices; x++) {
         frame->d[frame_idx+x] =  (ifx_Float_t)(samples[samp_idx+(2*x)] * 1.0);
      }

      for(uint32_t x = 0; x < frame->slices; x++) {
         frame->d[(SAMP_PER_FRAME/2) + frame_idx+x] =  (ifx_Float_t)(samples[samp_idx+((2*x)+1)] * 1.0);
      }
      
      samp_no +=  (cfg.num_samples_per_chirp*2 + HEADER_OFFSET);   
   }

   
   
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
