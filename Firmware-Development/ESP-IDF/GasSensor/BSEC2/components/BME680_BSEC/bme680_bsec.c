#include "bme680_bsec.h"

static bool callback_status = false;
static bme680_data_t bme680_data; 

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

/* Function to copy sensor data */
static bool bme680_copy_data(bme680_data_t* dest, bme680_data_t* src)
{
    if ((dest == NULL) && (src == NULL))
    {
        return false;
    }
    
    dest->iaq                   = src->iaq;
    dest->iaq_accuracy          = src->iaq_accuracy;
    dest->temperature           = src->temperature;
    dest->pressure              = src->pressure;
    dest->humidity              = src->humidity;
    dest->gas_resistance        = src->gas_resistance;
    dest->co2_equivalent        = src->co2_equivalent;
    dest->voc_equivalent        = src->voc_equivalent;
    dest->stabilization_status  = src->stabilization_status;
    dest->run_in_status         = src->run_in_status;
    return true;
}

/* Function that is looped forever */
bool bme680_get_data(bme680_data_t* data)
{
    /* Call the run function often so that the library can 
     * check if it is time to read new data from the sensor  
     * and process it.
     */
    if (!bsec2_run())
    {
        check_bsec_status();
        return false;
    }

    if (callback_status)
    {
        if (bme680_copy_data(data, &bme680_data))
        {
            callback_status = false;
            return true;
        }
    }

    return false;
}

void bme680_data_callback(const bme68x_data data, const bsec_outputs outputs)
{
    if (!outputs.n_outputs)
    {
        return;
    }
    #if BME680_TEST
        printf("BSEC outputs:\n\ttimestamp = %d\n",(int) (outputs.output[0].time_stamp / INT64_C(1000000)));
    #endif

    callback_status = true;

    for (uint8_t i = 0; i < outputs.n_outputs; i++)
    {
        const bsec_data output  = outputs.output[i];
        switch (output.sensor_id)
        {
            case BSEC_OUTPUT_IAQ:
            #if BME680_TEST
                printf("\tiaq = %0.2f\n",output.signal);
                printf("\tiaq accuracy = %d\n", (int) output.accuracy);
            #endif
                bme680_data.iaq = output.signal;
                bme680_data.iaq_accuracy = output.accuracy;
                break;

            case BSEC_OUTPUT_RAW_TEMPERATURE:
            #if BME680_TEST
                printf("\ttemperature = %0.2f\n", output.signal);
            #endif
                bme680_data.temperature = output.signal;
                break;

            case BSEC_OUTPUT_RAW_PRESSURE:
            #if BME680_TEST
                printf("\tpressure = %0.2f\n", output.signal);
            #endif
                bme680_data.pressure = output.signal;
                break;

            case BSEC_OUTPUT_RAW_HUMIDITY:
            #if BME680_TEST
                printf("\thumidity = %0.2f\n", output.signal);
            #endif
                bme680_data.humidity = output.signal;
                break;

            case BSEC_OUTPUT_RAW_GAS:
            #if BME680_TEST
                printf("\tgas resistance = %0.2f\n", output.signal);
            #endif
                bme680_data.gas_resistance = output.signal;
                break;

            case BSEC_OUTPUT_CO2_EQUIVALENT:
            #if BME680_TEST
                printf("\tCO2 Equivalent = %0.2f\n", output.signal);
            #endif
                bme680_data.co2_equivalent = output.signal;
                break;

            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
            #if BME680_TEST
                printf("\tVOC Equivalent = %0.2f\n", output.signal);
            #endif
                bme680_data.voc_equivalent = output.signal;
                break;

            case BSEC_OUTPUT_STABILIZATION_STATUS:
            #if BME680_TEST
                printf("\tstabilization status = %0.2f\n", output.signal);
            #endif
                bme680_data.stabilization_status = output.signal;
                break;

            case BSEC_OUTPUT_RUN_IN_STATUS:
            #if BME680_TEST
                printf("\trun in status = %0.2f\n", output.signal);
            #endif
                bme680_data.run_in_status = output.signal;
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

bool bme680_print_data(bme680_data_t* data)
{
    if (data == NULL)
    {
        return false;
    }
    
    printf("BSEC Outputs:\n");
    printf("\tiaq = %0.2f\n",data->iaq);    
    printf("\tiaq accuracy = %d\n", (int) data->iaq_accuracy);
    printf("\ttemperature = %0.2f °C\n", data->temperature);
    printf("\tpressure = %0.2f hPa\n", data->pressure);
    printf("\thumidity = %0.2f %%\n", data->humidity);
    printf("\tgas resistance = %0.2f Ω\n", data->gas_resistance);
    printf("\tCO2 Equivalent = %0.2f\n", data->co2_equivalent);
    printf("\tVOC Equivalent = %0.2f\n", data->voc_equivalent);
    printf("\tstabilization status = %d\n", data->stabilization_status);
    printf("\trun in status = %d\n", data->run_in_status);
    return true;
}