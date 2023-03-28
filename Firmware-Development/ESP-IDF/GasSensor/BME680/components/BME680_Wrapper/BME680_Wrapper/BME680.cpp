#include "BME680.hpp"

Adafruit_BME680 bme(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

esp_err_t BME680::setup(Precision precision)
{  
    printf("BME680 Starting\n");

    if (!bme.begin()) {
        printf("Could not find a valid BME680 sensor, check wiring!\n");
        while (1);
    }

    setPrecision(precision);
    bme.setGasHeater(320, 150); // 320*C for 150 ms
    return ESP_OK;
}

esp_err_t BME680::setPrecision(Precision precision)
{
    // Set up oversampling and filter initialization
    bme.setTemperatureOversampling( (precision) ? BME680_OS_16X : BME680_OS_8X);
    bme.setHumidityOversampling( (precision) ? BME680_OS_16X : BME680_OS_2X);
    bme.setPressureOversampling( (precision) ? BME680_OS_16X : BME680_OS_4X);
    bme.setIIRFilterSize( (precision) ? BME680_FILTER_SIZE_127 : BME680_FILTER_SIZE_3);
    
    return ESP_OK;
}

esp_err_t BME680::test(void)
{
    if (! bme.performReading()) {
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

esp_err_t BME680::read(Fields* fields)
{
    if (! bme.performReading()) {
        printf("Failed to perform reading :(\n");
        return ESP_FAIL;
    }

    fields->temperature = bme.temperature;
    fields->pressure = bme.pressure / 100.0;
    fields->humidity = bme.humidity;
    fields->gas_resistance = bme.gas_resistance / 1000.0;
    fields->altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    return ESP_OK;
}