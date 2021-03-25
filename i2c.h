/*
 * Read memeory from selected chip, starting at address.
 * len defines how long to read from that address, and ptr specifies where
 * to store the contents.
 */
void readFrom(int chipSel, int startAddr, int len, int* ptr);

/*
 * Write on I2C interface. Typically, you will need to write the chip ID
 * and then the address you want to read/write, then any options after that.
 */
int I2Cwrite(int msg);

/*
 * Sets up I2C interface with proper parameters for BME280.
 * Also sets up timer 2, which is used to timeout waiting for replies, etc.
 */
void setupI2C(void);

