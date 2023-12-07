#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include "esp_log.h"
#include "RF_blaster.h"

const char* TAG="RF_blaster";
#define DEBUG 1
void RF_Generic_API(void *data)
{
  #if DEBUG==1
  ESP_LOGI(TAG, "API FUNCTION");
  #endif
  cc1101_data *pData = (cc1101_data *)data;

  if (pData->event == SET_BUTTON)
  {
    setButton(pData);
  }
  else if (pData->event == FREQ_SWEEP)
  {
    pData->freq = sweepFrequency();
  }
  else if (pData->event == USER_INPUT)
  {
    setMHZ(pData->freq);
    #if DEBUG==1
    ESP_LOGI(TAG, "Frequency set to %f MHz", pData->freq);
    #endif
  }
  else if (pData->event == TRANSMITTING)
  {
    Transmitting(pData);
    #if DEBUG==1
    ESP_LOGI(TAG, "Transmitted");
    #endif
  }
  setSidle();
}

