#pragma once 

#include "driver/gpio.h"
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    gpio_num_t setPin;
    gpio_num_t resetPin;
    gpio_num_t rxdPin;
    gpio_num_t txdPin;
    uart_port_t uartInstance;
    int uartBuffSize;
} pmsConfig;

typedef struct {
    int stdPM1_0;
    int stdPM2_5;
    int stdpm10;
    int atmPM1_0;
    int atmPM2_5;
    int atmPM10;
} pmsData;

class PMS5003
{
    private:
    pmsConfig config;
    pmsData readings;
    public:
    PMS5003();
    PMS5003(pmsConfig * pConfig);
    void setup();
    void makeMeasurement();
    int processData(int len, uint8_t* data, pmsData* reading);
    void printData();
    void clearData();
    void addMeasurements(pmsData* newReadings);
    void divMeasurement();

    // char* getStandardPM();
    // char* getAtmospherePM();
};
