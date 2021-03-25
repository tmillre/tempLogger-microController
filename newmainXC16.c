#define DEVICE_ID "LIVING_ROOM"
#define UPDATE_PERIOD 1 //Period of updates in minutes

/*
 * Below are request header and body for the device. Header is ripped from
 * python library because server rejected all my more basic headers.
 * I should probably modify this...
 */
#define HEADER "POST /measurements/ HTTP/1.1\r\nHost: calm-reef-77547.herokuapp.com\r\nUser-Agent: python-requests/2.24.0\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n"
#define BODY "deviceID=" DEVICE_ID "&temp=%f&pressure=%f&humidity=%f"

/*
 * Some config bits for our device. 
 */
#pragma config GWRP = OFF // General Code Segment Write Protect (User program memory is not write-protected)
#pragma config GSS = OFF // General Segment Code Protection (User program memory is not code-protected)
#pragma config FNOSC = FRCPLL // Oscillator Mode (Internal Fast RC (FRC) w/ PLL)
#pragma config POSCMD = NONE // Primary Oscillator Source (Primary Oscillator Disabled)
#pragma config OSCIOFNC = OFF // OSC2 Pin Function (OSC2 pin has CLOCK OUT)
#pragma config FCKSM = CSECMD // Clock Switching and Monitor (Clock switching is enabled, Fail-Safe Clock Monitor is disabled)

/*
 * Turn off watchdog timer (which resets device if it gets stuck... but seems to reset during waiting loops used here)
 */
#pragma config FWDTEN = OFF             // Watchdog Timer Disabled
#pragma config JTAGEN = OFF             // JTAG Enable bit (JTAG is disabled)

#include <xc.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "xc.h"
#include "serial.h"
#include "ESPWifi.h"
#include "i2ctemp.h"

void IOSetup();
void configOSC();
void startTimer1();
long timer1Counter = 0;

char headBuffer[300];
char bodyBuffer[100];

int len(char * dataString){
    int l ,i;
    l = 0;
    for(i=0;dataString[i]!='\0';i++) l++;
    return l;
}

int main(void) {
    //Setup main components
    IOSetup();
    configOSC();
    setupUART1();
    startTimer1();
    setupTempSensor();
    double* help = getTempHumid();
    
    /*
     * Send data on startup
     */
    
    //we start up wifi, tcp to website
    setupWifi();
    setupTCP();
    //send our http post request
    sprintf(bodyBuffer, BODY, help[0], help[1], help[2]);
    sprintf(headBuffer, HEADER, len(bodyBuffer));
    sendTCPMessage(headBuffer);
    sendTCPMessage(bodyBuffer);
    //then shut it down and wait to reconnect after time interval
    closeTCP();
    timer1Counter = 0;
    for(;;)
    {   
        /*
         * Every UPDATE_PERIOD minutes, setup wifi, TCP and send new data
         */
        if (timer1Counter > UPDATE_PERIOD * 60 * 10)
        {
            setupWifi();
            setupTCP();
            help = getTempHumid();
            sprintf(bodyBuffer, BODY, help[0], help[1], help[2]);
            sprintf(headBuffer, HEADER, len(bodyBuffer));
            sendTCPMessage(headBuffer);
            sendTCPMessage(bodyBuffer);
            closeTCP();
            timer1Counter = 0;
        }
    }
}

void IOSetup(void){
    AD1PCFGLbits.PCFG0 = 1;       //Set A0 to digital (This pin is used to probe for errors in interrupt)
    TRISAbits.TRISA0 = 0;         //Set A0 as output
    TRISBbits.TRISB2 = 1;         //Set B2 to input (for UART RX)
    AD1PCFGLbits.PCFG4 = 1;       //Set B2 to ditial (for UART RX)
    LATAbits.LATA0 = 0;           //drive A0 low
}

void configOSC(void) {
    CLKDIVbits.PLLPRE = 0;
    PLLFBD = 38;
    CLKDIVbits.PLLPOST = 0; // N2=2 and N2 = 2 * (PLLPOST + 1)
    //F_OSC = F_IN (PLLDIV+2)/[(PLLPRE+2)*2*(PLLPOST+1)] = 7.3728*40/[4]
    //So we get F_OSC = 73.728 MHz
    //Instructions per second is F_OSC/2, or 36.862 MHz (Needed for timers).
    
    OSCTUN = 0; // No tuning of the oscillator is required
    __builtin_write_OSCCONH(0x01); //Initiate Clock Switch to FRC with PLL
    __builtin_write_OSCCONL(0x01);
    //FOSCbits.OSCIOFNC = 1;
    while (OSCCONbits.COSC != 0b01); //Wait for Clock switch to occur 
    while(!OSCCONbits.LOCK); 
}

void startTimer1(void) {
    // The oscillator setup creates FCY = 36.864 MHz, with period 2.7e-8 sec 
    // We select every 256th edge to increment on... so timer period of 2.7e-8 * 256 = 1/144000
    // If we wait 16000 periods, we get ~0.11 seconds.
    TMR1 = 0;
    T1CONbits.TCKPS = 3; //1:256 prescaler
    PR1 = 16000; // period of timer1 = instruction cycles - 1 (zero-based counting)
    _T1IF = 0; // clear the interrupt flag
    _T1IE = 1; // enable timer 1 interrupt
    T1CONbits.TON = 1; // start timer
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
    timer1Counter++;
    _T1IF = 0;
}