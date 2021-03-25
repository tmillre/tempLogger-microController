#include <xc.h>
#include <math.h>
#include <stdlib.h>
#include "i2c.h"

#define DELAY_1uS asm volatile ("REPEAT, #37"); Nop(); 

void setupTimer2(void);

void delay_XuS(float us){
    long i;
    for (i = 0; i < us; i++){
        DELAY_1uS
    }
}

void setupI2C(void){
    setupTimer2();
    ODCBbits.ODCB8 = 1;
	ODCBbits.ODCB9 = 1;
    I2C1CONbits.A10M = 0;
    //I2C1ADD = 0;
	//I2C1MSK = 0;
	I2C1BRG = 250;
    I2C1CONbits.I2CEN = 1;
    delay_XuS(1000);
}

void setupTimer2(void) {
    /* Setup for Timer 2, used for timing pulses from temperature sensor */
    T2CONbits.TCS = 0; // Use internal clock. (FCY - 36.864 MHz)
    TMR2 = 0; 
    T2CONbits.TCKPS = 2; //1:64 prescaler
    PR2 = 16000; // This should take less than 433 * 64 uS 
    T2CONbits.TON = 0; // keep timer off
}

int I2Cwrite(int msg){
    //Send integer over I2C. 
    TMR2 = 0; //reset timer
    //T2CONbits.TON = 1; // turn on timer
    while(I2C1STATbits.TBF == 1 && TMR2 < 15000); // wait until space in buffer
    while(I2C1STATbits.TRSTAT == 1 && TMR2 < 15000); // wait until no transmit in place
    //T2CONbits.TON = 0; // turn on timer
    if (TMR2 > 15000) return 0;
    I2C1TRN = msg; // try to add to buffer
    while(I2C1STATbits.IWCOL == 1){ //retry if write collision
        DELAY_1uS
        I2C1STATbits.IWCOL = 0;
        I2C1TRN = msg;
    }
    DELAY_1uS
    if (I2C1STATbits.ACKSTAT == 1) return 1;
    else return 0;
}

void readFrom(int chipSel, int startAddr, int len, int* ptr){
    //Initiates read from device chipSel at address startAddr to startAddr + len
    // and returns in ptr. It is done in single byte segments as opposed to bursts,
    // as bursts were giving weird outputs.
    int i = 0;
    for (i = 0; i < len; i++){
        int ac = 0;
        delay_XuS(100);
        I2C1CONbits.SEN = 1;

        ac += I2Cwrite(chipSel << 1); //(0x76 << 1); // Send address with write
        ac += I2Cwrite(startAddr + i); // Start reading from the temperature params data byte
        delay_XuS(100);
        I2C1CONbits.RSEN = 1;

        ac += I2Cwrite((chipSel << 1) + 1); //(0x76 << 1) + 1; // Send address with read
        delay_XuS(1000);
        I2C1CONbits.RCEN = 1; // Turn on receive mode

        TMR2 = 0; //reset timer
        T2CONbits.TON = 1; // turn on timer
        while(~I2C1STATbits.RBF && (TMR2 < 15000)); // wait until response or until timer clocks 15000.
        if (I2C1STATbits.RBF) *(ptr + i) = I2C1RCV;
        T2CONbits.TON = 0; // turn off timer
        I2C1CONbits.ACKDT = 1; // return NACK
        I2C1CONbits.ACKEN = 1; // actually send the NACK
        I2C1CONbits.PEN = 1; // end transmission
    }
}
