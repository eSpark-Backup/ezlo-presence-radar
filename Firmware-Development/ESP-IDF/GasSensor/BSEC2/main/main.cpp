#include <stdio.h>
#include "bme68xLibrary.h"
#include "bsec2.h"

#define I2C_MASTER_FREQ_HZ 100000
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {} ;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 18;
    conf.scl_io_num = 17;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(ACTIVE_I2C, &conf);
    return i2c_driver_install(ACTIVE_I2C, conf.mode, 0, 0, 0);
}

/**
 * @brief : This function checks the BSEC status, prints the respective error code. Halts in case of error
 * @param[in] bsec  : Bsec2 class object
 */
void checkBsecStatus(Bsec2 bsec);

/**
 * @brief : This function is called by the BSEC library when a new output is available
 * @param[in] input     : BME68X sensor data before processing
 * @param[in] outputs   : Processed BSEC BSEC output data
 * @param[in] bsec      : Instance of BSEC2 calling the callback
 */
void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);

/* Create an object of the class Bsec2 */
Bsec2 envSensor;

/* Entry point for the example */
void setup(void)
{
    /* Desired subscription list of BSEC2 outputs */
    bsecSensor sensorList[] = {
            BSEC_OUTPUT_IAQ,
            BSEC_OUTPUT_RAW_TEMPERATURE,
            BSEC_OUTPUT_RAW_PRESSURE,
            BSEC_OUTPUT_RAW_HUMIDITY,
            BSEC_OUTPUT_RAW_GAS,
            BSEC_OUTPUT_CO2_EQUIVALENT,
            BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
            BSEC_OUTPUT_STABILIZATION_STATUS,
            BSEC_OUTPUT_RUN_IN_STATUS
    };

    /* Initialize the library and interfaces */
    if (!envSensor.begin())
    {
        checkBsecStatus(envSensor);
    }

    /* Subsribe to the desired BSEC2 outputs */
    if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_LP))
    {
        checkBsecStatus(envSensor);
    }

    /* Whenever new data is available call the newDataCallback function */
    envSensor.attachCallback(newDataCallback);

    printf("BSEC library version %d.%d.%d.%d\n", envSensor.version.major, envSensor.version.minor, envSensor.version.major_bugfix, envSensor.version.minor_bugfix);
}

/* Function that is looped forever */
void loop(void)
{
    /* Call the run function often so that the library can 
     * check if it is time to read new data from the sensor  
     * and process it.
     */
    if (!envSensor.run())
    {
        checkBsecStatus(envSensor);
    }
}

void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
    if (!outputs.nOutputs)
    {
        return;
    }

    printf("BSEC outputs:\n\ttimestamp = %d\n",(int) (outputs.output[0].time_stamp / INT64_C(1000000)));
    for (uint8_t i = 0; i < outputs.nOutputs; i++)
    {
        const bsecData output  = outputs.output[i];
        switch (output.sensor_id)
        {
            case BSEC_OUTPUT_IAQ:
                printf("\tiaq = %0.2f\n",output.signal);
                printf("\tiaq accuracy = %d\n", (int) output.accuracy);
                break;
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                printf("\ttemperature = %0.2f\n", output.signal);
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                printf("\tpressure = %0.2f\n", output.signal);
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                printf("\thumidity = %0.2f\n", output.signal);
                break;
            case BSEC_OUTPUT_RAW_GAS:
                printf("\tgas resistance = %0.2f\n", output.signal);
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                printf("\tCO2 Equivalent = %0.2f\n", output.signal);
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                printf("\tVOC Equivalent = %0.2f\n", output.signal);
                break;
            case BSEC_OUTPUT_STABILIZATION_STATUS:
                printf("\tstabilization status = %0.2f\n", output.signal);
                break;
            case BSEC_OUTPUT_RUN_IN_STATUS:
                printf("\trun in status = %0.2f\n", output.signal);
                break;
            default:
                break;
        }
    }
}

void checkBsecStatus(Bsec2 bsec)
{
    if (bsec.status < BSEC_OK)
    {
        printf("BSEC error code : %d\n",bsec.status);
    }
    else if (bsec.status > BSEC_OK)
    {
        printf("BSEC warning code : %d\n", bsec.status);
    }

    if (bsec.sensor.status < BME68X_OK)
    {
        printf("BME68X error code : %d\n",bsec.sensor.status);
    }
    else if (bsec.sensor.status > BME68X_OK)
    {
        printf("BME68X warning code : %d\n",bsec.sensor.status);
    }
}


extern "C" void app_main(void)
{
    i2c_master_init();
    setup();
    while (1)
    {
       loop();
    }
    
}
