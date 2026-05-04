/**
 * @file    lm35_led.c
 * @brief   Indicador de rango de temperatura con LM35 y LED.
 *          PIC18F4550 - MPLAB X / XC8
 *
 * @details Lee la temperatura analogica del LM35 usando el ADC interno
 *          del PIC18F4550. El LED enciende solo dentro del rango definido:
 *            - temp < TEMP_MIN             -> LED OFF (frio)
 *            - TEMP_MIN <= temp <= TEMP_MAX -> LED ON  (en rango)
 *            - temp > TEMP_MAX             -> LED OFF (caliente)
 *
 *          Rango configurado para demostracion en Popayan (24-26C ambiente):
 *            TEMP_MIN = 30C
 *            TEMP_MAX = 36C
 *          Demo: dedo al sensor -> sube a rango -> LED ON
 *                mechera        -> supera 36C   -> LED OFF
 *
 *          Conversion ADC -> temperatura:
 *            El LM35 entrega 10mV por cada grado Celsius.
 *            Con Vref = 5V y ADC de 10 bits (1024 pasos):
 *            Temp(C) = (ADC * 500) / 1024
 *
 * @hardware
 *   - PIC18F4550
 *   - LM35 (Vout -> RA0/AN0), condensador 100nF entre Vout y GND
 *   - LED con resistencia 330 ohm en RD0
 *
 * @note  Oscilador interno 8MHz. Vref = VDD = 5V.
 *
 * @author  Tu nombre
 * @date    2026
 */

/* =========================================================
 * BITS DE CONFIGURACION
 * ========================================================= */
#pragma config FOSC   = INTOSC_EC
#pragma config WDT    = OFF
#pragma config LVP    = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE  = OFF
#pragma config XINST  = OFF
#pragma config PWRT   = ON
#pragma config DEBUG  = OFF

#include <xc.h>
#define _XTAL_FREQ 8000000

/* =========================================================
 * DEFINICIONES
 * ========================================================= */

/** @brief Canal ADC del LM35 (AN0 = RA0) */
#define LM35_CANAL      0

/** @brief Numero de muestras para el promedio (filtra ruido de protoboard) */
#define ADC_MUESTRAS    8

/**
 * @brief Temperatura minima del rango en grados Celsius.
 *        Por debajo de este valor el LED permanece apagado.
 */
#define TEMP_MIN        30

/**
 * @brief Temperatura maxima del rango en grados Celsius.
 *        Por encima de este valor el LED se apaga.
 */
#define TEMP_MAX        36

/** @brief LED indicador de rango en RD0 */
#define LED             LATDbits.LATD0
#define LED_DIR         TRISDbits.TRISD0

/* =========================================================
 * PROTOTIPOS
 * ========================================================= */
void         ADC_Init(void);
unsigned int ADC_Leer(unsigned char canal);
unsigned int ADC_LeerPromedio(unsigned char canal);
unsigned int LM35_LeerTemperatura(void);

/* =========================================================
 * FUNCIONES ADC
 * ========================================================= */

/**
 * @brief Inicializa el modulo ADC del PIC18F4550.
 *
 * @details Configuracion de registros:
 *
 *   ADCON1 = 0x0E:
 *     VCFG = 00   -> Vref- = GND, Vref+ = VDD (5V)
 *     PCFG = 1110 -> Solo AN0 es analogico, resto digitales
 *
 *   ADCON2 = 0xBD:
 *     ADFM  = 1   -> Resultado justificado a la DERECHA
 *     ACQT  = 111 -> Tiempo de adquisicion = 20 TAD
 *                    Necesario para LM35 (alta impedancia de salida)
 *     ADCS  = 101 -> Fosc/16, TAD = 2us con Fosc=8MHz
 *
 *   ADCON0 = 0x01:
 *     CHS  = 0000 -> Canal AN0
 *     ADON = 1    -> ADC encendido
 */
void ADC_Init(void) {
    TRISAbits.TRISA0 = 1;           /* RA0 como entrada analogica   */

    ADCON1 = 0x0E;                  /* Solo AN0 analogico, Vref=VDD */
    ADCON2 = 0xBD;                  /* ADFM=1, ACQT=20TAD, Fosc/16 */
    ADCON0 = 0x01;                  /* Canal AN0, ADC ON            */

    __delay_ms(5);                  /* Estabilizacion en fisico     */
}

/**
 * @brief Lee una conversion del ADC en el canal indicado.
 *
 * @param canal Canal ADC a leer (0 = AN0)
 * @return Valor ADC de 10 bits (0 a 1023)
 */
unsigned int ADC_Leer(unsigned char canal) {
    ADCON0 = (ADCON0 & 0xC3) | ((canal & 0x0F) << 2);

    __delay_us(50);                 /* Tiempo de adquisicion        */

    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);

    return ((unsigned int)(ADRESH << 8) | ADRESL);
}

/**
 * @brief Promedia varias lecturas del ADC para eliminar ruido.
 *
 * @details Toma ADC_MUESTRAS lecturas consecutivas y retorna el promedio.
 *          Elimina el ruido electrico tipico de la protoboard que hace
 *          saltar la lectura del LM35 varios grados entre muestras.
 *
 * @param canal Canal ADC a promediar
 * @return Promedio de ADC_MUESTRAS lecturas
 */
unsigned int ADC_LeerPromedio(unsigned char canal) {
    unsigned char i;
    unsigned long suma = 0;

    for(i = 0; i < ADC_MUESTRAS; i++) {
        suma += ADC_Leer(canal);
        __delay_ms(1);
    }

    return (unsigned int)(suma / ADC_MUESTRAS);
}

/**
 * @brief Convierte la lectura ADC del LM35 a grados Celsius.
 *
 * @details LM35 da 10mV por grado. Con Vref=5V y ADC de 10 bits:
 *          Temp(C) = (ADC * 500) / 1024
 *
 * @return Temperatura en grados Celsius (entero)
 */
unsigned int LM35_LeerTemperatura(void) {
    unsigned int adc_val;
    unsigned long temp;

    adc_val = ADC_LeerPromedio(LM35_CANAL);

    temp = (unsigned long)adc_val * 500;
    temp = temp / 1024;

    return (unsigned int)temp;
}

/* =========================================================
 * FUNCION PRINCIPAL
 * ========================================================= */

/**
 * @brief Indicador de rango de temperatura con LED.
 *
 * @details Logica de control:
 *   temp < 30C          -> LED OFF (por debajo del rango)
 *   30C <= temp <= 36C  -> LED ON  (dentro del rango)
 *   temp > 36C          -> LED OFF (por encima del rango)
 *
 *   Demo fisica:
 *   - Ambiente ~25C     -> LED apagado
 *   - Dedo al sensor    -> sube a ~32-35C -> LED enciende
 *   - Mechera al sensor -> supera 36C     -> LED apaga
 */
void main(void) {

    unsigned int temperatura;

    OSCCON = 0x72;                  /* Oscilador interno 8MHz       */

    LED_DIR = 0;                    /* RD0 como salida              */
    LED     = 0;                    /* LED apagado al inicio        */

    ADC_Init();

    __delay_ms(200);                /* Estabilizacion general       */

    while(1) {

        temperatura = LM35_LeerTemperatura();

        /* LED ON solo si temperatura esta dentro del rango */
        if(temperatura >= TEMP_MIN && temperatura <= TEMP_MAX) {
            LED = 1;
        } else {
            LED = 0;
        }

        __delay_ms(500);
    }
}