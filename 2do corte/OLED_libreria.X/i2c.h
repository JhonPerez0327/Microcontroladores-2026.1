#ifndef I2C_H
#define I2C_H

#include <xc.h>

void I2C_Init(void);
void I2C_Ready(void);
void I2C_Start(unsigned char addr);
void I2C_Write(unsigned char data);
void I2C_Stop(void);
void I2C_Restart(void);
unsigned char I2C_Read(unsigned char ack);

#endif