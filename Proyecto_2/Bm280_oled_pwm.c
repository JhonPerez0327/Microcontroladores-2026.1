#include <xc.h>
#include <stdio.h>

#pragma config FOSC = INTOSC_EC  
#pragma config WDT = OFF         
#pragma config PBADEN = OFF      
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000
#define OLED_ADDR   0x78
#define BME280_ADDR 0xEC

// Fuente oled ASCII 32-90
const unsigned char font5x8[][5] = {
    {0,0,0,0,0},{0,0,95,0,0},{0,7,0,7,0},{20,127,20,127,20},{36,42,127,42,18},{35,19,8,100,98},{54,73,85,34,80},{0,5,3,0,0},{0,28,34,65,0},{0,65,34,28,0},{8,42,28,42,8},{8,8,62,8,8},{0,80,48,0,0},{8,8,8,8,8},{0,96,96,0,0},{32,16,8,4,2},{62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},{24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,41,30},{0,102,102,0,0},{0,102,98,0,0},{8,20,34,65,0},{36,36,36,36,36},{0,65,34,20,8},{2,1,81,9,6},{50,73,121,65,62},{126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},{7,8,112,8,7},{97,81,73,69,67}
};

// Funciones I2C
void I2C_Init(void) {
    TRISBbits.TRISB0 = 1; TRISBbits.TRISB1 = 1; 
    SSPSTAT = 0x80; SSPCON1 = 0x28; SSPADD = 19; 
}
void I2C_Start(void) { SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN); }
void I2C_Stop(void)  { SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN); }
void I2C_Write(unsigned char d) { SSPBUF = d; while(SSPSTATbits.BF); __delay_us(5); }

unsigned char I2C_Read(unsigned char ack) {
    unsigned char t; SSPCON2bits.RCEN = 1; while(!SSPSTATbits.BF);
    t = SSPBUF; SSPCON2bits.ACKDT = (ack)?0:1; SSPCON2bits.ACKEN = 1;
    while(SSPCON2bits.ACKEN); return t;
}

/* ============== FUNCIONES BME280 ============== */
void BME280_Init(void) {
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xF2); I2C_Write(0x01); I2C_Stop();
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xF4); I2C_Write(0x27); I2C_Stop();
}

unsigned int BME280_ReadHum(void) {
    unsigned char m, l;
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xFD); I2C_Stop();
    I2C_Start(); I2C_Write(BME280_ADDR | 0x01); 
    m = I2C_Read(1); l = I2C_Read(0); 
    I2C_Stop();
    if(m == 0xFF) return 0;
    unsigned long h = ((unsigned long)m << 8) | l;
    return (unsigned int)(h * 100 / 65535);
}

/* ============== FUNCIONES OLED ============== */
void OLED_Cmd(unsigned char c) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x00); I2C_Write(c); I2C_Stop(); }
void OLED_Data(unsigned char d) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x40); I2C_Write(d); I2C_Stop(); }
void OLED_SetCursor(char p, char c) { OLED_Cmd(0xB0+p); OLED_Cmd(c & 0x0F); OLED_Cmd(0x10 | (c>>4)); }
void OLED_Char(char c) {
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
void OLED_Init(void) {
    __delay_ms(150);
    OLED_Cmd(0xAE); OLED_Cmd(0xA6); OLED_Cmd(0xAF);
    for(int i=0; i<8; i++){ OLED_SetCursor(i,0); for(int j=0; j<128; j++) OLED_Data(0x00); }
}

/* ============== FUNCIONES PWM ============== */
void PWM_Init(void) {
    TRISCbits.TRISC2 = 0; // CCP1 como salida
    PR2 = 249;            // Frecuencia ~2kHz
    CCP1CON = 0x0C;       // Modo PWM
    T2CON = 0x04;         // Timer2 On, Prescaler 1
}

void PWM_Set(unsigned int duty) {
    CCPR1L = duty >> 2;
    CCP1CONbits.DC1B = duty & 0x03;
}

// Principal
void main(void) {
    OSCCON = 0x72;      // 8MHz
    I2C_Init();
    OLED_Init();
    BME280_Init();      // Iniciar sensor
    PWM_Init();         // Iniciar PWM
    
    unsigned int hum;
    unsigned int duty;

    while(1) {
        hum = BME280_ReadHum(); // Leer sensor de humedad

        // Logica de control modificada
        if (hum > 47) {
            duty = 1023; // Maxima velocidad al superar el 47%
        } else if (hum > 44) {
            // Entre 44% y 47% aumenta proporcionalmente
            duty = (hum - 44) * 341; 
            if (duty > 1023) duty = 1023;
        } else {
            duty = 0; // Por debajo o igual a 44% se apaga
        }
        
        PWM_Set(duty); // Aplicar valor al motor

        // Mostrar en OLED
        OLED_SetCursor(2, 10); 
        OLED_Str("HUMEDAD: ");
        OLED_Num(hum);
        OLED_Str(" %   "); 

        OLED_SetCursor(5, 10);
        OLED_Str("VENT: ");
        OLED_Num((duty * 100) / 1023); // Mostrar porcentaje de potencia
        OLED_Str(" %   ");

        __delay_ms(300); 
    }
}