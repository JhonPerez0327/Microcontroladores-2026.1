#include <xc.h>
#define _XTAL_FREQ 8000000

/* ================= CONFIG ================= */
#pragma config FOSC = INTOSC_EC
#pragma config WDT  = OFF
#pragma config LVP  = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE = OFF

/* ============== DEFINICIONES ============== */

/* Canal ADC (cambiar según sensor: AN0, AN1, etc.) */
#define ADC_CANAL      0

/* Numero de muestras para filtrado */
#define ADC_MUESTRAS   8

/* Pines de salida (actuador) */
#define OUT            LATDbits.LATD0
#define OUT_DIR        TRISDbits.TRISD0

/* Umbrales (ajustar según sensor) */
#define UMBRAL_MIN     0
#define UMBRAL_MAX     300

/* ============== PROTOTIPOS ============== */
void ADC_Init(void);
unsigned int ADC_Read(unsigned char canal);
unsigned int ADC_ReadAvg(unsigned char canal);

/* ============== ADC ============== */

void ADC_Init(void) {
    TRISAbits.TRISA0 = 1;     /* ajustar según canal */

    ADCON1 = 0x0E;            /* AN0 analogico */
    ADCON2 = 0xBD;            /* 10 bits, 20TAD */
    ADCON0 = 0x01;            /* ADC ON */

    __delay_ms(5);
}

unsigned int ADC_Read(unsigned char canal) {
    ADCON0 = (ADCON0 & 0xC3) | (canal << 2);

    __delay_us(50);

    ADCON0bits.GO = 1;
    while (ADCON0bits.GO);

    return ((ADRESH << 8) | ADRESL);
}

unsigned int ADC_ReadAvg(unsigned char canal) {
    unsigned char i;
    unsigned long suma = 0;

    for (i = 0; i < ADC_MUESTRAS; i++) {
        suma += ADC_Read(canal);
        __delay_ms(1);
    }

    return (unsigned int)(suma / ADC_MUESTRAS);
}

/* ============== MAIN ============== */

void main(void) {

    unsigned int valor;

    OSCCON = 0x72;            /* 8 MHz */

    OUT_DIR = 0;
    OUT = 0;

    ADC_Init();

    while (1) {

        valor = ADC_ReadAvg(ADC_CANAL);

        /* Logica generica */
        if (valor > UMBRAL_MAX) {
            OUT = 1;
        } else {
            OUT = 0;
        }

        __delay_ms(200);
    }
}