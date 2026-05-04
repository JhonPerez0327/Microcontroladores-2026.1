#include <xc.h>
#define _XTAL_FREQ 8000000

// Config bits mínimos
#pragma config FOSC = HS
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF

// Declaraciones (porque estás usando un solo archivo)
void I2C_Init(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_String(unsigned char, unsigned char, const char*);

void main(void){

    I2C_Init();
    OLED_Init();

    OLED_Clear();

    OLED_String(0, 0, "Hola mundo");

    while(1){
    }
}
