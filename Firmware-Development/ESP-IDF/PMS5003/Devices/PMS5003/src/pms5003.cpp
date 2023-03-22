#include "pms5003.hpp"

PMS5003::PMS5003()
{
    config.setPin = GPIO_NUM_39;
    config.resetPin = GPIO_NUM_42;
    config.rxdPin = GPIO_NUM_41;
    config.txdPin = GPIO_NUM_40;
    config.uartInstance = UART_NUM_1;
    config.uartBuffSize = 128;
}

PMS5003::PMS5003(pmsConfig * pConfig)
{
    memcpy(&config, pConfig, sizeof(pmsConfig));
}

void PMS5003::setup()
{
    // Initializing UART interface
    uart_config_t uartConfig = 
    {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(config.uartInstance, &uartConfig);
    uart_set_pin(config.uartInstance, config.txdPin, config.rxdPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(config.uartInstance, config.uartBuffSize * 2, 0, 0, NULL, 0);

    // Initializing GPIO pins
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << config.setPin) | (1ULL << config.resetPin);
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    gpio_set_level(config.setPin, 0);
    gpio_set_level(config.resetPin, 1);
}

void PMS5003::makeMeasurement()
{
    uint8_t* data = (uint8_t*) malloc(config.uartBuffSize);

    gpio_set_level(config.setPin, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for a second...
    uart_flush(config.uartInstance);

    pmsData currentReading;
    clearData();

    int measurementsNumber = 0;
    while (measurementsNumber < 10)
    {
        int len = uart_read_bytes(config.uartInstance, data, config.uartBuffSize, 1);
        if (len > 0 && processData(len, data, &currentReading))
        {
            addMeasurements(&currentReading);
            measurementsNumber++;
        }
    }
    divMeasurement(); 

    gpio_set_level(config.setPin, 0);
}

int PMS5003::processData(int len, uint8_t* data, pmsData* newReading)
{
    // Length test
    if (len != 32)
        return 0;

    // Start of frame delimiter test
    if (data[0] != 0x42 || data[1] != 0x4D || data[2] != 0x00 || data[3] != 0x1C)
        return 0;

    // Checksum test
    int checkSum = 0, checkSumH, checkSumL;
    for (int i = 0; i < 30; i++)
        checkSum += data[i];
    checkSumH = (checkSum >> 8) & 0xFF;
    checkSumL = checkSum & 0xFF;
    if (data[30] != checkSumH || data[31] != checkSumL)
        return 0;

    // Reading data
    newReading->stdPM1_0   = (data[4] << 8) + data[5];
    newReading->stdPM2_5   = (data[6] << 8) + data[7];
    newReading->stdpm10    = (data[8] << 8) + data[9];
    newReading->atmPM1_0   = (data[10] << 8) + data[11];
    newReading->atmPM2_5   = (data[12] << 8) + data[13];
    newReading->atmPM10    = (data[14] << 8) + data[15];

    return 1;
}

void PMS5003::printData()
{
    printf("PM1.0 concentration in standard material:       %d ug/m^3\r\n", readings.stdPM1_0);
    printf("PM2.5 concentration in standard material:       %d ug/m^3\r\n", readings.stdPM2_5);
    printf("PM10  concentration in standard material:       %d ug/m^3\r\n", readings.stdpm10);
    printf("PM1.0 concentration in atmospheric environment: %d ug/m^3\r\n", readings.atmPM1_0);
    printf("PM2.5 concentration in atmospheric environment: %d ug/m^3\r\n", readings.atmPM2_5);
    printf("PM10  concentration in atmospheric environment: %d ug/m^3\r\n", readings.atmPM10);
    printf("\r\n");
}

void PMS5003::clearData()
{
    readings.stdPM1_0 = 0;
    readings.stdPM2_5 = 0;
    readings.stdpm10 = 0;
    readings.atmPM1_0 = 0;
    readings.atmPM2_5 = 0;
    readings.atmPM10 = 0;
}

void PMS5003::addMeasurements(pmsData* newReadings)
{
    readings.stdPM1_0   += newReadings->stdPM1_0;
    readings.stdPM2_5   += newReadings->stdPM2_5;
    readings.stdpm10    += newReadings->stdpm10;
    readings.atmPM1_0   += newReadings->atmPM1_0;
    readings.atmPM2_5   += newReadings->atmPM2_5;
    readings.atmPM10    += newReadings->atmPM10;
}

void PMS5003::divMeasurement()
{
    readings.stdPM1_0  /= 10;
    readings.stdPM2_5  /= 10;
    readings.stdpm10   /= 10;
    readings.atmPM1_0  /= 10;
    readings.atmPM2_5  /= 10;
    readings.atmPM10   /= 10;
}
