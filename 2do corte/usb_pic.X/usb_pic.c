#include <xc.h>
#define _XTAL_FREQ 8000000

// CONFIG
#pragma config FOSC   = INTOSC_EC
#pragma config WDT    = OFF
#pragma config LVP    = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE  = OFF
#pragma config XINST  = OFF
#pragma config PWRT   = ON
#pragma config DEBUG  = OFF

void UART_Init() {
    // Configurar pines
    TRISCbits.TRISC6 = 0; // TX
    TRISCbits.TRISC7 = 1; // RX

    // Baud rate 9600 @ 8MHz
    SPBRG = 51;
    SPBRGH = 0;
    BAUDCONbits.BRG16 = 0;
    TXSTAbits.BRGH = 0;

    // Configurar UART
    TXSTAbits.SYNC = 0; // Asíncrono
    RCSTAbits.SPEN = 1; // Habilitar puerto serial

    // Habilitar TX
    TXSTAbits.TXEN = 1;
}

void UART_Write(char data) {
    while(!PIR1bits.TXIF); // Esperar buffer vacío
    TXREG = data;
}

void main() {
    OSCCON = 0x72; // 8MHz interno

    UART_Init();

    while(1) {
        char mensaje[] = "0123456789\r\n";

        for(int i = 0; mensaje[i] != '\0'; i++) {
            UART_Write(mensaje[i]);
        }

        __delay_ms(1000);
    }
}