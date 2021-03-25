#include "i2ctemp.h"
#include <xc.h>
#include <math.h>
#include <stdlib.h>
#include "i2c.h"

#define DELAY_1uS asm volatile ("REPEAT, #37"); Nop(); 

#define TEMP_OFFSET -1.50
//BME280 self-heats. This offset measured around 22 C, might vary at different
//temperatures and with different component placements

void delay_uS(float us){
    long i;
    for (i = 0; i < us; i++){
        DELAY_1uS
    }
}

/*
 *  BME280 Conversion variables
 *  These variables are read from the device
 *  and then used to convert the raw reading into human readable form
 */

int params[35];
unsigned short dig_T1;
short dig_T2;
short dig_T3;
unsigned short dig_P1;
short dig_P2;
short dig_P3;
short dig_P4;
short dig_P5;
short dig_P6;
short dig_P7;
short dig_P8;
short dig_P9;
unsigned char dig_H1;
short dig_H2;
unsigned char dig_H3;
short dig_H4;
short dig_H5;
signed char dig_H6; 

void getParams(void){
    //Get conversion parameters for the chip, contained in globals defined above.
    //dig_T* for temperature, dig_P* for pressure and dig_H* for humidity
    readFrom(0x76, 0x88, 26, params);
    readFrom(0x76, 0xE1, 9, params + 26);
    
    dig_T1 = (unsigned short) (params[1] << 8) | params[0];
    dig_T2 = (params[3] << 8) | params[2];
    dig_T3 = (params[5] << 8) | params[4];
    
    dig_P1 = (unsigned short) (params[7] << 8) | params[6];
    dig_P2 = (params[9] << 8) | params[8];
    dig_P3 = (params[11] << 8) | params[10];
    dig_P4 = (params[13] << 8) | params[12];
    dig_P5 = (params[15] << 8) | params[14];
    dig_P6 = (params[17] << 8) | params[16];
    dig_P7 = (params[19] << 8) | params[18];
    dig_P8 = (params[21] << 8) | params[20];
    dig_P9 = (params[23] << 8) | params[22];
    
    dig_H1 = (unsigned char) params[25];
    dig_H2 = (params[27] << 8) | params[26];
    dig_H3 = (unsigned char) params[28];
    dig_H4 = (params[29] << 4) | (params[30] & 0b1111);
    dig_H5 = (params[31] << 4) | (params[30] >> 4);
    dig_H6 = params[32];
    
}

/*
 Sets up I2C peripheral and sets config options on BME280
 */
void setupTempSensor(void){
    //Setup Timer 2, I2C, and then configure BME280 for standard operation
    setupI2C();
    I2C1CONbits.SEN = 1; //Send start bit
    
    I2Cwrite(0x76 << 1); //To address 0x76 in write mode
    I2Cwrite(0xF4); //To byte 0xF4, which is a config byte
    I2Cwrite(0b00100111); //Configures x1 oversampling for temp [5:7] and pressure[2:4] and run in normal mode[0:1]
    I2Cwrite(0xF5); //To byte 0xF5, which is a config byte
    I2Cwrite(0b00000100); //0.5ms standy time [5:7], filter coeff 2[2:4], and run in i2c.
    
    delay_uS(10);
    I2C1CONbits.PEN = 1;
    
    getParams();
    delay_uS(1000);
}

/*
 * The following three functions convert the raw readings into human readable
 * form. getFineTemp gets a conversion constant used for the others.
 * It must be called first. It also can be converted to temperature later.
 * ( 5 * FineTemp + 128) / 256 = Temperature in Celsius.
 */

double output[3];
int raw[9];

long getFineTemp(void){
    //Converts raw reading to fine temperature, which is used in other measurements
    //This is then used to compute human readable temperature in Celcius (x100, but easy fix)
    
    long adc_T = (((long) raw[3]) << 12) | (((long) raw[4]) << 4) | (long) (raw[5]>> 4);
    long var1, var2, t_fine;
    var1 = (long)((adc_T / 8) - ((long)dig_T1 * 2));
    var1 = (var1 * ((long)dig_T2)) / 2048;
    var2 = (long)((adc_T / 16) - ((long)dig_T1));
    var2 = (((var2 * var2) / 4096) * ((long)dig_T3)) / 16384;
    t_fine = var1 + var2;
    return t_fine;
    //temperature = (t_fine * 5 + 128) / 256;
}

long getHumid(long t_fine){
    //Convert raw humidity reading to human readable format (% humidity * 1024).
    
    long adc_H = (((long) raw[6]) << 8) | (((long) raw[7]) );
    long v_x1_u32r;
    v_x1_u32r = (t_fine -((long)76800));
    v_x1_u32r = (((((adc_H << 14) -(((long)dig_H4) << 20) -(((long)dig_H5)
            * v_x1_u32r)) + ((long)16384)) >> 15) * (((((((v_x1_u32r * ((long)dig_H6)) >> 10) 
            * (((v_x1_u32r * ((long)dig_H3)) >> 11) + ((long)32768))) >> 10) + ((long)2097152)) * ((long)dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r -(((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
            * ((long)dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r); 
    v_x1_u32r = (v_x1_u32r > 419430400? 419430400: v_x1_u32r);
    return (unsigned long) (v_x1_u32r>>12);
}

long getPress(long t_fine){
    //Convert Raw reading into Human Readable (Pascals)
    long long adc_P = (((long long) raw[0]) << 12) | (((long long) raw[1]) << 4) | (long long) (raw[2]>> 4);
    long long var1;
    long long var2;
    long long unsigned int p;

    var1 = ((long long)t_fine) - 128000;
    var2 = var1 * var1 * (long long)dig_P6;
    var2 = var2 + ((var1*(long long)dig_P5)<<17);
    var2 = var2 + (((long long)dig_P4)<<35);
    var1 = ((var1 * var1 * (long long)dig_P3)>>8) + ((var1 * (long long)dig_P2)<<12);
    var1 = (((((long long)1)<<47)+var1))*((long long)dig_P1)>>33;
    if   (var1 == 0)    {
        return 0; // avoid exception caused by division by zero
    }       
    p = 1048576 - adc_P;
    p = (((p<<31) - var2)*3125)/var1;
    var1 = (((long long)dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((long long)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((long long)dig_P7)<<4);
    return (long) p / 256; 
}

/*
 * Compute median (O(n^2) time, but we only scale to n = 10 here at most)
 */

double findMedian(double* arr, int len){
    if (len < 2) return arr[0];
    int i, j, maxDex;
    double max, hold;
    //Do insertion sort N / 2 times.
    for (i = 0; i < (len + 1) / 2; i++){
        maxDex = 0;
        max = arr[0];
        for (j = 0; j < len - i; j++){
            if (arr[j] > max){
                max = arr[j];
                maxDex = j;
            }
        }
        if (i != (len + 1)/2){
            hold = arr[maxDex];
            arr[maxDex] = arr[len - i - 1];
            arr[len - i - 1] = hold;
        }
    }
    return max;
}

#define DATALEN 9

double* getTempHumid(void){
    // Take DATALEN samples, and then find the median of them and return it
    double temperature[DATALEN];
    double pressure[DATALEN];
    double humidity[DATALEN];
    int i = 0;
    for (i = 0; i < DATALEN; i++){
        readFrom(0x76, 0xF7, 9, raw);
    
        //Temperature Conversion (from BME280 Manual)
        long t_fine = getFineTemp();
        temperature[i] =  (((double) t_fine * 5 + 128.0) / 256.0) / 100.0 + TEMP_OFFSET; //Convert fine temperature to celcius
    
        //Pressure Conversion (from BME280 Manual)
        pressure[i] = ((double) getPress(t_fine)) / 100.0;
    
        //Humidity Conversion (from BME 280 Manual)
        humidity[i] = ((double) (getHumid(t_fine))) / 1024.0;
        delay_uS(5000);
    }
    
    output[0] = findMedian(temperature, DATALEN);
    output[1] = findMedian(pressure, DATALEN);
    output[2] = findMedian(humidity, DATALEN);
    
    return output;
}
