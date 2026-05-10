#include <xc.h>
#include <stdio.h>

#pragma config FOSC = INTOSC_EC  
#pragma config WDT = OFF         
#pragma config PBADEN = OFF      
#pragma config MCLRE = OFF
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000

/* ============== DIRECCIONES I2C ============== */
#define OLED_ADDR   0x78  
#define BME280_ADDR 0xEC  

/* ============== PINES Y CANALES ============== */
#define CANAL_LM35   0    
#define CANAL_LDR    1    
#define CANAL_MQ135  2    

#define RELE_CALOR LATDbits.LATD3  
#define LED_ALERTA LATDbits.LATD2  

unsigned int tick_count = 0;
unsigned int gas_timer = 0; // Contador para el retardo de 3s

/* ============== FUENTE OLED ============== */
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

/* ============== COMUNICACIÓN I2C ============== */
void I2C_Init() {
    TRISBbits.TRISB0 = 1; TRISBbits.TRISB1 = 1;
    SSPSTAT = 0x80; SSPCON1 = 0x28; SSPADD = 19; 
}
void I2C_Start() { SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN); }
void I2C_Stop()  { SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN); }
void I2C_Write(unsigned char d) { SSPBUF = d; while(SSPSTATbits.BF); __delay_us(5); }
unsigned char I2C_Read(unsigned char ack) {
    unsigned char t; SSPCON2bits.RCEN = 1; while(!SSPSTATbits.BF);
    t = SSPBUF; SSPCON2bits.ACKDT = (ack)?0:1; SSPCON2bits.ACKEN = 1;
    while(SSPCON2bits.ACKEN); return t;
}

/* ============== SENSOR BME280 ============== */
void BME280_Init() {
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xE0); I2C_Write(0xB6); I2C_Stop();
    __delay_ms(50);
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xF2); I2C_Write(0x01); I2C_Stop();
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xF4); I2C_Write(0x27); I2C_Stop();
}

unsigned int BME280_ReadHum() {
    unsigned char m, l;
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xFD); I2C_Stop();
    I2C_Start(); I2C_Write(BME280_ADDR | 0x01); m = I2C_Read(1); l = I2C_Read(0); I2C_Stop();
    unsigned long h_raw = ((unsigned long)m << 8) | l;
    if(h_raw == 0 || h_raw == 0xFFFF) return 40;
    return (unsigned int)(h_raw * 100 / 65535);
}

/* ============== PANTALLA OLED ============== */
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
void OLED_Init() {
    __delay_ms(150);
    OLED_Cmd(0xAE); OLED_Cmd(0xA6); OLED_Cmd(0xAF);
    for(int i=0; i<8; i++){ OLED_SetCursor(i,0); for(int j=0; j<128; j++) OLED_Data(0x00); }
}

/* ============== ADC Y PWM ============== */
void ADC_Init() { ADCON1 = 0x0C; ADCON2 = 0xBD; }
unsigned int ADC_Read(unsigned char ch) {
    ADCON0 = (ch << 2) | 0x01;
    __delay_us(30); ADCON0bits.GO = 1; while(ADCON0bits.GO);
    return (ADRESH << 8) | ADRESL;
}
void PWM_Init() { TRISCbits.TRISC2 = 0; PR2 = 249; CCP1CON = 0x0C; T2CON = 0x04; }
void PWM_Set(unsigned int d) {  
    if(d > 1023) d = 1023;
    CCPR1L = d >> 2; 
    CCP1CONbits.DC1B = d & 0x03; 
}

/* ============== PROGRAMA PRINCIPAL ============== */
void main() {
    OSCCON = 0x72; 
    TRISA = 0x07; 
    TRISD = 0x00; 
    LATD = 0x00;
    
    I2C_Init(); ADC_Init(); PWM_Init(); OLED_Init(); BME280_Init();
    
    unsigned int t_raw, l_raw, gas_raw, hum, temp, duty;
    unsigned char alarma_activa = 0;
    
    while(1) {
        t_raw   = ADC_Read(CANAL_LM35);
        l_raw   = ADC_Read(CANAL_LDR);
        gas_raw = ADC_Read(CANAL_MQ135);
        hum     = BME280_ReadHum();
        temp    = (unsigned int)((unsigned long)t_raw * 500 / 1023);
        
        // CONTROL CALEFACCIÓN
        if (temp < 35) RELE_CALOR = 1;
        else if (temp > 37) RELE_CALOR = 0;
        
        // CONTROL DE LUCES
        LATD &= 0x0F; 
        if (l_raw < 800) LATD |= 0x10; 
        if (l_raw < 600) LATD |= 0x20; 
        if (l_raw < 400) LATD |= 0x40; 
        if (l_raw < 200) LATD |= 0x80;

        // LÓGICA DE GAS CON RETARDO DE 3 SEGUNDOS PARA ALARMA
        if (gas_raw > 400) { 
            duty = 1023; // Ventilador al máximo inmediato por seguridad
            if (gas_timer < 15) {
                gas_timer++; // Aumentar contador (cada tick son 200ms)
            } else {
                alarma_activa = 1; // Se cumplieron los 3 segundos
            }
        } else {
            gas_timer = 0; // Resetear contador si el aire vuelve a ser bueno
            alarma_activa = 0;
            // Si no hay gas, ventilación normal por humedad
            if (hum > 47) duty = 1023; 
            else if (hum > 44) duty = (unsigned int)(hum - 44) * 341; 
            else duty = 0; 
        }
        
        PWM_Set(duty);
        
        // Manejo del LED de Alerta (solo si pasaron los 3 segundos)
        if (alarma_activa) {
            LED_ALERTA = (tick_count % 2); // Parpadeo
        } else {
            LED_ALERTA = 0;
        }
        
        /* ============== VISUALIZACIÓN OLED ============== */
        
        // Linea 1: Temp y Rele
        OLED_SetCursor(0, 0); 
        OLED_Str(" TEMP:"); OLED_Num(temp); OLED_Char('°C');
        OLED_SetCursor(0, 70);
        OLED_Str("R:"); OLED_Str(RELE_CALOR ? "E" : "A"); OLED_Str("   ");

        // Linea 2: Iluminacion y Escala de LEDs
        OLED_SetCursor(2, 0);
        OLED_Str(" ILU:"); OLED_Num(l_raw);
        OLED_SetCursor(2, 70);
        OLED_Str("L:");
        if(l_raw >= 800) OLED_Str("OFF");
        else if(l_raw >= 600) OLED_Str("B  ");
        else if(l_raw >= 400) OLED_Str("M  ");
        else if(l_raw >= 200) OLED_Str("MA ");
        else OLED_Str("MU "); 

        // Linea 3: Aire, Ventilador y Alarma
        OLED_SetCursor(4, 0);
        OLED_Str(" AIRE:"); OLED_Str(gas_raw > 400 ? "M" : "B");
        OLED_SetCursor(4, 55);
        OLED_Str("V:"); OLED_Str(duty > 0 ? "S" : "N");
        OLED_SetCursor(4, 90);
        OLED_Str("A:"); OLED_Str(alarma_activa ? "S" : "N");

        // Linea 4: Humedad y Ventilador
        OLED_SetCursor(6, 0);
        OLED_Str(" HUMEDAD:"); OLED_Num(hum); OLED_Char('%');
        OLED_SetCursor(6, 85);
        OLED_Str("V:"); OLED_Str(duty > 0 ? "S" : "N"); OLED_Str("  ");

        tick_count++;
        __delay_ms(200);
    }
}