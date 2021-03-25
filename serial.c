#include <xc.h>
#include <math.h>
#include <stdlib.h>
#include "serial.h"

int buffDex = 0;
static int recFlag = 0;
char buff[REC_BUFF_LEN];
char bad[] = "%%%%%";

char* getReply(void){
    if (recFlag != 0) {
        recFlag = 0;
        return buff;
    }
    else return bad;
}

void serialWrite(char *s)
{
    int i;
    for(i=0;s[i]!='\0';i++){
        while (U1STAbits.UTXBF==1);
        U1TXREG = s[i];
    }
}

void setupUART1(void){
    
    __builtin_write_OSCCONL(OSCCON & ~(1<<6));  //Unlock PPS registers, necessary
                                            //to set remappable pin (RPx)

    //RPOR7bits.RP15R = 3;                         //Put UART1 TX onto pin 12 (RP15/RB15)
    RPOR1bits.RP3R = 3;                         //Put UART1 TX onto pin 12 (RP15/RB15)
    RPINR18bits.U1RXR = 2;                       //UART1 receive set to RP2 (RB2) Pin 6

    __builtin_write_OSCCONL(OSCCON | (1<<6));   //Lock PPS registers


    //SETUP UART
    U1MODEbits.STSEL = 0;                       // 1-Stop bit
    U1MODEbits.PDSEL = 0;                       // No Parity, 8-Data bits
    U1MODEbits.ABAUD = 0;                       // Auto-Baud disabled
    U1MODEbits.BRGH = 0;                        // Standard-Speed mode
    U1BRG = BRGVAL;                             // Baud Rate setting for 19200
    U1MODEbits.UARTEN = 1;                      // Enable UART
    U1STAbits.UTXEN = 1;                        // Enable UART TX
    IFS0bits.U1RXIF = 0;                // clear RX interrupt flag
    IPC2bits.U1RXIP = 4;
	IEC0bits.U1RXIE = 0;	// Enable RX Interrupts
    SRbits.IPL=3;                       //set CPU priority to 3
}                                           

void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
    //if (U1STAbits.OERR != 0 || U1STAbits.FERR != 0) LATAbits.LATA0 = 1;
    U1STAbits.OERR =0;
    buff[buffDex++] = U1RXREG;
    buffDex++;
    if (buff[buffDex] == '\n' || buffDex > (REC_BUFF_LEN - 5)){
        recFlag = 1;
        buff[buffDex] = '\0';
        buffDex = 0;
        LATAbits.LATA0 = 0;
    }
    while(U1STAbits.UTXBF == 1){
        LATAbits.LATA0 =1;
    }
    IFS0bits.U1RXIF = 0;
}