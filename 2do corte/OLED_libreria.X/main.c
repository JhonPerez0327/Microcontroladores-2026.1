#include <xc.h>
#define _XTAL_FREQ 8000000

#include "i2c.h"
#include "oled.h"

// CONFIG BITS (mínimos necesarios)
#pragma config FOSC = HS
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF

void main(void) {

    I2C_Init();
    OLED_Init();

    OLED_Clear();

    OLED_String(0, 0, "Hola mundo");
    OLED_String(2, 0, "OLED OK");

    while(1){
    }
}