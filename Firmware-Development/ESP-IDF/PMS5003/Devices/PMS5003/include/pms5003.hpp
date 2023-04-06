#pragma once 

#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"

#define PMS_SET_PIN     GPIO_NUM_39
#define PMS_RESET_PIN   GPIO_NUM_42
#define PMS_RX_PIN      GPIO_NUM_41
#define PMS_TX_PIN      GPIO_NUM_40
#define PMS_UART_PORT   UART_NUM_1
#define PMS_UART_BUF_SIZE 128

/**
 * Structure containing the pms5003 sensor data.
 */
typedef struct pms5003_data
{
  uint16_t pm1cf;   // PM1.0 ug/m3 (ultrafine particles)
  uint16_t pm2_5cf; // PM2.5 ug/m3 (combustion particles, organic compounds, metals)
  uint16_t pm10cf;  // PM10 ug/m3  (dust, pollen, mould spores)
  uint16_t pm1at;   // PM1.0 ug/m3 (atmos env)
  uint16_t pm2_5at; // PM2.5 ug/m3 (atmos env)
  uint16_t pm10at;  // PM10 ug/m3 (atmos env)
  uint16_t gt0_3;   // >0.3um in 0.1L air
  uint16_t gt0_5;   // >0.5um in 0.1L air
  uint16_t gt1;     // >1.0um in 0.1L air
  uint16_t gt2_5;   // >2.5um in 0.1L air
  uint16_t gt5;     // >5.0um in 0.1L air
  uint16_t gt10;    // >10um in 0.1L air
} PMS5003_DATA;

/**
 * Initializes and configures the UART for the PMS5003 using the devices
 * default parameters (optimized for the Raspberry Pi).
 * The UART is set up for device Serial0, 9600 baud, 8-bit, no parity, 
 * no check bit, one stop bit, read-write in a nonblocking mode.
 * 
 * Returns a UART status/error code.
 */
void pms_init();
void pms_uart_setup();
void pms_gpio_config();

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
    int stdPM10;
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
    pmsData getData();
    void makeMeasurement();
    int processData(int len, uint8_t* data, pmsData* reading);
    void printData();
    void clearData();
    void addMeasurements(pmsData* newReadings);
    void divMeasurement();

    // char* getStandardPM();
    // char* getAtmospherePM();
};
