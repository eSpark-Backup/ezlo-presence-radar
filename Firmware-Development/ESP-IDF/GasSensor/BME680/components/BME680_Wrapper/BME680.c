#include "BME680.h"

esp_err_t bme680_setup(Precision precision)
{  
    printf("BME680 Starting\n");

    if (bme680_begin(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, BME68X_DEFAULT_ADDRESS, true)) {
        printf("Could not find a valid BME680 sensor, check wiring!\n");
        while (1);
    }

    bme680_set_precision(precision);
    bme680_set_gas_heater(320, 150); // 320*C for 150 ms
    return ESP_OK;
}

esp_err_t bme680_set_precision(Precision precision)
{
    // Set up oversampling and filter initialization
    bme680_set_temperature_oversampling( (precision) ? BME680_OS_16X : BME680_OS_8X);
    bme680_set_humidity_oversampling( (precision) ? BME680_OS_16X : BME680_OS_2X);
    bme680_set_pressure_oversampling( (precision) ? BME680_OS_16X : BME680_OS_4X);
    bme680_set_iir_filter_size( (precision) ? BME680_FILTER_SIZE_127 : BME680_FILTER_SIZE_3);
    
    return ESP_OK;
}

esp_err_t bme680_test(void)
{
    if (! bme680_perform_reading()) {
        printf("Failed to perform reading :(\n");
        return ESP_FAIL;
    }
    printf("Temperature = %0.2f *C\n",bme.temperature);

    printf("Pressure = %0.2f hPa\n",bme.pressure / 100.0);

    printf("Humidity = %0.2f %%\n",bme.humidity);

    printf("Gas = %0.2f KOhms\n",(bme.gas_resistance / 1000.0));

    printf("Approx. Altitude = %0.2f m\n\n",bme.readAltitude(SEALEVELPRESSURE_HPA));
    return ESP_OK;
}

esp_err_t bme680_read(Fields* fields)
{
    if (! bme680_perform_reading()) {
        printf("Failed to perform reading :(\n");
        return ESP_FAIL;
    }

    fields->temperature = bme.temperature;
    fields->pressure = bme.pressure / 100.0;
    fields->humidity = bme.humidity;
    fields->gas_resistance = bme.gas_resistance / 1000.0;
    fields->altitude = bme680_read_altitude(SEALEVELPRESSURE_HPA);

    return ESP_OK;
}