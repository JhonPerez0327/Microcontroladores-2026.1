#ifndef OLED_H
#define OLED_H

#include <xc.h>
#include "i2c.h"

#define OLED_ADDR 0x78
#define OLED_CMD  0x00
#define OLED_DATA 0x40

void OLED_Init(void);
void OLED_Comando(unsigned char cmd);
void OLED_Dato(unsigned char dato);
void OLED_Clear(void);
void OLED_SetCursor(unsigned char pagina, unsigned char col);
void OLED_Char(unsigned char c);
void OLED_String(unsigned char pagina, unsigned char col, const char *texto);

#endif