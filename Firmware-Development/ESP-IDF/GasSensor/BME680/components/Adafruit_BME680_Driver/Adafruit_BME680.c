// /*!
//  * @file Adafruit_BME680.cpp
//  *
//  * @mainpage Adafruit BME680 temperature, humidity, barometric pressure and gas
//  * sensor driver
//  *
//  * @section intro_sec Introduction
//  *
//  * This is the documentation for Adafruit's BME680 driver for the
//  * Arduino platform.  It is designed specifically to work with the
//  * Adafruit BME680 breakout: https://www.adafruit.com/products/3660
//  *
//  * These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
//  * to interface with the breakout.
//  *
//  * Adafruit invests time and resources providing this open source code,
//  * please support Adafruit and open-source hardware by purchasing
//  * products from Adafruit!
//  *
//  * @section author Author
//  *
//  * Written by Ladyada for Adafruit Industries.
//  *
//  * @section license License
//  *
//  * BSD license, all text here must be included in any redistribution.
//  *
//  */

#include "Adafruit_BME680.h"

  /** Value returned by remainingReadingMillis indicating no asynchronous
   * reading has been initiated by beginReading. **/
  static int reading_not_started = -1;
  /** Value returned by remainingReadingMillis indicating asynchronous reading
   * is complete and calling endReading will not block. **/
  static int reading_complete = 0;

    /** Temperature (Celsius) assigned after calling bme680_perform_reading() or
   * endReading() **/
  float temperature;
  /** Pressure (Pascals) assigned after calling bme680_perform_reading() or endReading()
   * **/
  uint32_t pressure;
  /** Humidity (RH %) assigned after calling bme680_perform_reading() or endReading()
   * **/
  float humidity;
  /** Gas resistor (ohms) assigned after calling bme680_perform_reading() or
   * endReading() **/
  uint32_t gas_resistance;


  int32_t _sensorID;
  uint32_t _meas_start = 0;
  uint16_t _meas_period = 0;

  struct bme68x_dev gas_sensor;
  struct bme68x_conf gas_conf;
  struct bme68x_heatr_conf gas_heatr_conf;

//#define BME680_DEBUG

/** Our hardware interface functions **/
static int8_t i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len,
                       void *interface);
static int8_t i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len,
                        void *interface);
static void delay_usec(uint32_t us, void *intf_ptr);

// PUBLIC FUNCTIONS

/*!
 *  @brief  Instantiates sensor with i2c.
 *  @param  *theWire
 *          optional Wire object
 */
// Adafruit_BME680::Adafruit_BME680(gpio_num_t sda, gpio_num_t scl)
//     : _meas_start(0), _meas_period(0) {
//   bmeSDA = sda;
//   bmeSCL = scl;
// }

/*!
 *  @brief  Initializes the sensor
 *          Hardware ss initialized, verifies it is in the I2C or SPI bus, then
 * reads calibration data in preparation for sensor reads.
 *  @param  addr
 *          Optional parameter for the I2C address of BME680. Default is 0x77
 *  @param  init_settings
 *          Optional parameter for initializing the sensor settings.
 *          Default is true.
 *  @return True on sensor initialization success. False on failure.
 */
#define check(x) printf("failed %d",x);
bool bme680_begin(gpio_num_t sda, gpio_num_t scl, uint8_t addr, bool init_settings) {
  int8_t rslt;

	i2c_config_t conf = { };
    conf.mode = I2C_MODE_MASTER,
    conf.sda_io_num = sda,
    conf.scl_io_num = scl,
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE,
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE,
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ,

	i2c_param_config(I2C_MASTER_NUM, &conf);
	ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0));

  gas_sensor.chip_id = addr;
  gas_sensor.intf = BME68X_I2C_INTF;
  // gas_sensor.intf_ptr = (void *)_i2cdev;
  gas_sensor.read = &i2c_read;
  gas_sensor.write = &i2c_write;

  gas_sensor.amb_temp = 25; /* The ambient temperature in deg C is used for
                               defining the heater temperature */
  gas_sensor.delay_us = delay_usec;

  rslt = bme68x_init(&gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("Init Result: "));
  Serial.println(rslt);
#endif

  if (rslt != BME68X_OK)
    return false;

#ifdef BME680_DEBUG
  Serial.print("T1 = ");
  Serial.println(gas_sensor.calib.par_t1);
  Serial.print("T2 = ");
  Serial.println(gas_sensor.calib.par_t2);
  Serial.print("T3 = ");
  Serial.println(gas_sensor.calib.par_t3);
  Serial.print("P1 = ");
  Serial.println(gas_sensor.calib.par_p1);
  Serial.print("P2 = ");
  Serial.println(gas_sensor.calib.par_p2);
  Serial.print("P3 = ");
  Serial.println(gas_sensor.calib.par_p3);
  Serial.print("P4 = ");
  Serial.println(gas_sensor.calib.par_p4);
  Serial.print("P5 = ");
  Serial.println(gas_sensor.calib.par_p5);
  Serial.print("P6 = ");
  Serial.println(gas_sensor.calib.par_p6);
  Serial.print("P7 = ");
  Serial.println(gas_sensor.calib.par_p7);
  Serial.print("P8 = ");
  Serial.println(gas_sensor.calib.par_p8);
  Serial.print("P9 = ");
  Serial.println(gas_sensor.calib.par_p9);
  Serial.print("P10 = ");
  Serial.println(gas_sensor.calib.par_p10);
  Serial.print("H1 = ");
  Serial.println(gas_sensor.calib.par_h1);
  Serial.print("H2 = ");
  Serial.println(gas_sensor.calib.par_h2);
  Serial.print("H3 = ");
  Serial.println(gas_sensor.calib.par_h3);
  Serial.print("H4 = ");
  Serial.println(gas_sensor.calib.par_h4);
  Serial.print("H5 = ");
  Serial.println(gas_sensor.calib.par_h5);
  Serial.print("H6 = ");
  Serial.println(gas_sensor.calib.par_h6);
  Serial.print("H7 = ");
  Serial.println(gas_sensor.calib.par_h7);
  Serial.print("G1 = ");
  Serial.println(gas_sensor.calib.par_gh1);
  Serial.print("G2 = ");
  Serial.println(gas_sensor.calib.par_gh2);
  Serial.print("G3 = ");
  Serial.println(gas_sensor.calib.par_gh3);
  Serial.print("G1 = ");
  Serial.println(gas_sensor.calib.par_gh1);
  Serial.print("G2 = ");
  Serial.println(gas_sensor.calib.par_gh2);
  Serial.print("G3 = ");
  Serial.println(gas_sensor.calib.par_gh3);
  Serial.print("Heat Range = ");
  Serial.println(gas_sensor.calib.res_heat_range);
  Serial.print("Heat Val = ");
  Serial.println(gas_sensor.calib.res_heat_val);
  Serial.print("SW Error = ");
  Serial.println(gas_sensor.calib.range_sw_err);
#endif

  if (init_settings) {
    bme680_set_iir_filter_size(BME68X_FILTER_SIZE_3);
    bme680_set_odr(BME68X_ODR_NONE);
    bme680_set_humidity_oversampling(BME68X_OS_2X);
    bme680_set_pressure_oversampling(BME68X_OS_4X);
    bme680_set_temperature_oversampling(BME68X_OS_8X);
    bme680_set_gas_heater(320, 150); // 320*C for 150 ms
  } else {
    bme680_set_gas_heater(0, 0);
  }
  // don't do anything till we request a reading
  rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &gas_sensor);

#ifdef BME680_DEBUG
  Serial.print(F("Opmode Result: "));
  Serial.println(rslt);
#endif

  if (rslt != BME68X_OK)
    return false;

  return true;
}

/*!
 *  @brief  Performs a reading and returns the ambient temperature.
 *  @return Temperature in degrees Centigrade
 */
float bme680_read_temperature(void) {
  bme680_perform_reading();
  return temperature;
}

/*!
 *  @brief Performs a reading and returns the barometric pressure.
 *  @return Barometic pressure in Pascals
 */
float bme680_read_pressure(void) {
  bme680_perform_reading();
  return pressure;
}

/*!
 *  @brief  Performs a reading and returns the relative humidity.
 *  @return Relative humidity as floating point
 */
float bme680_read_humidity(void) {
  bme680_perform_reading();
  return humidity;
}

/*!
 *  @brief Calculates the resistance of the MOX gas sensor.
 *  @return Resistance in Ohms
 */
uint32_t bme680_read_gas(void) {
  bme680_perform_reading();
  return gas_resistance;
}

/*!
 *  @brief  Calculates the altitude (in meters).
 *          Reads the current atmostpheric pressure (in hPa) from the sensor and
 * calculates via the provided sea-level pressure (in hPa).
 *  @param  sea_level
 *          Sea-level pressure in hPa
 *  @return Altitude in meters
 */
float bme680_read_altitude(float sea_level) {
  // Equation taken from BMP180 datasheet (page 16):
  //  http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

  // Note that using the equation from wikipedia can give bad results
  // at high altitude. See this thread for more information:
  //  http://forums.adafruit.com/viewtopic.php?f=22&t=58064

  float atmospheric = bme680_read_pressure() / 100.0F;
  return 44330.0 * (1.0 - pow(atmospheric / sea_level, 0.1903));
}

/*!
 *  @brief  Performs a full reading of all 4 sensors in the BME680.
 *          Assigns the internal Adafruit_BME680#temperature,
 * Adafruit_BME680#pressure, Adafruit_BME680#humidity and
 * Adafruit_BME680#gas_resistance member variables
 *  @return True on success, False on failure
 */
bool bme680_perform_reading(void) 
{ 
  return bme680_end_reading(); 
}

/*! @brief Begin an asynchronous reading.
 *  @return When the reading would be ready as absolute time in millis().
 */
uint32_t bme680_begin_reading(void) {
  if (_meas_start != 0) {
    /* A measurement is already in progress */
    return _meas_start + _meas_period;
  }

  int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("Opmode Result: "));
  Serial.println(rslt);
#endif
  if (rslt != BME68X_OK)
    return false;

  /* Calculate delay period in microseconds */
  uint32_t delayus_period = (uint32_t)bme68x_get_meas_dur(
                                BME68X_FORCED_MODE, &gas_conf, &gas_sensor) +
                            ((uint32_t)gas_heatr_conf.heatr_dur * 1000);
  // Serial.print("measure: ");
  // Serial.println(bme68x_get_meas_dur(BME68X_FORCED_MODE, &gas_conf,
  // &gas_sensor)); Serial.print("heater: ");
  // Serial.println((uint32_t)gas_heatr_conf.heatr_dur * 1000);

  _meas_start = millis();
  _meas_period = delayus_period / 1000;

  return _meas_start + _meas_period;
}

/*! @brief  End an asynchronous reading.
 *          If the asynchronous reading is still in progress, block until it
 * ends. If no asynchronous reading has started, this is equivalent to
 * bme680_perform_reading().
 *  @return Whether success.
 */
bool bme680_end_reading(void) {
  uint32_t meas_end = bme680_begin_reading();

  if (meas_end == 0) {
    return false;
  }

  int remaining_millis = bme680_remaining_reading_millis();

  if (remaining_millis > 0) {
#ifdef BME680_DEBUG
    Serial.print(F("Waiting (ms) "));
    Serial.println(remaining_millis);
#endif
    vTaskDelay(((float)((remaining_millis) * 2)) / portTICK_PERIOD_MS); /* Delay till the measurement is ready */
  }
  _meas_start = 0; /* Allow new measurement to begin */
  _meas_period = 0;

#ifdef BME680_DEBUG
  Serial.print(F("t_fine = "));
  Serial.println(gas_sensor.calib.t_fine);
#endif

  struct bme68x_data data;
  uint8_t n_fields;

#ifdef BME680_DEBUG
  Serial.println(F("Getting sensor data"));
#endif

  int8_t rslt =
      bme68x_get_data(BME68X_FORCED_MODE, &data, &n_fields, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("GetData Result: "));
  Serial.println(rslt);
#endif
  if (rslt != BME68X_OK)
    return false;

  if (n_fields) {
    temperature = data.temperature;
    humidity = data.humidity;
    pressure = data.pressure;

#ifdef BME680_DEBUG
    Serial.print(F("data.status 0x"));
    Serial.println(data.status, HEX);
#endif
    if (data.status & (BME68X_HEAT_STAB_MSK | BME68X_GASM_VALID_MSK)) {
      // Serial.print("Gas resistance: "); Serial.println(data.gas_resistance);
      gas_resistance = data.gas_resistance;
    } else {
      gas_resistance = 0;
      // Serial.println("Gas reading unstable!");
    }
  }

  return true;
}

/*! @brief  Get remaining time for an asynchronous reading.
 *          If the asynchronous reading is still in progress, how many millis
 * until its completion. If the asynchronous reading is completed, 0. If no
 * asynchronous reading has started, -1 or
 * Adafruit_BME680::reading_not_started. Does not block.
 *  @return Remaining millis until endReading will not block if invoked.
 */
int bme680_remaining_reading_millis(void) {
  if (_meas_start != 0) {
    /* A measurement is already in progress */
    int remaining_time = (int)_meas_period - (millis() - _meas_start);
    return remaining_time < 0 ? reading_complete : remaining_time;
  }
  return reading_not_started;
}

/*!
 *  @brief  Enable and configure gas reading + heater
 *  @param  heater_temp
 *          Desired temperature in degrees Centigrade
 *  @param  heater_time
 *          Time to keep heater on in milliseconds
 *  @return True on success, False on failure
 */
bool bme680_set_gas_heater(uint16_t heater_temp, uint16_t heater_time) {

  if ((heater_temp == 0) || (heater_time == 0)) {
    gas_heatr_conf.enable = BME68X_DISABLE;
  } else {
    gas_heatr_conf.enable = BME68X_ENABLE;
    gas_heatr_conf.heatr_temp = heater_temp;
    gas_heatr_conf.heatr_dur = heater_time;
  }

  int8_t rslt =
      bme68x_set_heatr_conf(BME68X_FORCED_MODE, &gas_heatr_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetHeaterConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Output Data Rate
 *  @param  odr
 *          Output data rate setting, can be BME68X_ODR_NONE,
 * BME68X_ODR_0_59_MS, BME68X_ODR_10_MS, BME68X_ODR_20_MS, BME68X_ODR_62_5_MS,
 * BME68X_ODR_125_MS, BME68X_ODR_250_MS, BME68X_ODR_500_MS, BME68X_ODR_1000_MS
 *  @return True on success, False on failure
 */

bool bme680_set_odr(uint8_t odr) {
  if (odr > BME68X_ODR_NONE)
    return false;

  gas_conf.odr = odr;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Temperature oversampling
 *  @param  oversample
 *          Oversampling setting, can be BME68X_OS_NONE (turn off Temperature
 * reading), BME68X_OS_1X, BME68X_OS_2X, BME68X_OS_4X, BME68X_OS_8X or
 * BME68X_OS_16X
 *  @return True on success, False on failure
 */

bool bme680_set_temperature_oversampling(uint8_t oversample) {
  if (oversample > BME68X_OS_16X)
    return false;

  gas_conf.os_temp = oversample;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Humidity oversampling
 *  @param  oversample
 *          Oversampling setting, can be BME68X_OS_NONE (turn off Humidity
 * reading), BME68X_OS_1X, BME68X_OS_2X, BME68X_OS_4X, BME68X_OS_8X or
 * BME68X_OS_16X
 *  @return True on success, False on failure
 */
bool bme680_set_humidity_oversampling(uint8_t oversample) {
  if (oversample > BME68X_OS_16X)
    return false;

  gas_conf.os_hum = oversample;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for Pressure oversampling
 *  @param  oversample
 *          Oversampling setting, can be BME68X_OS_NONE (turn off Pressure
 * reading), BME68X_OS_1X, BME68X_OS_2X, BME68X_OS_4X, BME68X_OS_8X or
 * BME68X_OS_16X
 *  @return True on success, False on failure
 */
bool bme680_set_pressure_oversampling(uint8_t oversample) {
  if (oversample > BME68X_OS_16X)
    return false;

  gas_conf.os_pres = oversample;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Setter for IIR filter.
 *  @param  filtersize
 *          Size of the filter (in samples).
 *          Can be BME68X_FILTER_SIZE_0 (no filtering), BME68X_FILTER_SIZE_1,
 * BME68X_FILTER_SIZE_3, BME68X_FILTER_SIZE_7, BME68X_FILTER_SIZE_15,
 * BME68X_FILTER_SIZE_31, BME68X_FILTER_SIZE_63, BME68X_FILTER_SIZE_127
 *  @return True on success, False on failure
 */
bool bme680_set_iir_filter_size(uint8_t filtersize) {
  if (filtersize > BME68X_FILTER_SIZE_127)
    return false;
  gas_conf.filter = filtersize;

  int8_t rslt = bme68x_set_conf(&gas_conf, &gas_sensor);
#ifdef BME680_DEBUG
  Serial.print(F("SetConf Result: "));
  Serial.println(rslt);
#endif
  return rslt == 0;
}

/*!
 *  @brief  Reads 8 bit values over I2C
 */
int8_t i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf) {

  esp_err_t _error;
  _error = i2c_master_write_read_device(I2C_MASTER_NUM, BME68X_DEFAULT_ADDRESS, &reg_addr, 1, reg_data, len, 1000 / portTICK_PERIOD_MS);

	if (_error != ESP_OK)
	{
		return -1;
	}

    return 0;
}

/*!
 *  @brief  Writes 8 bit values over I2C
 */
int8_t i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len,
                 void *intf) {
  
  esp_err_t _error;
  uint8_t write_buf[len+1];
  write_buf[0] = reg_addr;
  for (uint16_t i = 0; i < len; i++)
  {
    write_buf[i+1] = reg_data[i];
  }
  
	_error = i2c_master_write_to_device(I2C_MASTER_NUM, BME68X_DEFAULT_ADDRESS, write_buf, len+1, 1000 / portTICK_PERIOD_MS);

	if (_error != ESP_OK)
	{
		return -1;
	}

  return 0;
}

static void delay_usec(uint32_t us, void *intf_ptr) {
  (void)intf_ptr; // Unused parameter
  delay_microseconds(us);
}

#define NOP() asm volatile ("nop")

void IRAM_ATTR delay_microseconds(uint32_t us)
{
  uint64_t m = (uint64_t)esp_timer_get_time();
  if(us){
      uint64_t e = (m + us);
      if(m > e){ //overflow
          while((uint64_t)esp_timer_get_time() > e){
              NOP();
          }
      }
      while((uint64_t)esp_timer_get_time() < e){
          NOP();
      }
  }
}