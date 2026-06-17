/**
 * @file main.c
 * @brief Sistema de control ambiental con PIC18F4550.
 * @details Gestiona temperatura (LM35), iluminación (LDR), calidad de aire (MQ135) 
 * y humedad (BME280). Salidas vía PWM, Relé y Pantalla OLED I2C.
 * @date 2026
 */

#include <xc.h>
#include <stdio.h>

/* ============== CONFIGURACIÓN DE BITS ============== */
#pragma config FOSC = INTOSC_EC  /**< Oscilador interno, puerto RA6 disponible */
#pragma config WDT = OFF          /**< Watchdog Timer desactivado */
#pragma config PBADEN = OFF       /**< Pines PORTB como digitales al reset */
#pragma config MCLRE = OFF        /**< Pin Master Clear como entrada digital */
#pragma config LVP = OFF          /**< Programación de bajo voltaje desactivada */

#define _XTAL_FREQ 8000000        /**< Frecuencia de oscilador: 8MHz */

/* ============== DIRECCIONES I2C ============== */
#define OLED_ADDR   0x78          /**< Dirección I2C de la pantalla OLED */
#define BME280_ADDR 0xEC          /**< Dirección I2C del sensor BME280 */

/* ============== PINES Y CANALES ============== */
#define CANAL_LM35   0            /**< Canal ADC para sensor de temperatura */
#define CANAL_LDR    1            /**< Canal ADC para sensor de luz */
#define CANAL_MQ135  2            /**< Canal ADC para sensor de gas */

#define RELE_CALOR LATDbits.LATD3  /**< Salida para control de calefacción */
#define LED_ALERTA LATDbits.LATD2  /**< Salida para indicador de alarma */

unsigned int tick_count = 0;      /**< Contador de ciclos para parpadeos */
unsigned int gas_timer = 0;       /**< Contador para retardo de 3s en alarma de gas */

/* ============== FUENTE OLED ============== */
/**
 * @brief Matriz de caracteres 5x8 para la pantalla OLED.
 */
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

/**
 * @brief Inicializa el módulo MSSP en modo Maestro I2C.
 */
void I2C_Init() {
    TRISBbits.TRISB0 = 1; TRISBbits.TRISB1 = 1;
    SSPSTAT = 0x80; SSPCON1 = 0x28; SSPADD = 19; 
}

/** @brief Genera la condición de START en el bus I2C. */
void I2C_Start() { SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN); }

/** @brief Genera la condición de STOP en el bus I2C. */
void I2C_Stop()  { SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN); }

/**
 * @brief Escribe un byte en el bus I2C.
 * @param d Byte de datos a enviar.
 */
void I2C_Write(unsigned char d) { SSPBUF = d; while(SSPSTATbits.BF); __delay_us(5); }

/**
 * @brief Lee un byte desde el esclavo I2C.
 * @param ack 1 para enviar ACK, 0 para enviar NACK.
 * @return Byte leído.
 */
unsigned char I2C_Read(unsigned char ack) {
    unsigned char t; SSPCON2bits.RCEN = 1; while(!SSPSTATbits.BF);
    t = SSPBUF; SSPCON2bits.ACKDT = (ack)?0:1; SSPCON2bits.ACKEN = 1;
    while(SSPCON2bits.ACKEN); return t;
}

/* ============== SENSOR BME280 ============== */

/**
 * @brief Configura e inicializa el sensor BME280.
 */
void BME280_Init() {
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xE0); I2C_Write(0xB6); I2C_Stop();
    __delay_ms(50);
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xF2); I2C_Write(0x01); I2C_Stop();
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xF4); I2C_Write(0x27); I2C_Stop();
}

/**
 * @brief Realiza la lectura de humedad relativa.
 * @return Valor de humedad en porcentaje (0-100).
 */
unsigned int BME280_ReadHum() {
    unsigned char m, l;
    I2C_Start(); I2C_Write(BME280_ADDR); I2C_Write(0xFD); I2C_Stop();
    I2C_Start(); I2C_Write(BME280_ADDR | 0x01); m = I2C_Read(1); l = I2C_Read(0); I2C_Stop();
    unsigned long h_raw = ((unsigned long)m << 8) | l;
    if(h_raw == 0 || h_raw == 0xFFFF) return 40;
    return (unsigned int)(h_raw * 100 / 65535);
}

/* ============== PANTALLA OLED ============== */

/**
 * @brief Envía un comando a la pantalla OLED.
 * @param c Byte de comando.
 */
void OLED_Cmd(unsigned char c) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x00); I2C_Write(c); I2C_Stop(); }

/**
 * @brief Envía un dato (píxeles) a la pantalla OLED.
 * @param d Byte de datos.
 */
void OLED_Data(unsigned char d) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x40); I2C_Write(d); I2C_Stop(); }

/**
 * @brief Posiciona el cursor en la pantalla.
 * @param p Página (0-7).
 * @param c Columna (0-127).
 */
void OLED_SetCursor(char p, char c) { OLED_Cmd(0xB0+p); OLED_Cmd(c & 0x0F); OLED_Cmd(0x10 | (c>>4)); }

/**
 * @brief Dibuja un carácter ASCII en la posición actual.
 * @param c Carácter a imprimir.
 */
void OLED_Char(char c) {
    if(c < 32 || c > 90) c = 32;
    for(int i=0; i<5; i++) OLED_Data(font5x8[c-32][i]);
    OLED_Data(0x00);
}

/** @brief Imprime una cadena de caracteres. */
void OLED_Str(const char *s) { while(*s) OLED_Char(*s++); }

/** @brief Convierte un entero a texto e imprime en pantalla. */
void OLED_Num(unsigned int n) {
    char b[6]; int i=0;
    if(n==0){ OLED_Char('0'); return; }
    while(n>0){ b[i++]=(n%10)+'0'; n/=10; }
    while(i--) OLED_Char(b[i]);
}

/** @brief Inicializa y limpia la pantalla OLED. */
void OLED_Init() {
    __delay_ms(150);
    OLED_Cmd(0xAE); OLED_Cmd(0xA6); OLED_Cmd(0xAF);
    for(int i=0; i<8; i++){ OLED_SetCursor(i,0); for(int j=0; j<128; j++) OLED_Data(0x00); }
}

/* ============== ADC Y PWM ============== */

/** @brief Configura puertos analógicos y tiempos de adquisición. */
void ADC_Init() { ADCON1 = 0x0C; ADCON2 = 0xBD; }

/**
 * @brief Realiza lectura ADC de 10 bits.
 * @param ch Canal analógico.
 * @return Valor leído (0-1023).
 */
unsigned int ADC_Read(unsigned char ch) {
    ADCON0 = (ch << 2) | 0x01;
    __delay_us(30); ADCON0bits.GO = 1; while(ADCON0bits.GO);
    return (ADRESH << 8) | ADRESL;
}

/** @brief Inicializa PWM en pin RC2 con Timer2 a 2kHz aprox. */
void PWM_Init() { TRISCbits.TRISC2 = 0; PR2 = 249; CCP1CON = 0x0C; T2CON = 0x04; }

/**
 * @brief Ajusta el ciclo de trabajo del PWM.
 * @param d Valor de 10 bits (0-1023).
 */
void PWM_Set(unsigned int d) {  
    if(d > 1023) d = 1023;
    CCPR1L = d >> 2; 
    CCP1CONbits.DC1B = d & 0x03; 
}

/* ============== PROGRAMA PRINCIPAL ============== */

/**
 * @brief Función principal: Lógica de control y visualización.
 */
void main() {
    OSCCON = 0x72;  /**< Configura oscilador a 8MHz interno */
    TRISA = 0x07;   /**< AN0, AN1, AN2 como entradas */
    TRISD = 0x00;   /**< PORTD como salidas */
    LATD = 0x00;
    
    /* Inicialización de periféricos */
    I2C_Init(); ADC_Init(); PWM_Init(); OLED_Init(); BME280_Init();
    
    unsigned int t_raw, l_raw, gas_raw, hum, temp, duty;
    unsigned char alarma_activa = 0;
    
    while(1) {
        /* Lectura de Sensores */
        t_raw   = ADC_Read(CANAL_LM35);
        l_raw   = ADC_Read(CANAL_LDR);
        gas_raw = ADC_Read(CANAL_MQ135);
        hum     = BME280_ReadHum();
        temp    = (unsigned int)((unsigned long)t_raw * 500 / 1023);
        
        /* CONTROL CALEFACCIÓN (Histerésis) */
        if (temp < 35) RELE_CALOR = 1;
        else if (temp > 37) RELE_CALOR = 0;
        
        /* CONTROL DE LUCES (Escala PORTD) */
        LATD &= 0x0F; 
        if (l_raw < 800) LATD |= 0x10; 
        if (l_raw < 600) LATD |= 0x20; 
        if (l_raw < 400) LATD |= 0x40; 
        if (l_raw < 200) LATD |= 0x80;

        /* LÓGICA DE GAS CON RETARDO DE 3 SEGUNDOS */
        if (gas_raw > 400) { 
            duty = 1023; 
            if (gas_timer < 15) {
                gas_timer++; 
            } else {
                alarma_activa = 1; 
            }
        } else {
            gas_timer = 0; 
            alarma_activa = 0;
            /* Control Humedad */
            if (hum > 47) duty = 1023; 
            else if (hum > 44) duty = (unsigned int)(hum - 44) * 341; 
            else duty = 0; 
        }
        
        PWM_Set(duty);
        
        /* Control LED de Alarma */
        if (alarma_activa) {
            LED_ALERTA = (tick_count % 2); 
        } else {
            LED_ALERTA = 0;
        }
        
        /* VISUALIZACIÓN OLED */
        OLED_SetCursor(0, 0); 
        OLED_Str(" TEMP:"); OLED_Num(temp); OLED_Char('C');
        OLED_SetCursor(0, 70);
        OLED_Str("R:"); OLED_Str(RELE_CALOR ? "E" : "A"); OLED_Str("   ");

        OLED_SetCursor(2, 0);
        OLED_Str(" ILU:"); OLED_Num(l_raw);
        OLED_SetCursor(2, 70);
        OLED_Str("L:");
        if(l_raw >= 800) OLED_Str("OFF");
        else if(l_raw >= 600) OLED_Str("B  ");
        else if(l_raw >= 400) OLED_Str("M  ");
        else if(l_raw >= 200) OLED_Str("MA ");
        else OLED_Str("MU "); 

        OLED_SetCursor(4, 0);
        OLED_Str(" AIRE:"); OLED_Str(gas_raw > 400 ? "M" : "B");
        OLED_SetCursor(4, 55);
        OLED_Str("V:"); OLED_Str(duty > 0 ? "S" : "N");
        OLED_SetCursor(4, 90);
        OLED_Str("A:"); OLED_Str(alarma_activa ? "S" : "N");

        OLED_SetCursor(6, 0);
        OLED_Str(" HUMEDAD:"); OLED_Num(hum); OLED_Char('%');
        OLED_SetCursor(6, 85);
        OLED_Str("V:"); OLED_Str(duty > 0 ? "S" : "N"); OLED_Str("  ");

        tick_count++;
        __delay_ms(200);
    }
}