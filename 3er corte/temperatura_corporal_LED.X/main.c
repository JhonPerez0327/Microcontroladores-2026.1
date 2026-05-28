/**
 * @file    ds18b20_led.c
 * @brief   Indicador de temperatura corporal con DS18B20 y LED.
 *          PIC18F4550 - MPLAB X / XC8
 *
 * @details Lee la temperatura del DS18B20 usando protocolo 1-Wire
 *          por el pin RB0. El LED indica si la temperatura está
 *          en rango corporal normal:
 *            - temp < 36.0C              -> LED OFF (frío / hipotermia)
 *            - 36.0C <= temp <= 37.5C    -> LED ON  (normal)
 *            - temp > 37.5C              -> LED OFF (fiebre)
 *
 *          Protocolo 1-Wire ? tiempos críticos:
 *            Reset pulse  : 480us mínimo en LOW
 *            Presencia    : sensor responde 60-240us en LOW
 *            Write 0      : 60us en LOW
 *            Write 1      : 1-15us en LOW, luego soltar
 *            Read bit     : 1us LOW para iniciar, leer en ~14us
 *
 * @hardware
 *   - DS18B20: GND=Pin1, DATA=Pin2 (RB0), VDD=Pin3
 *   - Pull-up 4.7kOhm entre DATA y +5V (OBLIGATORIO)
 *   - LED con resistencia 330ohm en RD0
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
 * DEFINICIONES 1-WIRE (DS18B20 en RB0)
 * ========================================================= */

/** @brief Poner línea DATA en LOW (salida = 0) */
#define OW_LOW()    do { TRISBbits.TRISB0 = 0; LATBbits.LATB0 = 0; } while(0)

/** @brief Soltar línea DATA (entrada, pull-up la lleva a HIGH) */
#define OW_RELEASE()  do { TRISBbits.TRISB0 = 1; } while(0)

/** @brief Leer estado de la línea DATA */
#define OW_READ()     (PORTBbits.RB0)

/* Comandos DS18B20 */
#define CMD_SKIP_ROM      0xCC   /**< Saltar verificacion de ROM (1 solo sensor) */
#define CMD_CONVERT_T     0x44   /**< Iniciar conversion de temperatura           */
#define CMD_READ_SCRATCH  0xBE   /**< Leer scratchpad (9 bytes con temperatura)   */

/* Umbrales temperatura corporal */
#define TEMP_MIN_ENT    36       /**< Parte entera minima (36.0C)  */
#define TEMP_MIN_DEC    0        /**< Parte decimal minima         */
#define TEMP_MAX_ENT    37       /**< Parte entera maxima (37.5C)  */
#define TEMP_MAX_DEC    5        /**< Parte decimal maxima         */

/** @brief LED en RD0 */
#define LED         LATDbits.LATD0
#define LED_DIR     TRISDbits.TRISD0

/* =========================================================
 * PROTOTIPOS
 * ========================================================= */
unsigned char OW_Reset(void);
void          OW_WriteByte(unsigned char dato);
unsigned char OW_ReadByte(void);
void          DS18B20_IniciarConversion(void);
void          DS18B20_LeerTemperatura(unsigned char *entero, unsigned char *decimal);

/* =========================================================
 * FUNCIONES 1-WIRE
 * ========================================================= */

/**
 * @brief Envía pulso de reset y detecta presencia del DS18B20.
 *
 * @details Secuencia:
 *   1. Poner línea en LOW por 480us (reset pulse)
 *   2. Soltar línea y esperar 70us
 *   3. Leer: si está en LOW el sensor está presente
 *   4. Esperar resto del slot (410us)
 *
 * @return 1 si sensor presente, 0 si no hay respuesta
 */
unsigned char OW_Reset(void) {
    unsigned char presencia;

    OW_LOW();
    __delay_us(480);        /* Reset pulse minimo 480us             */

    OW_RELEASE();
    __delay_us(70);         /* Esperar respuesta del sensor         */

    presencia = !OW_READ(); /* LOW = sensor presente                */
    __delay_us(410);        /* Completar slot de presencia (480us)  */

    return presencia;
}

/**
 * @brief Escribe un byte en el bus 1-Wire, bit a bit (LSB primero).
 *
 * @details Para cada bit:
 *   - Write 0: LOW por 60us, soltar
 *   - Write 1: LOW por 6us, soltar, esperar 54us
 *
 * @param dato Byte a transmitir
 */
void OW_WriteByte(unsigned char dato) {
    unsigned char i;

    for(i = 0; i < 8; i++) {
        if(dato & 0x01) {
            /* Escribir bit 1 */
            OW_LOW();
            __delay_us(6);
            OW_RELEASE();
            __delay_us(54);
        } else {
            /* Escribir bit 0 */
            OW_LOW();
            __delay_us(60);
            OW_RELEASE();
            __delay_us(2);
        }
        dato >>= 1;         /* Siguiente bit (LSB primero)          */
    }
}

/**
 * @brief Lee un byte del bus 1-Wire, bit a bit (LSB primero).
 *
 * @details Para cada bit:
 *   1. Pulso LOW de 1-2us para iniciar slot de lectura
 *   2. Soltar y esperar ~12us
 *   3. Leer: si HIGH el bit es 1, si LOW el bit es 0
 *   4. Esperar resto del slot (45us)
 *
 * @return Byte leído del sensor
 */
unsigned char OW_ReadByte(void) {
    unsigned char i, dato = 0;

    for(i = 0; i < 8; i++) {
        dato >>= 1;

        OW_LOW();
        __delay_us(2);      /* Iniciar slot de lectura              */

        OW_RELEASE();
        __delay_us(12);     /* Esperar dato valido del sensor       */

        if(OW_READ()) {
            dato |= 0x80;   /* Bit 1: señal en HIGH                 */
        }

        __delay_us(45);     /* Completar slot de 60us total         */
    }

    return dato;
}

/* =========================================================
 * FUNCIONES DS18B20
 * ========================================================= */

/**
 * @brief Ordena al DS18B20 iniciar conversión de temperatura.
 *
 * @details La conversión tarda hasta 750ms (resolución 12 bits).
 *          Se usa Skip ROM porque hay un solo sensor en el bus.
 */
void DS18B20_IniciarConversion(void) {
    OW_Reset();
    OW_WriteByte(CMD_SKIP_ROM);
    OW_WriteByte(CMD_CONVERT_T);
    __delay_ms(750);        /* Esperar conversion completa          */
}

/**
 * @brief Lee la temperatura del scratchpad del DS18B20.
 *
 * @details El scratchpad tiene 9 bytes. Los primeros 2 son la temperatura:
 *   Byte 0: parte baja (LSB)
 *   Byte 1: parte alta (MSB)
 *
 *   Formato de 12 bits (default):
 *   Bits 15-11: signo (0 = positivo)
 *   Bits 10-4:  parte entera
 *   Bits 3-0:   parte decimal (cada bit = 0.0625C)
 *
 *   Decimal aproximado: (fraccion * 625) / 1000 -> 1 cifra
 *
 * @param entero   Puntero donde se guarda la parte entera (ej: 36)
 * @param decimal  Puntero donde se guarda la parte decimal (ej: 5 = 0.5C)
 */
void DS18B20_LeerTemperatura(unsigned char *entero, unsigned char *decimal) {
    unsigned char byte_bajo, byte_alto;
    unsigned int  raw;
    unsigned long frac;

    OW_Reset();
    OW_WriteByte(CMD_SKIP_ROM);
    OW_WriteByte(CMD_READ_SCRATCH);

    byte_bajo = OW_ReadByte();      /* Byte 0: temperatura LSB      */
    byte_alto = OW_ReadByte();      /* Byte 1: temperatura MSB      */

    raw = ((unsigned int)byte_alto << 8) | byte_bajo;

    /* Parte entera: bits 11-4 */
    *entero  = (unsigned char)(raw >> 4);

    /* Parte decimal: bits 3-0, cada uno vale 0.0625C
     * (fraccion * 625 / 1000) da primera cifra decimal             */
    frac     = (raw & 0x0F) * 625UL;
    *decimal = (unsigned char)(frac / 1000);
}

/* =========================================================
 * FUNCION PRINCIPAL
 * ========================================================= */

/**
 * @brief Indicador de rango de temperatura corporal con LED.
 *
 * @details
 *   temp < 36.0C           -> LED OFF
 *   36.0C <= temp <= 37.5C -> LED ON
 *   temp > 37.5C           -> LED OFF
 */
void main(void) {
    unsigned char temp_ent, temp_dec;
    unsigned char en_rango;

    OSCCON = 0x72;              /* Oscilador interno 8MHz           */

    LED_DIR = 0;                /* RD0 como salida                  */
    LED     = 0;

    __delay_ms(200);

    while(1) {
        DS18B20_IniciarConversion();
        DS18B20_LeerTemperatura(&temp_ent, &temp_dec);

        /* Verificar si está dentro del rango corporal normal */
        en_rango = 0;

        if(temp_ent > TEMP_MIN_ENT && temp_ent < TEMP_MAX_ENT) {
            /* Entre 37C y 36C exclusivo: siempre en rango */
            en_rango = 1;
        } else if(temp_ent == TEMP_MIN_ENT && temp_dec >= TEMP_MIN_DEC) {
            /* Exactamente 36.0C o más */
            en_rango = 1;
        } else if(temp_ent == TEMP_MAX_ENT && temp_dec <= TEMP_MAX_DEC) {
            /* Hasta 37.5C */
            en_rango = 1;
        }

        LED = en_rango;

        __delay_ms(500);
    }
}