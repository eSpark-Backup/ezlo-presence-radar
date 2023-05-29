/*
 *	An Arduino library for the Hi-Link LD2410 24Ghz FMCW radar sensor.
 *
 *  This sensor is a Frequency Modulated Continuous Wave radar, which makes it good for presence detection and its sensitivity at different ranges to both static and moving targets can be configured.
 *
 *	The code in this library is based off the manufacturer datasheet and reading of this initial piece of work for ESPHome https://github.com/rain931215/ESPHome-LD2410.
 *
 *	https://github.com/ncmreynolds/ld2410
 *
 *	Released under LGPL-2.1 see https://github.com/ncmreynolds/ld2410/LICENSE for full license
 *
 */
#ifndef ld2410_h
#define ld2410_h

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ld2410_uart.h"

typedef unsigned char byte;
#define delay(x) vTaskDelay(x/portTICK_RATE_MS)
#define millis() esp_timer_get_time()/1000

#define LD2410_MAX_FRAME_LENGTH 40
#define LD2410_DEBUG_INITIALIZATION
//#define LD2410_DEBUG_DATA
#define LD2410_DEBUG_COMMANDS
//#define LD2410_DEBUG_PARSE

typedef enum
{
	distance_0cm = 0,
	distance_75cm,
	distance_150cm,
	distance_225cm,
	distance_300cm,
	distance_375cm,
	distance_450cm,
	distance_525cm,
	distance_600cm,
} distance;
	
#ifdef __cplusplus
extern "C" {
#endif												//Destructor function
bool ld2410_begin(bool waitForRadar);				//Start the ld2410 // defaults to true
bool ld2410_isConnected();
bool ld2410_read();
bool ld2410_presenceDetected();
bool ld2410_stationaryTargetDetected();
uint16_t ld2410_stationaryTargetDistance();
uint8_t ld2410_stationaryTargetEnergy();
bool ld2410_movingTargetDetected();
uint16_t ld2410_movingTargetDistance();
uint8_t ld2410_movingTargetEnergy();
bool ld2410_requestFirmwareVersion();				//Request the firmware version
bool ld2410_requestCurrentConfiguration();			//Request current configuration

bool ld2410_requestRestart();
bool ld2410_requestFactoryReset();
bool ld2410_requestStartEngineeringMode();
bool ld2410_requestEndEngineeringMode();
bool ld2410_setMaxValues(uint16_t moving, uint16_t stationary, uint16_t inactivityTimer);	//Realistically gate values are 0-8 but sent as uint16_t
bool ld2410_setGateSensitivityThreshold(uint8_t gate, uint8_t moving, uint8_t stationary);
// bool ld2410_setGateSensitivityThreshold(uint8_t* moving, uint8_t* stationary);
#ifdef __cplusplus
}
#endif	
#endif
