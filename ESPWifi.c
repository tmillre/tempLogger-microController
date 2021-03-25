#include <xc.h>
#include <math.h>
#include <stdlib.h>
#include "serial.h"
#include "ESPWifi.h"

#define DELAY_433uS asm volatile ("REPEAT, #16000"); Nop(); // 433uS delay in assembly

void delay(void){
    /* ESP chip needs time to process, so we wait ~ 0.25 seconds between messages */
    int k;
    for (k = 0; k < 5000; k++){
        DELAY_433uS
    }
}

int timerCounter = 0;

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void) {
    timerCounter++;
    _T3IF = 0;
}

void startTimer3(void) {
    // The oscillator setup creates FCY = 36.864 MHz, with period 2.7e-8 sec 
    // We selecter every 256th edge to increment on... so timer period of 2.7e-8 * 256 = 1/144000
    // If we wait 16000 periods, we get ~0.11 seconds.
    TMR3 = 0;
    T3CONbits.TCKPS = 3; //1:256 prescaler
    PR3 = 16000; // period of timer1 = instruction cycles - 1 (zero-based counting)
    _T3IF = 0; // clear the interrupt flag
    _T3IE = 1; // enable timer 1 interrupt
    T3CONbits.TON = 1; // start timer
}

char good[] = "OK\r\n";
void awaitReply(void){
    /* Function looks for reply of 'OK\n'*/
    char c;
    int k = 0;
    timerCounter = 0;
    T3CONbits.TON = 1; // turn timer on
    while(1){
        c = U1RXREG;
        if (c == good[k]) k++;
        if (k > 3 || timerCounter > 100) break;
    }
    T3CONbits.TON = 0; //turn timer off
}
void setupWifi(void){
    startTimer3();
    serialWrite(ECHO_OFF);
    awaitReply();
    delay();
    serialWrite(SET_STATION_MODE);
    awaitReply();
    delay();
    serialWrite(CONNECT_START);
    awaitReply();
}

void setupTCP(void){
    /* setup TCP connection to given port and IP */
    serialWrite(SET_DNS);
    awaitReply();
    serialWrite(START_TCP);
    awaitReply();
}

char dlen[5];
void setLen(char* dataString)
{
    /* determines the length of the datastring. This is needed for passing the ESP chip any message for TCP*/
    int len = 0;
    int i, j;
    for(i=0;dataString[i]!='\0';i++) len++;
    if (len > 1023) len = 1023;
    j = 0;
    for (i = 1000; i >= 1; i/=10){
        dlen[j] = len / i + '0';
        len %= i;
        j++;
    }
    dlen[5] = (char) '\0';
}

void sendTCPMessage(char* dataString){
    /* function tells ESP chip to send input string as TCP message to open connection */
    setLen(dataString);
    serialWrite(TCP_BUFF_SEND);
    serialWrite(dlen);
    serialWrite(MSG_END);
    delay();
    serialWrite(dataString);
    awaitReply();
    //serialWrite(TCP_BUFF_FLUSH);
    //awaitReply();
}

void closeTCP(void){
    serialWrite(CLOSE_TCP);
    delay();
    //serialWrite(RESET);
}
