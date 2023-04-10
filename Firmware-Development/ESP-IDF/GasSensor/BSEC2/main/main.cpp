#include <stdio.h>
#include "bme68xLibrary.h"

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

Bme68x bme;

/**
 * @brief Initializes the sensor and hardware settings
 */
void setup(void)
{
	/* Initializes the sensor based on SPI library */
	bme.begin();

	if(bme.checkStatus())
	{
		if (bme.checkStatus() == BME68X_ERROR)
		{
			printf("Sensor error: %s\n", bme.statusString().c_str());
			return;
		}
		else if (bme.checkStatus() == BME68X_WARNING)
		{
			printf("Sensor Warning: %s\n", bme.statusString().c_str());
		}
	}

	/* Set the default configuration for temperature, pressure and humidity */
	bme.setTPH();

	/* Heater temperature in degree Celsius */
	uint16_t tempProf[10] = { 100, 200, 320 };
	/* Heating duration in milliseconds */
	uint16_t durProf[10] = { 150, 150, 150 };

	bme.setSeqSleep(BME68X_ODR_250_MS);
	bme.setHeaterProf(tempProf, durProf, 3);
	bme.setOpMode(BME68X_SEQUENTIAL_MODE);

	printf("TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status, Gas index\n");
}

void loop(void)
{
	bme68xData data;
	uint8_t nFieldsLeft = 0;

	vTaskDelay(150/ portTICK_PERIOD_MS);

	if (bme.fetchData())
	{
		do
		{
			nFieldsLeft = bme.getData(data);
			//if (data.status == NEW_GAS_MEAS)
			{
				printf("TimeStamp %0.2f, ", data.temperature);
				printf("%0.2f, ", data.pressure);
				printf("%0.2f, ", data.humidity);
				printf("%0.2f, ", data.gas_resistance);
				printf("%d, ", data.status);
				printf("%d, \n", data.gas_index);
				if(data.gas_index == 2) /* Sequential mode sleeps after this measurement */
					vTaskDelay(250/ portTICK_PERIOD_MS);
			}
		} while (nFieldsLeft);
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
