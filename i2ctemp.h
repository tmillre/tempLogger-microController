/*
 * Sets up Temperature Sensor peripherals (i2c, timer2)
 * Must be called before getTemphumid
 */
void setupTempSensor(void);


/*
 * Reads temperature, humidity and pressure off of BME280 device.
 * Returns pointer to array with the data in order 0 - temperature, 1 - pressure,
 * 2 - humidity.
 */
double* getTempHumid(void);

