#include "i2c.h"
#define _XTAL_FREQ 8000000
#include <xc.h>
#define I2C_BAUDRATE 19

void I2C_Ready(void){
    while(SSPSTATbits.BF || (SSPCON2 & 0x1F));
    __delay_us(5);
}

void I2C_Start(unsigned char addr){
    I2C_Ready();
    SSPCON2bits.SEN = 1;
    while(SSPCON2bits.SEN);
    SSPBUF = addr;
    while(!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;
}

void I2C_Write(unsigned char data){
    I2C_Ready();
    SSPBUF = data;
    while(!PIR1bits.SSPIF);
    PIR1bits.SSPIF = 0;
}

void I2C_Stop(void){
    I2C_Ready();
    SSPCON2bits.PEN = 1;
    while(SSPCON2bits.PEN);
    PIR1bits.SSPIF = 0;
}

void I2C_Init(void){
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 1;
    SSPSTAT = 0x80;
    SSPCON1 = 0x28;
    SSPCON2 = 0x00;
    SSPADD = I2C_BAUDRATE;
    PIR1bits.SSPIF = 0;
}

unsigned char I2C_Read(unsigned char ack) {
    unsigned char temp;
    I2C_Ready();
    SSPCON2bits.RCEN = 1;
    while(!SSPSTATbits.BF);
    temp = SSPBUF;
    I2C_Ready();
    SSPCON2bits.ACKDT = (ack) ? 0 : 1;
    SSPCON2bits.ACKEN = 1;
    return temp;
}

void I2C_Restart(void) {
    I2C_Ready();
    SSPCON2bits.RSEN = 1;
    while(SSPCON2bits.RSEN);
}