#include "BME680.hpp"

Adafruit_BME680 bme(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

esp_err_t BME680::setup(void)
{  
    printf("BME680 Starting\n");

    if (!bme.begin()) {
        printf("Could not find a valid BME680 sensor, check wiring!\n");
        while (1);
    }

    // Set up oversampling and filter initialization
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms
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

esp_err_t BME680::read(GasSensorOut* GasSensorOut)
{
    if (! bme.performReading()) {
        printf("Failed to perform reading :(\n");
        return ESP_FAIL;
    }

    GasSensorOut->temperature = bme.temperature;
    GasSensorOut->pressure = bme.pressure / 100.0;
    GasSensorOut->humidity = bme.humidity;
    GasSensorOut->gas_resistance = bme.gas_resistance / 1000.0;
    GasSensorOut->altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

    return ESP_OK;
}