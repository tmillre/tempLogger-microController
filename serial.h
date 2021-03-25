#define FP 36900000                      //Actual clock freq must be used here

#define BAUDRATE 115200                  //Desired Baudrate
#define BRGVAL ((FP/BAUDRATE)/16)-1      //Function to determine BRG value

#define REC_BUFF_LEN 60                  //Receive Buffer Length

/*
 * Sets up UART peripheral with proper Baud Rate and parameters
 */
void setupUART1(void);

/*
 * Writes character string over UART interface.
 */
void serialWrite(char *s);

/*
 * Polls for response on UART. Returns character array with response.
 */
char* getReply(void);

