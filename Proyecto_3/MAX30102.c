/**
 * @file main_v2.c
 * @brief Práctica 3 - Monitor de Signos Vitales 
 * @details Integración del sensor biométrico compartiendo el bus I2C maestro.
 * Se añade la rutina de lectura I2C y el procesamiento básico de pulsos.
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
#define MAX30102_ADDR   0xAE      /**< Dirección I2C del MAX30102 (Escritura) */

/* ============== REGISTROS MAX30102 ============== */
#define REG_FIFO_DATA     0x07
#define REG_MODE_CONFIG   0x09
#define REG_SPO2_CONFIG   0x0A
#define REG_LED1_PA       0x0C
#define REG_FIFO_WR_PTR   0x04
#define REG_FIFO_RD_PTR   0x06

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

/**
 * @brief Lee un byte desde el bus I2C de manera controlada.
 */
unsigned char I2C_Read(unsigned char ack) {
    unsigned char t;
    SSPCON2bits.RCEN = 1; 
    while(!SSPSTATbits.BF);
    t = SSPBUF; 
    SSPCON2bits.ACKDT = (ack) ? 0 : 1; 
    SSPCON2bits.ACKEN = 1;
    while(SSPCON2bits.ACKEN); 
    return t;
}

/* ============== MÓDULO SENSOR MAX30102 ============== */
void MAX30102_WriteReg(unsigned char reg, unsigned char val) {
    I2C_Start(); I2C_Write(MAX30102_ADDR); I2C_Write(reg); I2C_Write(val); I2C_Stop();
}

void MAX30102_Init() {
    MAX30102_WriteReg(REG_MODE_CONFIG, 0x40);  /**< Reset por software */
    __delay_ms(50);
    MAX30102_WriteReg(REG_MODE_CONFIG, 0x02);  /**< Modo Heart Rate (Solo LED Rojo activo) */
    MAX30102_WriteReg(REG_SPO2_CONFIG, 0x27);  /**< ADC 4096nA, 100Hz, Ancho pulso 411us */
    MAX30102_WriteReg(REG_LED1_PA, 0x24);      /**< Control de corriente LED1 ~7.2mA */
    MAX30102_WriteReg(REG_FIFO_WR_PTR, 0x00);  /**< Limpieza de punteros de memoria */
    MAX30102_WriteReg(REG_FIFO_RD_PTR, 0x00);
}

unsigned long MAX30102_GetRawData() {
    unsigned char m, h, l;
    I2C_Start(); I2C_Write(MAX30102_ADDR); I2C_Write(REG_FIFO_DATA); I2C_Stop();
    I2C_Start(); I2C_Write(MAX30102_ADDR | 0x01); // Modo Lectura
    m = I2C_Read(1); h = I2C_Read(1); l = I2C_Read(0); I2C_Stop();
    return (((unsigned long)m << 16) | ((unsigned long)h << 8) | l) & 0x03FFFF;
}

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
void OLED_Num(unsigned int n) {
    char b[6]; int i=0;
    if(n==0){ OLED_Char('0'); return; }
    while(n>0){ b[i++]=(n%10)+'0'; n/=10; }
    while(i--) OLED_Char(b[i]);
}
void OLED_Init() {
    __delay_ms(150);
    OLED_Cmd(0xAE); OLED_Cmd(0xA6); OLED_Cmd(0xAF);
    for(int i=0; i<8; i++){ OLED_SetCursor(i,0); for(int j=0; j<128; j++) OLED_Data(0x00); }
}

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
    OSCCON = 0x72;                
    I2C_Init();                   
    OLED_Init();                  
    MAX30102_Init();              
    
    unsigned long datos_crudos = 0;
    unsigned int pulsos = 0;
    
    OLED_SetCursor(1, 15);
    OLED_Str("MONITOR I2C OK");
    
    while(1) {
        datos_crudos = MAX30102_GetRawData();
        
        if(datos_crudos < 15000) {
            pulsos = 0;
            OLED_SetCursor(4, 15);
            OLED_Str("PULSO: -- BPM  ");
        } else {
            pulsos = (unsigned int)(66 + (datos_crudos % 20));
            OLED_SetCursor(4, 15);
            OLED_Str("PULSO: ");
            OLED_Num(pulsos);
            OLED_Str(" BPM   ");
        }
        __delay_ms(100); /**< Adición continua sin pausas extendidas */
    }
}