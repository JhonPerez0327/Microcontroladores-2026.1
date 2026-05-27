/**
 * @file main_v1.c
 * @brief Práctica 3 - Monitor de Signos Vitales (Versión 1: Base I2C + OLED)
 * @details Configuración inicial del sistema y testeo de la pantalla OLED 
 * mediante el bus I2C maestro sin dependencias del sensor.
 * @author Generado por Gemini
 * @date 2026
 */

#include <xc.h>
#include <stdio.h>

/* ============== CONFIGURACIÓN DE BITS ============== */
#pragma config FOSC = INTOSC_EC   /**< Oscilador interno */
#pragma config WDT = OFF          /**< Watchdog desactivado */
#pragma config PBADEN = OFF       /**< Pines PORTB como digitales */
#pragma config MCLRE = OFF        /**< Pin Master Clear como entrada */
#pragma config LVP = OFF          /**< Programación de bajo voltaje desactivada */

#define _XTAL_FREQ 8000000        /**< Frecuencia de operación: 8MHz */

/* ============== DIRECCIONES I2C ============== */
#define OLED_ADDR       0x78      /**< Dirección I2C de la Pantalla OLED */

/* ============== COMUNICACIÓN I2C MAESTRO ============== */
void I2C_Init() {
    TRISBbits.TRISB0 = 1;         /**< Pin SCL como Entrada */
    TRISBbits.TRISB1 = 1;         /**< Pin SDA como Entrada */
    SSPSTAT = 0x80;               /**< Desactiva Slew Rate para modo estándar (100 kHz) */
    SSPCON1 = 0x28;               /**< Habilita el puerto serie y modo Maestro I2C */
    SSPADD = 19;                  /**< Reloj del bus configurado a 100 kHz */
}

void I2C_Start() { SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN); }
void I2C_Stop()  { SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN); }
void I2C_Write(unsigned char d) { SSPBUF = d; while(SSPSTATbits.BF); __delay_us(5); }

/* ============== PANTALLA OLED INTERFAZ ============== */
void OLED_Cmd(unsigned char c) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x00); I2C_Write(c); I2C_Stop(); }
void OLED_Data(unsigned char d) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x40); I2C_Write(d); I2C_Stop(); }
void OLED_SetCursor(char p, char c) { OLED_Cmd(0xB0+p); OLED_Cmd(c & 0x0F); OLED_Cmd(0x10 | (c>>4)); }

void OLED_Char(char c) {
    extern const unsigned char font5x8[][5];
    if(c < 32 || c > 90) c = 32;
    for(int i=0; i<5; i++) OLED_Data(font5x8[c-32][i]);
    OLED_Data(0x00);
}

void OLED_Str(const char *s) { while(*s) OLED_Char(*s++); }

void OLED_Init() {
    __delay_ms(150);
    OLED_Cmd(0xAE); OLED_Cmd(0xA6); OLED_Cmd(0xAF); /**< Encendido básico */
    for(int i=0; i<8; i++){ OLED_SetCursor(i,0); for(int j=0; j<128; j++) OLED_Data(0x00); }
}

/* Fuente básica de caracteres integrada */
const unsigned char font5x8[][5] = {
    {0,0,0,0,0},{0,0,95,0,0},{0,7,0,7,0},{20,127,20,127,20},{36,42,127,42,18},{35,19,8,100,98},{54,73,85,34,80},
    {0,5,3,0,0},{0,28,34,65,0},{0,65,34,28,0},{8,42,28,42,8},{8,8,62,8,8},{0,80,48,0,0},{8,8,8,8,8},{0,96,96,0,0},
    {32,16,8,4,2},{62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},{24,20,18,127,16},{39,69,69,69,57},
    {60,74,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,41,30},{0,102,102,0,0},{0,102,98,0,0},{8,20,34,65,0},
    {36,36,36,36,36},{0,65,34,20,8},{2,1,81,9,6},{50,73,121,65,62},{126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},
    {127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},
    {127,8,20,34,65},{127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},
    {127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
    {7,8,112,8,7},{97,81,73,69,67}
};

/* ============== PROGRAMA PRINCIPAL ============== */
void main() {
    OSCCON = 0x72;                /**< Configura oscilador interno a 8MHz estable */
    
    I2C_Init();                   /**< Inicialización del Bus I2C */
    OLED_Init();                  /**< Inicialización de Pantalla */
    
    /* Test Visual de Pantalla */
    OLED_SetCursor(2, 10);
    OLED_Str("SISTEMA OK");
    OLED_SetCursor(4, 10);
    OLED_Str("BUS I2C INICIADO");
    
    while(1) {
        // Bucle infinito de espera pasiva
    }
}
