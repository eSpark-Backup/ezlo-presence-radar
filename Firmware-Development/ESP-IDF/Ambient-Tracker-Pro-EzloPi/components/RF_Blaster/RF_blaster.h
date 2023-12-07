#ifndef __radioFrequency__
#define __radioFrequency__

#include <stdint.h>
#include <driver/spi_master.h>
#include <string.h>
#include "esp_log.h"
#include "RCSwitch.h"
#include "ezlopi_spi_master.h"
#include "ezlopi_uart.h"

// typedef uint8_t byte;
// #define HIGH 1
// #define LOW 0

typedef enum events
{
    TRANSMITTING = 0,
    SET_BUTTON,
    USER_INPUT,
    FREQ_SWEEP
} events;

typedef struct cc1101_data
{
    float freq;
    events event;
    unsigned long code;
} cc1101_data;

void RF_Generic_API(void* );


#endif