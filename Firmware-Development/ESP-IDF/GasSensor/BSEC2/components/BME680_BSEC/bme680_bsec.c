#include "bme680_bsec.h"
/* Entry point for the example */
void bme680_setup(void)
{
    /* Desired subscription list of BSEC2 outputs */
    bsec_sensor sensor_list[] = {
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

    bsec2_setup();
    /* Initialize the library and interfaces */
    if (!bsec2_begin())
    {
        check_bsec_status();
    }

    /* Subsribe to the desired BSEC2 outputs */
    if (!bsec2_update_subscription(sensor_list, ARRAY_LEN(sensor_list), BSEC_SAMPLE_RATE_LP))
    {
        check_bsec_status();
    }

    /* Whenever new data is available call the data_callback function */
    bsec2_attach_callback(bme680_data_callback);

    bsec_version_t bsec2_version = bsec2_get_version();
    printf("BSEC library version %d.%d.%d.%d\n", bsec2_version.major, bsec2_version.minor, bsec2_version.major_bugfix, bsec2_version.minor_bugfix);
}

/* Function that is looped forever */
void bme680_test(void)
{
    /* Call the run function often so that the library can 
     * check if it is time to read new data from the sensor  
     * and process it.
     */
    if (!bsec2_run())
    {
        check_bsec_status();
    }
}

void bme680_data_callback(const bme68x_data data, const bsec_outputs outputs)
{
    if (!outputs.n_outputs)
    {
        return;
    }

    printf("BSEC outputs:\n\ttimestamp = %d\n",(int) (outputs.output[0].time_stamp / INT64_C(1000000)));
    for (uint8_t i = 0; i < outputs.n_outputs; i++)
    {
        const bsec_data output  = outputs.output[i];
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

void check_bsec_status()
{
    if (bsec2_get_status() < BSEC_OK)
    {
        printf("BSEC error code : %d\n",bsec2_get_status());
    }
    else if (bsec2_get_status() > BSEC_OK)
    {
        printf("BSEC warning code : %d\n", bsec2_get_status());
    }

    if (bsec2_get_sensor_status()< BME68X_OK)
    {
        printf("BME68X error code : %d\n",bsec2_get_sensor_status());
    }
    else if (bsec2_get_sensor_status() > BME68X_OK)
    {
        printf("BME68X warning code : %d\n",bsec2_get_sensor_status());
    }
}