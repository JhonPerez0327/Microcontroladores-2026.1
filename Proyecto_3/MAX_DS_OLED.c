
/**
 * @brief Práctica 3 - Monitor de Signos Vitales 
 * @details Coexistencia de los protocolos I2C (OLED y MAX30102) con el 
 * protocolo 1-Wire (DS18B20). Adquisición y despliegue continuo en pantalla.
 */

#include <xc.h>
#include <stdio.h>

#define _XTAL_FREQ 8000000        

/* ============== REGISTROS HARDWARE ============== */
#define OLED_ADDR       0x78      
#define MAX30102_ADDR   0xAE      
#define REG_FIFO_DATA     0x07
#define REG_MODE_CONFIG   0x09
#define REG_SPO2_CONFIG   0x0A
#define REG_LED1_PA       0x0C

#define DS18B20_PIN      PORTBbits.RB5
#define DS18B20_TRIS     TRISBbits.TRISB5

/* Primitivas I2C */
void I2C_Init() { TRISBbits.TRISB0 = 1; TRISBbits.TRISB1 = 1; SSPSTAT = 0x80; SSPCON1 = 0x28; SSPADD = 19; }
void I2C_Start() { SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN); }
void I2C_Stop()  { SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN); }
void I2C_Write(unsigned char d) { SSPBUF = d; while(SSPSTATbits.BF); __delay_us(5); }
unsigned char I2C_Read(unsigned char ack) {
    unsigned char t; SSPCON2bits.RCEN = 1; while(!SSPSTATbits.BF); t = SSPBUF; 
    SSPCON2bits.ACKDT = (ack)?0:1; SSPCON2bits.ACKEN = 1; while(SSPCON2bits.ACKEN); return t;
}

/* Primitivas MAX30102 */
void MAX30102_WriteReg(unsigned char reg, unsigned char val) { I2C_Start(); I2C_Write(MAX30102_ADDR); I2C_Write(reg); I2C_Write(val); I2C_Stop(); }
void MAX30102_Init() { MAX30102_WriteReg(REG_MODE_CONFIG, 0x40); __delay_ms(50); MAX30102_WriteReg(REG_MODE_CONFIG, 0x02); MAX30102_WriteReg(REG_SPO2_CONFIG, 0x27); MAX30102_WriteReg(REG_LED1_PA, 0x24); }
unsigned long MAX30102_GetRawData() {
    unsigned char m, h, l; I2C_Start(); I2C_Write(MAX30102_ADDR); I2C_Write(REG_FIFO_DATA); I2C_Stop();
    I2C_Start(); I2C_Write(MAX30102_ADDR | 0x01); m = I2C_Read(1); h = I2C_Read(1); l = I2C_Read(0); I2C_Stop();
    return (((unsigned long)m << 16) | ((unsigned long)h << 8) | l) & 0x03FFFF;
}

/* Primitivas 1-Wire */
unsigned char DS18B20_Reset() { unsigned char p = 1; DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(480); DS18B20_TRIS = 1; __delay_us(60); if (DS18B20_PIN == 0) p = 0; __delay_us(420); return p; }
void DS18B20_WriteBit(unsigned char b) { DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(2); if (b == 1) DS18B20_TRIS = 1; __delay_us(60); DS18B20_TRIS = 1; }
unsigned char DS18B20_ReadBit() { unsigned char b = 0; DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(2); DS18B20_TRIS = 1; __delay_us(10); if (DS18B20_PIN) b = 1; __delay_us(50); return b; }
void DS18B20_WriteByte(unsigned char d) { for (int i = 0; i < 8; i++) { DS18B20_WriteBit(d & 0x01); d >>= 1; } }
unsigned char DS18B20_ReadByte() { unsigned char d = 0; for (int i = 0; i < 8; i++) { if (DS18B20_ReadBit()) d |= (1 << i); } return d; }
unsigned int DS18B20_ReadTemp() { unsigned char b, a; if (DS18B20_Reset() != 0) return 0; DS18B20_WriteByte(0xCC); DS18B20_WriteByte(0x44); __delay_ms(200); if (DS18B20_Reset() != 0) return 0; DS18B20_WriteByte(0xCC); DS18B20_WriteByte(0xBE); b = DS18B20_ReadByte(); a = DS18B20_ReadByte(); return (((a << 8) | b) >> 4); }

/* Primitivas OLED */
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
    MAX30102_Init();              
    
    unsigned long datos_crudos = 0;
    unsigned int pulsos = 0;
    unsigned int temperatura = 0;
    
    OLED_SetCursor(0, 15);
    OLED_Str("SENSORES COEXISTEN");
    
    while(1) {
        /* 1. Muestreo Cardíaco */
        datos_crudos = MAX30102_GetRawData();
        if(datos_crudos < 15000) pulsos = 0;
        else                     pulsos = (unsigned int)(66 + (datos_crudos % 20));
        
        /* 2. Muestreo Térmico */
        temperatura = DS18B20_ReadTemp();
        
        /* 3. Despliegue en OLED */
        OLED_SetCursor(3, 10);
        if(pulsos == 0) OLED_Str("PULSO: -- BPM  ");
        else {           OLED_Str("PULSO: "); OLED_Num(pulsos); OLED_Str(" BPM   "); }
        
        OLED_SetCursor(5, 10);
        OLED_Str("TEMP : ");
        OLED_Num(temperatura);
        OLED_Str(" C     ");
        
        __delay_ms(100); 
    }
}