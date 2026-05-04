#include <xc.h>
#define _XTAL_FREQ 8000000

/* ================= CONFIG ================= */
#pragma config FOSC = INTOSC_EC
#pragma config WDT  = OFF
#pragma config LVP  = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE = OFF

/* ============== DEFINICIONES ============== */

/* Canales ADC */
#define CANAL_LM35   0   // AN0
#define CANAL_LDR    1   // AN1

#define ADC_MUESTRAS 8

/* LEDs */
#define LED_TEMP     LATDbits.LATD0
#define LED_LUZ      LATDbits.LATD1

#define LED_TEMP_DIR TRISDbits.TRISD0
#define LED_LUZ_DIR  TRISDbits.TRISD1

/* Umbrales LM35 (en ADC) */
#define TEMP_MIN_ADC 60    // ~29°C
#define TEMP_MAX_ADC 80    // ~39°C

/* Umbral LDR */
#define LUZ_UMBRAL   500   // ajustar según pruebas

/* ============== PROTOTIPOS ============== */
void ADC_Init(void);
unsigned int ADC_Read(unsigned char canal);
unsigned int ADC_ReadAvg(unsigned char canal);

/* ============== ADC ============== */

void ADC_Init(void) {
    TRISAbits.TRISA0 = 1;  // AN0
    TRISAbits.TRISA1 = 1;  // AN1

    ADCON1 = 0x0D;  // AN0 y AN1 analógicos
    ADCON2 = 0xBD;
    ADCON0 = 0x01;

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

    unsigned int temp_adc;
    unsigned int luz_adc;

    OSCCON = 0x72;

    LED_TEMP_DIR = 0;
    LED_LUZ_DIR  = 0;

    LED_TEMP = 0;
    LED_LUZ  = 0;

    ADC_Init();

    while (1) {

        /* Lecturas */
        temp_adc = ADC_ReadAvg(CANAL_LM35);
        luz_adc  = ADC_ReadAvg(CANAL_LDR);

        /* ===== LM35 (indicador de confort) ===== */
        if (temp_adc >= TEMP_MIN_ADC && temp_adc <= TEMP_MAX_ADC) {
            LED_TEMP = 1;
        } else {
            LED_TEMP = 0;
        }

        /* ===== LDR (enciende en oscuridad) ===== */
        if (luz_adc > LUZ_UMBRAL) {
            LED_LUZ = 1;
        } else {
            LED_LUZ = 0;
        }

        __delay_ms(200);
    }
}