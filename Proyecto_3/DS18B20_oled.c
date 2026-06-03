/**
 * @brief Práctica 3 - Monitor de Signos Vitales 
 * @details Prueba unitaria del protocolo 1-Wire. Se verifica la lectura de 
 * temperatura digital aislando los tiempos críticos de microsegundos.
 */

#include <xc.h>
#include <stdio.h>

/* ============== CONFIGURACIÓN DE BITS ============== */
#pragma config FOSC = INTOSC_EC  
#pragma config WDT = OFF          
#pragma config PBADEN = OFF       
#pragma config MCLRE = OFF        
#pragma config LVP = OFF          

#define _XTAL_FREQ 8000000        

/* ============== PROTOCOLO 1-WIRE (DS18B20 en RB5) ============== */
#define DS18B20_PIN      PORTBbits.RB5
#define DS18B20_TRIS     TRISBbits.TRISB5

/**
 * @brief Envía el pulso de Reset y detecta la presencia del DS18B20.
 * @return 0 si detecta el sensor, 1 si no hay respuesta.
 */
unsigned char DS18B20_Reset() {
    unsigned char presencia = 1;
    DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(480); /** Pulso de Reset */
    DS18B20_TRIS = 1;                  __delay_us(60);  /** Espera de estabilización */
    if (DS18B20_PIN == 0) presencia = 0;               /** Pulso de presencia detectado */
    __delay_us(420);                                    /** Final del ciclo de reset */
    return presencia;
}

void DS18B20_WriteBit(unsigned char b) {
    DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(2);   /** Slot de tiempo de inicio */
    if (b == 1) DS18B20_TRIS = 1;                       /** Deja la línea en alta si es '1' */
    __delay_us(60); 
    DS18B20_TRIS = 1;                                   /** Libera el bus */
}

unsigned char DS18B20_ReadBit() {
    unsigned char b = 0;
    DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(2);
    DS18B20_TRIS = 1;                  __delay_us(10);  /** Ventana de muestreo */
    if (DS18B20_PIN) b = 1;
    __delay_us(50);
    return b;
}

void DS18B20_WriteByte(unsigned char dato) {
    for (int i = 0; i < 8; i++) { 
        DS18B20_WriteBit(dato & 0x01); 
        dato >>= 1; 
    }
}

unsigned char DS18B20_ReadByte() {
    unsigned char dato = 0;
    for (int i = 0; i < 8; i++) { 
        if (DS18B20_ReadBit()) dato |= (1 << i); 
    }
    return dato;
}

/**
 * @brief Solicita conversión y extrae la temperatura entera del Scratchpad.
 */
unsigned int DS18B20_ReadTemp() {
    unsigned char bajo, alto;
    if (DS18B20_Reset() != 0) return 0; /** Error: No hay sensor */
    DS18B20_WriteByte(0xCC);            /** Comando: Skip ROM */
    DS18B20_WriteByte(0x44);            /** Comando: Convert T */
    __delay_ms(200);                    /** Conversión rápida parcial tolerada */
    
    if (DS18B20_Reset() != 0) return 0;
    DS18B20_WriteByte(0xCC);            /** Comando: Skip ROM */
    DS18B20_WriteByte(0xBE);            /** Comando: Read Scratchpad */
    
    bajo = DS18B20_ReadByte(); 
    alto = DS18B20_ReadByte();
    return (((alto << 8) | bajo) >> 4); /** Filtrado de la parte entera superior */
}

/* ============== PANTALLA OLED INTERFAZ ============== */
#define OLED_ADDR       0x78
void I2C_Init() { TRISBbits.TRISB0 = 1; TRISBbits.TRISB1 = 1; SSPSTAT = 0x80; SSPCON1 = 0x28; SSPADD = 19; }
void I2C_Start() { SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN); }
void I2C_Stop()  { SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN); }
void I2C_Write(unsigned char d) { SSPBUF = d; while(SSPSTATbits.BF); __delay_us(5); }
void OLED_Cmd(unsigned char c) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x00); I2C_Write(c); I2C_Stop(); }
void OLED_Data(unsigned char d) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x40); I2C_Write(d); I2C_Stop(); }
void OLED_SetCursor(char p, char c) { OLED_Cmd(0xB0+p); OLED_Cmd(c & 0x0F); OLED_Cmd(0x10 | (c>>4)); }
void OLED_Char(char c) { extern const unsigned char font5x8[][5]; if(c < 32 || c > 90) c = 32; for(int i=0; i<5; i++) OLED_Data(font5x8[c-32][i]); OLED_Data(0x00); }
void OLED_Str(const char *s) { while(*s) OLED_Char(*s++); }
void OLED_Num(unsigned int n) { char b[6]; int i=0; if(n==0){ OLED_Char('0'); return; } while(n>0){ b[i++]=(n%10)+'0'; n/=10; } while(i--) OLED_Char(b[i]); }
void OLED_Init() { __delay_ms(150); OLED_Cmd(0xAE); OLED_Cmd(0xA6); OLED_Cmd(0xAF); for(int i=0; i<8; i++){ OLED_SetCursor(i,0); for(int j=0; j<128; j++) OLED_Data(0x00); } }

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
    
    unsigned int temperatura_leida = 0;
    
    OLED_SetCursor(1, 10);
    OLED_Str("TEST 1-WIRE OK");
    
    while(1) {
        /* Lectura directa del sensor térmico digital */
        temperatura_leida = DS18B20_ReadTemp();
        
        OLED_SetCursor(4, 15);
        OLED_Str("TEMP : ");
        OLED_Num(temperatura_leida);
        OLED_Str(" C     ");
        
        __delay_ms(200); 
    }
}
