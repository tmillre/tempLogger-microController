#define SSID ""
#define PASSWORD ""

#define SERVER_IP ""
#define PORT "80"

/*SETUP STRINGS*/
#define ECHO_OFF "ATE1\r\n"
#define SET_STATION_MODE "AT+CWMODE_CUR=1\r\n"
#define CONNECT_START "AT+CWJAP=\"" SSID "\",\"" PASSWORD "\"\r\n"

#define DNS_IP "iot.espressif.cn"
#define SET_DNS "AT+CIPDOMAIN=\"" DNS_IP "\"\r\n"

/*START TCP STRINGS*/
#define START_TCP "AT+CIPSTART=\"TCP\",\"" SERVER_IP "\"," PORT "\r\n"

/*CLOSE TCP STRINGS*/
#define CLOSE_TCP "AT+CIPCLOSE\r\n"

/*TCP MESSAGE STRINGS*/
#define TCP_BUFF_SEND "AT+CIPSENDBUF="
#define MSG_END "\r\n"
#define TCP_BUFF_FLUSH "AT+CIPSEND\r\n"

#define RESET "AT+RST\r\n"


/*
 * Sets up Wifi on given network.
 */
void setupWifi(void);

/*
 * Sets up TCP connection with given address/url (above)
 */
void setupTCP(void);

/*
 * Computes length of message contained in dataString
 */
void setLen(char* dataString);

/*
 * Sends a TCP message contained in dataString over previously established TCP
 * connection.
 */
void sendTCPMessage(char* dataString);

/*
 * closes TCP connection
 */
void closeTCP(void);
